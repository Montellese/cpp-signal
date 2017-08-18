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

SCENARIO("return values from slots can be accumulated", "[aggregation]")
{
  GIVEN("A signal and slots with a non-void return value")
  {
    cpp_signal<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the signal is emitted")
    {
      const int init_value = 3;
      int init = init_value;
      const int value = 5;
      const int accumulated = signal.accumulate(init, value);

      THEN("the slots are called and their return values are accumulated")
      {
        REQUIRE(slot_count > 0);
        REQUIRE(accumulated == init_value + slot_one(value) + slot_two(value));
      }
    }
  }

  GIVEN("A signal and slots with a non-void return value")
  {
    cpp_signal<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the signal is emitted")
    {
      const int init_value = 3;
      int init = init_value;
      const int value = 5;
      const int accumulated = signal.accumulate_op(init, std::minus<int>(), value);

      THEN("the slots are called and their return values are accumulated using the given binary operator")
      {
        REQUIRE(slot_count > 0);
        REQUIRE(accumulated == std::minus<int>()(std::minus<int>()(init_value, slot_one(value)), slot_two(value)));
      }
    }
  }
}

SCENARIO("return values from slots can be aggregated", "[aggregation]")
{
  GIVEN("A signal and slots with a non-void return value")
  {
    cpp_signal<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the signal is emitted")
    {
      const int value = 5;
      auto accumulated = signal.aggregate<std::vector<int>>(value);

      THEN("the slots are called and their return values are aggregated")
      {
        REQUIRE(slot_count > 0);

        std::vector<int> result { slot_two(value), slot_one(value) };
        REQUIRE(accumulated == result);
      }
    }
  }
}

SCENARIO("return values from slots can be collected", "[aggregation]")
{
  GIVEN("A signal and slots with a non-void return value")
  {
    cpp_signal<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    unsigned int collected_count = 0;
    auto collector = [&collected_count](int value) { collected_count += value; };
    REQUIRE(collected_count == 0);

    WHEN("the signal is emitted")
    {
      const int value = 5;
      signal.collect(collector, value);

      THEN("the slots are called and their return values are passed to the collector")
      {
        REQUIRE(slot_count > 0);

        // save the result as collected_count will be modified by the subsequent calls
        const unsigned int collected_count_final = collected_count;

        // manually call the collector and the slots
        collected_count = 0;
        collector(slot_one(value));
        collector(slot_two(value));

        REQUIRE(collected_count_final == collected_count);
      }
    }
  }
}
