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

#ifndef CPP_SIGNAL_H_
#define CPP_SIGNAL_H_

#include <cstdint>
#include <forward_list>
#include <memory>
#include <mutex>
#include <type_traits>

class cpp_signal_no_locking
{
public:
  cpp_signal_no_locking() = default;
  ~cpp_signal_no_locking() = default;

  inline void lock() { }
  inline void unlock() { }
};

class cpp_signal_global_locking
{
public:
  cpp_signal_global_locking() = default;
  ~cpp_signal_global_locking() = default;

  inline void lock()
  {
    mutex().lock();
  }

  inline void unlock()
  {
    mutex().unlock();
  }

private:
  static std::mutex& mutex()
  {
    static std::mutex mutex;
    return mutex;
  }
};

using cpp_signal_local_locking = std::mutex;
using cpp_signal_recursive_local_locking = std::recursive_mutex;

class cpp_signal_util
{
public:
  cpp_signal_util() = delete;

  /********************* slot ********************/
  using slot_key = std::pair<std::uintptr_t, std::uintptr_t>;

  template <typename TReturn> class slot;
  template <typename TReturn, typename... TArgs>
  class slot<TReturn(TArgs...)>
  {
  public:
    using result_type = TReturn;
    using object = void*;
    using function = result_type(*)(void*, TArgs...);

    slot(object obj, function fun)
      : obj_(obj)
      , fun_(fun)
    { }

    slot(const slot_key& key)
      : obj_(reinterpret_cast<object>(key.first))
      , fun_(reinterpret_cast<function>(key.second))
    { }

    ~slot()
    {
      obj_ = nullptr;
      fun_ = nullptr;
    }

    inline slot_key key() const
    {
      return make_key(obj_, fun_);
    }

    template<typename... TCallArgs>
    inline result_type operator()(TCallArgs&&... args)
    {
      call(std::forward<TCallArgs>(args)...);
    }

    template<typename... TCallArgs>
    inline result_type call(TCallArgs&&... args)
    {
      return (*fun_)(obj_, std::forward<TCallArgs>(args)...);
    }

    // callable pointer to object
    template<typename TObject>
    inline static slot_key bind(TObject* obj)
    {
      return make_key(
        obj,
        [](void* obj, TArgs... args)
      { return static_cast<TObject*>(obj)->operator()(std::forward<TArgs>(args)...); }
      );
    }

    // static/global function
    template<TReturn(*TFunction)(TArgs...)>
    inline static slot_key bind()
    {
      return make_key(
        nullptr,
        [](void* /* nullptr */, TArgs... args)
      { return (*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

    // member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline static slot_key bind(TObject* obj)
    {
      return make_key(
        obj,
        [](void* obj, TArgs... args)
      { return (static_cast<TObject*>(obj)->*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

    // const member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline static slot_key bind(TObject* obj)
    {
      return make_key(
        obj,
        [](void* obj, TArgs... args)
      { return (static_cast<TObject*>(obj)->*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

  private:
    inline static slot_key make_key(object obj, function fun)
    {
      return std::make_pair(reinterpret_cast<std::uintptr_t>(obj), reinterpret_cast<std::uintptr_t>(fun));
    }

    object obj_;
    function fun_;
  };
};

template<class TLockingPolicy = cpp_signal_no_locking>
class cpp_signal
{
public:
  using locking_policy = TLockingPolicy;

  cpp_signal() = delete;

  template<typename TReturn> class signal;

  /********************* slot_tracker ********************/
  class slot_tracker : protected locking_policy
  {
  private:
    template<typename TReturn> friend class signal;

    struct connected_slot
    {
      const cpp_signal_util::slot_key key;
      slot_tracker* tracker;
      const bool call;
    };
    using slots = std::forward_list<connected_slot>;

  public:
    slot_tracker() = default;
    virtual ~slot_tracker()
    {
      clear();
    }

    // make slot_tracker non-copyable
    slot_tracker(const slot_tracker&) = delete;
    slot_tracker& operator=(const slot_tracker&) = delete;

  protected:
    template<class TSlot, typename... TCallArgs>
    void call(TCallArgs&&... args)
    {
      std::lock_guard<locking_policy> lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
      }
    }

    template<class TInit, class TSlot, typename... TCallArgs>
    TInit accumulate(TInit&& init, TCallArgs&&... args)
    {
      static_assert(std::is_same<TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      std::lock_guard<locking_policy> lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        init = init + TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
      }

      return init;
    }

    template<class TInit, class TBinaryOperation, class TSlot, typename... TCallArgs>
    TInit accumulate_op(TInit&& init, TBinaryOperation&& binary_op, TCallArgs&&... args)
    {
      static_assert(std::is_same<TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      std::lock_guard<locking_policy> lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        init = binary_op(init, TSlot(slot.key).call(std::forward<TCallArgs>(args)...));
      }

      return init;
    }

    template<class TContainer, class TSlot, typename... TCallArgs>
    TContainer aggregate(TCallArgs&&... args)
    {
      static_assert(std::is_same<TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      TContainer container;
      auto iterator = std::inserter(container, container.end());

      std::lock_guard<locking_policy> lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        *iterator = TSlot(slot.key).call(std::forward<TCallArgs>(args)...);
      }

      return container;
    }

    template<class TCollector, class TSlot, typename... TCallArgs>
    void collect(TCollector&& collector, TCallArgs&&... args)
    {
      static_assert(std::is_same<TSlot::result_type, void>::value == false, "Cannot collect slot return values of type 'void'");

      std::lock_guard<locking_policy> lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        collector(TSlot(slot.key).call(std::forward<TCallArgs>(args)...));
      }
    }

    inline void add_to_call(const cpp_signal_util::slot_key& key, slot_tracker* tracker)
    {
      add(key, tracker, true);
    }

    inline void add_to_track(const cpp_signal_util::slot_key& key, slot_tracker* tracker)
    {
      add(key, tracker, false);
    }

    inline void add(const cpp_signal_util::slot_key& key, slot_tracker* tracker, bool call)
    {
      std::lock_guard<locking_policy> lock(*this);
      slots_.emplace_front(connected_slot{ key, tracker, call });
    }

    inline void remove(const cpp_signal_util::slot_key& key, slot_tracker* tracker)
    {
      std::lock_guard<locking_policy> lock(*this);
      slots_.remove_if([&key, tracker](const connected_slot& slot)
      {
        return slot.key == key && slot.tracker == tracker;
      });
    }

    void clear()
    {
      std::lock_guard<locking_policy> lock(*this);
      for (auto& slot : slots_)
      {
        if (slot.tracker != this)
          slot.tracker->remove(slot.key, this);
      }

      slots_.clear();
    }

    inline bool empty()
    {
      std::lock_guard<locking_policy> lock(*this);
      return slots_.empty();
    }

  private:
    slots slots_;
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
    ~signal() = default;

    template<typename... TEmitArgs>
    inline void operator()(TEmitArgs&&... args)
    {
      emit(std::forward<TEmitArgs>(args)...);
    }

    template<typename... TEmitArgs>
    inline void emit(TEmitArgs&&... args)
    {
      slot_tracker::call<connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, typename... TEmitArgs>
    inline TInit accumulate(TInit&& init, TEmitArgs&&... args)
    {
      return slot_tracker::accumulate<TInit, connected_slot>(std::forward<TInit>(init), std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, class TBinaryOperation, typename... TEmitArgs>
    inline TInit accumulate_op(TInit&& init, TBinaryOperation&& binary_op, TEmitArgs&&... args)
    {
      return slot_tracker::accumulate_op<TInit, TBinaryOperation, connected_slot>(std::forward<TInit>(init), std::forward<TBinaryOperation>(binary_op), std::forward<TEmitArgs>(args)...);
    }

    template<class TContainer, typename... TEmitArgs>
    inline TContainer aggregate(TEmitArgs&&... args)
    {
      return slot_tracker::aggregate<TContainer, connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<typename TCollector, typename... TEmitArgs>
    inline void collect(TCollector&& collector, TEmitArgs&&... args)
    {
      slot_tracker::collect<TCollector, connected_slot>(std::forward<TCollector>(collector), std::forward<TEmitArgs>(args)...);
    }

    // callable object
    template<typename TObject>
    inline void connect(TObject& callable)
    {
      connect<TObject>(std::addressof(callable));
    }

    template<typename TObject>
    inline void disconnect(TObject& callable)
    {
      disconnect<TObject>(std::addressof(callable));
    }

    // callable pointer to object
    template<typename TObject>
    inline void connect(TObject* callable)
    {
      add<TObject>(connected_slot::template bind<TObject>(callable), callable);
    }

    template<typename TObject>
    inline void disconnect(TObject* callable)
    {
      remove<TObject>(connected_slot::template bind<TObject>(callable), callable);
    }

    // static/global function
    template<TReturn(*TFunction)(TArgs...)>
    inline void connect()
    {
      slot_tracker::add_to_call(connected_slot::template bind<TFunction>(), this);
    }

    template<TReturn(*TFunction)(TArgs...)>
    inline void disconnect()
    {
      slot_tracker::remove(connected_slot::template bind<TFunction>(), this);
    }

    // member function from object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void connect(TObject& obj)
    {
      connect<TObject, TFunction>(std::addressof(obj));
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void disconnect(TObject& obj)
    {
      disconnect<TObject, TFunction>(std::addressof(obj));
    }

    // member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void connect(TObject* obj)
    {
      add<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline void disconnect(TObject* obj)
    {
      remove<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    // const member function from object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void connect(TObject& obj)
    {
      connect<TObject, TFunction>(std::addressof(obj));
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void disconnect(TObject& obj)
    {
      disconnect<TObject, TFunction>(std::addressof(obj));
    }

    // const member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void connect(TObject* obj)
    {
      add<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline void disconnect(TObject* obj)
    {
      remove<TObject>(connected_slot::template bind<TObject, TFunction>(obj), obj);
    }

  private:
    // connect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, typename TObject::slot_tracker* tracker)
    {
      slot_tracker::add_to_call(key, tracker);
      tracker->add_to_track(key, this);
    }

    // connect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, ...)
    {
      slot_tracker::add_to_call(key, this);
    }

    // disconnect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, typename TObject::slot_tracker* tracker)
    {
      slot_tracker::remove(key, tracker);
      tracker->remove(key, this);
    }

    // disconnect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, ...)
    {
      slot_tracker::remove(key, this);
    }
  };
};

#endif  // CPP_SIGNAL_H_
