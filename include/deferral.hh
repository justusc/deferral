/*
 * MIT License
 *
 * Copyright (c) 2024 Justus Calvin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <exception>
#include <limits>
#include <type_traits>
#include <utility>

#if defined(__has_feature)
#define DEFERRAL_HAS_FEATURE(x) __has_feature(x)
#else
#define DEFERRAL_HAS_FEATURE(x) 0
#endif // defined(__has_feature)

// attribute hidden
#if defined(_MSC_VER)
#define DEFERRAL_VISIBILITY_HIDDEN
#elif defined(__GNUC__)
#define DEFERRAL_VISIBILITY_HIDDEN __attribute__((__visibility__("hidden")))
#else
#define DEFERRAL_VISIBILITY_HIDDEN
#endif // defined(_MSC_VER)

/**
 * DEFERRAL_ANONYMOUS_VARIABLE(str) introduces an identifier starting with
 * str and ending with a number that varies with the line.
 */
#ifndef DEFERRAL_ANONYMOUS_VARIABLE
#define DEFERRAL_CONCATENATE_IMPL(s1, s2) s1##s2
#define DEFERRAL_CONCATENATE(s1, s2)      DEFERRAL_CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
// Modular builds build each module with its own preprocessor state, meaning
// `__COUNTER__` no longer provides a unique number across a TU.  Instead of
// calling back to just `__LINE__`, use a mix of `__COUNTER__` and `__LINE__`
// to try provide as much uniqueness as possible.
#if DEFERRAL_HAS_FEATURE(modules)
#define DEFERRAL_ANONYMOUS_VARIABLE(str)                                                           \
  DEFERRAL_CONCATENATE(DEFERRAL_CONCATENATE(DEFERRAL_CONCATENATE(str, __COUNTER__), _), __LINE__)
#else
#define DEFERRAL_ANONYMOUS_VARIABLE(str) DEFERRAL_CONCATENATE(str, __COUNTER__)
#endif
#else
#define DEFERRAL_ANONYMOUS_VARIABLE(str) DEFERRAL_CONCATENATE(str, __LINE__)
#endif
#endif

// DEFERRAL_NODISCARD encourages the compiler to issue a warning if the return value is discarded.
#if !defined(DEFERRAL_NODISCARD)
#if defined(__clang__)
#if(__clang_major__ * 10 + __clang_minor__) >= 39 && __cplusplus >= 201703L
#define DEFERRAL_NODISCARD [[nodiscard]]
#else
#define DEFERRAL_NODISCARD __attribute__((__warn_unused_result__))
#endif
#elif defined(__GNUC__)
#if __GNUC__ >= 7 && __cplusplus >= 201703L
#define DEFERRAL_NODISCARD [[nodiscard]]
#else
#define DEFERRAL_NODISCARD __attribute__((__warn_unused_result__))
#endif
#elif defined(_MSC_VER)
#if _MSC_VER >= 1911 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#define DEFERRAL_NODISCARD [[nodiscard]]
#elif defined(_Check_return_)
#define DEFERRAL_NODISCARD _Check_return_
#else
#define DEFERRAL_NODISCARD
#endif
#else
#define DEFERRAL_NODISCARD
#endif
#endif

// DEFERRAL_MAYBE_UNUSED suppresses compiler warnings on unused entities, if any.
#if !defined(DEFERRAL_MAYBE_UNUSED)
#if defined(__clang__)
#if(__clang_major__ * 10 + __clang_minor__) >= 39 && __cplusplus >= 201703L
#define DEFERRAL_MAYBE_UNUSED [[maybe_unused]]
#else
#define DEFERRAL_MAYBE_UNUSED __attribute__((__unused__))
#endif
#elif defined(__GNUC__)
#if __GNUC__ >= 7 && __cplusplus >= 201703L
#define DEFERRAL_MAYBE_UNUSED [[maybe_unused]]
#else
#define DEFERRAL_MAYBE_UNUSED __attribute__((__unused__))
#endif
#elif defined(_MSC_VER)
#if _MSC_VER >= 1911 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#define DEFERRAL_MAYBE_UNUSED [[maybe_unused]]
#else
#define DEFERRAL_MAYBE_UNUSED __pragma(warning(suppress : 4100 4101 4189))
#endif
#else
#define DEFERRAL_MAYBE_UNUSED
#endif
#endif

namespace deferral {
namespace internal {

class OnExitNoCheckPolicy {
public:
  static constexpr bool expect_execute{true};
  static constexpr bool require_noexcept{false};

  static void release() noexcept {}
  static constexpr bool should_execute() noexcept { return true; }
}; // class OnExitNoCheckPolicy

class OnExitPolicy {
  bool active{true};

public:
  static constexpr bool expect_execute{true};
  static constexpr bool require_noexcept{false};

  void release() noexcept { active = false; }
  bool should_execute() const noexcept { return active; }
}; // class OnExitPolicy

class OnFailPolicy {
  int exception_count{std::uncaught_exceptions()};

public:
  static constexpr bool expect_execute{false};
  static constexpr bool require_noexcept{true};

  void release() noexcept { exception_count = std::numeric_limits<int>::max(); }
  bool should_execute() const noexcept { return exception_count < std::uncaught_exceptions(); }
}; // class OnFailPolicy

class OnSuccessPolicy {
  int exception_count{std::uncaught_exceptions()};

public:
  static constexpr bool expect_execute{true};
  static constexpr bool require_noexcept{true};

  void release() noexcept { exception_count = -1; }
  bool should_execute() const noexcept { return exception_count >= std::uncaught_exceptions(); }
}; // class OnSuccessPolicy

template <typename funcT, typename policyT>
class DEFERRAL_VISIBILITY_HIDDEN DeferBase : policyT {
private:
  using policy_t = policyT;
  using func_t   = typename std::decay<funcT>::type;

#if defined(__cpp_lib_is_invocable) && (__cpp_lib_is_invocable >= 201703L)
  static_assert(std::is_invocable_v<func_t>, "deferral function must be callable");
#endif // defined(__cpp_lib_is_invocable) && (__cpp_lib_is_invocable >= 201703L)

  func_t func;

  void* operator new(std::size_t) = delete;
  void operator delete(void*)     = delete;

  template <typename F>
  static constexpr bool is_nothrow_constructible =
      std::is_nothrow_constructible<func_t, typename std::decay<F>::type>::value ||
      std::is_nothrow_constructible<func_t, typename std::decay<F>::type&>::value;

public:
  /**
   * @brief Constructs a DeferExit object with the specified function.
   *
   * @param f The function to be executed.
   * @tparam F The type of the function.
   * @exception noexcept If the construction of the function object is noexcept.
   */
  template <typename F>
  explicit DeferBase(F&& f) noexcept(is_nothrow_constructible<F>) :
      policyT{}, func{std::move_if_noexcept(f)} {}

  /**
   * @brief Move constructs a DeferBase object from another DeferExit object.
   *
   * @param other The other DeferExit object to be moved from.
   * @exception noexcept If the move construction of the function object is noexcept.
   */
  DeferBase(DeferBase&& other) noexcept(is_nothrow_constructible<func_t>) :
      policy_t{std::move(other)}, func{std::forward<func_t>(other.func)} {
    other.release();
  }

  /**
   * @brief `DeferBase` object is not copy constructible.
   */
  DeferBase(const DeferBase&) = delete;

  /**
   * @brief Destructor.
   *
   * If the `DeferBase` object is active, it calls the stored function.
   */
  ~DeferBase() noexcept(noexcept(func())) {
    if(__builtin_expect(policy_t::should_execute(), policy_t::expect_execute)) { func(); }
  }

  /**
   * @brief `DeferBase` object is not copy assignable.
   */
  DeferBase& operator=(const DeferBase&) = delete;

  /**
   * @brief `DeferBase` object is move assignable.
   */
  DeferBase& operator=(DeferBase&&) = delete;

  /**
   * @brief Releases the scope_exit object.
   *
   * Sets the active state of the scope_exit object to false.
   */
  using policy_t::release;
}; // class DeferExit

} // namespace internal

// Add explicit classes for deduction guides 9if C++17 is available).

/**
 * @class DeferExit
 * @brief A class that executes a function when it goes out of scope.
 *
 * The DeferExit class provides a way to execute a function when the scope in which it is defined
 * ends. This can be useful for performing cleanup or releasing resources when an object goes out of
 * scope.
 *
 * @tparam funcT The type of the function to be executed.
 */
template <typename funcT>
struct DEFERRAL_NODISCARD DEFERRAL_VISIBILITY_HIDDEN DeferExit
    : internal::DeferBase<funcT, internal::OnExitPolicy> {
  using internal::DeferBase<funcT, internal::OnExitPolicy>::DeferBase;
}; // class DeferExit

/**
 * @brief A class that provides scope-based failure behavior.
 *
 * The `DeferFail` class is a derived class of `DeferExit` and provides a way to execute a function
 * or lambda when the scope is exited due to an exception. It is designed to be used in situations
 * where you want to ensure that a certain action is performed when the scope is exited due to an
 * exception, regardless of how the scope is exited (e.g., through normal execution or another
 * exception).
 *
 * @tparam funcT The type of the function or lambda to be executed.
 */
template <typename funcT>
struct DEFERRAL_NODISCARD DEFERRAL_VISIBILITY_HIDDEN DeferFail
    : internal::DeferBase<funcT, internal::OnFailPolicy> {
  using internal::DeferBase<funcT, internal::OnFailPolicy>::DeferBase;
}; // class DeferFail

/**
 * @brief A class that provides scope-based success behavior.
 *
 * The `DeferSuccess` class is a derived class of DeferExit and provides a way to execute a
 * function or lambda when the scope is successfully exited. It is designed to be used in situations
 * where you want to ensure that a certain action is performed when the scope is successfully
 * completed, regardless of how the scope is exited (e.g., through normal execution or an
 * exception).
 *
 * @tparam funcT The type of the function or lambda to be executed.
 */
template <typename funcT>
struct DEFERRAL_NODISCARD DEFERRAL_VISIBILITY_HIDDEN DeferSuccess
    : internal::DeferBase<funcT, internal::OnSuccessPolicy> {
  using internal::DeferBase<funcT, internal::OnSuccessPolicy>::DeferBase;
}; // class DeferSuccess

// Add deduction guide if C++17 is available.
#if __cplusplus >= 201703L

template <typename funcT>
DeferExit(funcT) -> DeferExit<funcT>;

template <typename funcT>
DeferFail(funcT) -> DeferFail<funcT>;

template <typename funcT>
DeferSuccess(funcT) -> DeferSuccess<funcT>;

#endif // __cplusplus >= 201703L

/**
 * @brief Creates a `DeferExit` object.
 *
 * @param f The function to be executed.
 * @return A `DeferExit` object with the specified function.
 * @tparam funcT The type of the function.
 */
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferExit<funcT> make_defer_exit(funcT&& f) noexcept(
    noexcept(DeferExit<funcT>{std::forward<funcT>(f)})) {
  return DeferExit<funcT>{std::forward<funcT>(f)};
}

/**
 * @brief Factory function to create a DeferFail object.
 *
 * @tparam funcT The type of the function or lambda to be executed.
 * @param f The function or lambda to be executed.
 * @param a A boolean value indicating whether the DeferFail object is active. Default is true.
 * @return A DeferFail object.
 */
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferFail<funcT> make_defer_fail(funcT&& f) noexcept(
    noexcept(DeferFail<funcT>{std::forward<funcT>(f)})) {
  return DeferFail<funcT>{std::forward<funcT>(f)};
}

/**
 * @brief Factory function to create a `DeferSuccess` object.
 *
 * @tparam funcT The type of the function or lambda to be executed.
 * @param f The function or lambda to be executed.
 * @param a A boolean value indicating whether the DeferSuccess object is active. Default is true.
 * @return A DeferSuccess object.
 */
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferSuccess<funcT> make_defer_success(funcT&& f) noexcept(
    noexcept(DeferSuccess<funcT>{std::forward<funcT>(f)})) {
  return DeferSuccess<funcT>{std::forward<funcT>(f)};
}

namespace internal {
// The following enums and `+` opererator are used for the macro `defer`, `defer_success`,
// `defer_fail` below.

enum class DeferOnExitNoCheck {};
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferBase<funcT, OnExitNoCheckPolicy> operator+(
    DeferOnExitNoCheck,
    funcT&& f) noexcept(noexcept(DeferBase<funcT, OnExitNoCheckPolicy>{std::forward<funcT>(f)})) {
  return DeferBase<funcT, OnExitNoCheckPolicy>{std::forward<funcT>(f)};
}

enum class DeferOnExit {};
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferExit<funcT> operator+(DeferOnExit, funcT&& f) noexcept(
    noexcept(make_defer_exit(std::forward<funcT>(f)))) {
  return make_defer_exit(std::forward<funcT>(f));
}

enum class DeferOnFail {};
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferFail<funcT> operator+(DeferOnFail, funcT&& f) noexcept(
    noexcept(make_defer_fail(std::forward<funcT>(f)))) {
  return make_defer_fail(std::forward<funcT>(f));
}

enum class DeferOnSuccess {};
template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferSuccess<funcT> operator+(DeferOnSuccess, funcT&& f) noexcept(
    noexcept(make_defer_success(std::forward<funcT>(f)))) {
  return make_defer_success(std::forward<funcT>(f));
}

} // namespace internal
} // namespace deferral

#if !defined(DEFERRAL_NO_MACROS)

/**
 * @brief Capture code that shall be run when the current scope exits.
 * @def DEFER_(name)
 *
 * The code within `DEFER_`'s braces shall execute as if the code was in the
 * destructor of an object instantiated at the point of `DEFER_`. A variable
 * name is required to be passed to `DEFER_`, and creates a DeferExit object
 * with the specified name.
 *
 * If you need to skip the cleanup code, you can call `release()` on the
 * DeferExit object.
 *
 * Variables used within `DEFER_` are captured by reference.
 *
 * Example usage:
 * @code
 * { // open scope
 *   some_resource_t resource;
 *   some_resource_init(resource);
 *
 *   // create a DeferExit object with variable name d
 *   DEFER_(d) { some_resource_fini(resource); };
 *
 *   if (fini_not_needed)
 *    d.release(); // the cleanup does not happen
 *
 *   if (!cond)
 *     throw 0; // the cleanup happens at end of the scope
 *   else
 *     return; // the cleanup happens at end of the scope
 *
 *   use_some_resource(resource); // may throw; cleanup will happen
 *  } // close scope
 * @endcode
 *
 * The code in the braces passed to `DEFER_` executes at the end of the
 * containing scope as if the code is the content of the destructor of an
 * object instantiated at the point of the `defer`, where the destructor
 * reference-captures all local variables it uses.
 *
 * The cleanup code - the code in the braces passed to `DEFER_` - always
 * executes at the end of the scope, regardless of whether the scope exits
 * normally or erroneously as if via the throw statement.
 *
 * @note If you do not need to release the DeferExit object, you can use `DEFER`
 * instead.
 *
 * @warning Suitable for coroutine functions only when the cleanup code does
 * not use captured references to thread-local objects. Recall that there is
 * no assumption that coroutines resume from co-await, co-yield, or co-return
 * in the same thread as the one in which they suspend. If you need to capture
 * thread-local objects, consider using
 *
 * @warning May not execute if the scope exits erroneously but stack unwinding
 * is skipped, or if the scope does not exit at all such as with std::abort or
 * setcontext, which fibers use.
 */
#define DEFER_(x) auto x = ::deferral::internal::DeferOnExit() + [&]()

/**
 * @brief Capture code that shall be run when the current scope exits.
 * @def DEFER
 *
 * Like `DEFER_`, but a variable name is implicitily created.
 *
 * Example usage:
 * @code
 * { // open scope
 *   some_resource_t resource;
 *   some_resource_init(resource);
 *   DEFER { some_resource_fini(resource); };
 *
 *   if (!cond)
 *     throw 0; // the cleanup happens at end of the scope
 *   else
 *     return; // the cleanup happens at end of the scope
 *
 *   use_some_resource(resource); // may throw; cleanup will happen
 *  } // close scope
 * @endcode
 *
 * The code in the braces passed to `DEFER` executes at the end of the
 * containing scope as if the code is the content of the destructor of an
 * object instantiated at the point of the `defer`, where the destructor
 * reference-captures all local variables it uses.
 *
 * The cleanup code - the code in the braces passed to `DEFER` - always
 * executes at the end of the scope, regardless of whether the scope exits
 * normally or erroneously as if via the throw statement.
 *
 * @warning Suitable for coroutine functions only when the cleanup code does
 * not use captured references to thread-local objects. Recall that there is
 * no assumption that coroutines resume from co-await, co-yield, or co-return
 * in the same thread as the one in which they suspend. If you need to capture
 * thread-local objects, consider using
 *
 * @warning May not execute if the scope exits erroneously but stack unwinding
 * is skipped, or if the scope does not exit at all such as with std::abort or
 * setcontext, which fibers use.
 */
#define DEFER                                                                                      \
  auto DEFERRAL_ANONYMOUS_VARIABLE(DEFERRAL_STATE) =                                               \
      ::deferral::internal::DeferOnExitNoCheck() + [&]()

/**
 * Capture code to run if the scope exits with an exception.
 * @def DEFER_FAIL_
 *
 * Like `DEFER_`, but only executes the code if the scope exited due to an
 * exception.
 *
 * May be useful in situations where the caller requests a resource where
 * initializations of the resource is multi-step and may fail.
 *
 * Example:
 * @code
 *   some_resource_t resource;
 *   some_resource_init(resource);
 *
 *   // create a DeferFail object with varible name d
 *   DEFER_FAIL_(d) { some_resource_fini(resource); };
 *
 *   if (fini_not_needed)
 *     d.release(); // the cleanup does not happen
 *
 *   if (do_throw)
 *     throw 0; // the cleanup happens at the end of the scope
 *   else
 *     return resource; // the cleanup does not happen
 * @endcode
 *
 * @warning Not suitable for coroutine functions.
 */
#define DEFER_FAIL_(x) auto x = ::deferral::internal::DeferOnFail() + [&]() noexcept

/**
 * @brief Capture code to run if the scope exits with an exception.
 * @def DEFER_FAIL
 *
 * Like `DEFER_FAIL_`, but a variable name is implicitily created.
 *
 * Example:
 * @code
 *   some_resource_t resource;
 *   some_resource_init(resource);
 *   DEFER_FAIL { some_resource_fini(resource); };
 *   if (do_throw)
 *     throw 0; // the cleanup happens at the end of the scope
 *   else
 *     return resource; // the cleanup does not happen
 * @endcode
 *
 * @warning Not suitable for coroutine functions.
 */
#define DEFER_FAIL                                                                                 \
  DEFERRAL_MAYBE_UNUSED DEFER_FAIL_(DEFERRAL_ANONYMOUS_VARIABLE(DEFERRAL_FAIL_STATE))

/**
 * @brief Capture code to run on scope exits without an exception.
 * @def DEFER_SUCCESS_
 *
 * Like `DEFER_`, but does not execute the code if the scope exited due to an
 * exception. In a sense, the opposite of `DEFER_SUCCESS_`.
 *
 * Example:
 * @code
 * some_resource_t resource;
 * some_resource_init(resource);
 * DEFER_SUCCESS_(d) {
 *   log_success();
 *   some_resource_fini(resource);
 * };
 *
 * if (fini_not_needed)
 *  d.release(); // the cleanup does not happen
 *
 * if (do_throw)
 *   throw 0; // the cleanup does not happen; log failure
 * else
 *   return; // the cleanup happens at the end of the scope; log success
 * @endcode
 *
 * @warning Not suitable for coroutine functions.
 */
#define DEFER_SUCCESS_(x) auto x = ::deferral::internal::DeferOnSuccess() + [&]() noexcept

/**
 * @brief Capture code to run on scope exits without an exception.
 * @def DEFER_SUCCESS
 *
 * Like `DEFER_SUCCESS_`, but a varibule name is implicitily created.
 *
 * Example:
 * @code
 * some_resource_t resource;
 * some_resource_init(resource);
 * DEFER_SUCCESS {
 *   log_success();
 *   some_resource_fini(resource);
 * };
 *
 * if (do_throw)
 *   throw 0; // the cleanup does not happen
 * else
 *   return; // the cleanup happens at the end of the scope; log success
 * @endcode
 *
 * @warning Not suitable for coroutine functions.
 */
#define DEFER_SUCCESS                                                                              \
  DEFERRAL_MAYBE_UNUSED DEFER_SUCCESS_(DEFERRAL_ANONYMOUS_VARIABLE(DEFERRAL_SUCCESS_STATE))

#if !defined(DEFERRAL_NO_KEYWORDS)

#define defer_(x)         DEFER_(x)
#define defer             DEFER
#define defer_fail_(x)    DEFER_FAIL_(x)
#define defer_fail        DEFER_FAIL
#define defer_success_(x) DEFER_SUCCESS_(x)
#define defer_success     DEFER_SUCCESS

#endif // !defined(DEFERRAL_NO_KEYWORDS)
#endif // !defined(DEFERRAL_NO_MACROS)
