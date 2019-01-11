/*
 *  Copyright (C) Sascha Montellese
 */

#ifndef CPP_SIGNAL_UTIL_H_
#define CPP_SIGNAL_UTIL_H_

#include <cstdint>
#include <type_traits>
#include <utility>

class cpp_signal_util
{
public:
  cpp_signal_util() = delete;

  /********************* helpers ********************/
  template<typename T>
  using decay_t = typename std::decay<T>::type;

  template<typename T>
  using result_of_t = typename std::result_of<T>::type;

  /********************* slot ********************/
  using slot_key = std::pair<std::uintptr_t, std::uintptr_t>;

  template <typename TReturn> class slot;
  template <typename TReturn, typename... TArgs>
  class slot<TReturn(TArgs...)>
  {
  public:
    using result_type = TReturn;
    using object = void*;
    using function = result_type(*)(void*, TArgs...);

    slot(object obj, function fun) noexcept
      : obj_(obj)
      , fun_(fun)
    { }

    slot(const slot_key& key) noexcept
      : obj_(reinterpret_cast<object>(key.first))
      , fun_(reinterpret_cast<function>(key.second))
    { }

    slot(const slot& other) noexcept
      : obj_(other.obj_)
      , fun_(other.fun_)
    { }

    slot(slot&& other) noexcept
      : obj_(std::move(other.obj_))
      , fun_(std::move(other.fun_))
    { }

    ~slot() noexcept
    {
      obj_ = nullptr;
      fun_ = nullptr;
    }

    static slot_key copy_key(const slot_key& key, object copied_obj) noexcept
    {
      object obj = reinterpret_cast<object>(key.first);

      // nothing to do if there's no object related to the slot
      if (obj == nullptr)
        return key;

      return std::make_pair(reinterpret_cast<std::uintptr_t>(copied_obj), key.second);
    }

    inline slot_key key() const noexcept
    {
      return make_key(obj_, fun_);
    }

    template<typename... TCallArgs>
    inline result_type operator()(TCallArgs&&... args)
    {
      return call(std::forward<TCallArgs>(args)...);
    }

    template<typename... TCallArgs>
    inline result_type call(TCallArgs&&... args)
    {
      return (*fun_)(obj_, std::forward<TCallArgs>(args)...);
    }

    // callable pointer to object
    template<typename TObject>
    inline static slot_key bind(TObject* obj) noexcept
    {
      return make_key(
        obj,
        [](void* ob, TArgs... args)
      { return static_cast<TObject*>(ob)->operator()(std::forward<TArgs>(args)...); }
      );
    }

    // static/global function
    template<TReturn(*TFunction)(TArgs...)>
    inline static slot_key bind() noexcept
    {
      return make_key(
        nullptr,
        [](void* /* nullptr */, TArgs... args)
      { return (*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

    // member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...)>
    inline static slot_key bind(TObject* obj) noexcept
    {
      return make_key(
        obj,
        [](void* ob, TArgs... args)
      { return (static_cast<TObject*>(ob)->*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

    // const member function from pointer to object
    template<typename TObject, TReturn(TObject::*TFunction)(TArgs...) const>
    inline static slot_key bind(TObject* obj) noexcept
    {
      return make_key(
        obj,
        [](void* ob, TArgs... args)
      { return (static_cast<TObject*>(ob)->*TFunction)(std::forward<TArgs>(args)...); }
      );
    }

  private:
    inline static slot_key make_key(object obj, function fun) noexcept
    {
      return std::make_pair(reinterpret_cast<std::uintptr_t>(obj), reinterpret_cast<std::uintptr_t>(fun));
    }

    object obj_;
    function fun_;
  };
};

#endif  // CPP_SIGNAL_UTIL_H_
