/*
 *  Copyright (C) Sascha Montellese
 */

#ifndef CPP_SIGNAL_LOCKING_H_
#define CPP_SIGNAL_LOCKING_H_

#include <mutex>

class cpp_signal_no_locking
{
public:
  cpp_signal_no_locking() = default;
  ~cpp_signal_no_locking() = default;

  inline void lock() { }
  inline void unlock() { }
};

class cpp_signal_global_locking
{
public:
  cpp_signal_global_locking() = default;
  ~cpp_signal_global_locking() = default;

  inline void lock()
  {
    mutex().lock();
  }

  inline void unlock()
  {
    mutex().unlock();
  }

private:
  static std::mutex& mutex()
  {
    static std::mutex mutex;
    return mutex;
  }
};

using cpp_signal_local_locking = std::mutex;
using cpp_signal_recursive_local_locking = std::recursive_mutex;

#endif  // CPP_SIGNAL_LOCKING_H_
