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
/// \file number.hpp
/// \brief Provides the matcher for a JSON number.
#ifndef PEEJAY_MATCHERS_NUMBER_HPP
#define PEEJAY_MATCHERS_NUMBER_HPP

#include <cstdint>
#include <system_error>
#include <type_traits>
#include <variant>

#ifndef PEEJAY_CONCEPTS_HPP
#include "peejay/concepts.hpp"
#endif
#ifndef PEEJAY_DETAILS_NUMBER_ACCUMULATOR_HPP
#include "peejay/details/number_accumulator.hpp"
#endif
#include "peejay/details/portab.hpp"

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

/// \brief Matches a JSON number and reports its value to the backend.
///
/// The characters of the number are accumulated, and the result converted, by number_accumulator. That class
/// depends only on the policies' integer type (through the number of digits retained) and floating-point type, so
/// parsers whose backends have the same policies share its code. This class is the thin layer which connects it to
/// the parser and the backend.
///
/// Result selection:
/// - A number with no fraction and no exponent that is representable in the
///   policy's integer_type is passed to the backend's integer_value().
/// - Everything else is passed to float_value() as the correctly rounded (nearest,
///   ties to even) float_type (see number_accumulator::to_floating()).
/// - If the policy's float_type is no_float_type, a number which cannot be passed to integer_value() raises
///   error::number_out_of_range.
///
/// max_digits is 38 by default (39 for 128-bit integers, whose limits have 39 digits) and can be raised with the
/// policy's `max_significant_digits`.
///
/// \tparam Backend  A type meeting the backend<> requirements. Its `policies` member type supplies the integer and
///                  floating-point types of the results and the optional `max_significant_digits` setting. The
///                  integer_value() and float_value() member functions of the backend receive the result.
template <backend Backend> class number_matcher {
  using policies = std::remove_reference_t<Backend>::policies;
  using parser_type = parser<Backend>;
  using integer_type = std::remove_cv_t<typename policies::integer_type>;
  using float_type = std::remove_cv_t<typename policies::float_type>;

  /// \brief Optional policy: `static constexpr unsigned max_significant_digits`. The matcher keeps
  ///        only this many significant digits of the input.
  ///
  /// A value that lies strictly between two retainable neighbours of a rounding boundary cannot be
  /// rounded correctly from fewer digits, so it is reported as number_out_of_range rather than
  /// guessed. The default (38, or 39 for 128-bit integers) is also the minimum: a smaller value is
  /// ignored.
  static constexpr int max_digits =
      retained_digits(integer_traits<integer_type>::value_bits, get_requested_digits<policies>());

public:
  /// \brief Feeds one character of the input to the matcher.
  ///
  /// Advances the number grammar from the parser's current state. A character that continues the number is
  /// accumulated and moves the parser to the next state of the grammar. A character that cannot continue the
  /// number either ends it, in which case the value is posted to the backend and the matcher is popped, or is an
  /// error, which is recorded in the parser before the matcher is popped. In both cases the character is not
  /// consumed: a character that ended the number is offered to the matcher that is then on top of the stack.
  ///
  /// \param parser  The parser that owns this matcher. Its state is read and updated; errors are reported to it and
  ///                completed values are passed to its backend.
  /// \param c  The next UTF-8 code unit of the input.
  /// \returns true if \p c was consumed as part of the number, false if it ended the number or was an error.
  PEEJAY_ALWAYS_INLINE bool consume(parser_type& parser, char8_t const c) noexcept {
    auto const current = parser.get_state();
    // Most characters are digits of the integer or fractional part: handle those without a call.
    if (acc_.append_digit(current, c)) {
      return true;
    }
    auto const code = acc_.consume(current, c);
    if (!number_grammar::is_state(code)) {
      if (auto const err = static_cast<error>(code); err == error::none) {
        this->post_result(parser);
      } else {
        parser.set_error_and_pop(err);
      }
      return false;
    }
    if (code != to_underlying(current)) {
      parser.set_state(static_cast<state>(code));
    }
    return true;
  }

  /// \brief Signals that the input ended while this matcher was active.
  ///
  /// The end of the input ends the number. A lone '-' is reported as error::expected_digits. Otherwise the result
  /// is posted exactly as if a terminating character had been seen; see post_result().
  ///
  /// \param parser  The parser that owns this matcher. Errors are reported to it and a completed value is passed
  ///                to its backend. The matcher is popped from the parser's stack.
  void eof(parser_type& parser) {
    if (parser.get_state() == state::number_minus) {
      parser.set_error_and_pop(error::expected_digits);
      return;
    }
    this->post_result(parser);
  }

private:
  /// \brief Delivers the completed number to the backend.
  ///
  /// Called once the number has ended. If the grammar is not in an accepting state (for example, the input was
  /// "1." or "1e"), error::number_out_of_range is reported. Otherwise a plain integer that fits in the policy's
  /// integer type is passed to the backend's integer_value(), and every other value to its float_value(). If
  /// floating point is disabled, or the value cannot be converted (see number_accumulator::to_floating()), the
  /// error is reported instead. In every case the matcher is popped from the parser's stack.
  ///
  /// \param parser  The parser that owns this matcher. It receives any error, and its backend receives the value.
  void post_result(parser_type& parser) const noexcept {
    if (!number_grammar::complete(parser.get_state())) {
      parser.set_error_and_pop(error::number_out_of_range);
      return;
    }

    if (auto const i = acc_.template to_integer<integer_type>()) {
      parser.set_error(parser.backend().integer_value(*i));
      parser.pop();
      return;
    }

    if constexpr (std::is_same_v<float_type, no_float_type>) {
      parser.set_error_and_pop(error::number_out_of_range);
    } else {
      if (auto const f = acc_.template to_floating<float_type>()) {
        parser.set_error(parser.backend().float_value(*f));
        parser.pop();
      } else {
        parser.set_error_and_pop(error::number_out_of_range);
      }
    }
  }

  /// The digits, sign, scale and exponent of the number seen so far.
  number_accumulator<max_digits> acc_;
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_NUMBER_HPP
