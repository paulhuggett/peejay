//===- include/peejay/details/number_accumulator.hpp ------*- mode: C++ -*-===//
//*                 _                              *
//*  _ _ _  _ _ __ | |__  ___ _ _                  *
//* | ' \ || | '  \| '_ \/ -_) '_|                 *
//* |_||_\_,_|_|_|_|_.__/\___|_|                   *
//*                                                *
//*                            _      _            *
//*  __ _ __ __ _  _ _ __ _  _| |__ _| |_ ___ _ _  *
//* / _` / _/ _| || | '  \ || | / _` |  _/ _ \ '_| *
//* \__,_\__\__|\_,_|_|_|_\_,_|_\__,_|\__\___/_|   *
//*                                                *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
/// \file number_accumulator.hpp
/// \brief Provides peejay::details::number_accumulator, which accumulates the characters of a JSON number and
///   converts the result to an integer or floating-point value. Nothing here depends on the parser's backend.
#ifndef PEEJAY_DETAILS_NUMBER_ACCUMULATOR_HPP
#define PEEJAY_DETAILS_NUMBER_ACCUMULATOR_HPP

#include <algorithm>
#include <bit>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include "peejay/concepts.hpp"
#include "peejay/details/big_uint.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/details/states.hpp"
#include "peejay/error.hpp"

namespace peejay::details {

#if defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD == 0 && \
    !(defined(PEEJAY_NUMBER_PARSER_NO_FAST_PATH) && PEEJAY_NUMBER_PARSER_NO_FAST_PATH)
/// True if Clinger's fast path may be used to convert numbers to floating point.
constexpr auto number_parser_fast_path_available = true;
#else
/// True if Clinger's fast path may be used to convert numbers to floating point.
constexpr auto number_parser_fast_path_available = false;
#endif

/// The state transitions of the number grammar.
namespace number_grammar {

/// \param c  A UTF-8 code unit.
/// \returns True if \p c is a minus sign.
constexpr bool is_minus(char8_t const c) noexcept { return c == '-'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is the digit zero.
constexpr bool is_zero(char8_t const c) noexcept { return c == '0'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is a decimal point.
constexpr bool is_dot(char8_t const c) noexcept { return c == '.'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is a digit in the range [0, 9].
constexpr bool is_digit(char8_t const c) noexcept { return c >= '0' && c <= '9'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is a digit in the range [1, 9].
constexpr bool is_digit9(char8_t const c) noexcept { return c >= '1' && c <= '9'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is an exponent marker ('e' or 'E').
constexpr bool is_exp(char8_t const c) noexcept { return c == 'e' || c == 'E'; }
/// \param c  A UTF-8 code unit.
/// \returns True if \p c is a plus or minus sign.
constexpr bool is_sign(char8_t const c) noexcept { return c == '+' || c == '-'; }

// The result of next() is a single byte which holds either a state or an error. This works because every error
// value is smaller than every number state. error::string_too_long is the last enumerator of peejay::error.
static_assert(to_underlying(error::string_too_long) < to_underlying(state::number_start),
              "error values and number states must not overlap");

/// Decides on the next state for the number grammar.
///
/// \param s  The current state. Must be one of the number states.
/// \param c  The UTF-8 code unit to be consumed.
/// \returns If \p c continues the number, the underlying value of the next state. Otherwise the underlying value
///   of an error: error::none if \p c ends the number, or the error to be reported. Use is_state() to tell the two
///   apart.
// clang-format off
constexpr std::uint8_t next(state const s, char8_t const c) noexcept {
  auto const error_value = [](error const erc) constexpr {
    auto const result = to_underlying(erc);
    assert(result >= 0 && static_cast<unsigned>(result) < std::numeric_limits<std::uint8_t>::max());
    return static_cast<std::uint8_t>(result);
  };
  auto const isd = is_digit(c);
  using enum state;
  switch (s) {
  case number_minus:
    if (is_zero  (c)) { return to_underlying(number_zero      ); }
    if (is_digit9(c)) { return to_underlying(number_int_digits); }
    return error_value(error::expected_digits);
  case number_zero:
    if (is_dot(c)) { return to_underlying(number_dot     ); }
    if (is_exp(c)) { return to_underlying(number_exp_mark); }
    return error_value(error::none);
  case number_int_digits:
    if (isd      ) { return to_underlying(number_int_digits); }
    if (is_dot(c)) { return to_underlying(number_dot       ); }
    if (is_exp(c)) { return to_underlying(number_exp_mark  ); }
    return error_value(error::none);
  case number_dot:
    if (isd) { return to_underlying(number_frac_digits       ); }
    return error_value(error::unrecognized_token);
  case number_frac_digits:
    if (isd      ) { return to_underlying(number_frac_digits); }
    if (is_exp(c)) { return to_underlying(number_exp_mark   ); }
    return error_value(error::none);
  case number_exp_mark:
    if (isd       ) { return to_underlying(number_exp_digits); }
    if (is_sign(c)) { return to_underlying(number_exp_sign  ); }
    return error_value(error::unrecognized_token);
  case number_exp_sign:
    if (isd) { return to_underlying(number_exp_digits     ); }
    return error_value(error::unrecognized_token);
  case number_exp_digits:
    if (isd) { return to_underlying(number_exp_digits); }
    return error_value(error::none);
  case number_start:
    if (is_minus (c)) { return to_underlying(number_minus     ); }
    if (is_zero  (c)) { return to_underlying(number_zero      ); }
    if (is_digit9(c)) { return to_underlying(number_int_digits); }
    return error_value(error::unrecognized_token); // LCOV_EXCL_LINE
  default: unreachable();  // LCOV_EXCL_LINE: s is always one of the number states.
  }
}
// clang-format on

/// \param code  A value returned by next().
/// \returns True if \p code is a state, false if it is an error (error::none if the number ended).
constexpr bool is_state(std::uint8_t const code) noexcept {
  return code >= to_underlying(state::number_start);
}

/// \param st  The current state of the number grammar.
/// \returns True if the characters consumed so far form a complete, valid number.
[[nodiscard]] constexpr bool complete(state const st) noexcept {
  switch (st) {
  case state::number_zero:
  case state::number_int_digits:
  case state::number_frac_digits:
  case state::number_exp_digits: return true;
  default: return false;
  }
}

}  // end namespace number_grammar

/// \param max_mantissa  The largest integer that a floating-point type represents exactly.
/// \returns The largest k such that 5^k <= \p max_mantissa, i.e. 10^k is exact in the floating-point type.
constexpr std::int64_t get_exact_pow10(std::uint64_t max_mantissa) {
  std::uint64_t p = 1;
  std::int64_t k = 0;
  while (p <= max_mantissa / 5) {
    p *= 5;
    ++k;
  }
  return k;
}

/// Computes the number of 32-bit limbs needed by the working bignum that converts a decimal number to \p Float.
/// The bignum must be large enough for both directions: scaled-up division for tiny values and multiplication by a
/// power of five for huge ones.
///
/// \tparam Float  The floating-point type.
/// \param underflow_decimal  A decimal exponent below which every value certainly rounds to zero.
/// \param max_digits  The number of significant decimal digits that are retained.
/// \param mant_limbs  The number of limbs used to hold the retained digits.
/// \returns The number of limbs required.
template <typename Float>
constexpr std::size_t get_work_limbs(std::int64_t underflow_decimal, int max_digits, std::size_t mant_limbs) {
  using float_limits = std::numeric_limits<std::remove_cv_t<Float>>;
  constexpr std::int64_t mant_bits = float_limits::digits;

  std::int64_t const count_max = underflow_decimal + 1 + max_digits + 2;
  // 1193/512 is an approximation of log2(5) ≈ 2.32.
  std::int64_t const negative_bits = (count_max * 1193 + 511) / 512 + mant_bits + 3;
  std::int64_t const positive_bits = float_limits::max_exponent + 8;
  auto const floor_bits = static_cast<std::int64_t>(mant_limbs * 32);
  auto const bits = std::max(negative_bits > positive_bits ? negative_bits : positive_bits, floor_bits);
  return static_cast<std::size_t>((bits + 64 + 31) / 32);
}
/// \tparam Policies  The parser policies.
/// \returns The value of Policies::max_significant_digits if the policy provides it, otherwise 0 (meaning that the
///   default number of digits is retained).
template <policies Policies>
static constexpr int get_requested_digits() {
  if constexpr (requires { Policies::max_significant_digits; }) {
    static_assert(Policies::max_significant_digits <= std::numeric_limits<std::uint16_t>::max(),
                  "policies::max_significant_digits is too large");
    return static_cast<int>(Policies::max_significant_digits);
  } else {
    return 0;
  }
}


/// Facts about the integer types the matcher can deliver. Standard integers are
/// described through std::numeric_limits; the 128-bit extension types are described
/// explicitly.
template <typename T> struct integer_traits;

/// The common implementation of integer_traits<>.
///
/// \tparam Signed  The signed integer type.
/// \tparam Unsigned  The unsigned counterpart of \p Signed.
/// \tparam Magnitude  An unsigned type wide enough for |value| of \p Signed.
/// \tparam ValueBits  The number of value bits of \p Signed.
template <typename Signed, typename Unsigned, typename Magnitude, int ValueBits> struct integer_traits_base {
  /// The type handed to the backend (always signed)
  using signed_type = Signed;
  /// The unsigned counterpart of signed_type
  using unsigned_type = Unsigned;
  /// An unsigned type wide enough for |value| of signed_type
  using magnitude_type = Magnitude;

  /// The number of value bits of signed_type (63 for int64_t, 127 for __int128)
  static constexpr auto value_bits = ValueBits;
  /// The width of magnitude_type
  static constexpr auto magnitude_bits =
      static_cast<int>(sizeof(Magnitude) * static_cast<std::size_t>(std::numeric_limits<unsigned char>::digits));
  /// The largest value of signed_type, as a magnitude
  static constexpr Magnitude max_positive = (Magnitude{1} << ValueBits) - 1;
};

/// Standard integers of up to 64 bits (bool excluded).
template <typename T>
  requires(std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) <= sizeof(std::uint64_t))
struct integer_traits<T> : integer_traits_base<std::make_signed_t<T>, std::make_unsigned_t<T>, std::uint64_t,
                                               std::numeric_limits<std::make_signed_t<T>>::digits> {};

#ifdef __SIZEOF_INT128__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
/// 128-bit signed integers.
template <>
struct integer_traits<__int128> : integer_traits_base<__int128, unsigned __int128, unsigned __int128, 127> {};
/// 128-bit unsigned integers.
template <>
struct integer_traits<unsigned __int128> : integer_traits_base<__int128, unsigned __int128, unsigned __int128, 127> {};
#pragma GCC diagnostic pop
#endif  // __SIZEOF_INT128__

/// Integer types the matcher can deliver: standard integers of up to 64 bits
/// and, where the compiler provides them, __int128 / unsigned __int128.
template <typename T>
concept parser_integer = requires { typename integer_traits<std::remove_cv_t<T>>::signed_type; };

/// Computes the number of significant decimal digits the matcher retains: \p requested if that
/// is larger, otherwise the minimum. The minimum is never less than 38 and is
/// enough to hold every value of the signed result type exactly: 2^value_bits
/// has floor(value_bits * log10(2)) + 1 digits (30103/100000 slightly exceeds
/// log10(2)).
///
/// \param value_bits  The number of value bits of the signed result type.
/// \param requested  The number of digits requested by the policy, or 0 if none was requested.
/// \returns The number of significant decimal digits to retain.
[[nodiscard]] constexpr int retained_digits(int const value_bits, int const requested = 0) {
  int const needed = std::max((value_bits * 30103) / 100000 + 1, 38);
  return requested > needed ? requested : needed;
}

/// Computes the number of 32-bit limbs needed to hold any \p digits-digit decimal number
/// (3322/1000 slightly exceeds log2(10), so this rounds up).
///
/// \param digits  The number of decimal digits.
/// \returns The number of 32-bit limbs required.
[[nodiscard]] constexpr std::size_t limbs_for_digits(int const digits) {
  int const bits = (digits * 3322 + 999) / 1000;
  return static_cast<std::size_t>((bits + 31) / 32);
}

// The use of powers of 5 comes from the identity 10^x = 5^x × 2^x. A floating-point
// number is mantissa × 2^exponent, so the 2^x part costs nothing and only the 5^x part needs
// exact integer arithmetic. The parser keeps the significant digits as an integer W, so the value
// is W × 10^e = (W × 5^e) × 2^e.
//
// - Positive exponent (e ≥ 0): multiply W by 5^e in the bignum, giving an exact integer n, and the
//   result is n × 2^e. This is done in steps of 5^13, the largest power of five that fits in a
//   32-bit limb (1,220,703,125), plus one remainder step.
// - Negative exponent (e < 0): the value is W / 5^|e| × 2^-|e|. The parser first shifts W left so
//   the quotient keeps at least mant_bits + 3 bits, then divides by 5^13 chunks. If any division
//   leaves a non-zero remainder, a sticky flag records that the true value lies strictly above the
//   truncated quotient. That flag is what lets the final rounding to nearest-even be correct. The
//   shift amount is sized using log2(5) ≈ 2.32, which is why the code has the 1193/512
//   approximation.
// - Fast path: 10^k is exactly representable as a double only while 5^k fits in the mantissa,
//   i.e. k ≤ 22 for double (5^22 < 2^53). That is the source of exact_pow10, and it is why
//   Clinger’s single multiply or divide is exact for |e10| ≤ 22.

/// 5^13 is the largest power of five below 2^32; used to cut bignum passes.
static constexpr std::uint32_t pow5_chunk = 1'220'703'125U;
/// The exponent of pow5_chunk.
static constexpr std::int64_t pow5_chunk_exp = 13;

/// \param exponent  The power to which five is raised. Must be less than or equal to pow5_chunk_exp.
/// \returns 5^\p exponent.
constexpr std::uint32_t small_pow5(std::int64_t const exponent) noexcept {
  auto p = std::uint32_t{1};
  for (auto i = std::int64_t{0}; i < exponent; ++i) {
    p *= 5U;
  }
  return p;
}

/// Computes n *= 5^exponent.
///
/// \tparam Limbs  The number of limbs of \p n.
/// \param n  The value to be multiplied. Updated in place.
/// \param exponent  The power of five by which \p n is multiplied. Must not be negative.
/// \returns Non-zero if any step carried out of the top limb (that is, the result did not fit).
template <std::size_t Limbs>
constexpr std::uint32_t multiply_by_pow5(big_uint<Limbs>& n, std::int64_t exponent) noexcept {
  auto carry_out = std::uint32_t{0};
  for (; exponent >= pow5_chunk_exp; exponent -= pow5_chunk_exp) {
    carry_out |= n.mul_add(pow5_chunk, 0U);
  }
  if (exponent > 0) {
    carry_out |= n.mul_add(small_pow5(exponent), 0U);
  }
  return carry_out;
}

/// Computes n = floor(n / 5^exponent).
///
/// floor(floor(x/a)/b) == floor(x/(a*b)), and the final remainder is non-zero
/// iff any step's remainder was.
///
/// \tparam Limbs  The number of limbs of \p n.
/// \param n  The value to be divided. Updated in place.
/// \param exponent  The power of five by which \p n is divided. Must not be negative.
/// \returns True if the division was inexact.
template <std::size_t Limbs> constexpr bool divide_by_pow5(big_uint<Limbs>& n, std::int64_t exponent) noexcept {
  bool inexact = false;
  for (; exponent >= pow5_chunk_exp; exponent -= pow5_chunk_exp) {
    if (n.div_small(pow5_chunk) != 0) {
      inexact = true;
    }
  }
  if (exponent > 0 && n.div_small(small_pow5(exponent)) != 0) {
    inexact = true;
  }
  return inexact;
}

/// Radix-2 floating-point types with at most 64 mantissa bits.
template <typename T>
concept parser_float = std::numeric_limits<T>::is_specialized && !std::numeric_limits<T>::is_integer &&
                       std::numeric_limits<T>::radix == 2 && std::numeric_limits<T>::digits <= 64;

/// \brief Converts a decimal value, held as an integer significand and a power of ten, to the nearest value of a
///   floating-point type.
///
/// The conversion runs on exact integers in three steps:
///   - to_binary() rewrites w * 10^e10 as n * 2^p2, exactly, apart from a sticky flag that records non-zero bits
///     lost on the way;
///   - round_to_mantissa() keeps the top mant_bits bits and rounds half to even;
///   - assemble() builds the \p Float, or reports overflow.
///
/// The working bignum is a stack-local object that only exists while convert() runs. Its size is derived from
/// \p Float (about 40 bytes for float, 140 for double, 2 KiB for x87 long double).
///
/// This class depends only on the floating-point type and the number of retained digits, not on the parser's
/// backend. Parsers whose backends have the same policies therefore share a single copy of its code.
///
/// \tparam Float  The floating-point type of the result.
/// \tparam MaxDigits  The maximum number of significant decimal digits in the significand.
template <parser_float Float, int MaxDigits> class float_converter {
  using float_limits = std::numeric_limits<Float>;

public:
  /// The number of limbs used to hold the decimal significand.
  static constexpr std::size_t mant_limbs = limbs_for_digits(MaxDigits);

  /// The number of bits in the mantissa of \p Float.
  static constexpr std::int64_t mant_bits = float_limits::digits;
  /// The lsb of the smallest subnormal is 2^-subnormal_bits.
  static constexpr std::int64_t subnormal_bits = mant_bits - float_limits::min_exponent;
  /// A decimal exponent below which every value certainly rounds to zero. 30103/100000 slightly exceeds log10(2),
  /// so this errs on the safe side.
  static constexpr std::int64_t underflow_decimal = ((subnormal_bits + 1) * 30103) / 100000 + 1;
  /// A decimal exponent above which every value certainly overflows.
  static constexpr std::int64_t overflow_decimal = float_limits::max_exponent10 + 1;
  /// The largest integer that \p Float represents exactly: 2^mant_bits - 1.
  static constexpr std::uint64_t max_mantissa =
      mant_bits >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << (mant_bits % 64)) - 1;
  /// The largest k such that 5^k <= max_mantissa, i.e. 10^k is exact in \p Float.
  static constexpr std::int64_t exact_pow10 = get_exact_pow10(max_mantissa);
  /// Stack bignum capacity (bits -> limbs) for the worst case of both directions: scaled-up division for tiny
  /// values, 5^e multiplication for huge ones.
  static constexpr std::size_t work_limbs = get_work_limbs<Float>(underflow_decimal, MaxDigits, mant_limbs);

  /// Where the true value lies relative to the decimal that is converted. The conversion is exact for the decimal
  /// itself; `above` / `below` ask for the result for a value infinitesimally larger / smaller, which is how an
  /// open interval (W, W+1) * 10^e10 is bracketed.
  enum class tail_bias : std::uint8_t { none, above, below };

  /// \brief Converts a decimal value to the nearest floating-point value.
  ///
  /// Runs the three conversion steps in order: to_binary(), round_to_mantissa() and assemble(). The result is
  /// correctly rounded, to nearest with ties to even.
  ///
  /// \param w  The decimal significand.
  /// \param e10  The power of ten by which \p w is scaled.
  /// \param bias  Whether the value to be converted lies infinitesimally above or below w * 10^e10, or exactly on it.
  /// \returns The nearest \p Float to w * 10^e10, unsigned, or std::nullopt if it overflows.
  static std::optional<Float> convert(big_uint<mant_limbs> const& w, std::int64_t e10, tail_bias bias) noexcept;

private:
  /// The type of the working bignum.
  using work_uint = big_uint<work_limbs>;

  /// An exact value n * 2^p2, plus a flag if anything non-zero lies below n.
  struct binary_value {
    work_uint n;            ///< The significand.
    std::int64_t p2 = 0;    ///< The power of two by which n is scaled.
    bool sticky = false;    ///< Non-zero bits lie below n.
    bool below = false;     ///< The true value is infinitesimally less than n * 2^p2 (+ sticky).
    bool overflow = false;  ///< The product did not fit; work_limbs is sized so it cannot.
  };

  /// A rounded significand and the power of two that scales it.
  struct rounded_value {
    std::uint64_t mantissa = 0;  ///< The rounded significand.
    std::int64_t exponent = 0;   ///< The power of two by which mantissa is scaled.
    bool overflow = false;       ///< The value overflowed during conversion.
  };

  /// \brief Pre-scales a dividend so that division by a power of five keeps enough precision.
  ///
  /// Shifts \p n left so that floor(n / 5^count) still has at least mant_bits + 3 significant bits: enough for the
  /// mantissa, a round bit and a guard bit. The number of bits required is overestimated slightly (1193/512 is a
  /// little more than log2(5)), and nothing is shifted if \p n is already wide enough.
  ///
  /// \param n  The dividend. It is shifted left in place.
  /// \param count  The power of five by which \p n will be divided. Must not be negative.
  /// \returns The number of bits by which \p n was shifted left. The caller subtracts this from the binary exponent.
  static std::int64_t scale_up_for_division(work_uint& n, std::int64_t count) noexcept;

  /// \brief Converts a decimal value to an exact binary value.
  ///
  /// Rewrites w * 10^e10 as n * 2^p2 using 10^e = 5^e * 2^e. For a non-negative \p e10, \p w is multiplied by
  /// 5^e10, which is exact. For a negative one, \p w is first scaled up (see scale_up_for_division()) and then
  /// divided by 5^-e10; a non-zero remainder sets `sticky`. \p bias records whether the value to be rounded lies
  /// infinitesimally above or below w * 10^e10, so that the endpoints of an open interval can be rounded.
  ///
  /// \param w  The decimal significand.
  /// \param e10  The power of ten by which \p w is scaled.
  /// \param bias  Where the true value lies relative to w * 10^e10: tail_bias::above sets `sticky`, tail_bias::below
  ///              sets `below`, and tail_bias::none sets neither.
  /// \returns The binary value with its `sticky`, `below` and `overflow` flags set. `overflow` cannot be set if
  ///   work_limbs is large enough, which the constant is chosen to guarantee.
  static binary_value to_binary(big_uint<mant_limbs> const& w, std::int64_t e10, tail_bias bias) noexcept;

  /// \brief Calculates how many low bits must be discarded to form the floating-point mantissa.
  ///
  /// Enough bits are discarded to leave at most mant_bits. More are discarded if necessary so that no remaining bit
  /// is worth less than the least significant bit of the smallest subnormal, which gives gradual underflow.
  ///
  /// \param length  The number of significant bits in the binary significand.
  /// \param p2  The power of two by which the significand is scaled.
  /// \returns The number of low bits to discard. Never negative.
  [[nodiscard]] static std::int64_t discarded_bits(std::int64_t length, std::int64_t p2) noexcept;

  /// \brief Rounds a truncated mantissa to nearest, ties to even.
  ///
  /// Adds one unit in the last place when the discarded bits call for it: when they are more than half a unit,
  /// or exactly half and the mantissa is odd. If the mantissa was all ones (possible only when mant_bits is 64),
  /// it is renormalised and the exponent incremented. A value infinitesimally below an exact tie is not a tie, so
  /// it rounds down.
  ///
  /// \param r  The truncated mantissa and its exponent. Updated in place.
  /// \param round_bit  The most significant discarded bit (worth half a unit in the last place).
  /// \param sticky  true if any discarded bit below \p round_bit, or any part of the value already lost, is
  ///                non-zero.
  /// \param below  true if the true value lies infinitesimally below the value represented by the bits.
  static void round_half_even(rounded_value& r, bool round_bit, bool sticky, bool below) noexcept;

  /// \brief Rounds an exact binary value to a floating-point mantissa and exponent.
  ///
  /// Discards the low bits of v.n that do not fit (see discarded_bits()), folding all but the most significant of
  /// them into the sticky flag, then rounds the remainder with round_half_even().
  ///
  /// \param v  The exact binary value. Its significand is shifted right and its sticky flag updated in place.
  /// \returns The rounded mantissa and the power of two that scales it, with the overflow flag of \p v.
  static rounded_value round_to_mantissa(binary_value& v) noexcept;

  /// \brief Builds a floating-point value from a rounded mantissa and exponent.
  ///
  /// \param r  The rounded mantissa, its power of two, and an overflow flag.
  /// \returns mantissa * 2^exponent as \p Float, or std::nullopt if \p r is flagged as overflowing or its value
  ///   exceeds the largest finite \p Float.
  static std::optional<Float> assemble(rounded_value const& r) noexcept;
};

template <parser_float Float, int MaxDigits>
std::int64_t float_converter<Float, MaxDigits>::scale_up_for_division(work_uint& n, std::int64_t const count) noexcept {
  std::int64_t const need = (count * 1193 + 511) / 512 + mant_bits + 3;  // >= count*log2(5)+...
  auto const have = static_cast<std::int64_t>(n.bit_length());
  std::int64_t const up = need > have ? need - have : 0;
  n.shift_left(static_cast<std::size_t>(up));
  return up;
}

template <parser_float Float, int MaxDigits>
auto float_converter<Float, MaxDigits>::to_binary(big_uint<mant_limbs> const& w, std::int64_t const e10,
                                                  tail_bias const bias) noexcept -> binary_value {
  binary_value v;
  for (std::size_t i = 0; i < mant_limbs; ++i) {
    v.n.set_limb(i, w.limb(i));
  }
  v.sticky = bias == tail_bias::above;
  v.below = bias == tail_bias::below;
  if (e10 >= 0) {
    v.overflow = multiply_by_pow5(v.n, e10) != 0;
    v.p2 = e10;
  } else {
    std::int64_t const count = -e10;
    std::int64_t const up = float_converter::scale_up_for_division(v.n, count);
    if (divide_by_pow5(v.n, count)) {
      v.sticky = true;
    }
    v.p2 = -count - up;
  }
  return v;
}

template <parser_float Float, int MaxDigits>
std::int64_t float_converter<Float, MaxDigits>::discarded_bits(std::int64_t const length,
                                                               std::int64_t const p2) noexcept {
  std::int64_t shift = length - mant_bits;
  if (p2 + shift < -subnormal_bits) {
    shift = -subnormal_bits - p2;
  }
  return std::max(shift, std::int64_t{0});
}

template <parser_float Float, int MaxDigits>
void float_converter<Float, MaxDigits>::round_half_even(rounded_value& r, bool const round_bit, bool const sticky,
                                                        bool const below) noexcept {
  bool const round_up = below ? (round_bit && sticky) : (round_bit && (sticky || (r.mantissa & 1U) != 0));
  if (round_up) {
    if (r.mantissa == ~std::uint64_t{0}) {  // only when mant_bits == 64
      r.mantissa = std::uint64_t{1} << 63;
      ++r.exponent;
    } else {
      ++r.mantissa;
    }
  }
}

template <parser_float Float, int MaxDigits>
auto float_converter<Float, MaxDigits>::round_to_mantissa(binary_value& v) noexcept -> rounded_value {
  std::int64_t const shift = float_converter::discarded_bits(static_cast<std::int64_t>(v.n.bit_length()), v.p2);
  auto round_bit = false;
  if (shift > 0) {
    auto const s = static_cast<std::size_t>(shift);
    round_bit = v.n.bit(s - 1);
    v.sticky = v.sticky || v.n.any_below(s - 1);
    v.n.shift_right(s);
  }
  rounded_value r{.mantissa = v.n.low64(), .exponent = v.p2 + shift, .overflow = v.overflow};
  float_converter::round_half_even(r, round_bit, v.sticky, v.below);
  return r;
}

template <parser_float Float, int MaxDigits>
std::optional<Float> float_converter<Float, MaxDigits>::assemble(rounded_value const& r) noexcept {
  if (r.overflow) {
    return std::nullopt;  // LCOV_EXCL_LINE: work_limbs is sized so that this cannot happen.
  }
  if (r.mantissa == 0) {
    return Float{0};
  }
  if (static_cast<std::int64_t>(std::bit_width(r.mantissa)) + r.exponent > float_limits::max_exponent) {
    return std::nullopt;
  }
  return std::ldexp(static_cast<Float>(r.mantissa), static_cast<int>(r.exponent));
}

template <parser_float Float, int MaxDigits>
std::optional<Float> float_converter<Float, MaxDigits>::convert(big_uint<mant_limbs> const& w, std::int64_t const e10,
                                                                tail_bias const bias) noexcept {
  binary_value value = float_converter::to_binary(w, e10, bias);
  return float_converter::assemble(float_converter::round_to_mantissa(value));
}

/// \brief Accumulates the characters of a JSON number and converts the result to an integer or floating-point value.
///
/// An allocation-free, incremental (push) recognizer for the grammar:
///
///     number = '-'? ( '0' | digit9 {digit}* ) [ '.' {digit}+ ] [ ('E'|'e') ['+'|'-'] {digit}+ ]
///     digit  = '0' | digit9
///     digit9 = '1' | ... | '9'
///
/// The state of the grammar is held by the caller (it lives on the parser's stack) and passed to consume().
///
/// Retained state (about 40 bytes, 48 for \p MaxDigits of 39): the first \p MaxDigits significant decimal digits as
/// a multi-limb integer, a decimal scale, the exponent, a "non-zero digits were dropped" flag, and a few flags.
/// Nothing is allocated and nothing throws.
///
/// Compile-time option: define PEEJAY_NUMBER_PARSER_NO_FAST_PATH to a non-zero value to disable the
/// exact-IEEE-operation shortcut and always use the bignum path (the results are identical; the shortcut is merely
/// faster). The shortcut is also disabled unless FLT_EVAL_METHOD is 0.
///
/// This class depends only on the number of retained digits, not on the parser's backend. Parsers whose backends
/// have the same policies therefore share a single copy of its code.
///
/// \tparam MaxDigits  The maximum number of significant decimal digits that are retained (see retained_digits()).
template <int MaxDigits> class number_accumulator {
public:
  static_assert(MaxDigits > 0 && MaxDigits <= std::numeric_limits<std::uint16_t>::max(),
                "policies::max_significant_digits is too large");
  /// The maximum number of significant decimal digits that are retained.
  static constexpr int max_digits = MaxDigits;
  /// The number of limbs used to hold the retained digits.
  static constexpr std::size_t mant_limbs = limbs_for_digits(max_digits);

  /// \brief Feeds one character of the input to the accumulator.
  ///
  /// Advances the number grammar from state \p current. A character that continues the number updates the retained
  /// digits, sign, scale or exponent.
  ///
  /// \param current  The current state of the number grammar. Must be one of the number states.
  /// \param c  The next UTF-8 code unit of the input.
  /// \returns The value returned by number_grammar::next(): if \p c continues the number, the underlying value of
  ///   the next state. Otherwise the underlying value of an error: error::none if \p c ends the number (in which
  ///   case it is not consumed), or the error to be reported. Use number_grammar::is_state() to tell the two apart.
  std::uint8_t consume(state current, char8_t c) noexcept;

  /// \brief The fast path for the common case: a digit of the integer or fractional part.
  ///
  /// If \p current is state::number_int_digits or state::number_frac_digits and \p c is a digit, the digit is
  /// accumulated. This has exactly the same effect as consume() would (the state is unchanged) but is small enough
  /// to be inlined into the caller.
  ///
  /// \param current  The current state of the number grammar.
  /// \param c  The next UTF-8 code unit of the input.
  /// \returns True if \p c was accumulated, false if it must instead be passed to consume().
  PEEJAY_ALWAYS_INLINE bool append_digit(state const current, char8_t const c) noexcept {
    if (!number_grammar::is_digit(c)) {
      return false;
    }
    if (current == state::number_int_digits) {
      this->add_digit(c - '0', false);
      return true;
    }
    if (current == state::number_frac_digits) {
      this->add_digit(c - '0', true);
      return true;
    }
    return false;
  }

  /// \brief Returns the number as an integer if it is exactly representable as one.
  ///
  /// The number qualifies only if it was written as a plain integer: no '.', no exponent and no digits dropped
  /// beyond max_digits. Its magnitude must also fit the signed integer type, whose negative range is one larger
  /// than its positive range. "-0" yields 0.
  ///
  /// \tparam Integer  The integer type of the result. Its signed counterpart is returned.
  /// \returns The value, or std::nullopt if the number must be reported as floating point.
  template <parser_integer Integer>
  [[nodiscard]] std::optional<typename integer_traits<Integer>::signed_type> to_integer() const noexcept;

  /// \brief Converts the number to the nearest floating-point value.
  ///
  /// Values whose decimal magnitude is certainly too large are rejected and values that are certainly too small
  /// become a signed zero, both without any bignum arithmetic. If can_use_fast_path() allows it, the value is
  /// computed with a single, correctly rounded, floating-point operation (Clinger's fast path; see fast_path()).
  /// Otherwise the value is converted with float_converter::convert().
  ///
  /// Inputs with more than max_digits significant digits cannot be retained. The true value then lies in the open
  /// interval (W, W+1) * 10^e10. Both endpoints are converted, and the result is accepted only if they round to the
  /// same value: if a rounding boundary lies inside the interval, the correct result cannot be known and
  /// error::number_out_of_range is reported rather than guessing.
  ///
  /// \tparam Float  The floating-point type of the result.
  /// \returns The correctly rounded value, carrying the sign of the input (so "-0" yields negative zero), or
  ///   error::number_out_of_range if the value overflows or too many digits were dropped to determine the correct
  ///   rounding.
  template <parser_float Float> [[nodiscard]] std::optional<Float> to_floating() const noexcept;

private:
  /// The type used to count the significant digits held.
  using digit_count_type = std::conditional_t<max_digits <= 255U, std::uint8_t, std::uint16_t>;
  // The written exponent saturates here only to avoid overflow while reading an
  // absurdly long exponent. It cannot be a floating-point limit: the exponent
  // is combined with scale_, which grows with the number of mantissa digits
  // ("1" + a million zeros + "e-1000000" is exactly 1), so the cap must exceed
  // any input length that could ever be streamed. 10^17 keeps exp_ * 10 + 9
  // inside int64_t.
  static constexpr std::int64_t exp_cap = 100'000'000'000'000'000;

  /// \brief Accumulates one decimal digit of the mantissa.
  ///
  /// Leading zeros are not stored; a leading fractional zero only moves the decimal point (decrements `scale_`).
  /// While fewer than max_digits significant digits are held, the digit is appended to `mant_`. Each fractional
  /// digit stored decrements `scale_`. After that, digits are dropped: a dropped integer digit increments `scale_`
  /// so that the value keeps its magnitude, and a dropped non-zero digit sets `sticky_` to record that the true
  /// value lies strictly above the retained one.
  ///
  /// \param digit  The value of the digit, in the range [0, 9].
  /// \param fractional  true if the digit follows the decimal point, false if it is part of the integer part.
  void add_digit(int digit, bool fractional) noexcept;

  /// \brief Checks whether the retained digits fit in the magnitude type of \p Integer.
  ///
  /// Only the bits held in `mant_` are considered. The sign and any dropped digits are ignored.
  ///
  /// \tparam Integer  The integer type of the result.
  /// \returns true if the value of `mant_` can be represented by the magnitude type, false otherwise.
  template <parser_integer Integer> [[nodiscard]] bool magnitude_fits() const noexcept;

  /// \brief Returns the retained digits as an unsigned magnitude.
  ///
  /// Reads the low 64 bits of `mant_`, and the next 64 bits too when the magnitude type is 128 bits wide.
  ///
  /// \tparam Integer  The integer type of the result.
  /// \pre magnitude_fits() is true; otherwise the high bits of the value are silently lost.
  /// \returns The value of `mant_` as the magnitude type of \p Integer.
  template <parser_integer Integer>
  [[nodiscard]] typename integer_traits<Integer>::magnitude_type magnitude() const noexcept;

  /// \brief Checks whether the value can be computed by Clinger's fast path.
  ///
  /// The fast path is exact when no digits were dropped, the retained significand is no larger than
  /// max_mantissa (so it converts to \p Float exactly), and 10^|e10| is itself exact in \p Float. It is
  /// never used when number_parser_fast_path_available is false.
  ///
  /// \tparam Float  The floating-point type of the result.
  /// \param e10  The power of ten by which the retained significand is scaled.
  /// \returns true if fast_path() may be called with \p e10, false otherwise.
  template <parser_float Float> [[nodiscard]] bool can_use_fast_path(std::int64_t e10) const noexcept;

  /// \brief Computes the value with a single floating-point multiplication or division.
  ///
  /// Both operands, the significand and 10^|e10|, are exact in \p Float, so one correctly rounded IEEE
  /// operation gives the correctly rounded result.
  ///
  /// \tparam Float  The floating-point type of the result.
  /// \pre can_use_fast_path(e10) is true.
  /// \param e10  The power of ten by which the retained significand is scaled.
  /// \returns The correctly rounded value, carrying the sign of the input.
  template <parser_float Float> Float fast_path(std::int64_t e10) const noexcept;

  big_uint<mant_limbs> mant_;         ///< The first (up to) max_digits significant digits.
  std::int64_t scale_ = 0;            ///< value = mant_ * 10^(scale_ + exponent).
  std::int64_t exp_ = 0;              ///< The magnitude of the written exponent (saturating).
  digit_count_type digit_count_ = 0;  ///< The number of significant digits held in mant_ (<= max_digits).
  bool negative_ = false;             ///< True if the number has a leading minus sign.
  bool is_integer_ = true;            ///< True if no '.' and no exponent seen.
  bool exp_negative_ = false;         ///< True if the exponent has a leading minus sign.
  bool sticky_ = false;               ///< True if a non-zero digit was dropped.
};

template <int MaxDigits>
std::uint8_t number_accumulator<MaxDigits>::consume(state const current, char8_t const c) noexcept {
  auto const code = number_grammar::next(current, c);
  if (number_grammar::is_state(code)) {
    switch (static_cast<state>(code)) {
    case state::number_minus: negative_ = true; break;
    case state::number_zero:
    case state::number_int_digits: this->add_digit(c - '0', false); break;
    case state::number_dot:
    case state::number_exp_mark: is_integer_ = false; break;
    case state::number_frac_digits: this->add_digit(c - '0', true); break;
    case state::number_exp_sign: exp_negative_ = (c == '-'); break;
    case state::number_exp_digits:
      if (exp_ < exp_cap) {
        exp_ = (exp_ * 10) + (c - '0');
      }
      break;
    default: break;  // LCOV_EXCL_LINE: next() only returns the number states handled above.
    }
  }
  return code;
}

template <int MaxDigits>
PEEJAY_ALWAYS_INLINE void number_accumulator<MaxDigits>::add_digit(int const digit, bool const fractional) noexcept {
  if (digit_count_ == 0 && digit == 0) {  // leading zero: only moves the point
    if (fractional) {
      --scale_;
    }
    return;
  }
  if (digit_count_ < max_digits) {
    mant_.mul_add(10U, static_cast<std::uint32_t>(digit));
    ++digit_count_;
    if (fractional) {
      --scale_;
    }
  } else {
    if (!fractional) {
      ++scale_;
    }
    if (digit != 0) {
      sticky_ = true;
    }
  }
}

template <int MaxDigits>
template <parser_integer Integer>
auto number_accumulator<MaxDigits>::to_integer() const noexcept
    -> std::optional<typename integer_traits<Integer>::signed_type> {
  using traits = integer_traits<Integer>;
  using magnitude_type = traits::magnitude_type;
  using signed_type = traits::signed_type;
  if (!is_integer_ || scale_ != 0 || !this->template magnitude_fits<Integer>()) {
    return std::nullopt;
  }
  constexpr magnitude_type max_positive = traits::max_positive;
  constexpr magnitude_type max_negative = max_positive + 1;
  magnitude_type const magnitude = this->template magnitude<Integer>();
  if (!negative_ && magnitude <= max_positive) {
    return static_cast<signed_type>(magnitude);
  }
  if (negative_ && magnitude <= max_negative) {
    return static_cast<signed_type>(magnitude_type{0} - magnitude);
  }
  return std::nullopt;
}

template <int MaxDigits>
template <parser_integer Integer>
bool number_accumulator<MaxDigits>::magnitude_fits() const noexcept {
  if constexpr (integer_traits<Integer>::magnitude_bits <= 64) {
    return mant_.fits_u64();
  } else {
    return mant_.bit_length() <= static_cast<std::size_t>(integer_traits<Integer>::magnitude_bits);
  }
}

template <int MaxDigits>
template <parser_integer Integer>
auto number_accumulator<MaxDigits>::magnitude() const noexcept -> typename integer_traits<Integer>::magnitude_type {
  using magnitude_type = integer_traits<Integer>::magnitude_type;
  auto value = static_cast<magnitude_type>(mant_.low64());
  if constexpr (integer_traits<Integer>::magnitude_bits > 64) {
    auto const high = (static_cast<std::uint64_t>(mant_.limb(3)) << 32U) | mant_.limb(2);
    value |= static_cast<magnitude_type>(high) << 64;
  }
  return value;
}

template <int MaxDigits>
template <parser_float Float>
bool number_accumulator<MaxDigits>::can_use_fast_path(std::int64_t const e10) const noexcept {
  if constexpr (!number_parser_fast_path_available) {
    return false;
  } else {
    using converter = float_converter<Float, MaxDigits>;
    return !sticky_ && e10 >= -converter::exact_pow10 && e10 <= converter::exact_pow10 && mant_.fits_u64() &&
           mant_.low64() <= converter::max_mantissa;
  }
}

template <int MaxDigits>
template <parser_float Float>
Float number_accumulator<MaxDigits>::fast_path(std::int64_t const e10) const noexcept {
  if constexpr (!number_parser_fast_path_available) {
    assert(false && "fast path is disabled");
    return 0;
  } else {
    auto const m = static_cast<Float>(mant_.low64());
    auto p = Float{1};
    for (std::int64_t i = 0, ae10 = std::abs(e10); i < ae10; ++i) {
      p *= Float{10};  // exact up to 10^exact_pow10
    }
    auto const sign = static_cast<Float>(negative_ ? -1 : 1);
    return sign * (e10 < 0 ? m / p : m * p);
  }
}

template <int MaxDigits>
template <parser_float Float>
std::optional<Float> number_accumulator<MaxDigits>::to_floating() const noexcept {
  using converter = float_converter<Float, MaxDigits>;
  using tail_bias = converter::tail_bias;

  auto const sign = static_cast<Float>(negative_ ? -1 : 1);
  if (digit_count_ == 0) {
    return sign * Float{0};
  }

  std::int64_t const e10 = scale_ + (exp_negative_ ? -exp_ : exp_);
  std::int64_t const magnitude = e10 + digit_count_;  // value is in [10^(m-1), 10^m)
  if (magnitude > converter::overflow_decimal) {
    return std::nullopt;  // certainly overflows
  }
  if (magnitude < -converter::underflow_decimal) {
    return sign * Float{0};  // certainly below half the smallest subnormal
  }

  // Clinger's fast path: both operands exact, one correctly rounded IEEE op.
  if (this->template can_use_fast_path<Float>(e10)) {
    return this->template fast_path<Float>(e10);
  }

  auto const low = converter::convert(mant_, e10, sticky_ ? tail_bias::above : tail_bias::none);

  // The true value lies in the open interval (W, W+1) * 10^e10, so bracket it
  // from just above W and just below W+1: both must round to the same value.
  // (W+1 may itself be an exact halfway point; the value is strictly below it.)
  if (sticky_ && converter::convert(mant_ + 1U, e10, tail_bias::below) != low) {
    return std::nullopt;
  }
  if (!low.has_value()) {
    return std::nullopt;
  }
  return sign * low.value();
}

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_NUMBER_ACCUMULATOR_HPP
