/*
 *  Copyright (C) Sascha Montellese
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with cpp-signals; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CPP_SIGNAL_ASYNC_H_
#define CPP_SIGNAL_ASYNC_H_

#include <condition_variable>
#include <forward_list>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

#include <cpp-signal.h>

template<class TLockingPolicy = std::mutex>
class cpp_signal_async
{
public:
  using locking_policy = TLockingPolicy;

  cpp_signal_async() = delete;

  template<typename TReturn> class signal;

  /********************* slot_tracker ********************/
  class slot_tracker : protected locking_policy
  {
  private:
    template<typename TReturn> friend class signal;

    static const std::launch async_launch_policy = std::launch::async;

    struct tracked_slot
    {
      const cpp_signal_util::slot_key key;
      slot_tracker* tracker;
      const bool call;
    };
    using slots = std::forward_list<tracked_slot>;

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
        tracker_.cond_var_.notify_all();  // TODO: notify_one() ???
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
      : slots_()
      , cond_var_()
      , sem_(*this, 0)
    { }

    slot_tracker(const slot_tracker& other) noexcept
      : slots_()
      , cond_var_()
      , sem_(*this, 0)
    {
      copy(other);
    }

    virtual ~slot_tracker() noexcept
    {
      clear();
    }

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
        [this](cpp_signal_util::decay_t<TCollector>&& collector, TCallArgs&&... args) -> void
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

    inline void add_to_call(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      add(key, tracker, true);
    }

    inline void add_to_track(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      add(key, tracker, false);
    }

    inline void add(const cpp_signal_util::slot_key& key, slot_tracker* tracker, bool call) noexcept
    {
      scoped_semaphore sem(sem_);
      slots_.emplace_front(tracked_slot{ key, tracker, call });
    }

    inline void remove(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      scoped_semaphore sem(sem_);
      slots_.remove_if([&key, tracker](const tracked_slot& slot) -> bool
      {
        return slot.key == key && slot.tracker == tracker;
      });
    }

    void clear() noexcept
    {
      scoped_semaphore sem(sem_);
      for (auto& slot : slots_)
      {
        if (slot.tracker != this)
          slot.tracker->remove(slot.key, this);
      }

      slots_.clear();
    }

    inline bool empty() noexcept
    {
      scoped_semaphore sem(sem_);
      return slots_.empty();
    }

    void copy(const slot_tracker& other) noexcept
    {
      scoped_semaphore sem(sem_);
      for (const auto& tracked_slot : other.slots_)
      {
        // if this is a signal we keep the key of the slot to be called
        // but if it's a tracked slot we need to adjust the key to point at us
        cpp_signal_util::slot_key copied_key = tracked_slot.key;
        if (!tracked_slot.call)
        {
          // the signature of the slot template is irrelevant here
          copied_key = cpp_signal_util::slot<void()>::copy_key(tracked_slot.key, this);
        }

        // this is a non-tracked slot so just copy it and point the tracker to us
        if (tracked_slot.tracker == &other)
          add(copied_key, this, tracked_slot.call);
        else
        {
          // keep track as well
          add(copied_key, tracked_slot.tracker, tracked_slot.call);

          // if this is a signal tell the tracker to track this signal
          if (tracked_slot.call)
            tracked_slot.tracker->add_to_track(copied_key, this);
          // if this is a tracked slot tell the signal to call this slot
          else
            tracked_slot.tracker->add_to_call(copied_key, this);
        }
      }
    }

  private:
    // this is an alternative to std::async which allows the caller to discard the returned
    // std::future
    template<class TFunction, typename... TArgs>
    std::future<cpp_signal_util::result_of_t<cpp_signal_util::decay_t<TFunction>(cpp_signal_util::decay_t<TArgs>...)>> async(TFunction&& fun, TArgs&&... args)
    {
      using result_t = cpp_signal_util::result_of_t<cpp_signal_util::decay_t<TFunction>(cpp_signal_util::decay_t<TArgs>...)>;

      // create a task for the given function
      std::packaged_task<result_t(cpp_signal_util::decay_t<TArgs>...)> task(std::forward<TFunction>(fun));
      // get the future of the task
      auto result = task.get_future();

      // run the task in a new thread passing the given arguments to it
      std::thread task_thread(std::move(task), std::forward<TArgs>(args)...);
      task_thread.detach();

      return result;
    }

    slots slots_;

    std::condition_variable_any cond_var_;
    semaphore sem_;
  };

  /********************* signal ********************/
  template<typename TReturn, typename... TArgs>
  class signal<TReturn(TArgs...)> : public slot_tracker
  {
  private:
    using connected_slot = cpp_signal_util::slot<TReturn(TArgs...)>;

  public:
    using result_type = TReturn;

    signal() = default;

    signal(const signal& other) noexcept
      : slot_tracker(other)
    { }

    ~signal() = default;

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

    // callable object
    template<typename TObject>
    inline void connect(TObject& callable) noexcept
    {
      connect<TObject>(std::addressof(callable));
    }

    template<typename TObject>
    inline void disconnect(TObject& callable) noexcept
    {
      disconnect<TObject>(std::addressof(callable));
    }

    // callable pointer to object
    template<typename TObject>
    inline void connect(TObject* callable) noexcept
    {
      add<TObject>(connected_slot::template bind<TObject>(callable), callable);
    }

    template<typename TObject>
    inline void disconnect(TObject* callable) noexcept
    {
      remove<TObject>(connected_slot::template bind<TObject>(callable), callable);
    }

    // static/global function
    template<TReturn(*TFunction)(TArgs...)>
    inline void connect() noexcept
    {
      slot_tracker::add_to_call(connected_slot::template bind<TFunction>(), this);
    }

    template<TReturn(*TFunction)(TArgs...)>
    inline void disconnect() noexcept
    {
      slot_tracker::remove(connected_slot::template bind<TFunction>(), this);
    }

    // member function from object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void connect(TObject& obj) noexcept
    {
      connect<TObject, TFunction>(std::addressof(obj));
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void disconnect(TObject& obj) noexcept
    {
      disconnect<TObject, TFunction>(std::addressof(obj));
    }

    // member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void connect(TObject* obj) noexcept
    {
      add<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void disconnect(TObject* obj) noexcept
    {
      remove<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    // const member function from object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void connect(TObject& obj) noexcept
    {
      connect<TObject, TFunction>(std::addressof(obj));
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void disconnect(TObject& obj) noexcept
    {
      disconnect<TObject, TFunction>(std::addressof(obj));
    }

    // const member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void connect(TObject* obj) noexcept
    {
      add<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void disconnect(TObject* obj) noexcept
    {
      remove<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

  private:
    // connect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, typename TObject::slot_tracker* tracker) noexcept
    {
      slot_tracker::add_to_call(key, tracker);
      tracker->add_to_track(key, this);
    }

    // connect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, ...) noexcept
    {
      slot_tracker::add_to_call(key, this);
    }

    // disconnect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, typename TObject::slot_tracker* tracker) noexcept
    {
      slot_tracker::remove(key, tracker);
      tracker->remove(key, this);
    }

    // disconnect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, ...) noexcept
    {
      slot_tracker::remove(key, this);
    }
  };
};

#endif  // CPP_SIGNAL_ASYNC_H_
