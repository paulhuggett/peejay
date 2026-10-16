//===- include/peejay/matchers/number.hpp -----------------*- mode: C++ -*-===//
//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
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
#ifndef PEEJAY_MATCHERS_NUMBER_HPP
#define PEEJAY_MATCHERS_NUMBER_HPP

#include <bit>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#ifndef PEEJAY_CONCEPTS_HPP
#include "peejay/concepts.hpp"
#endif
#ifndef PEEJAY_DETAILS_BIG_UINT_HPP
#include "peejay/details/big_uint.hpp"
#endif
#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif
#ifndef PEEJAY_ERROR_HPP
#include "peejay/error.hpp"
#endif

namespace peejay::details {

namespace number_grammar {

constexpr bool is_minus(char8_t const c) noexcept { return c == '-'; }
constexpr bool is_zero(char8_t const c) noexcept { return c == '0'; }
constexpr bool is_dot(char8_t const c) noexcept { return c == '.'; }
constexpr bool is_digit(char8_t const c) noexcept { return c >= '0' && c <= '9'; }
constexpr bool is_digit9(char8_t const c) noexcept { return c >= '1' && c <= '9'; }
constexpr bool is_exp(char8_t const c) noexcept { return c == 'e' || c == 'E'; }
constexpr bool is_sign(char8_t const c) noexcept { return c == '+' || c == '-'; }
constexpr bool is_done(char8_t const) noexcept { return true; }

using terminal = bool (*)(char8_t);
class term {
public:
  constexpr explicit term(terminal const g, state const s) : g_{g}, value_{to_underlying(s)} {}
  constexpr explicit term(error const err) : g_{is_done}, value_{to_underlying(err)} {}
  constexpr terminal get_terminal() const noexcept { return g_; }
  constexpr explicit operator state() const { return static_cast<state>(value_); }
  constexpr explicit operator error() const { return static_cast<error>(value_); }

private:
  terminal g_;
  std::common_type_t<std::underlying_type_t<state>, std::underlying_type_t<peejay::error>> value_;
};

constexpr std::array number_start = {term{is_minus, state::number_minus}, term{is_zero, state::number_zero}, term{is_digit9, state::number_int_digits}, term{error::none}};
constexpr std::array number_minus = {term{is_zero, state::number_zero}, term{is_digit9, state::number_int_digits}, term{error::expected_digits}};
constexpr std::array number_zero = {term{is_dot, state::number_dot}, term{is_exp, state::number_exp_mark}, term{error::none}};
constexpr std::array number_int_digits = {term{is_digit, state::number_int_digits}, term{is_dot, state::number_dot}, term{is_exp, state::number_exp_mark}, term{error::none}};
constexpr std::array number_dot = {term{is_digit, state::number_frac_digits}, term{error::unrecognized_token}};
constexpr std::array number_frac_digits = {term{is_digit, state::number_frac_digits}, term{is_exp, state::number_exp_mark}, term{error::none}};
constexpr std::array number_exp_mark = {term{is_sign, state::number_exp_sign}, term{is_digit, state::number_exp_digits}, term{error::unrecognized_token}};
constexpr std::array number_exp_sign = {term{is_digit, state::number_exp_digits}, term{error::unrecognized_token}};
constexpr std::array number_exp_digits = {term{is_digit, state::number_exp_digits}, term{error::none}};

constexpr std::array grammar{
    number_start.data(),      number_minus.data(),    number_zero.data(),
    number_int_digits.data(), number_dot.data(),      number_frac_digits.data(),
    number_exp_mark.data(),   number_exp_sign.data(), number_exp_digits.data(),
};

constexpr std::variant<std::error_code, state> transition(state const s, char8_t const c) noexcept {
  using state_ut = std::underlying_type_t<state>;
  assert(static_cast<state_ut>(s) >= static_cast<state_ut>(state::number_start));
  auto const index = static_cast<std::size_t>(static_cast<state_ut>(s) - static_cast<state_ut>(state::number_start));
  assert(index < grammar.size());

  for (auto const* p = grammar[index];; ++p) {
    auto* const handler = p->get_terminal();
    if (handler == is_done) {
      return error{*p};
    }
    if (handler(c)) {
      return state{*p};
    }
  }
}

/// True if the characters consumed so far form a complete, valid number.
[[nodiscard]] constexpr bool complete(state const st) noexcept {
  switch (st) {
  case state::number_zero:
  case state::number_int_digits:
  case state::number_frac_digits:
  case state::number_exp_digits: return true;
  default: return false;
  }
}

}  // end namespace grammar

// Largest k such that 5^k <= max_mantissa, i.e. 10^k is exact in float_type.
constexpr std::int64_t get_exact_pow10(std::uint64_t max_mantissa) {
  std::uint64_t p = 1;
  std::int64_t k = 0;
  while (p <= max_mantissa / 5) {
    p *= 5;
    ++k;
  }
  return k;
}

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


/// Facts about the integer types the matcher can deliver. Standard integers are
/// described through <limits>; the 128-bit extension types are described
/// explicitly.
template <typename T> struct integer_traits;

template <typename Signed, typename Unsigned, typename Magnitude, int ValueBits> struct integer_traits_base {
  /// The type handed to the backend (always signed)
  using signed_type = Signed;
  /// The unsigned counterpart of signed_type
  using unsigned_type = Unsigned;
  /// An unsigned type wide enough for |value| of signed_type
  using magnitude_type = Magnitude;

  /// The number value bits of signed_type (63 for int64_t, 127 for __int128)
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

#if __SIZEOF_INT128__
template <>
struct integer_traits<__int128> : integer_traits_base<__int128, unsigned __int128, unsigned __int128, 127> {};
template <>
struct integer_traits<unsigned __int128> : integer_traits_base<__int128, unsigned __int128, unsigned __int128, 127> {};
#endif  // __SIZEOF_INT128__

// Integer types the matcher can deliver: standard integers of up to 64 bits
// and, where the compiler provides them, __int128 / unsigned __int128.
template <typename T>
concept parser_integer = requires { typename integer_traits<std::remove_cv_t<T>>::signed_type; };

// Number of significant decimal digits the matcher retains. It must be able to
// hold every value of the signed result type exactly (2^value_bits has
// floor(value_bits * log10(2)) + 1 digits. 30103/100000 slightly exceeds
// log10(2)), and is never less than 38.
constexpr int retained_digits(int const value_bits, int const requested = 0) {
  int needed = (value_bits * 30103) / 100000 + 1;
  if (needed < 38) {
    needed = 38;
  }
  return requested > needed ? requested : needed;
}

// 32-bit limbs needed to hold any `digits`-digit decimal number
// (3322/1000 slightly exceeds log2(10), so this rounds up).
constexpr std::size_t limbs_for_digits(int const digits) {
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

// 5^13 is the largest power of five below 2^32; used to cut bignum passes.
static constexpr std::uint32_t pow5_chunk = 1'220'703'125U;
static constexpr std::int64_t pow5_chunk_exp = 13;

constexpr std::uint32_t small_pow5(std::int64_t const exponent) noexcept {
  auto p = std::uint32_t{1};
  for (auto i = std::int64_t{0}; i < exponent; ++i) {
    p *= 5U;
  }
  return p;
}

// n *= 5^exponent. Returns non-zero if any step carried out of the top limb.
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

// n = floor(n / 5^exponent). Returns true if the division was inexact.
// floor(floor(x/a)/b) == floor(x/(a*b)), and the final remainder is non-zero
// iff any step's remainder was.
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

// Radix-2 floating-point types with at most 64 mantissa bits.
template <typename T>
concept parser_float = std::numeric_limits<T>::is_specialized && !std::numeric_limits<T>::is_integer &&
                       std::numeric_limits<T>::radix == 2 && std::numeric_limits<T>::digits <= 64;

// Allocation-free, incremental (push) parser for the grammar:
//
//   number = '-'? ( '0' | digit9 {digit}* ) [ '.' {digit}+ ] [ ('E'|'e') ['+'|'-'] {digit}+ ]
//   digit  = '0' | digit9
//   digit9 = '1' | ... | '9'
//
// Retained state (about 40 bytes, 48 for 128-bit integers): the first 38
// significant decimal digits (39 for 128-bit integers, whose limits have 39
// digits) as a multi-limb integer, a decimal scale, the exponent, a "non-zero
// digits were dropped" flag, and a few flags. Nothing is allocated and nothing
// throws.
//
// Compile-time option: define NUMBER_PARSER_NO_FAST_PATH to disable the
// exact-IEEE-operation shortcut and always use the bignum path (the results
// are identical; the shortcut is merely faster).
//
// Result selection:
//  * A number with no fraction and no exponent that is representable in
//    IntegerType is returned as IntegerType (for an unsigned IntegerType,
//    "-0" is the only negative number that qualifies).
//  * Everything else is returned as the correctly rounded (nearest, ties to
//    even) FloatType, computed with exact integer arithmetic on a small
//    stack-local bignum that only exists while result() runs. Its size is
//    derived from FloatType (about 40 bytes for float, 140 for double,
//    2 KiB for x87 long double).
//  * Inputs with more than 38 significant digits cannot be retained. The
//    parser then knows the value lies strictly between W and W+1 (in units of
//    the last retained digit). If both ends round to the same FloatType that
//    value is returned, which is exact. If a rounding boundary lies inside the
//    interval, the correct result cannot be known and result() returns
//    std::nullopt rather than guessing.
//  * result() also returns std::nullopt for incomplete/invalid input and for
//    magnitudes too large for a finite FloatType.
template <backend Backend> class number_matcher {
  using policies = std::remove_reference_t<Backend>::policies;

  using parser_type = parser<Backend>;
  using int_traits = details::integer_traits<std::remove_cv_t<typename policies::integer_type>>;
  using uinteger_type = int_traits::unsigned_type;
  using sinteger_type = int_traits::signed_type;
  using magnitude_type = int_traits::magnitude_type;

  /// A radix-2 floating-point type with at most 64 mantissa bits:
  /// float, double, or an x87 80-bit long double. Subnormals are
  /// assumed to be supported.
  using float_type = std::remove_cv_t<typename policies::float_type>;

  using float_limits = std::numeric_limits<float_type>;

public:
  // Feeds one character. Returns true if this character is to be consumed, false otherwise.
  bool consume(parser_type& parser, char8_t const c) noexcept {
    auto const next = number_grammar::transition(parser.get_state(), c);
    if (auto const* const err = std::get_if<std::error_code>(&next)) {
      if (!*err) {
        this->post_result(parser);
      } else {
        parser.set_error_and_pop(*err);
      }
      return false;
    }
    switch (std::get<state>(next)) {
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
    default: break;
    }
    parser.set_state(std::get<state>(next));
    return true;
  }

  void eof(parser_type& parser) {
    switch (parser.get_state()) {
    case state::number_minus: parser.set_error_and_pop(error::expected_digits); break;
    default: this->post_result(parser); break;
    }
  }

private:
  // Call after the final character. See the notes at the top of the file.
  void post_result(parser_type& parser) const noexcept {
    if (!number_grammar::complete(parser.get_state())) {
      parser.set_error_and_pop(error::number_out_of_range);
      return;
    }

    if (auto const i = this->to_integer()) {
      parser.set_error(parser.backend().integer_value(*i));
      parser.pop();
      return;
    }

    if constexpr (std::is_same_v<float_type, no_float_type>) {
      parser.set_error_and_pop(error::number_out_of_range);
    } else {
      auto const tf = this->to_floating();
      if (auto const* const f = std::get_if<float_type>(&tf)) {
        parser.set_error(parser.backend().float_value(*f));
        parser.pop();
      } else {
        parser.set_error_and_pop(std::get<std::error_code>(tf));
      }
    }
  }

  // The text as a sinteger_type, if it was a plain integer (no fraction, no
  // exponent, no dropped digits) that fits.
  [[nodiscard]] std::optional<sinteger_type> to_integer() const noexcept {
    if (!is_integer_ || scale_ != 0 || !this->magnitude_fits()) {
      return std::nullopt;
    }
    constexpr magnitude_type max_positive = int_traits::max_positive;
    constexpr magnitude_type max_negative = max_positive + 1;
    magnitude_type const magnitude = this->magnitude();
    if (!negative_ && magnitude <= max_positive) {
      return static_cast<sinteger_type>(magnitude);
    }
    if (negative_ && magnitude <= max_negative) {
      return static_cast<sinteger_type>(magnitude_type{0} - magnitude);
    }
    return std::nullopt;
  }

  // True if the retained digits form a value that fits in magnitude_type.
  [[nodiscard]] bool magnitude_fits() const noexcept {
    if constexpr (int_traits::magnitude_bits <= 64) {
      return mant_.fits_u64();
    } else {
      return mant_.bit_length() <= static_cast<std::size_t>(int_traits::magnitude_bits);
    }
  }

  // The retained digits as a magnitude_type; only valid if magnitude_fits().
  [[nodiscard]] magnitude_type magnitude() const noexcept {
    auto value = static_cast<magnitude_type>(mant_.low64());
    if constexpr (int_traits::magnitude_bits > 64) {
      auto const high = (static_cast<std::uint64_t>(mant_.limb(3)) << 32U) | mant_.limb(2);
      value |= static_cast<magnitude_type>(high) << 64;
    }
    return value;
  }

  template <std::size_t LimbCount> using big_uint = details::big_uint<LimbCount>;

  // Optional policy: `static constexpr int max_significant_digits`. The matcher keeps
  // only this many significant digits of the input. A value that lies strictly
  // between two retainable neighbours of a rounding boundary cannot be rounded
  // correctly from fewer digits, so it is reported as number_out_of_range rather
  // than guessed (see to_floating()). The default (38, or 39 for 128-bit
  // integers) leaves that possible only for inputs that agree with an exact
  // halfway point to ~38 digits; 768 digits make it impossible, because no double's
  // halfway point has more than 767 significant digits. The cost: 4 bytes of
  // object state per 32 bits retained (about 340 bytes in total for 768 digits)
  // and a pass over those limbs for every digit read.
  static constexpr int requested_digits = [] {
    if constexpr (requires { policies::max_significant_digits; }) {
      return static_cast<int>(policies::max_significant_digits);
    } else {
      return 0;
    }
  }();
  static constexpr int max_digits =
      details::retained_digits(int_traits::value_bits, requested_digits);
  static_assert(max_digits <= std::numeric_limits<std::uint16_t>::max(),
                "policies::max_significant_digits is too large");
  using digit_count_type = std::conditional_t<(max_digits <= 255), std::uint8_t, std::uint16_t>;
  // The written exponent saturates here only to avoid overflow while reading an
  // absurdly long exponent. It cannot be a floating-point limit: the exponent
  // is combined with scale_, which grows with the number of mantissa digits
  // ("1" + a million zeros + "e-1000000" is exactly 1), so the cap must exceed
  // any input length that could ever be streamed. 10^17 keeps exp_ * 10 + 9
  // inside int64_t.
  static constexpr std::int64_t exp_cap = 100'000'000'000'000'000;
  static constexpr std::size_t mant_limbs = details::limbs_for_digits(max_digits);

  static constexpr std::int64_t mant_bits = float_limits::digits;
  // The lsb of the smallest subnormal is 2^-subnormal_bits.
  static constexpr std::int64_t subnormal_bits = mant_bits - float_limits::min_exponent;
  // 30103/100000 slightly exceeds log10(2), so this errs on the safe side.
  static constexpr std::int64_t underflow_decimal = ((subnormal_bits + 1) * 30103) / 100000 + 1;
  static constexpr std::int64_t overflow_decimal = float_limits::max_exponent10 + 1;

  // Largest integer that FloatType represents exactly: 2^mant_bits - 1.
  static constexpr std::uint64_t max_mantissa =
      mant_bits >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << (mant_bits % 64)) - 1;

  // Largest k such that 5^k <= max_mantissa, i.e. 10^k is exact in float_type.
  static constexpr std::int64_t exact_pow10 = get_exact_pow10(max_mantissa);

  // Stack bignum capacity (bits -> limbs) for the worst case of both
  // directions: scaled-up division for tiny values, 5^e multiplication for
  // huge ones.
  static constexpr std::size_t work_limbs = get_work_limbs<float_type>(underflow_decimal, max_digits, mant_limbs);

  /// \brief Digit accumulation
  ///
  /// Retains the digit in `mant_` while there is room. Afterwards digits are
  /// dropped: integer digits move the scale up, non-zero ones set `sticky_`.
  void add_digit(int const digit, bool const fractional) noexcept {
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

  // ------------------------------------------------ decimal -> float ------
  // The conversion runs on exact integers in three steps:
  //   to_binary()         rewrites w * 10^e10 as n * 2^p2, exactly, apart from a
  //                       sticky flag that records non-zero bits lost on the way;
  //   round_to_mantissa() keeps the top mant_bits bits and rounds half to even;
  //   assemble()          builds the float_type, or reports overflow.

  using work_uint = big_uint<work_limbs>;

  // Where the true value lies relative to the decimal that is converted. The
  // conversion is exact for the decimal itself; `above` / `below` ask for the
  // result for a value infinitesimally larger / smaller, which is how an open
  // interval (W, W+1) * 10^e10 is bracketed.
  enum class tail_bias { none, above, below };

  // An exact value n * 2^p2, plus a flag if anything non-zero lies below n.
  struct binary_value {
    work_uint n;
    std::int64_t p2 = 0;
    bool sticky = false;
    bool below = false;     // the true value is infinitesimally less than n * 2^p2 (+ sticky)
    bool overflow = false;  // the product did not fit; work_limbs is sized so it cannot
  };

  // A rounded significand and the power of two that scales it.
  struct rounded_value {
    std::uint64_t mantissa = 0;
    std::int64_t exponent = 0;
    bool overflow = false;
  };

  // Shifts n left so that floor(n / 5^count) keeps at least mant_bits + 3 bits.
  // Returns the number of bits shifted.
  static std::int64_t scale_up_for_division(work_uint& n, std::int64_t const count) noexcept {
    std::int64_t const need = (count * 1193 + 511) / 512 + mant_bits + 3;  // >= count*log2(5)+...
    auto const have = static_cast<std::int64_t>(n.bit_length());
    std::int64_t const up = need > have ? need - have : 0;
    n.shift_left(static_cast<std::size_t>(up));
    return up;
  }

  // w * 10^e10 as n * 2^p2, adjusted infinitesimally up or down according to `bias`.
  static binary_value to_binary(big_uint<mant_limbs> const& w, std::int64_t const e10,
                                tail_bias const bias) noexcept {
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
      std::int64_t const up = number_matcher::scale_up_for_division(v.n, count);
      if (divide_by_pow5(v.n, count)) {
        v.sticky = true;
      }
      v.p2 = -count - up;
    }
    return v;
  }

  // Number of low bits to discard from a `length`-bit significand scaled by 2^p2
  // so that at most mant_bits remain and no bit is lighter than the smallest
  // subnormal.
  static std::int64_t discarded_bits(std::int64_t const length, std::int64_t const p2) noexcept {
    std::int64_t shift = length - mant_bits;
    if (p2 + shift < -subnormal_bits) {
      shift = -subnormal_bits - p2;
    }
    if (shift < 0) {
      shift = 0;
    }
    return shift;
  }

  // Rounds to nearest, ties to even: adds one unit in the last place when the
  // discarded bits call for it, renormalising if the mantissa was all ones. A
  // value infinitesimally `below` an exact tie is not a tie: it rounds down.
  static void round_half_even(rounded_value& r, bool const round_bit, bool const sticky,
                              bool const below) noexcept {
    bool const round_up = below ? (round_bit && sticky)
                                : (round_bit && (sticky || (r.mantissa & 1U) != 0));
    if (round_up) {
      if (r.mantissa == ~std::uint64_t{0}) {  // only when mant_bits == 64
        r.mantissa = std::uint64_t{1} << 63;
        ++r.exponent;
      } else {
        ++r.mantissa;
      }
    }
  }

  // Keeps the top mant_bits bits of v.n and rounds them.
  static rounded_value round_to_mantissa(binary_value& v) noexcept {
    std::int64_t const shift = number_matcher::discarded_bits(
        static_cast<std::int64_t>(v.n.bit_length()), v.p2);
    auto round_bit = false;
    if (shift > 0) {
      auto const s = static_cast<std::size_t>(shift);
      round_bit = v.n.bit(s - 1);
      v.sticky = v.sticky || v.n.any_below(s - 1);
      v.n.shift_right(s);
    }
    rounded_value r{.mantissa = v.n.low64(), .exponent=v.p2 + shift, .overflow=v.overflow};
    number_matcher::round_half_even(r, round_bit, v.sticky, v.below);
    return r;
  }

  // mantissa * 2^exponent as a float_type; std::nullopt if it exceeds the
  // largest finite value.
  static std::optional<float_type> assemble(rounded_value const& r) noexcept {
    if (r.overflow) {
      return std::nullopt;
    }
    if (r.mantissa == 0) {
      return float_type{0};
    }
    auto const width = static_cast<std::int64_t>(std::bit_width(r.mantissa));
    if (width + r.exponent > float_limits::max_exponent) {
      return std::nullopt;
    }
    return std::ldexp(static_cast<float_type>(r.mantissa), static_cast<int>(r.exponent));
  }

  // Nearest float_type to w * 10^e10 (nudged infinitesimally up or down according
  // to `bias`). std::nullopt means overflow.
  static std::optional<float_type> convert(big_uint<mant_limbs> const& w, std::int64_t const e10,
                                           tail_bias const bias) noexcept {
    binary_value value = number_matcher::to_binary(w, e10, bias);
    return number_matcher::assemble(number_matcher::round_to_mantissa(value));
  }

  [[nodiscard]] std::variant<std::error_code, float_type> to_floating() const noexcept {
    if constexpr (std::is_same_v<float_type, no_float_type>) {
      return make_error_code(error::number_out_of_range);
    } else {
      auto const sign = static_cast<float_type>(negative_ ? -1 : 1);
      if (digit_count_ == 0) {
        return sign * float_type{0};
      }

      std::int64_t const e10 = scale_ + (exp_negative_ ? -exp_ : exp_);
      std::int64_t const magnitude = e10 + digit_count_;  // value is in [10^(m-1), 10^m)
      if (magnitude > overflow_decimal) {
        return make_error_code(error::number_out_of_range);  // certainly overflows
      }
      if (magnitude < -underflow_decimal) {
        return sign * float_type{0};  // certainly below half the smallest subnormal
      }

  #if defined(FLT_EVAL_METHOD) && (FLT_EVAL_METHOD == 0) && !defined(NUMBER_PARSER_NO_FAST_PATH)
      // Clinger's fast path: both operands exact, one correctly rounded IEEE op.
      if (!sticky_ && e10 >= -exact_pow10 && e10 <= exact_pow10 && mant_.fits_u64() &&
          mant_.low64() <= max_mantissa) {
        auto p = float_type{1};
        for (std::int64_t i = 0; i < (e10 < 0 ? -e10 : e10); ++i) {
          p *= float_type{10};  // exact up to 10^exact_pow10
        }
        auto const m = static_cast<float_type>(mant_.low64());
        return sign * (e10 < 0 ? m / p : m * p);
      }
  #endif

      std::optional<float_type> const low =
          number_matcher::convert(mant_, e10, sticky_ ? tail_bias::above : tail_bias::none);
      if (sticky_) {
        // The true value lies in the OPEN interval (W, W+1) * 10^e10, so bracket it
        // from just above W and just below W+1: both must round to the same value.
        // (W+1 may itself be an exact halfway point; the value is strictly below it.)
        big_uint<mant_limbs> upper = mant_;
        upper.mul_add(1U, 1U);  // upper = mant_ + 1
        if (number_matcher::convert(upper, e10, tail_bias::below) != low) {
          return make_error_code(error::number_out_of_range);
        }
      }
      if (!low) {
        return make_error_code(error::number_out_of_range);
      }
      return sign * *low;
    }
  }

  big_uint<mant_limbs> mant_;     // first (up to) max_digits significant digits
  std::int64_t scale_ = 0;        // value = mant_ * 10^(scale_ + exponent)
  std::int64_t exp_ = 0;          // magnitude of the written exponent (saturating)
  digit_count_type digit_count_ = 0;  // significant digits held in mant_ (<= max_digits)
  bool negative_ = false;
  bool is_integer_ = true;        // True if no '.' and no exponent seen
  bool exp_negative_ = false;
  bool sticky_ = false;           // a non-zero digit was dropped
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_NUMBER_HPP
