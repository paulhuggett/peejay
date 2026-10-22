//===- include/peejay/details/states.hpp ------------------*- mode: C++ -*-===//
//*      _        _             *
//*  ___| |_ __ _| |_ ___  ___  *
//* / __| __/ _` | __/ _ \/ __| *
//* \__ \ || (_| | ||  __/\__ \ *
//* |___/\__\__,_|\__\___||___/ *
//*                             *
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
/// \file states.hpp
/// \brief Defines the states of the parser's state machine.
#ifndef PEEJAY_DETAILS_STATES_HPP
#define PEEJAY_DETAILS_STATES_HPP

#include "peejay/details/portab.hpp"

namespace peejay::details {

/// The number of low bits of a state value which distinguish the states within a group. The remaining high bits
/// identify the group.
constexpr auto group_shift = 5U;

/// \brief The groups of parser states. Each group is handled by a single matcher.
enum class group : std::uint8_t {
  whitespace = 0U << group_shift,
  eof = 1U << group_shift,
  root = 2U << group_shift,
  token = 3U << group_shift,
  string = 4U << group_shift,
  number = 5U << group_shift,
  array = 6U << group_shift,
  object = 7U << group_shift,
};

/// \brief The states of the parser.
///
/// The high bits of each state value give the group (and hence the matcher) to which the state belongs; see
/// get_group().
enum class state : std::uint8_t {
  /// Normal whitespace scanning.
  whitespace_start = to_underlying(group::whitespace),
  /// Handles the LF part of a Windows-style CR/LF pair
  whitespace_crlf,

  /// Expecting the end of the input (or trailing whitespace).
  eof_start = to_underlying(group::eof),

  /// Expecting a value, possibly preceded by whitespace.
  root_start = to_underlying(group::root),
  /// Expecting the first character of a value.
  root_new_token,

  /// Matching the remaining characters of a keyword.
  token_start = to_underlying(group::token),

  /// Expecting the first character of a number.
  number_start = to_underlying(group::number),
  /// A leading minus sign has been consumed.
  number_minus,
  /// A leading zero has been consumed.
  number_zero,
  /// Consuming the digits of the integer part.
  number_int_digits,
  /// A decimal point has been consumed.
  number_dot,
  /// Consuming the digits of the fractional part.
  number_frac_digits,
  /// An exponent marker ('e' or 'E') has been consumed.
  number_exp_mark,
  /// The sign of the exponent has been consumed.
  number_exp_sign,
  /// Consuming the digits of the exponent.
  number_exp_digits,

  /// The opening quote of a string has been consumed.
  string_start = to_underlying(group::string),
  /// Consuming the characters of a string.
  string_normal_char,
  /// Expecting the first hex digit of a \\u escape.
  string_hex1,
  /// Expecting the second hex digit of a \\u escape.
  string_hex2,
  /// Expecting the third hex digit of a \\u escape.
  string_hex3,
  /// Expecting the fourth hex digit of a \\u escape.
  string_hex4,
  /// A backslash has been consumed.
  string_escape,

  /// The opening bracket of an array has been consumed.
  array_start = to_underlying(group::array),
  /// Expecting the first member of an array or the closing bracket.
  array_first_object,
  /// Expecting a member of an array.
  array_object,
  /// Expecting a comma or the closing bracket.
  array_comma,

  /// The opening brace of an object has been consumed.
  object_start = to_underlying(group::object),
  /// Expecting the first key of an object or the closing brace.
  object_first_key,
  /// Expecting the key of an object member.
  object_key,
  /// Expecting a colon.
  object_colon,
  /// Expecting the value of an object member.
  object_value,
  /// Expecting a comma or the closing brace.
  object_comma,
};

/// \param s  A parser state.
/// \returns The group to which state \p s belongs.
constexpr group get_group(state const s) noexcept {
  return static_cast<group>(to_underlying(s) & ~((1U << group_shift) - 1U));
}

}  // namespace peejay::details

#endif  // PEEJAY_DETAILS_STATES_HPP
