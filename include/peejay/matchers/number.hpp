//===- include/peejay/matchers/number.hpp -----------------*- mode: C++ -*-===//
//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
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
#ifndef PEEJAY_MATCHERS_NUMBER_HPP
#define PEEJAY_MATCHERS_NUMBER_HPP

#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <variant>

#include "peejay/concepts.hpp"
#include "peejay/error.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

template <typename FloatType, std::unsigned_integral UIntegerType> struct float_accumulator {
  static_assert(std::is_floating_point_v<FloatType>);
  /// Promote from integer.
  explicit constexpr float_accumulator(UIntegerType v) noexcept : value{static_cast<FloatType>(v)} {}
  /// Assign an explicit float.
  explicit constexpr float_accumulator(FloatType v) noexcept : value{v} {}

  void add_digit(unsigned const digit) {
    assert(digit < 10);
    ++frac_digits;
    frac_part = frac_part * static_cast<FloatType>(10.0) + digit;
  }

  unsigned frac_digits = 0;
  FloatType frac_part = static_cast<FloatType>(0.0);
  FloatType value = FloatType{0.0};
  bool exp_is_negative = false;
  unsigned exponent = 0U;
};

template <std::unsigned_integral UIntegerType> struct float_accumulator<no_float_type, UIntegerType> {};

//*                 _              *
//*  _ _ _  _ _ __ | |__  ___ _ _  *
//* | ' \ || | '  \| '_ \/ -_) '_| *
//* |_||_\_,_|_|_|_|_.__/\___|_|   *
//*                                *
// Grammar (from RFC 7159, March 2014)
//     number = [ minus ] int [ frac ] [ exp ]
//     decimal-point = %x2E       ; .
//     digit1-9 = %x31-39         ; 1-9
//     e = %x65 / %x45            ; e E
//     exp = e [ minus / plus ] 1*DIGIT
//     frac = decimal-point 1*DIGIT
//     int = zero / ( digit1-9 *DIGIT )
//     minus = %x2D               ; -
//     plus = %x2B                ; +
//     zero = %x30                ; 0
/// Matches a number.
template <backend Backend> class number_matcher {
public:
  using policies = typename std::remove_reference_t<Backend>::policies;

  using parser_type = parser<Backend>;
  using uinteger_type = std::make_unsigned_t<typename policies::integer_type>;
  using sinteger_type = std::make_signed_t<typename policies::integer_type>;
  using float_type = std::remove_cv_t<typename policies::float_type>;

  bool consume(parser_type &parser, std::optional<char32_t> ch);

private:
  bool do_leading_minus_state(parser_type &parser, char32_t c);
  /// Implements the first character of the 'int' production.
  bool do_integer_initial_digit_state(parser_type &parser, char32_t c);
  bool do_integer_digit_state(parser_type &parser, char32_t c);
  bool do_frac_state(parser_type &parser, char32_t c);
  bool do_frac_digit_state(parser_type &parser, char32_t c);
  bool do_exponent_sign_state(parser_type &parser, char32_t c);
  bool do_exponent_digit_state(parser_type &parser, char32_t c);

  void complete(parser_type &parser);

  void make_result(parser_type &parser);
  bool end(parser_type &parser);

  static constexpr bool is_digit(char32_t const c) noexcept { return c >= '0' && c <= '9'; }

  bool is_neg_ = false;
  float_accumulator<float_type, uinteger_type> &number_is_float() {
    if constexpr (!std::is_same_v<float_type, no_float_type>) {
      if (auto *const uit = std::get_if<uinteger_type>(&acc_)) {
        acc_ = float_accumulator<float_type, uinteger_type>{*uit};
      }
    }
    return std::get<float_accumulator<float_type, uinteger_type>>(acc_);
  }

  std::variant<uinteger_type, float_accumulator<float_type, uinteger_type>> acc_;
};

// leading minus state
// ~~~~~~~~~~~~~~~~~~~
template <backend Backend> bool number_matcher<Backend>::do_leading_minus_state(parser_type &parser, char32_t c) {
  bool match = true;
  if (c == '-') {
    parser.stack_.top() = state::number_integer_initial_digit;
    is_neg_ = true;
  } else if (is_digit(c)) {
    parser.stack_.top() = state::number_integer_initial_digit;
    match = do_integer_initial_digit_state(parser, c);
  } else {
    // minus MUST be followed by the 'int' production.
    parser.set_error(error::number_out_of_range);
  }
  return match;
}

// frac state
// ~~~~~~~~~~
template <backend Backend> bool number_matcher<Backend>::do_frac_state(parser_type &parser, char32_t const c) {
  bool match = true;
  switch (c) {
  case '.': parser.stack_.top() = state::number_frac_initial_digit; break;
  case 'e':
  case 'E': parser.stack_.top() = state::number_exponent_sign; break;
  default:
    // the 'frac' production is optional.
    match = false;
    this->complete(parser);
    break;
  }
  return match;
}

// frac digit
// ~~~~~~~~~~
template <backend Backend> bool number_matcher<Backend>::do_frac_digit_state(parser_type &parser, char32_t const c) {
  bool match = true;
  if constexpr (std::is_same_v<float_type, no_float_type>) {
    parser.set_error(error::number_out_of_range);
  } else {
    if (is_digit(c)) {
      this->number_is_float().add_digit(c - '0');
      parser.stack_.top() = state::number_frac_digit;
    } else {
      if (parser.stack_.top() == state::number_frac_initial_digit) {
        parser.set_error(error::unrecognized_token);
        return true;
      }

      if (c == 'e' || c == 'E') {
        this->number_is_float();
        parser.stack_.top() = state::number_exponent_sign;
      } else {
        match = false;
        this->complete(parser);
      }
    }
  }
  return match;
}

// exponent sign state
// ~~~~~~~~~~~~~~~~~~~
template <backend Backend> bool number_matcher<Backend>::do_exponent_sign_state(parser_type &parser, char32_t c) {
  bool match = true;
  if constexpr (std::is_same_v<float_type, no_float_type>) {
    parser.set_error(error::number_out_of_range);
  } else {
    auto &fp_acc = this->number_is_float();
    parser.stack_.top() = state::number_exponent_initial_digit;
    switch (c) {
    case '+': fp_acc.exp_is_negative = false; break;
    case '-': fp_acc.exp_is_negative = true; break;
    default: match = this->do_exponent_digit_state(parser, c); break;
    }
  }
  return match;
}

// complete
// ~~~~~~~~
template <backend Backend> void number_matcher<Backend>::complete(parser_type &parser) {
  this->make_result(parser);
  parser.pop();
}

// exponent digit
// ~~~~~~~~~~~~~~
template <backend Backend>
bool number_matcher<Backend>::do_exponent_digit_state(parser_type &parser, char32_t const c) {
  assert((std::holds_alternative<float_accumulator<float_type, uinteger_type>>(acc_)));

  bool match = true;
  if constexpr (std::is_same_v<float_type, no_float_type>) {
    parser.set_error(error::number_out_of_range);
  } else {
    if (is_digit(c)) {
      auto &fp_acc = std::get<float_accumulator<float_type, uinteger_type>>(acc_);
      fp_acc.exponent = fp_acc.exponent * 10U + static_cast<unsigned>(c - '0');
      parser.stack_.top() = state::number_exponent_digit;
    } else {
      if (parser.stack_.top() == state::number_exponent_initial_digit) {
        parser.set_error(error::unrecognized_token);
      } else {
        match = false;
        this->complete(parser);
      }
    }
  }
  return match;
}

// do integer initial digit state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
template <backend Backend>
bool number_matcher<Backend>::do_integer_initial_digit_state(parser_type &parser, char32_t const c) {
  using namespace std::string_view_literals;

  assert(parser.stack_.top() == state::number_integer_initial_digit);
  assert(std::holds_alternative<uinteger_type>(acc_));
  if (c == '0') {
    parser.stack_.top() = state::number_frac;
  } else if (c >= '1' && c <= '9') {
    assert(std::get<uinteger_type>(acc_) == 0);
    std::get<uinteger_type>(acc_) = static_cast<uinteger_type>(c) - '0';
    parser.stack_.top() = state::number_integer_digit;
  } else {
    parser.set_error(error::unrecognized_token);
  }
  return true;
}

// do integer digit state
// ~~~~~~~~~~~~~~~~~~~~~~
template <backend Backend> bool number_matcher<Backend>::do_integer_digit_state(parser_type &parser, char32_t const c) {
  assert(parser.stack_.top() == state::number_integer_digit);
  assert(std::holds_alternative<uinteger_type>(acc_));

  bool match = true;
  if (c == '.') {
    if constexpr (std::is_same_v<float_type, no_float_type>) {
      parser.set_error(error::number_out_of_range);
    } else {
      parser.stack_.top() = state::number_frac_initial_digit;
      number_is_float();
    }
  } else if (c == 'e' || c == 'E') {
    parser.stack_.top() = state::number_exponent_sign;
    number_is_float();
  } else if (is_digit(c)) {
    auto &int_acc = std::get<uinteger_type>(acc_);
    auto const new_acc = static_cast<uinteger_type>(int_acc * 10U + static_cast<uinteger_type>(c) - '0');
    if (new_acc < int_acc) {  // Did this overflow?
      parser.set_error(error::number_out_of_range);
    }
    int_acc = new_acc;
  } else {
    match = false;
    this->complete(parser);
  }
  return match;
}

// end
// ~~~
template <backend Backend> bool number_matcher<Backend>::end(parser_type &parser) {
  assert(!parser.has_error());
  switch (parser.stack_.top()) {
  case state::number_exponent_digit:
  case state::number_frac_digit:
  case state::number_frac:
  case state::number_integer_digit: this->complete(parser); break;
  default: parser.set_error(error::expected_digits); break;
  }
  return true;
}

// consume
// ~~~~~~~
template <backend Backend> bool number_matcher<Backend>::consume(parser_type &parser, std::optional<char32_t> ch) {
  if (!ch) {
    return this->end(parser);
  }

  bool match = true;
  auto const c = *ch;
  switch (parser.stack_.top()) {
  case state::number_start: match = this->do_leading_minus_state(parser, c); break;
  case state::number_integer_initial_digit: match = this->do_integer_initial_digit_state(parser, c); break;
  case state::number_integer_digit: match = this->do_integer_digit_state(parser, c); break;
  case state::number_frac: match = this->do_frac_state(parser, c); break;
  case state::number_frac_initial_digit:
  case state::number_frac_digit: match = this->do_frac_digit_state(parser, c); break;
  case state::number_exponent_sign: match = this->do_exponent_sign_state(parser, c); break;
  case state::number_exponent_initial_digit:
  case state::number_exponent_digit: match = this->do_exponent_digit_state(parser, c); break;
  default: unreachable(); break;
  }

  return match;
}

// make result
// ~~~~~~~~~~~
template <backend Backend> void number_matcher<Backend>::make_result(parser_type &parser) {
  if (parser.has_error()) {
    return;
  }

  if (std::holds_alternative<uinteger_type>(acc_)) {
    constexpr auto min = std::numeric_limits<sinteger_type>::min();
    constexpr auto umin = static_cast<uinteger_type>(min);
    constexpr auto umax = static_cast<uinteger_type>(std::numeric_limits<sinteger_type>::max());

    // TODO: this doesn't automatically promote v. large integers to float.
    // Perhaps it should?
    auto &int_acc = std::get<uinteger_type>(acc_);
    if (is_neg_) {
      if (int_acc > umin) {
        parser.set_error(error::number_out_of_range);
      } else {
        parser.set_error(
            parser.backend().integer_value((int_acc == umin) ? min : -static_cast<sinteger_type>(int_acc)));
      }
    } else {
      if (int_acc > umax) {
        parser.set_error(error::number_out_of_range);
      } else {
        parser.set_error(parser.backend().integer_value(static_cast<sinteger_type>(int_acc)));
      }
    }
    return;
  }

  if constexpr (std::is_same_v<float_type, no_float_type>) {
    parser.set_error(error::number_out_of_range);
  } else {
    auto &fp_acc = std::get<float_accumulator<float_type, uinteger_type>>(acc_);
    auto xf = static_cast<float_type>(
        fp_acc.value +
        static_cast<float_type>(fp_acc.frac_part / static_cast<float_type>(std::pow(10, fp_acc.frac_digits))));
    auto exp = static_cast<float_type>(std::pow(10, fp_acc.exponent));
    if (std::isinf(exp)) {
      parser.set_error(error::number_out_of_range);
      return;
    }
    if (fp_acc.exp_is_negative) {
      exp = static_cast<float_type>(1.0) / exp;
    }

    xf *= exp;
    if (is_neg_) {
      xf = -xf;
    }

    // Is the fractional part of the float 0 (i.e. could we potentially cast it to
    // one of the integer types)? This enables us to treat input such as "1.0" in
    // the same way as "1".
    auto ipart = float_type{0};
    std::modf(xf, &ipart);
    auto const frac_part = std::abs(xf - ipart);

    if (frac_part < std::numeric_limits<float_type>::epsilon() * 128 &&
        xf >= static_cast<float_type>(std::numeric_limits<sinteger_type>::min()) &&
        xf <= static_cast<float_type>(std::numeric_limits<sinteger_type>::max())) {
      parser.set_error(parser.backend().integer_value(static_cast<sinteger_type>(xf)));
      return;
    }

    parser.set_error(parser.backend().float_value(xf));
  }
}

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_NUMBER_HPP
