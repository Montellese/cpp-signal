# cpp-signal #
cpp-signal is a single header-only pure C++11 library providing signal and slot functionality. Using it is as easy as
```cpp
#include <cpp-signal.h>
...
auto slot = [](int value) -> int
{
  std::cout << "slot(" << value << << ")" << std::endl;

  return 2 * value;
}

cpp_signal<>::signal<int(int)> signal;
signal.connect(slot);

signal.emit(1);  // prints "slot(1)"
signal.emit(2);  // prints "slot(2)"

signal.disconnect(slot);

signal.emit(3);  // this will not call the slot
```
which will result in the following output on standard out:
```
slot(1)
slot(2)
```
----------
### Requirements ###
The only thing required to use cpp-signal is a C++11 compliant compiler.

#### Buildsystem ####
To be able to run the tests provided with cpp-signal [CMake](https://cmake.org/) 3.0 or newer is required. Run the following commands to build the tests and execute them:
```bash
# mkdir build
# cd build
# cmake ..
# cmake --build
# ctest -C Debug
```

### Features ###
 * [Type-safety](#type-safety)
 * [Managed connections](#managed-connections)
 * [Flexible slots](#flexible-slots)
   * Static (global) methods
   * Static class methods
   * Non-static class methods
   * Callables (including lambda expressions)
 * [Signal chaining](#signal-chaining)
 * [Copying](#copying)
 * [Slot result collection](#slot-result-collectCion)
 * [Threading policy](#threading-policy)

#### Type-safety ####
Thanks to C++11's variadic templates cpp-signal calls are completely type-safe. It is not possible to connect a slot with mismatching return type and or parameter list to a signal. Furthermore it is not possible to emit a signal with mismatching parameters.

#### Managed connections ####
With cpp-signal's `slot_tracker` class it is possible to automatically keep track of all methods in a class that have been connected to different signals as slots. It's as simple as deriving from `slot_tracker` which will take care of automatically disconnecting all connected slots when an object is destroyed.
```cpp
class tracked_class : public cpp_signal<>::slot_tracker
{
  ...
}
```

#### Flexible slots ####
cpp-signal is very flexible when it comes to connecting slots to a signal.

##### Static (global) methods #####
```cpp
static void global_slot(int value)
{
  std::cout << "global_slot(" << value << ")" << std::endl;
}
...
cpp_signal<>::signal<void(int)> signal;
signal.connect<global_slot>();

signal.emit(1);  // prints "global_slot(1)"
```

##### Static and non-static class methods #####
```cpp
class test_class
{
  public:
    static void static_slot(int value)
    {
      std::cout << "static_slot(" << value << ")" << std::endl;
    }

    static void slot(int value)
    {
      std::cout << "slot(" << value << ")" << std::endl;
    }
};

test_class test;

cpp_signal<>::signal<void(int)> signal;
signal.connect<&test_class::static_slot>();  // connects the static class method
signal.connect<test_class, &test_class::slot>(test);  // connects the non-static class method

signal.emit(1);  // prints "static_slot(1)" and "slot(1)"
```

##### Callables #####
```cpp
class test_class
{
  public:
    void operator()(int value)
    {
      std::cout << "test_class(" << value << ")" << std::endl;
    }
};

test_class test;

auto lambda = [](int value) { std::cout << "lambda(" << value << ")" << std::endl; };

cpp_signal<>::signal<void(int)> signal;
signal.connect(test);  // connects the callable test_class object
signal.connect(lambda);  // connects the callable lambda expression

signal.emit(1);  // prints "test_class(1)" and "lambda(1)"
```

#### Signal chaining ####
Due to the fact that any callable object can be used as a slot it is also possible to use cpp-signal's `signal` class as a slot therefore effectively chaining multiple signals together. There's nothing more to it.
```cpp
cpp_signal<>::signal<void(int)> signal;
cpp_signal<>::signal<void(int)> chained_signal;

auto slot = [](int value) { std::cout << "slot(" << value << << ")" << std::endl; }

signal.connect(slot);
signal.connect(chained_signal);
chained_signal.connect(slot);

signal.emit(1);
```
will result in `slot` being called twice: once as a direct slot of `signal` and once as a slot of `chained_signal`.

#### Copying ####
cpp-signal features full support for copying classes deriving from `slot_tracker` including `signal`:

 * when copying a tracked slot connected to a signal the copied slot will automatically also be connected to the same signal
 * when copying a signal with connected slots the copied signal will have the same slots connected

#### Slot result collection ####
cpp-signal does not only support signals and slots with return type `void` but any copyable return type. A signal emitted using `signal::emit()` will simply discard the returned value(s). But using the special `signal::emit_collect()` method with a matching collector (which can be any callable object) it is possible to access and possibly collect all the values returned by connected slots. As with the rest of cpp-signal any collector needs to match the signal's return type or it will not even compile.
```cpp
cpp_signal<>::signal<int(int)> signal;

auto slot_one = [](int value) -> int { return value; };
signal.connect(slot_one);
auto slot_two = [](int value) -> int { return value * 2; };
signal.connect(slot_two);

signal.emit(1);  // the values returned by the slots is discarded

int total = 0;
auto collector = [&total](int value) { total += value; }

signal.emit_collect(collector, 1);  // total == 3
```

#### Threading policy ####
When it comes to threading there are many different applications out there with different needs. Some applications run in a single thread and concurrency is not an issue. Other applications use multiple threads which can all potentially interact with signals and it is necessary to protect the signal's internal state. Because locking isn't free and has a negative impact on performance cpp-signal does not enforce locking but rather provides the possibility to choose the best fitting threading policy. This is achieved by specifying the threading policy in `cppsignal<TThreadingPolicy>`. cpp-signal comes with the following threading policies:

 * No locking (default): `cpp_signal_no_locking`
 * Global locking: `cpp_signal_global_locking`
 * Local locking: `cpp_signal_local_locking = std::mutex`
 * Local recursive locking: `cpp_signal_recursive_local_locking = std::recursive_mutex`

It is also possible to implement and use custom threading policies. All that is required by any threading policy is to implement the following methods (matching `std::mutex`):
```cpp
void lock();
void unlock();
```

### Performance ###
Based on the benchmarks used by [nano-signal-slot](https://github.com/NoAvailableAlias/signal-slot-benchmarks) cpp-signal ranks fourth right on [nano-signal-slot](https://github.com/NoAvailableAlias/nano-signal-slot)'s and Wink-Signals tails:

| Library | Construct | Destruct | Connect | Emission | Combined | Threaded | Total |
| ------- | ---------:| --------:| -------:| --------:| --------:| --------:|-----:|
|jeffomatic jl_signal|67404|17460|48014|37695|12649|0|183223|
|nano-signal-slot|87259|12767|10344|37560|5907|0|153836|
|Wink-Signals|82085|15923|10335|37565|6286|0|152193|
|cpp-signal|85122|12172|10481|37321|5934|0|151030|
|Yassi|82603|9621|6149|37554|3853|0|139779|
|amc522 Signal11|78651|9986|6529|37266|3996|0|136427|
|\* fr00b0 nod|74990|11400|7309|35454|4289|2667|136110|
|mwthinker Signal|74392|8844|7109|37401|4045|0|131791|
|pbhogan Signals|74262|10067|7113|35696|4460|0|131598|
|joanrieu signal11|66285|13515|8529|32561|4873|0|125764|
|\* Kosta signals-cpp|80267|9720|1859|20420|1419|11|113696|
|EvilTwin Observer|71103|6423|2834|28855|1932|0|111147|
|\* lsignal|58744|5658|2789|34431|1813|941|104375|
|supergrover sigslot|16461|2787|3555|37073|1481|0|61357|
|\* winglot Signals|11870|4367|4742|35282|1990|961|59212|

*\* library is designed to always be thread-safe*

### The Future ###
There are several ideas for improving and extending cpp-signal:

 * Asynchronous signal emitting
 * ???