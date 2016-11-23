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

SCENARIO("lambda methods can be used as slots", "[lambda]")
{
  GIVEN("A signal and a lambda method without parameters")
  {
    int slot_count = 0;
    auto lambda = [&slot_count]() { ++slot_count; };

    cpp_signal<>::signal<void()> signal;
    signal.connect(lambda);

    REQUIRE(slot_count == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the lambda method is called")
      {
        REQUIRE(slot_count == 1);

        slot_count = 0;
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect(lambda);

      THEN("the lambda method isn't called")
      {
        signal.emit();

        REQUIRE(slot_count == 0);
      }
    }
  }

  GIVEN("A signal and a lambda method taking an integer parameter")
  {
    int slot_count = 0;
    auto lambda = [&slot_count](int count) { slot_count += count; };

    cpp_signal<>::signal<void(int)> signal;
    signal.connect(lambda);

    REQUIRE(slot_count == 0);

    WHEN("the signal is emitted")
    {
      int count = 5;

      signal.emit(count);

      THEN("the lambda method is called")
      {
        REQUIRE(slot_count == count);

        slot_count = 0;
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect(lambda);

      THEN("the lambda method isn't called")
      {
        signal.emit(1);

        REQUIRE(slot_count == 0);
      }
    }
  }
}
