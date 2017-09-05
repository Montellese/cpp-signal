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
#include <type_traits>

#include <cpp-signal-locking.h>
#include <cpp-signal-util.h>

template<class TLockingPolicy = cpp_signal_no_locking>
class cpp_signal_base
{
public:
  using locking_policy = TLockingPolicy;

  cpp_signal_base() = delete;

  template<typename TSlotTracker, typename TReturn> class signal;

  /********************* slot_tracker ********************/
  class slot_tracker : protected locking_policy
  {
  private:
    template<typename TSlotTracker, typename TReturn> friend class signal;

  protected:
    struct tracked_slot
    {
      const cpp_signal_util::slot_key key;
      slot_tracker* tracker;
      const bool call;
    };
    using slots = std::forward_list<tracked_slot>;

  public:
    slot_tracker() = default;

    slot_tracker(const slot_tracker& other) noexcept
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
    virtual inline void add_to_call(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      add(key, tracker, true);
    }

    virtual inline void add_to_track(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      add(key, tracker, false);
    }

    virtual inline void add(const cpp_signal_util::slot_key& key, slot_tracker* tracker, bool call) noexcept
    {
      slots_.emplace_front(tracked_slot{ key, tracker, call });
    }

    virtual inline void remove(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      slots_.remove_if([&key, tracker](const tracked_slot& slot) -> bool
                       {
                         return slot.key == key && slot.tracker == tracker;
                       });
    }

    virtual void clear() noexcept
    {
      for (auto& slot : slots_)
      {
        if (slot.tracker != this)
          slot.tracker->remove(slot.key, this);
      }

      slots_.clear();
    }

    virtual inline bool empty() noexcept
    {
      return slots_.empty();
    }

    virtual void copy(const slot_tracker& other) noexcept
    {
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

    slots slots_;
  };

  /********************* signal ********************/
  template<typename TSlotTracker, typename TReturn, typename... TArgs>
  class signal<TSlotTracker, TReturn(TArgs...)> : public TSlotTracker
  {
    static_assert(std::is_base_of<slot_tracker, TSlotTracker>::value, "TSlotTracker must be derived from cpp_signal_base<>::slot_tracker");

  private:
    using connected_slot = cpp_signal_util::slot<TReturn(TArgs...)>;

  public:
    using result_type = TReturn;

    signal() = default;

    signal(const signal& other) noexcept
      : TSlotTracker(other)
    { }

    ~signal() noexcept override = default;

    // callable object
    template<typename TObject>
    inline signal& connect(TObject& callable) noexcept
    {
      return connect<TObject>(std::addressof(callable));
    }

    template<typename TObject>
    inline signal& operator+=(TObject& callable) noexcept
    {
      return connect<TObject>(callable);
    }

    template<typename TObject>
    inline signal& disconnect(TObject& callable) noexcept
    {
      return disconnect<TObject>(std::addressof(callable));
    }

    template<typename TObject>
    inline signal& operator-=(TObject& callable) noexcept
    {
      return disconnect<TObject>(callable);
    }

    // callable pointer to object
    template<typename TObject>
    inline signal& connect(TObject* callable) noexcept
    {
      add<TObject>(connected_slot::template bind<TObject>(callable), callable);

      return *this;
    }

    template<typename TObject>
    inline signal& operator+=(TObject* callable) noexcept
    {
      return connect<TObject>(callable);
    }

    template<typename TObject>
    inline signal& disconnect(TObject* callable) noexcept
    {
      remove<TObject>(connected_slot::template bind<TObject>(callable), callable);

      return *this;
    }

    template<typename TObject>
    inline signal& operator-=(TObject* callable) noexcept
    {
      return disconnect<TObject>(callable);
    }

    // static/global function
    template<TReturn(*TFunction)(TArgs...)>
    inline void connect() noexcept
    {
      TSlotTracker::add_to_call(connected_slot::template bind<TFunction>(), this);
    }

    template<TReturn(*TFunction)(TArgs...)>
    inline void disconnect() noexcept
    {
      TSlotTracker::remove(connected_slot::template bind<TFunction>(), this);
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

  protected:
    // connect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      TSlotTracker::add_to_call(key, tracker);
      tracker->add_to_track(key, this);
    }

    // connect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void add(const cpp_signal_util::slot_key& key, ...) noexcept
    {
      TSlotTracker::add_to_call(key, this);
    }

    // disconnect tracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, slot_tracker* tracker) noexcept
    {
      TSlotTracker::remove(key, tracker);
      tracker->remove(key, this);
    }

    // disconnect untracked slot (relying on SFINAE)
    template <typename TObject>
    inline void remove(const cpp_signal_util::slot_key& key, ...) noexcept
    {
      TSlotTracker::remove(key, this);
    }
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
  class slot_tracker : public cpp_signal_base<locking_policy>::slot_tracker
  {
  private:
    template<typename TReturn> friend class signal;

    using slot_tracker_base = typename cpp_signal_base<locking_policy>::slot_tracker;
    using slot_tracker_base::slots_;

    using lock_guard = std::lock_guard<locking_policy>;

  public:
    slot_tracker() = default;

    slot_tracker(const slot_tracker& other) noexcept
      : slot_tracker_base(other)
    { }

    ~slot_tracker() noexcept override = default;

    slot_tracker& operator=(const slot_tracker& other) noexcept
    {
      copy(other);

      return *this;
    }

  protected:
    template<class TSlot, typename... TCallArgs>
    void call(TCallArgs&&... args)
    {
      lock_guard lock(*this);
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
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      lock_guard lock(*this);
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
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      lock_guard lock(*this);
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
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot accumulate slot return values of type 'void'");

      TContainer container;
      auto iterator = std::inserter(container, container.end());

      lock_guard lock(*this);
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
      static_assert(std::is_same<typename TSlot::result_type, void>::value == false, "Cannot collect slot return values of type 'void'");

      lock_guard lock(*this);
      for (const auto& slot : slots_)
      {
        if (!slot.call)
          continue;

        collector(TSlot(slot.key).call(std::forward<TCallArgs>(args)...));
      }
    }

    inline void add(const cpp_signal_util::slot_key& key, slot_tracker_base* tracker, bool call) noexcept override
    {
      lock_guard lock(*this);
      slot_tracker_base::add(key, tracker, call);
    }

    inline void remove(const cpp_signal_util::slot_key& key, slot_tracker_base* tracker) noexcept override
    {
      lock_guard lock(*this);
      slot_tracker_base::remove(key, tracker);
    }

    void clear() noexcept override
    {
      lock_guard lock(*this);
      slot_tracker_base::clear();
    }

    inline bool empty() noexcept override
    {
      lock_guard lock(*this);
      return slot_tracker_base::empty();
    }

  private:
    void copy(const slot_tracker_base& other) noexcept override
    {
      lock_guard lock(*this);
      slot_tracker_base::copy(other);
    }
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
    inline void operator()(TEmitArgs&&... args)
    {
      emit(std::forward<TEmitArgs>(args)...);
    }

    template<typename... TEmitArgs>
    inline void emit(TEmitArgs&&... args)
    {
      slot_tracker::template call<connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, typename... TEmitArgs>
    inline TInit accumulate(TInit&& init, TEmitArgs&&... args)
    {
      return slot_tracker::template accumulate<TInit, connected_slot>(std::forward<TInit>(init), std::forward<TEmitArgs>(args)...);
    }

    template<class TInit, class TBinaryOperation, typename... TEmitArgs>
    inline TInit accumulate_op(TInit&& init, TBinaryOperation&& binary_op, TEmitArgs&&... args)
    {
      return slot_tracker::template accumulate_op<TInit, TBinaryOperation, connected_slot>(std::forward<TInit>(init), std::forward<TBinaryOperation>(binary_op), std::forward<TEmitArgs>(args)...);
    }

    template<class TContainer, typename... TEmitArgs>
    inline TContainer aggregate(TEmitArgs&&... args)
    {
      return slot_tracker::template aggregate<TContainer, connected_slot>(std::forward<TEmitArgs>(args)...);
    }

    template<typename TCollector, typename... TEmitArgs>
    inline void collect(TCollector&& collector, TEmitArgs&&... args)
    {
      slot_tracker::template collect<TCollector, connected_slot>(std::forward<TCollector>(collector), std::forward<TEmitArgs>(args)...);
    }
  };
};

#endif  // CPP_SIGNAL_H_
