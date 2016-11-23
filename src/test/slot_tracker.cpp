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

class tracked_test_class : public cpp_signal<>::slot_tracker
{
public:
  tracked_test_class(unsigned int& void_value, unsigned int& int_value)
    : m_void(void_value)
    , m_int(int_value)
  { }
  ~tracked_test_class() = default;

  unsigned int get_void() const { return m_void; }
  unsigned int get_int() const { return m_int; }

  inline void slot_void() { ++m_void; }
  inline void slot_int(int count) { m_int += count; }

private:
  unsigned int& m_void;
  unsigned int& m_int;
};

SCENARIO("tracked slots can be connected to a signal", "[slot_tracker]")
{
  GIVEN("A signal and a connected tracked slot without parameters")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;
    tracked_test_class test(void_value, int_value);

    cpp_signal<>::signal<void()> signal;
    signal.connect<tracked_test_class, &tracked_test_class::slot_void>(test);

    REQUIRE(test.get_void() == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the tracked slot is called")
      {
        REQUIRE(test.get_void() == 1);
      }
    }

    WHEN("the tracked slot is disconnected")
    {
      signal.disconnect<tracked_test_class, &tracked_test_class::slot_void>(test);

      THEN("the tracked slot isn't called")
      {
        signal.emit();

        REQUIRE(test.get_void() == 0);
      }
    }
  }

  GIVEN("A signal and a connected tracked slot with an integer parameter")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;
    tracked_test_class test(void_value, int_value);

    cpp_signal<>::signal<void(int)> signal;
    signal.connect<tracked_test_class, &tracked_test_class::slot_int>(test);

    REQUIRE(test.get_int() == 0);

    WHEN("the signal is emitted")
    {
      const int count = 5;

      signal.emit(count);

      THEN("the tracked slot is called")
      {
        REQUIRE(test.get_int() == count);
      }
    }

    WHEN("the tracked slot is disconnected")
    {
      signal.disconnect<tracked_test_class, &tracked_test_class::slot_int>(test);

      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit(1);

        REQUIRE(test.get_int() == 0);
      }
    }
  }
}

SCENARIO("tracked slots connected to a signal are automatically disconnected on destruction", "[slot_tracker]")
{
  GIVEN("A signal and a connected tracked slot without parameters")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;

    cpp_signal<>::signal<void()> signal;

    // open an addition scope for auto-destruction of the tracked_test_class object
    {
      tracked_test_class test(void_value, int_value);
      signal.connect<tracked_test_class, &tracked_test_class::slot_void>(test);

      REQUIRE(void_value == 0);

      WHEN("the signal is emitted")
      {
        signal.emit();

        THEN("the tracked slot is called")
        {
          REQUIRE(void_value == 1);
        }
      }
    }

    WHEN("the tracked slot is destroyed")
    {
      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit();

        REQUIRE(void_value == 0);
      }
    }
  }

  GIVEN("A signal and a connected tracked slot with an integer parameter")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;

    cpp_signal<>::signal<void(int)> signal;

    // open an addition scope for auto-destruction of the tracked_test_class object
    {
      tracked_test_class test(void_value, int_value);
      signal.connect<tracked_test_class, &tracked_test_class::slot_int>(test);

      REQUIRE(int_value == 0);

      WHEN("the signal is emitted")
      {
        const int count = 5;

        signal.emit(count);

        THEN("the tracked slot is called")
        {
          REQUIRE(int_value == count);
        }
      }
    }

    WHEN("the tracked slot is destroyed")
    {
      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit(1);

        REQUIRE(int_value == 0);
      }
    }
  }
}
