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

static unsigned int global_slot_count = 0;

static void global_slot()
{
  ++global_slot_count;
}

static unsigned int global_slot_int_count = 0;

static void global_slot_int(int count)
{
  global_slot_int_count += count;
}

SCENARIO("global methods can be used as slots", "[global]")
{
  GIVEN("A signal and a global method without parameters")
  {
    cpp_signal<>::signal<void()> signal;
    signal.connect<global_slot>();

    global_slot_count = 0;

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the global method is called")
      {
        REQUIRE(global_slot_count == 1);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<global_slot>();

      THEN("the global method isn't called")
      {
        signal.emit();

        REQUIRE(global_slot_count == 0);
      }
    }
  }

  GIVEN("A signal and a global method taking an integer parameter")
  {
    cpp_signal<>::signal<void(int)> signal;
    signal.connect<global_slot_int>();

    global_slot_int_count = 0;

    WHEN("the signal is emitted")
    {
      int count = 5;

      signal.emit(count);

      THEN("the global method is called")
      {
        REQUIRE(global_slot_int_count == count);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<global_slot_int>();

      THEN("the global method isn't called")
      {
        signal.emit(1);

        REQUIRE(global_slot_int_count == 0);
      }
    }
  }
}
