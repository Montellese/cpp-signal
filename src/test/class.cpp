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

class test_class
{
public:
  test_class()
    : m_void_callable(0)
    , m_int_callable(0)
    , m_void(0)
    , m_int(0)
  { }
  ~test_class() = default;

  unsigned int get_void_callable() const { return m_void_callable; }
  unsigned int get_int_callable() const { return m_int_callable; }
  unsigned int get_void() const { return m_void; }
  unsigned int get_int() const { return m_int; }
  unsigned int get_void_static() const { return m_void_static; }
  unsigned int get_int_static() const { return m_int_static; }

  void reset_void_static() { m_void_static = 0; }
  void reset_int_static() { m_int_static = 0; }

  inline void operator()() { ++m_void_callable; }
  inline void operator()(int count) { m_int_callable += count; }

  inline void slot_void() { ++m_void; }
  inline void slot_int(int count) { m_int += count; }

  inline static void slot_void_static() { ++m_void_static; }
  inline static void slot_int_static(int count) { m_int_static += count; }

private:
  unsigned int m_void_callable;
  unsigned int m_int_callable;
  unsigned int m_void;
  unsigned int m_int;

  static unsigned int m_void_static;
  static unsigned int m_int_static;
};

unsigned int test_class::m_void_static = 0;
unsigned int test_class::m_int_static = 0;

SCENARIO("callable classes can be used as slots", "[class]")
{
  GIVEN("A signal and a callable class without parameters")
  {
    test_class test;

    cpp_signal<>::signal<void()> signal;
    signal.connect(test);

    REQUIRE(test.get_void_callable() == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the class's callable method is called")
      {
        REQUIRE(test.get_void_callable() == 1);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect(test);

      THEN("the class's callable method isn't called")
      {
        signal.emit();

        REQUIRE(test.get_void_callable() == 0);
      }
    }
  }

  GIVEN("A signal and a callable class with an integer parameter")
  {
    test_class test;

    cpp_signal<>::signal<void(int)> signal;
    signal.connect(test);

    REQUIRE(test.get_int_callable() == 0);

    WHEN("the signal is emitted")
    {
      unsigned int count = 5;

      signal.emit(count);

      THEN("the class's callable method is called")
      {
        REQUIRE(test.get_int_callable() == count);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect(test);

      THEN("the class's callable method isn't called")
      {
        signal.emit(1);

        REQUIRE(test.get_int_callable() == 0);
      }
    }
  }
}

SCENARIO("class methods can be used as slots", "[class]")
{
  GIVEN("A signal and a class method without parameters")
  {
    test_class test;

    cpp_signal<>::signal<void()> signal;
    signal.connect<test_class, &test_class::slot_void>(test);

    REQUIRE(test.get_void() == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the class method is called")
      {
        REQUIRE(test.get_void() == 1);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<test_class, &test_class::slot_void>(test);

      THEN("the class method isn't called")
      {
        signal.emit();

        REQUIRE(test.get_void() == 0);
      }
    }
  }

  GIVEN("A signal and a class method with an integer parameter")
  {
    test_class test;

    cpp_signal<>::signal<void(int)> signal;
    signal.connect<test_class, &test_class::slot_int>(test);

    REQUIRE(test.get_int() == 0);

    WHEN("the signal is emitted")
    {
      unsigned int count = 5;

      signal.emit(count);

      THEN("the class method is called")
      {
        REQUIRE(test.get_int() == count);
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<test_class, &test_class::slot_int>(test);

      THEN("the class method isn't called")
      {
        signal.emit(1);

        REQUIRE(test.get_int() == 0);
      }
    }
  }
}

SCENARIO("static class methods can be used as slots", "[class]")
{
  GIVEN("A signal and a static class method without parameters")
  {
    test_class test;

    cpp_signal<>::signal<void()> signal;
    signal.connect<&test_class::slot_void_static>();

    REQUIRE(test.get_void_static() == 0);

    WHEN("the signal is emitted")
    {
      signal.emit();

      THEN("the static class method is called")
      {
        REQUIRE(test.get_void_static() == 1);

        test.reset_void_static();
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<&test_class::slot_void_static>();

      THEN("the static class method isn't called")
      {
        signal.emit();

        REQUIRE(test.get_void_static() == 0);
      }
    }
  }

  GIVEN("A signal and a static class method with an integer parameter")
  {
    test_class test;

    cpp_signal<>::signal<void(int)> signal;
    signal.connect<&test_class::slot_int_static>();

    REQUIRE(test.get_int_static() == 0);

    WHEN("the signal is emitted")
    {
      unsigned int count = 5;

      signal.emit(count);

      THEN("the static class method is called")
      {
        REQUIRE(test.get_int_static() == count);

        test.reset_int_static();
      }
    }

    WHEN("the slot is disconnected")
    {
      signal.disconnect<&test_class::slot_int_static>();

      THEN("the static class method isn't called")
      {
        signal.emit(1);

        REQUIRE(test.get_int_static() == 0);
      }
    }
  }
}
