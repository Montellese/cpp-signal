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

#include <chrono>
#include <thread>

#include <catch.hpp>

#include <cpp-signal-async.h>

class tracked_async_test_class : public cpp_signal_async<>::slot_tracker
{
public:
  tracked_async_test_class(unsigned int& void_value, unsigned int& int_value)
    : m_void(void_value)
    , m_int(int_value)
  { }
  ~tracked_async_test_class() noexcept override = default;

  unsigned int get_void() const { return m_void; }
  unsigned int get_int() const { return m_int; }

  inline void slot_void() { ++m_void; }
  inline void slot_int(int count) { m_int += count; }

private:
  unsigned int& m_void;
  unsigned int& m_int;
};

SCENARIO("async signals can be emitted without parameters", "[async]")
{
  GIVEN("An async signal and slot without parameters")
  {
    cpp_signal_async<>::signal<void()> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count]() { ++slot_count; };
    REQUIRE(slot_count == 0);

    signal.connect(slot);

    WHEN("the async signal is emitted without parameters")
    {
      auto future = signal.emit();

      THEN("the slot is called without parameters")
      {
        future.wait();

        REQUIRE(slot_count == 1);
      }
    }
  }
}

SCENARIO("async signals can be emitted with a single parameter", "[async]")
{
  GIVEN("An async signal and slot with a single parameter")
  {
    cpp_signal_async<>::signal<void(unsigned int)> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count](unsigned int count) { slot_count += count; };
    REQUIRE(slot_count == 0);

    signal += slot;

    WHEN("the async signal is emitted with a single parameter")
    {
      const unsigned int count = 5;

      auto future = signal.emit(count);

      THEN("the slot is called with the parameter")
      {
        future.wait();

        REQUIRE(slot_count == count);
      }
    }
  }
}

SCENARIO("async signals can be emitted with multiple parameters", "[async]")
{
  GIVEN("An async signal and slot with multiple parameters")
  {
    cpp_signal_async<>::signal<void(unsigned int, unsigned int)> signal;

    unsigned int slot_count = 0;
    auto slot = [&slot_count](unsigned int add, unsigned int subtract)
    {
      slot_count += add;
      slot_count -= subtract;
    };
    REQUIRE(slot_count == 0);

    signal.connect(slot);

    WHEN("the async signal is emitted with multiple parameters")
    {
      const unsigned int add = 5;
      const unsigned int subtract = 3;

      auto future = signal.emit(add, subtract);

      THEN("the slot is called with the parameters")
      {
        future.wait();

        REQUIRE(slot_count == (add - subtract));
      }
    }
  }
}

SCENARIO("async signals can be emitted with complex parameters", "[async]")
{
  GIVEN("An async signal and slot with complex parameters")
  {
    cpp_signal_async<>::signal<void(std::string, const std::vector<char>&)> signal;

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

    WHEN("the async signal is emitted with the complex parameters")
    {
      const std::string str = "hello";
      const std::vector<char> vec = { 'w', 'o', 'r', 'l', 'd' };

      auto future = signal.emit(str, vec);

      THEN("the slot is called with the complex parameters")
      {
        future.wait();

        REQUIRE(slot_str == str);
        REQUIRE(slot_vec == vec);
      }
    }
  }
}

SCENARIO("slots can be connected to an async signal", "[async]")
{
  GIVEN("An async signal and multiple slots")
  {
    cpp_signal_async<>::signal<void()> signal;

    unsigned int slot_one_count = 0;
    unsigned int slot_two_count = 0;

    auto slot_one = [&slot_one_count]() { ++slot_one_count; };
    auto slot_two = [&slot_two_count]() { ++slot_two_count; };

    REQUIRE(slot_one_count == 0);
    REQUIRE(slot_two_count == 0);

    WHEN("one slot is connected to the async signal")
    {
      signal.connect(slot_one);

      AND_WHEN("the async signal is emitted")
      {
        auto future = signal.emit();

        THEN("the connected slot is called and the unconnected slot isn't called")
        {
          future.wait();

          REQUIRE(slot_one_count == 1);
          REQUIRE(slot_two_count == 0);
        }
      }

      AND_WHEN("the other slot is connected to the async signal as well")
      {
        signal.connect(slot_two);

        AND_WHEN("the async signal is emitted")
        {
          auto future = signal.emit();

          THEN("both connected slots are called")
          {
            future.wait();

            REQUIRE(slot_one_count == 1);
            REQUIRE(slot_two_count == 1);
          }
        }
      }
    }
  }
}

SCENARIO("slots can be disconnected from an asycn signal", "[async]")
{
  GIVEN("An async signal and multiple connected slots")
  {
    cpp_signal_async<>::signal<void()> signal;

    unsigned int slot_one_count = 0;
    unsigned int slot_two_count = 0;
    unsigned int slot_three_count = 0;

    auto slot_one = [&slot_one_count]() { ++slot_one_count; };
    auto slot_two = [&slot_two_count]() { ++slot_two_count; };
    auto slot_three = [&slot_three_count]() { ++slot_three_count; };

    REQUIRE(slot_one_count == 0);
    REQUIRE(slot_two_count == 0);
    REQUIRE(slot_three_count == 0);

    signal.connect(slot_one);
    signal += slot_two;
    signal.connect(slot_three);

    WHEN("the async signal is emitted")
    {
      auto future = signal.emit();

      THEN("both connected slots are called")
      {
        future.wait();

        REQUIRE(slot_one_count == 1);
        REQUIRE(slot_two_count == 1);
        REQUIRE(slot_three_count == 1);
      }

      AND_WHEN("some of the slots are disconnected")
      {
        signal.disconnect(slot_one);
        signal -= slot_three;

        AND_WHEN("the async signal is emitted")
        {
          future = signal.emit();

          THEN("the still connected slot is still called and the disconnected slot isn't")
          {
            future.wait();

            REQUIRE(slot_one_count == 1);
            REQUIRE(slot_two_count == 2);
            REQUIRE(slot_three_count == 1);
          }
        }
      }
    }
  }
}

SCENARIO("async signals can't be modified while being emitted", "[async]")
{
  GIVEN("An async signal and a connected slot")
  {
    static const unsigned int signal_emitting = 0;
    static const unsigned int slot_called = 1;
    static const unsigned int slot_finished = 2;
    static const unsigned int slot_connected = 10;
    static const unsigned int slot_disconnected = 11;

    cpp_signal_async<>::signal<void()> signal;

    std::vector<unsigned int> actions;

    auto slot = [&actions]()
    {
      actions.push_back(slot_called);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      actions.push_back(slot_finished);
    };

    REQUIRE(actions.empty());

    signal.connect(slot);

    WHEN("the async signal is emitted and the slot is connected again")
    {
      actions.push_back(signal_emitting);
      auto future = signal.emit();

      signal.connect(slot);
      actions.push_back(slot_connected);

      THEN("the slot has been called and afterwards disconnected")
      {
        future.wait();

        REQUIRE(actions.size() == 4);
        REQUIRE(actions.at(0) == signal_emitting);
        REQUIRE(actions.at(1) == slot_called);
        REQUIRE(actions.at(2) == slot_finished);
        REQUIRE(actions.at(3) == slot_connected);
      }
    }

    WHEN("the async signal is emitted and the connected slot disconnected")
    {
      actions.push_back(signal_emitting);
      auto future = signal.emit();

      signal.disconnect(slot);
      actions.push_back(slot_disconnected);

      THEN("the slot has been called and afterwards disconnected")
      {
        future.wait();

        REQUIRE(actions.size() == 4);
        REQUIRE(actions.at(0) == signal_emitting);
        REQUIRE(actions.at(1) == slot_called);
        REQUIRE(actions.at(2) == slot_finished);
        REQUIRE(actions.at(3) == slot_disconnected);
      }
    }
  }
}

SCENARIO("tracked slots can be connected to an async signal", "[async]")
{
  GIVEN("An asyng signal and a connected tracked slot without parameters")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;
    tracked_async_test_class test(void_value, int_value);

    cpp_signal_async<>::signal<void()> signal;
    signal.connect<tracked_async_test_class, &tracked_async_test_class::slot_void>(test);

    REQUIRE(test.get_void() == 0);

    WHEN("the async signal is emitted")
    {
      auto future = signal.emit();

      THEN("the tracked slot is called")
      {
        future.wait();

        REQUIRE(test.get_void() == 1);
      }
    }

    WHEN("the tracked slot is disconnected")
    {
      signal.disconnect<tracked_async_test_class, &tracked_async_test_class::slot_void>(test);

      THEN("the tracked slot isn't called")
      {
        signal.emit().wait();

        REQUIRE(test.get_void() == 0);
      }
    }
  }

  GIVEN("An async signal and a connected tracked slot with an integer parameter")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;
    tracked_async_test_class test(void_value, int_value);

    cpp_signal_async<>::signal<void(int)> signal;
    signal.connect<tracked_async_test_class, &tracked_async_test_class::slot_int>(test);

    REQUIRE(test.get_int() == 0);

    WHEN("the async signal is emitted")
    {
      const int count = 5;

      auto future = signal.emit(count);

      THEN("the tracked slot is called")
      {
        future.wait();

        REQUIRE(test.get_int() == count);
      }
    }

    WHEN("the tracked slot is disconnected")
    {
      signal.disconnect<tracked_async_test_class, &tracked_async_test_class::slot_int>(test);

      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit(1).wait();

        REQUIRE(test.get_int() == 0);
      }
    }
  }
}

SCENARIO("tracked slots connected to an async signal are automatically disconnected on destruction", "[async]")
{
  GIVEN("An async signal and a connected tracked slot without parameters")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;

    cpp_signal_async<>::signal<void()> signal;

    // open an addition scope for auto-destruction of the tracked_async_test_class object
    {
      tracked_async_test_class test(void_value, int_value);
      signal.connect<tracked_async_test_class, &tracked_async_test_class::slot_void>(test);

      REQUIRE(void_value == 0);

      WHEN("the async signal is emitted")
      {
        auto future = signal.emit();

        THEN("the tracked slot is called")
        {
          future.wait();

          REQUIRE(void_value == 1);
        }
      }
    }

    WHEN("the tracked slot is destroyed")
    {
      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit().wait();

        REQUIRE(void_value == 0);
      }
    }
  }

  GIVEN("An async signal and a connected tracked slot with an integer parameter")
  {
    unsigned int void_value = 0;
    unsigned int int_value = 0;

    cpp_signal_async<>::signal<void(int)> signal;

    // open an addition scope for auto-destruction of the tracked_async_test_class object
    {
      tracked_async_test_class test(void_value, int_value);
      signal.connect<tracked_async_test_class, &tracked_async_test_class::slot_int>(test);

      REQUIRE(int_value == 0);

      WHEN("the async signal is emitted")
      {
        const int count = 5;

        auto future = signal.emit(count);

        THEN("the tracked slot is called")
        {
          future.wait();

          REQUIRE(int_value == count);
        }
      }
    }

    WHEN("the tracked slot is destroyed")
    {
      THEN("the destroyed tracked slot isn't called")
      {
        signal.emit(1).wait();

        REQUIRE(int_value == 0);
      }
    }
  }
}

SCENARIO("return values from slots called through an async signal can be accumulated", "[async]")
{
  GIVEN("An async signal and slots with a non-void return value")
  {
    cpp_signal_async<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the async signal is emitted")
    {
      const int init_value = 3;
      const int value = 5;
      auto future = signal.accumulate(init_value, value);

      THEN("the slots are called and the slots return values are accumulated")
      {
        future.wait();

        REQUIRE(slot_count > 0);

        REQUIRE(future.get() == init_value + slot_one(value) + slot_two(value));
      }
    }
  }

  GIVEN("An async signal and slots with a non-void return value")
  {
    cpp_signal_async<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the async signal is emitted")
    {
      const int init_value = 3;
      const int value = 5;
      auto future = signal.accumulate_op(init_value, std::minus<int>(), value);

      THEN("the slots are called and the slots return values are accumulated using the given binary operator")
      {
        future.wait();

        REQUIRE(slot_count > 0);

        REQUIRE(future.get() == std::minus<int>()(std::minus<int>()(init_value, slot_one(value)), slot_two(value)));
      }
    }
  }
}

SCENARIO("return values from slots called through an async signal can be aggregated", "[async]")
{
  GIVEN("An async signal and slots with a non-void return value")
  {
    cpp_signal_async<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    WHEN("the async signal is emitted")
    {
      const int value = 5;
      auto future = signal.aggregate<std::vector<int>>(value);

      THEN("the slots are called and the slots return values are aggregated")
      {
        future.wait();

        REQUIRE(slot_count > 0);

        std::vector<int> result { slot_two(value), slot_one(value) };
        REQUIRE(future.get() == result);
      }
    }
  }
}

SCENARIO("return values from slots called through an async signal can be collected", "[async]")
{
  GIVEN("An async signal and slots with a non-void return value")
  {
    cpp_signal_async<>::signal<int(int)> signal;

    unsigned int slot_count = 0;
    auto slot_one = [&slot_count](int value) -> int { ++slot_count; return value; };
    signal.connect(slot_one);
    auto slot_two = [&slot_count](int value) -> int { ++slot_count; return value * 2; };
    signal.connect(slot_two);

    unsigned int collected_count = 0;
    auto collector = [&collected_count](int value) { collected_count += value; };
    REQUIRE(collected_count == 0);

    WHEN("the async signal is emitted")
    {
      const int value = 5;
      auto future = signal.collect(collector, value);

      THEN("the slots are called and the slots return values are passed to the collector")
      {
        future.wait();

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
