//===- include/peejay/details/states.hpp ------------------*- mode: C++ -*-===//
//*      _        _             *
//*  ___| |_ __ _| |_ ___  ___  *
//* / __| __/ _` | __/ _ \/ __| *
//* \__ \ || (_| | ||  __/\__ \ *
//* |___/\__\__,_|\__\___||___/ *
//*                             *
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
#ifndef PEEJAY_DETAILS_STATES_HPP
#define PEEJAY_DETAILS_STATES_HPP

#include "peejay/details/portab.hpp"

namespace peejay::details {

constexpr auto group_shift = 5U;

enum class group {
  whitespace = 0U << group_shift,
  eof = 1U << group_shift,
  root = 2U << group_shift,
  token = 3U << group_shift,
  string = 4U << group_shift,
  number = 5U << group_shift,
  array = 6U << group_shift,
  object = 7U << group_shift,
};

enum class state {
  whitespace_start = to_underlying(group::whitespace),
  /// Normal whitespace scanning. The "body" is the whitespace being consumed
  whitespace_body,
  /// Handles the LF part of a Windows-style CR/LF pair
  whitespace_crlf,

  eof_start = to_underlying(group::eof),

  root_start = to_underlying(group::root),
  root_new_token,

  token_start = to_underlying(group::token),
  token_last,

  number_start = to_underlying(group::number),
  number_integer_initial_digit,
  number_integer_digit,
  number_frac,
  number_frac_initial_digit,
  number_frac_digit,
  number_exponent_sign,
  number_exponent_initial_digit,
  number_exponent_digit,

  string_start = to_underlying(group::string),
  string_normal_char,
  string_hex1,
  string_hex2,
  string_hex3,
  string_hex4,
  string_escape,

  array_start = to_underlying(group::array),
  array_first_object,
  array_object,
  array_comma,

  object_start = to_underlying(group::object),
  object_first_key,
  object_key,
  object_colon,
  object_value,
  object_comma,
};

constexpr group get_group(state const s) noexcept {
  return static_cast<group>(to_underlying(s) & ~((1U << group_shift) - 1U));
}

}  // namespace peejay::details

#endif  // PEEJAY_DETAILS_STATES_HPP
