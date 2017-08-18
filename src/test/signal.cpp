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

SCENARIO("signals can be emitted without parameters", "[signal]")
{
  GIVEN("A signal and slot without parameters")
  {
    cpp_signal<>::signal<void()> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count]() { ++slot_count; };
    REQUIRE(slot_count == 0);

    signal.connect(slot);

    WHEN("the signal is emitted without parameters")
    {
      signal.emit();

      THEN("the slot is called without parameters")
      {
        REQUIRE(slot_count == 1);
      }
    }
  }
}

SCENARIO("signals can be emitted with a single parameter", "[signal]")
{
  GIVEN("A signal and slot with a single parameter")
  {
    cpp_signal<>::signal<void(unsigned int)> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count](unsigned int count) { slot_count += count; };
    REQUIRE(slot_count == 0);

    signal.connect(slot);

    WHEN("the signal is emitted with a single parameter")
    {
      const unsigned int count = 5;

      signal.emit(count);

      THEN("the slot is called with the parameter")
      {
        REQUIRE(slot_count == count);
      }
    }
  }
}

SCENARIO("signals can be emitted with multiple parameters", "[signal]")
{
  GIVEN("A signal and slot with multiple parameters")
  {
    cpp_signal<>::signal<void(unsigned int, unsigned int)> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count](unsigned int add, unsigned int subtract)
    {
      slot_count += add;
      slot_count -= subtract;
    };
    REQUIRE(slot_count == 0);

    signal.connect(slot);

    WHEN("the signal is emitted with multiple parameters")
    {
      const unsigned int add = 5;
      const unsigned int subtract = 3;

      signal.emit(add, subtract);

      THEN("the slot is called with the parameters")
      {
        REQUIRE(slot_count == (add - subtract));
      }
    }
  }
}

SCENARIO("signals can be emitted with complex parameters", "[signal]")
{
  GIVEN("A signal and slot with complex parameters")
  {
    cpp_signal<>::signal<void(std::string, const std::vector<char>&)> signal;

    std::string slot_str;
    std::vector<char> slot_vec;
    auto slot = [&slot_str, &slot_vec](std::string str, const std::vector<char>& vec)
    {
      slot_str = str;
      slot_vec = vec;
    };
    REQUIRE(slot_str.empty());
    REQUIRE(slot_vec.empty());

    signal.connect(slot);

    WHEN("the signal is emitted with the complex parameters")
    {
      const std::string str = "hello";
      const std::vector<char> vec = { 'w', 'o', 'r', 'l', 'd' };

      signal.emit(str, vec);

      THEN("the slot is called with the complex parameters")
      {
        REQUIRE(slot_str == str);
        REQUIRE(slot_vec == vec);
      }
    }
  }
}

SCENARIO("slots can be connected to a signal", "[signal]")
{
  GIVEN("A signal and multiple slots")
  {
    cpp_signal<>::signal<void()> signal;

    unsigned int slot_one_count = 0;
    unsigned int slot_two_count = 0;

    auto slot_one = [&slot_one_count]() { ++slot_one_count; };
    auto slot_two = [&slot_two_count]() { ++slot_two_count; };

    REQUIRE(slot_one_count == 0);
    REQUIRE(slot_two_count == 0);

    WHEN("one slot is connected to the signal")
    {
      signal.connect(slot_one);

      AND_WHEN("the signal is emitted")
      {
        signal.emit();

        THEN("the connected slot is called and the unconnected slot isn't called")
        {
          REQUIRE(slot_one_count == 1);
          REQUIRE(slot_two_count == 0);
        }
      }

      AND_WHEN("the other slot is connected to the signal as well")
      {
        signal.connect(slot_two);

        AND_WHEN("the signal is emitted")
        {
          signal.emit();

          THEN("both connected slots are called")
          {
            REQUIRE(slot_one_count == 1);
            REQUIRE(slot_two_count == 1);
          }
        }
      }
    }
  }
}

SCENARIO("slots can be disconnected from a signal", "[signal]")
{
  GIVEN("A signal and multiple connected slots")
  {
    cpp_signal<>::signal<void()> signal;

    unsigned int slot_one_count = 0;
    unsigned int slot_two_count = 0;

    auto slot_one = [&slot_one_count]() { ++slot_one_count; };
    auto slot_two = [&slot_two_count]() { ++slot_two_count; };

    REQUIRE(slot_one_count == 0);
    REQUIRE(slot_two_count == 0);

    signal.connect(slot_one);
    signal.connect(slot_two);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("both connected slots are called")
      {
        REQUIRE(slot_one_count == 1);
        REQUIRE(slot_two_count == 1);
      }

      AND_WHEN("one of the slots is disconnected")
      {
        signal.disconnect(slot_one);

        AND_WHEN("the signal is emitted")
        {
          signal.emit();

          THEN("the still connected slot is still called and the disconnected slot isn't")
          {
            REQUIRE(slot_one_count == 1);
            REQUIRE(slot_two_count == 2);
          }
        }
      }
    }
  }
}
