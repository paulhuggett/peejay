//===- include/peejay/details/uinteger.hpp ----------------*- mode: C++ -*-===//
//*        _       _                        *
//*  _   _(_)_ __ | |_ ___  __ _  ___ _ __  *
//* | | | | | '_ \| __/ _ \/ _` |/ _ \ '__| *
//* | |_| | | | | | ||  __/ (_| |  __/ |    *
//*  \__,_|_|_| |_|\__\___|\__, |\___|_|    *
//*                        |___/            *
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
#ifndef PEEJAY_DETAILS_UINTEGER_HPP
#define PEEJAY_DETAILS_UINTEGER_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace peejay {

template <std::unsigned_integral T> constexpr unsigned bits_required(T value) {
  if (value == 0U) {
    return 0U;
  }
  return 1U + bits_required(value >> 1);
}

/// \brief Yields the smallest unsigned integral type with at least \p N bits.
template <std::size_t N>
  requires(N <= 64)
struct uinteger {
  /// The type of an unsigned integral with at least \p N bits.
  using type = typename uinteger<N + 1>::type;
};
/// \brief A helper type for convenient use of uinteger<N>::type.
template <std::size_t N> using uinteger_t = typename uinteger<N>::type;
/// \brief Yields an unsigned integral type of 8 bits or more.
template <> struct uinteger<8> {
  /// Smallest unsigned integer type with width of at least 8 bits.
  using type = std::uint_least8_t;
};
/// \brief Yields an unsigned integral type of 16 bits or more.
template <> struct uinteger<16> {
  /// Smallest unsigned integer type with width of at least 16 bits.
  using type = std::uint_least16_t;
};
/// \brief Yields an unsigned integral type of 32 bits or more.
template <> struct uinteger<32> {
  /// Smallest unsigned integer type with width of at least 32 bits.
  using type = std::uint_least32_t;
};
/// \brief Yields an unsigned integral type of 64 bits or more.
template <> struct uinteger<64> {
  /// Smallest unsigned integer type with width of at least 64 bits.
  using type = std::uint_least64_t;
};

}  // end namespace peejay

#endif  // PEEJAY_DETAILS_UINTEGER_HPP
