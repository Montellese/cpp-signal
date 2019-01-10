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

#include <catch.hpp>

#include <cpp-signal.h>

class copy_test_class : public cpp_signal<>::slot_tracker
{
public:
  copy_test_class()
    : m_int(0)
  { }
  ~copy_test_class() noexcept override = default;

  unsigned int get_int() const { return m_int; }
  inline void slot_int(int count) { m_int += count; }

private:
  unsigned int m_int;
};

SCENARIO("slot-tracking classes can be copied", "[copy]")
{
  GIVEN("A signal and two instances of a slot-tracking class")
  {
    copy_test_class slot;
    copy_test_class slot_copy;

    cpp_signal<>::signal<void(int)> signal;

    REQUIRE(slot.get_int() == 0);
    REQUIRE(slot_copy.get_int() == 0);

    const int value = 1;

    WHEN("connecting one slot to the signal and emitting the signal")
    {
      signal.connect<copy_test_class, &copy_test_class::slot_int>(slot);
      signal.emit(value);

      THEN("the connected (and tracked) slot is called and the unconnected slot isn't called")
      {
        REQUIRE(slot.get_int() == value);
        REQUIRE(slot_copy.get_int() == 0);
      }
    }

    WHEN("the connected slot is copied to the unconnected slot and the signal is emitted")
    {
      signal.connect<copy_test_class, &copy_test_class::slot_int>(slot);
      slot_copy = slot;

      signal.emit(value);

      THEN("the previously connected slot is called again and the copied slot is called as well")
      {
        REQUIRE(slot.get_int() == value);
        REQUIRE(slot_copy.get_int() == value);
      }
    }
  }
}

SCENARIO("signals can be copied", "[copy]")
{
  GIVEN("A signal and an untracked connected slot")
  {
    int slot_count = 0;
    auto lambda = [&slot_count]() { ++slot_count; };

    cpp_signal<>::signal<void()> signal;
    signal.connect(lambda);

    REQUIRE(slot_count == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the slot is called")
      {
        REQUIRE(slot_count == 1);
      }
    }

    WHEN("the signal is copied and both signals are emitted")
    {
      auto signal_copy = signal;

      signal.emit();
      signal_copy.emit();

      THEN("the slot is called again twice")
      {
        REQUIRE(slot_count == 2);
      }
    }
  }

  GIVEN("A signal and a tracked connected slot")
  {
    copy_test_class slot;

    cpp_signal<>::signal<void(int)> signal;
    signal.connect<copy_test_class, &copy_test_class::slot_int>(slot);

    REQUIRE(slot.get_int() == 0);

    const int value = 1;

    WHEN("the signal is emitted")
    {
      signal.emit(value);

      THEN("the slot is called")
      {
        REQUIRE(slot.get_int() == value);
      }
    }

    WHEN("the signal is copied and both signals are emitted")
    {
      auto signal_copy = signal;

      signal.emit(value);
      signal_copy.emit(value);

      THEN("the slot is called twice")
      {
        REQUIRE(slot.get_int() == 2 * value);
      }
    }
  }

  GIVEN("Two chained signals and a tracked connected slot")
  {
    copy_test_class slot;

    cpp_signal<>::signal<void(int)> signal;
    cpp_signal<>::signal<void(int)> chained_signal;
    signal.connect(chained_signal);
    chained_signal.connect<copy_test_class, &copy_test_class::slot_int>(slot);

    REQUIRE(slot.get_int() == 0);

    const int value = 1;

    WHEN("the signal is emitted")
    {
      signal.emit(value);

      THEN("the chainged signal is called and calls the slot with the forwarded parameter")
      {
        REQUIRE(slot.get_int() == value);
      }
    }

    WHEN("the chained signal is copied and the signal is emitted")
    {
      auto chained_signal_copy = chained_signal;

      signal.emit(value);

      THEN("the chained signals are called and the call the slot with the forwarded parameter")
      {
        REQUIRE(slot.get_int() == 2 * value);
      }
    }

    WHEN("the signal is copied and both signals are emitted")
    {
      auto signal_copy = signal;

      signal.emit(value);
      signal_copy.emit(value);

      THEN("the chained signals are called and the call the slot with the forwarded parameter")
      {
        REQUIRE(slot.get_int() == 2 * value);
      }
    }
  }
}