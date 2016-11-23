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

SCENARIO("signals can be chained", "[signal_chain]")
{
  GIVEN("Two signals and slot with a parameter")
  {
    cpp_signal<>::signal<void(unsigned int)> signal;
    cpp_signal<>::signal<void(unsigned int)> chained_signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count](unsigned int count) { slot_count += count; };
    REQUIRE(slot_count == 0);

    signal.connect(chained_signal);
    chained_signal.connect(slot);

    WHEN("the signal is emitted with a parameter")
    {
      const unsigned int count = 5;

      signal.emit(count);

      THEN("the chainged signal is called and calls the slot with the forwarded parameter")
      {
        REQUIRE(slot_count == count);
      }
    }
  }
}
