//===- include/peejay/details/portab.hpp ------------------*- mode: C++ -*-===//
//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_DETAILS__PORTAB_HPP
#define PEEJAY_DETAILS__PORTAB_HPP

#include <cassert>
#include <type_traits>
#include <utility>
#include <version>

namespace peejay {

// unreachable
// ~~~~~~~~~~~
#if defined(__cpp_lib_unreachable)
/// Executing unreachable() results in undefined behavior.
///
/// An implementation may, for example, optimize impossible code branches away
/// or trap to prevent further execution.
[[noreturn, maybe_unused]] inline void unreachable() {
  assert(false && "unreachable");
  std::unreachable();
}
#elif defined(__GNUC__)  // GCC 4.8+, Clang, Intel and other compilers
[[noreturn]] inline __attribute__((always_inline)) void unreachable() {
  assert(false && "unreachable");
  __builtin_unreachable();
}
#elif defined(_MSC_VER)
[[noreturn, maybe_unused]] __forceinline void unreachable() {
  assert(false && "unreachable");
  __assume(false);
}
#else
// Unknown compiler so no extension is used, Undefined behavior is still raised
// by an empty function body and the noreturn attribute.
[[noreturn, maybe_unused]] inline void unreachable() {
  assert(false && "unreachable");
}
#endif

/// Converts an enumeration value to its underlying type
///
/// \param e  The enumeration value to convert
/// \returns The integer value of the underlying type of Enum, converted from \p e.
template <typename Enum>
  requires std::is_enum_v<Enum>
[[nodiscard]] constexpr std::underlying_type_t<Enum> to_underlying(Enum const e) noexcept {
#if defined(__cpp_lib_to_underlying) && __cpp_lib_to_underlying > 202102L
  return std::to_underlying(e);
#else
  return static_cast<std::underlying_type_t<Enum>>(e);
#endif
}

}  // end namespace peejay

#endif  // PEEJAY_DETAILS__PORTAB_HPP
