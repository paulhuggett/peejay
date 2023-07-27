//===- include/peejay/almost_equal.hpp --------------------*- mode: C++ -*-===//
//*        _                     _                            _  *
//*   __ _| |_ __ ___   ___  ___| |_    ___  __ _ _   _  __ _| | *
//*  / _` | | '_ ` _ \ / _ \/ __| __|  / _ \/ _` | | | |/ _` | | *
//* | (_| | | | | | | | (_) \__ \ |_  |  __/ (_| | |_| | (_| | | *
//*  \__,_|_|_| |_| |_|\___/|___/\__|  \___|\__, |\__,_|\__,_|_| *
//*                                            |_|               *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_ALMOST_EQUAL_HPP
#define PEEJAY_ALMOST_EQUAL_HPP

#include <cmath>
#include <cstring>
#include <limits>

#include "peejay/uinteger.hpp"

namespace peejay {

namespace details {

template <typename FpType>
using raw_bits = uinteger_t<8 * sizeof (FpType)>;

template <typename FpType>
union fp {
  explicit constexpr fp (FpType const v) noexcept : value{v} {}
  FpType value;       ///< The floating-point number.
  raw_bits<FpType> bits;  ///< The raw bits.
};
template <typename FpType>
fp (FpType f) -> fp<FpType>;

template <typename FpType>
struct constants {
  /// The number of bits in the floating pointer number type.
  static constexpr std::size_t total_bits = 8 * sizeof (FpType);

  /// The number of bits making up the fractional part of the number.
  static constexpr std::size_t num_fraction_bits =
      std::numeric_limits<FpType>::digits - 1;

  /// The number of bits in the exponent of an instance of \p FpType.
  static constexpr std::size_t num_exponent_bits =
      total_bits - 1 - num_fraction_bits;

  static constexpr raw_bits<FpType> sign_mask =
      static_cast<raw_bits<FpType>> (1) << (total_bits - 1);
  static constexpr raw_bits<FpType> fraction_mask =
      ~static_cast<raw_bits<FpType>> (0) >> (num_exponent_bits + 1);
  static constexpr raw_bits<FpType> exponent_mask =
      ~(sign_mask | fraction_mask);
};

/// Returns a floating-point value's exponent bits.
template <typename FpType>
constexpr raw_bits<FpType> exponent_bits (FpType const f) noexcept {
  return fp{f}.bits & constants<FpType>::exponent_mask;
}
/// Returns a floating-point value's fraction bits.
template <typename FpType>
constexpr raw_bits<FpType> fraction_bits (FpType const f) noexcept {
  return fp{f}.bits & constants<FpType>::fraction_mask;
}

template <typename FpType>
static raw_bits<FpType> fp_to_biased (FpType const f) noexcept {
  auto const sam = fp{f}.bits;
  if ((constants<FpType>::sign_mask & sam) != 0) {
    return ~sam + 1;  // a negative number.
  }
  return constants<FpType>::sign_mask | sam;
}

/// Returns the distance between two floating point values.
template <typename FpType>
raw_bits<FpType> distance_between (FpType const f1, FpType const f2) noexcept {
  auto const b1 = fp_to_biased (f1);
  auto const b2 = fp_to_biased (f2);
  return b1 >= b2 ? b1 - b2 : b2 - b1;
}

}  // end namespace details

/// \brief Returns true if and only if \p lhs and \p rhs are at most \p MaxULPs
///   away from one another.
///
/// \tparam FpType A floating-point type.
/// \tparam MaxULPs  How many Units in the Last Place (ULPs) we want to
///   tolerate when comparing two numbers.  The larger the value, the more
///   error is allowed.
/// \param lhs  One of the two values to be compared.
/// \param rhs  One of the two values to be compared.
/// \returns True if \p rhs and \p lhs are "pretty much" equal, false otherwise.
template <typename FpType, unsigned MaxULPs = 4,
          typename = std::enable_if_t<std::is_floating_point_v<FpType>>>
bool almost_equal (FpType const lhs, FpType const rhs) {
  if (std::isnan (lhs) || std::isnan (rhs)) {
    return false;  // Comparison between NaNs is always false.
  }
  // If either input is +/-infinity, return true if they are exactly equal.
  if (std::isinf (lhs) || std::isinf (rhs)) {
    return std::memcmp (&lhs, &rhs, sizeof (lhs)) == 0;  // return lhs == rhs;
  }
  return details::distance_between (lhs, rhs) <= MaxULPs;
}

}  // end namespace peejay

#endif  // PEEJAY_ALMOST_EQUAL_HPP
