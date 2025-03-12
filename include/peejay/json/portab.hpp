//===- include/peejay/json/portab.hpp ---------------------*- mode: C++ -*-===//
//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file portab.hpp
/// \brief A collection of definitions which provide portability between
///   compilers and C++ language versions.
#ifndef PEEJAY_PORTAB_HPP
#define PEEJAY_PORTAB_HPP

#include <iterator>
#include <memory>
#include <new>
#include <utility>

#if __cplusplus >= 202002L
#define PEEJAY_CXX20 (1)
#elif defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
// MSVC does not set the value of __cplusplus correctly unless the
// /Zc:__cplusplus is supplied. We have to detect C++20 using its
// compiler-specific macros instead.
#define PEEJAY_CXX20 (1)
#else
#define PEEJAY_CXX20 (0)
#endif

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
#include <bit>
#endif

/// \brief Defined as 1 if the standard library's __cpp_lib_ranges macro is available and 0 otherwise.
/// \hideinitializer
#ifdef __cpp_lib_ranges
#define PEEJAY_CPP_LIB_RANGES_DEFINED (1)
#else
#define PEEJAY_CPP_LIB_RANGES_DEFINED (0)
#endif

/// \brief Tests for the availability of library support for C++ 20 ranges.
/// \hideinitializer
#define PEEJAY_HAVE_RANGES (PEEJAY_CPP_LIB_RANGES_DEFINED && __cpp_lib_ranges >= 201811L)

#if PEEJAY_CXX20 && defined(__has_cpp_attribute)
#if __has_cpp_attribute(unlikely) >= 201803L
#define PEEJAY_UNLIKELY_ATTRIBUTE [[unlikely]]
#endif
#if __has_cpp_attribute(no_unique_address)
#define PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE [[no_unique_address]]
#endif
#endif  // PEEJAY_CXX20 && defined(__has_cpp_attribute)
#ifndef PEEJAY_UNLIKELY_ATTRIBUTE
#define PEEJAY_UNLIKELY_ATTRIBUTE
#endif
#ifndef PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE
#define PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE
#endif

namespace peejay {

// to address
// ~~~~~~~~~~
#if defined(__cpp_lib_to_address)
template <typename T> [[nodiscard]] constexpr auto to_address(T&& p) noexcept {
  return std::to_address(std::forward<T>(p));
}
#else
// True if std::pointer_traits<T>::to_address is available.
template <typename T, typename = void> inline constexpr bool has_to_address = false;
template <typename T>
inline constexpr bool
    has_to_address<T, std::void_t<decltype(std::pointer_traits<T>::to_address(std::declval<T const&>()))>> = true;

template <typename T> [[nodiscard]] constexpr T* to_address(T* const p) noexcept {
  static_assert(!std::is_function_v<T>, "T must not be a function type");
  return p;
}
template <typename T> [[nodiscard]] constexpr auto to_address(T&& p) noexcept {
  using P = std::decay_t<T>;
  if constexpr (has_to_address<P>) {
    return std::pointer_traits<P>::to_address(std::forward<T>(p));
  } else {
    return to_address(p.operator->());
  }
}
#endif  // defined(__cpp_lib_to_address)

/// Converts an enumeration value to its underlying type
///
/// \param e  The enumeration value to convert
/// \returns The integer value of the underlying type of Enum, converted from \p e.
template <typename Enum>
  requires(std::is_enum_v<Enum>)
[[nodiscard]] constexpr std::underlying_type_t<Enum> to_underlying(Enum const e) noexcept {
#if defined(__cpp_lib_to_underlying) && __cpp_lib_to_underlying > 202102L
  return std::to_underlying(e);
#else
  return static_cast<std::underlying_type_t<Enum>>(e);
#endif
}

// pointer cast
// ~~~~~~~~~~~~
template <typename To, typename From> constexpr To* pointer_cast(From* const p) noexcept {
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
  return std::bit_cast<To*>(p);
#else
  return reinterpret_cast<To*>(p);
#endif
}

// forward iterator
// ~~~~~~~~~~~~~~~~
#if PEEJAY_HAVE_CONCEPTS
template <typename Iterator>
concept forward_iterator = std::forward_iterator<Iterator>;
#else
template <typename Iterator>
constexpr bool forward_iterator =
    std::is_convertible_v<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>;
#endif  // PEEJAY_HAVE_CONCEPTS

// input iterator
// ~~~~~~~~~~~~~~
#if PEEJAY_HAVE_CONCEPTS
template <typename Iterator>
concept input_iterator = std::input_iterator<Iterator>;
#else
template <typename Iterator>
constexpr bool input_iterator =
    std::is_convertible_v<typename std::iterator_traits<Iterator>::iterator_category, std::input_iterator_tag>;
#endif  // PEEJAY_HAVE_CONCEPTS

// unreachable
// ~~~~~~~~~~~
#if defined(__cpp_lib_unreachable)
/// Executing unreachable() results in undefined behavior.
///
/// An implementation may, for example, optimize impossible code branches away
/// or trap to prevent further execution.
[[noreturn, maybe_unused]] inline void unreachable() {
  std::unreachable();
}
#elif defined(__GNUC__)  // GCC 4.8+, Clang, Intel and other compilers
[[noreturn]] inline __attribute__((always_inline)) void unreachable() {
  __builtin_unreachable();
}
#elif defined(_MSC_VER)
[[noreturn, maybe_unused]] __forceinline void unreachable() {
  __assume(false);
}
#else
// Unknown compiler so no extension is used, Undefined behavior is still raised
// by an empty function body and the noreturn attribute.
[[noreturn, maybe_unused]] inline void unreachable() {
}
#endif

}  // end namespace peejay

#endif  // PEEJAY_PORTAB_HPP
