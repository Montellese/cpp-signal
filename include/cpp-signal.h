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

#include <type_traits>
#include <utility>

#include <cpp-signal-base.h>
#include <cpp-signal-locking.h>
#include <cpp-signal-util.h>

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
