//===- include/peejay/matchers/string.hpp -----------------*- mode: C++ -*-===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
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
#ifndef PEEJAY_MATCHERS_STRING_HPP
#define PEEJAY_MATCHERS_STRING_HPP

#include <cassert>
#include <cstdint>
#include <string_view>

// icubaby
#include "icubaby/icubaby.hpp"

// Local includes
#include "peejay/concepts.hpp"
#include "peejay/details/arrayvec.hpp"
#include "peejay/details/cbii.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/error.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

/// Matches a string.
template <backend Backend> class string_matcher {
public:
  using parser_type = parser<Backend>;
  using policies = std::remove_reference_t<Backend>::policies;

  constexpr explicit string_matcher(bool const is_key) noexcept : is_key_{is_key} {}

  bool consume(parser_type& parser, char8_t code_unit);
  void eof(parser_type &parser);

private:
  /// Process a single "normal" (i.e. not part of an escape or hex sequence)
  /// character.
  ///
  /// \param parser  The parent parser instance.
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  bool normal(parser_type& parser, char8_t code_unit);

  /// Processes a code point as part of a escape sequence for a string.
  /// \param parser  The owning parser instance.
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  void escape(parser_type& parser, char8_t code_unit);

  /// Processes a code point as part of a hex escape sequence (uXXXX) for a string.
  /// \param parser  The owning parser instance.
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  void hex(parser_type& parser, char8_t code_unit);
  //// True if this string is an object's property name, false otherwise.
  bool is_key_;
  /// UTF-16 to UTF-8 converter.
  icubaby::t16_8 utf_16_to_8_;
  /// Used to accumulate the code point value from the four hex digits. After
  /// the four digits have been consumed, this UTF-16 code point value is
  /// converted to UTF-8 and added to the output.
  uint_least16_t hex_ = 0U;
  /// The accumulated string contents.
  arrayvec<char8_t, policies::max_length> str_;
};

// normal
// ~~~~~~
template <backend Backend> bool string_matcher<Backend>::normal(parser_type& parser, char8_t const code_unit) {
  if (code_unit == '\\') {
    parser.set_state(state::string_escape);
    return true;
  }
  // Check whether:
  // a) We processed part of a Unicode UTF-16 code point (in which case the rest needs to be expressed using the '\u'
  //   escape).
  // b) Control characters U+0000 through U+001F MUST be escaped.
  if (utf_16_to_8_.partial() || code_unit <= 0x1F) {
    return parser.set_error_and_pop(error::bad_unicode_code_point);
  }
  // The end of the string?
  if (code_unit == '"') {
    auto &backend = parser.backend();
    auto const result = std::u8string_view{str_.data(), str_.size()};
    parser.set_error(is_key_ ? backend.key(result) : backend.string_value(result));
    parser.pop();  // unconditionally pop this matcher.
    return true;
  }

  // Remember this code unit.
  if (str_.size() == str_.capacity()) {
    parser.set_error_and_pop(error::string_too_long);
  } else {
    str_.push_back(code_unit);
  }
  return true;
}

// escape
// ~~~~~~
template <backend Backend> void string_matcher<Backend>::escape(parser_type& parser, char8_t code_unit) {
  switch (code_unit) {
  case '"':
  case '/':
  case '\\':
    // code points are appended as-is.
    break;
  case 'b': code_unit = '\b'; break;
  case 'f': code_unit = '\f'; break;
  case 'n': code_unit = '\n'; break;
  case 'r': code_unit = '\r'; break;
  case 't': code_unit = '\t'; break;
  case 'u':
    hex_ = 0;
    parser.set_state(state::string_hex1);
    return;
  default: parser.set_error_and_pop(error::invalid_escape_char); return;
  }
  // We're adding this code point to the output string and returning to the "normal" state.
  if (str_.size() == str_.capacity()) {
    parser.set_error_and_pop(error::string_too_long);
    return;
  }
  str_.push_back(code_unit);
  parser.set_state(state::string_normal_char);
}

template <backend Backend> void string_matcher<Backend>::hex(parser_type& parser, char8_t const code_unit) {
  auto &state = parser.stack_.top();
  assert(to_underlying(state) >= to_underlying(state::string_hex1) &&
         to_underlying(state) <= to_underlying(state::string_hex4) && "We must be in one of the hex states");
  assert((state != state::string_hex1 || hex_ == 0) && "hex_ must be zero in the hex1 state");
  auto offset = std::uint_least16_t{0};
  if (code_unit >= '0' && code_unit <= '9') {
    offset = static_cast<std::uint_least16_t>('0');
  } else if (code_unit >= 'a' && code_unit <= 'f') {
    offset = static_cast<std::uint_least16_t>('a' - 10U);
  } else if (code_unit >= 'A' && code_unit <= 'F') {
    offset = static_cast<std::uint_least16_t>('A' - 10U);
  } else {
    parser.set_error_and_pop(error::invalid_hex_char);
    return;
  }
  hex_ = static_cast<std::uint_least16_t>((16U * hex_) + static_cast<std::uint_least16_t>(code_unit) - offset);
  if (state < state::string_hex4) {
    // More hex characters to go.
    state = static_cast<enum state>(to_underlying(state) + 1);
    return;
  }
  // Convert the UTF-16 code unit to UTF-8.
  bool overflow = false;
  utf_16_to_8_(hex_, checked_back_insert_iterator<decltype(str_), char8_t>(&str_, &overflow));
  if (!utf_16_to_8_.well_formed()) {
    parser.set_error_and_pop(error::bad_unicode_code_point);
    return;
  }
  if (overflow) {
    parser.set_error_and_pop(error::string_too_long);
    return;
  }
  state = state::string_normal_char;
}

// consume
// ~~~~~~~
template <backend Backend> bool string_matcher<Backend>::consume(parser_type& parser, char8_t const code_unit) {
  bool match = true;
  switch (parser.stack_.top()) {
  case state::string_start:
    str_.clear();
    parser.set_state(state::string_normal_char);
    [[fallthrough]];
  case state::string_normal_char: match = this->normal(parser, code_unit); break;
  case state::string_escape: this->escape(parser, code_unit); break;

  case state::string_hex1:
  case state::string_hex2:
  case state::string_hex3:
  case state::string_hex4: this->hex(parser, code_unit); break;

  default: unreachable(); break;
  }
  return match;
}

// eof
// ~~~
template <backend Backend> void string_matcher<Backend>::eof(parser_type &parser) {
  parser.set_error_and_pop(error::expected_close_quote);
}

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_STRING_HPP
