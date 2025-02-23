//===- include/peejay/json/uinteger.hpp -------------------*- mode: C++ -*-===//
//*        _       _                        *
//*  _   _(_)_ __ | |_ ___  __ _  ___ _ __  *
//* | | | | | '_ \| __/ _ \/ _` |/ _ \ '__| *
//* | |_| | | | | | ||  __/ (_| |  __/ |    *
//*  \__,_|_|_| |_|\__\___|\__, |\___|_|    *
//*                        |___/            *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file uinteger.hpp
/// \brief Provides peejay::uinteger which yields the smallest unsigned
///   integral type with at least \p N bits
#ifndef PEEJAY_UINTEGER_HPP
#define PEEJAY_UINTEGER_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace peejay {

template <typename T,
          typename = typename std::enable_if_t<std::is_unsigned_v<T>>>
constexpr unsigned bits_required (T value) {
  if (value == 0U) {
    return 0U;
  }
  return 1U + bits_required (value >> 1);
}

/// \brief Yields the smallest unsigned integral type with at least \p N bits.
template <std::size_t N, typename = typename std::enable_if_t<(N <= 64)>>
struct uinteger {
  /// The type of an unsigned integral with at least \p N bits.
  using type = typename uinteger<N + 1>::type;
};
/// \brief A helper type for convenient use of uinteger<N>::type.
template <std::size_t N>
using uinteger_t = typename uinteger<N>::type;
/// \brief Yields an unsigned integral type of 8 bits or more.
template <>
struct uinteger<8> {
  /// Smallest unsigned integer type with width of at least 8 bits.
  using type = std::uint_least8_t;
};
/// \brief Yields an unsigned integral type of 16 bits or more.
template <>
struct uinteger<16> {
  /// Smallest unsigned integer type with width of at least 16 bits.
  using type = std::uint_least16_t;
};
/// \brief Yields an unsigned integral type of 32 bits or more.
template <>
struct uinteger<32> {
  /// Smallest unsigned integer type with width of at least 32 bits.
  using type = std::uint_least32_t;
};
/// \brief Yields an unsigned integral type of 64 bits or more.
template <>
struct uinteger<64> {
  /// Smallest unsigned integer type with width of at least 64 bits.
  using type = std::uint_least64_t;
};

}  // end namespace peejay

#endif  // PEEJAY_UINTEGER_HPP
