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
