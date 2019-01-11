/*
 *  Copyright (C) Sascha Montellese
 */

#ifndef CPP_SIGNAL_ASYNC_H_
#define CPP_SIGNAL_ASYNC_H_

#if defined(_MSC_VER) && _MSC_VER < 1910
#define MSVC_LEGACY
#endif

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include <cpp-signal-base.h>
#include <cpp-signal-locking.h>
#include <cpp-signal-util.h>

template<class TLockingPolicy = std::mutex>
class cpp_signal_async
{
public:
  using locking_policy = TLockingPolicy;

  cpp_signal_async() = delete;

  template<typename TReturn> class signal;

  /********************* slot_tracker ********************/
  class slot_tracker : public cpp_signal_base<locking_policy>::slot_tracker
  {
  private:
    template<typename TSlotTracker, typename TReturn> friend class cpp_signal_base<locking_policy>::signal;
    template<typename TReturn> friend class signal;

    using slot_tracker_base = typename cpp_signal_base<locking_policy>::slot_tracker;
    using slot_tracker_base::slots_;

    static const std::launch async_launch_policy = std::launch::async;

    class semaphore
    {
    public:
      semaphore(slot_tracker& tracker, int count = 0) noexcept
        : tracker_(tracker)
        , count_(count)
      { }

      semaphore(const semaphore&) = delete;
      semaphore& operator=(const semaphore&) = delete;

      inline void notify()
      {
        std::unique_lock<locking_policy> lock(tracker_);
        ++count_;
        tracker_.cond_var_.notify_one();
      }

      inline void wait()
      {
        std::unique_lock<locking_policy> lock(tracker_);
        tracker_.cond_var_.wait(lock, [this]() -> bool { return count_ >= 0; });
        --count_;
      }

    private:
      slot_tracker& tracker_;
      int count_;
    };

    friend class semaphore;

    class scoped_semaphore
    {
    public:
      scoped_semaphore(semaphore& sem)
        : sem_(sem)
      {
        sem_.wait();
      }

      ~scoped_semaphore()
      {
        sem_.notify();
      }

      scoped_semaphore(const scoped_semaphore&) = delete;
      scoped_semaphore& operator=(const scoped_semaphore&) = delete;

    private:
      semaphore& sem_;
    };

  public:
    slot_tracker() noexcept
      : slot_tracker_base()
      , cond_var_()
      , sem_(*this, 0)
    { }

    slot_tracker(const slot_tracker& other) noexcept
      : slot_tracker_base(other)
      , cond_var_()
      , sem_(*this, 0)
    { }

    ~slot_tracker() noexcept override = default;

    slot_tracker& operator=(const slot_tracker& other) noexcept
    {
      copy(other);

      return *this;
    }

  protected:
    template<class TSlot, typename... TCallArgs>
    std::future<void> call(TCallArgs&&... args)
    {
      sem_.wait();
      return async(
        [this](TCallArgs&&... args) -> void
        {
          for (const auto& slot : slots_)
          {
            if (!slot.call)
              continue;

            TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
          }

          sem_.notify();
        }, std::forward<TCallArgs>(args)...);
    }

    template<class TInit, class TSlot, typename... TCallArgs>
    std::future<cpp_signal_util::decay_t<TInit>> accumulate(TInit&& init, TCallArgs&&... args)
    {
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      sem_.wait();
      return async(
        [this, init](TCallArgs&&... args) -> cpp_signal_util::decay_t<TInit>
        {
          cpp_signal_util::decay_t<TInit> value = init;
          for (const auto& slot : slots_)
          {
            if (!slot.call)
              continue;

            value = value + TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
          }

          sem_.notify();

          return value;
        }, std::forward<TCallArgs>(args)...);
    }

    template<class TInit, class TBinaryOperation, class TSlot, typename... TCallArgs>
    std::future<cpp_signal_util::decay_t<TInit>> accumulate_op(TInit&& init, TBinaryOperation&& binary_op, TCallArgs&&... args)
    {
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      sem_.wait();
      return async(
        [this, init](TBinaryOperation&& binary_op, TCallArgs&&... args) -> cpp_signal_util::decay_t<TInit>
        {
          cpp_signal_util::decay_t<TInit> value = init;
          for (const auto& slot : slots_)
          {
            if (!slot.call)
              continue;

            value = binary_op(value, TSlot(slot.key).call(std::forward<TCallArgs>(args)...));
          }

          sem_.notify();

          return value;
        }, std::forward<TBinaryOperation>(binary_op), std::forward<TCallArgs>(args)...);
    }

    template<class TContainer, class TSlot, typename... TCallArgs>
    std::future<TContainer> aggregate(TCallArgs&&... args)
    {
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      sem_.wait();
      return async(
        [this](TCallArgs&&... args) -> TContainer
        {
          TContainer container;
          auto iterator = std::inserter(container, container.end());

          for (const auto& slot : slots_)
          {
            if (!slot.call)
              continue;

            *iterator = TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
          }

          sem_.notify();

          return container;
        }, std::forward<TCallArgs>(args)...);
    }

    template<class TCollector, class TSlot, typename... TCallArgs>
    std::future<void> collect(TCollector&& collector, TCallArgs&&... args)
    {
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot collect slot return values of type 'void'");

      sem_.wait();
      return async(
#ifdef MSVC_LEGACY
        [this](TCollector&& collector, TCallArgs&&... args) -> void
#else
        [this](cpp_signal_util::decay_t<TCollector>&& collector, TCallArgs&&... args) -> void
#endif
        {
          for (const auto& slot : slots_)
          {
            if (!slot.call)
              continue;

            collector(TSlot(slot.key).call(std::forward<TCallArgs>(args)...));
          }

          sem_.notify();
        }, std::forward<TCollector>(collector), std::forward<TCallArgs>(args)...);
    }

    inline void add(const cpp_signal_util::slot_key& key, slot_tracker_base* tracker, bool call) noexcept override
    {
      scoped_semaphore sem(sem_);
      slot_tracker_base::add(key, tracker, call);
    }

    inline void remove(const cpp_signal_util::slot_key& key, slot_tracker_base* tracker) noexcept override
    {
      scoped_semaphore sem(sem_);
      slot_tracker_base::remove(key, tracker);
    }

    void clear() noexcept override
    {
      scoped_semaphore sem(sem_);
      slot_tracker_base::clear();
    }

    inline bool empty() noexcept override
    {
      scoped_semaphore sem(sem_);
      return slot_tracker_base::empty();
    }

    void copy(const slot_tracker_base& other) noexcept override
    {
      scoped_semaphore sem(sem_);
      slot_tracker_base::copy(other);
    }

  private:
    // this is an alternative to std::async which allows the caller to discard the returned
    // std::future
    template<class TFunction, typename... TArgs>
#ifdef MSVC_LEGACY
    std::future<cpp_signal_util::decay_t<cpp_signal_util::result_of_t<TFunction(TArgs...)>>> async(TFunction&& fun, TArgs&&... args)
#else
    std::future<cpp_signal_util::result_of_t<cpp_signal_util::decay_t<TFunction>(cpp_signal_util::decay_t<TArgs>...)>> async(TFunction&& fun, TArgs&&... args)
#endif
    {
#ifdef MSVC_LEGACY
      using result_t = cpp_signal_util::decay_t<cpp_signal_util::result_of_t<TFunction(TArgs...)>>;
#else
      using result_t = cpp_signal_util::result_of_t<cpp_signal_util::decay_t<TFunction>(cpp_signal_util::decay_t<TArgs>...)>;
#endif

      // create a task for the given function
#ifdef MSVC_LEGACY
      std::packaged_task<result_t(TArgs...)> task(std::forward<TFunction>(fun));
#else
      std::packaged_task<result_t(cpp_signal_util::decay_t<TArgs>...)> task(std::forward<TFunction>(fun));
#endif
      // get the future of the task
      auto result = task.get_future();

      // run the task in a new thread passing the given arguments to it
      std::thread task_thread(std::move(task), std::forward<TArgs>(args)...);
      task_thread.detach();

      return result;
    }

    std::condition_variable_any cond_var_;
    semaphore sem_;
  };

  /********************* signal ********************/
  template<typename TReturn, typename... TArgs>
  class signal<TReturn(TArgs...)> : public cpp_signal_base<locking_policy>::template signal<slot_tracker, TReturn(TArgs...)>
  {
  private:
    using signal_base = typename cpp_signal_base<locking_policy>::template signal<slot_tracker, TReturn(TArgs...)>;
    using connected_slot = cpp_signal_util::slot<TReturn(TArgs...)>;

  public:
    using result_type = TReturn;

    signal() = default;

    signal(const signal& other) noexcept
      : signal_base(other)
    { }

    ~signal() noexcept override = default;

    template<typename... TEmitArgs>
    inline std::future<void> operator()(TEmitArgs&&... args)
    {
      return emit(std::forward<TEmitArgs>(args)...);
    }

    template<typename... TEmitArgs>
    inline std::future<void> emit(TEmitArgs&&... args)
    {
      return slot_tracker::template call<connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, typename... TEmitArgs>
    inline std::future<cpp_signal_util::decay_t<TInit>> accumulate(TInit&& init, TEmitArgs&&... args)
    {
      return slot_tracker::template accumulate<TInit, connected_slot>(std::forward<TInit>(init), std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, class TBinaryOperation, typename... TEmitArgs>
    inline std::future<cpp_signal_util::decay_t<TInit>> accumulate_op(TInit&& init, TBinaryOperation&& binary_op, TEmitArgs&&... args)
    {
      return slot_tracker::template accumulate_op<TInit, TBinaryOperation, connected_slot>(std::forward<TInit>(init), std::forward<TBinaryOperation>(binary_op), std::forward<TEmitArgs>(args)...);
    }

    template<class TContainer, typename... TEmitArgs>
    inline std::future<TContainer> aggregate(TEmitArgs&&... args)
    {
      return slot_tracker::template aggregate<TContainer, connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<typename TCollector, typename... TEmitArgs>
    inline std::future<void> collect(TCollector&& collector, TEmitArgs&&... args)
    {
      return slot_tracker::template collect<TCollector, connected_slot>(std::forward<TCollector>(collector), std::forward<TEmitArgs>(args)...);
    }
  };
};

#endif  // CPP_SIGNAL_ASYNC_H_
