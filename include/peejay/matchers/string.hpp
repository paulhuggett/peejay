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
#ifndef PEEJAY_MATCHERS__STRING_HPP
#define PEEJAY_MATCHERS__STRING_HPP

#include <cassert>
#include <cstdint>
#include <optional>
#include <string_view>

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
  using policies = typename std::remove_reference_t<Backend>::policies;

  constexpr explicit string_matcher(bool is_key) noexcept : is_key_{is_key} {}

  bool consume(parser_type &parser, std::optional<char32_t> ch);

private:
  /// Process a single "normal" (i.e. not part of an escape or hex sequence)
  /// character. This function wraps consume_normal(). That function does the
  /// real work but this wrapper performs any necessary mutations of the state
  /// machine.
  ///
  /// \param parser  The parent parser instance.
  /// \param code_point  The Unicode character being processed.
  bool normal(parser_type &parser, char32_t code_point);

  /// Processes a code point as part of a escape sequence for a string.
  /// \p parser  The owning parser instance.
  /// \p code_point  The Unicode code point to be added to the escape sequence.
  void escape(parser_type &parser, char32_t code_point);

  /// Processes a code point as part of a hex escape sequence (\uXXXX) for a string.
  /// \p parser  The owning parser instance.
  /// \p code_point  The Unicode code point to be added to the escape sequence.
  void hex(parser_type &parser, char32_t code_point);

  bool is_key_;
  /// UTF-16 to UTF-8 converter.
  icubaby::t16_8 utf_16_to_8_;
  /// Used to accumulate the code point value from the four hex digits. After
  /// the four digits have been consumed, this UTF-16 code point value is
  /// converted to UTF-8 and added to the output.
  uint_least16_t hex_ = 0U;

  icubaby::t32_8 utf_32_to_8_;
  arrayvec<typename policies::char_type, policies::max_length> str_;
};

// normal
// ~~~~~~
template <backend Backend> bool string_matcher<Backend>::normal(parser_type &parser, char32_t code_point) {
  if (code_point == '\\') {
    parser.set_state(state::string_escape);
    return true;
  }
  // Check whether:
  // a) We processed part of a Unicode UTF-16 code point (in which case the rest needs to be expressed using the '\u'
  // escape. b) Control characters U+0000 through U+001F MUST be escaped.
  if (utf_16_to_8_.partial() || code_point <= 0x1F) {
    parser.set_error(error::bad_unicode_code_point);
    return true;
  }
  if (code_point == '"') {
    auto &backend = parser.backend();
    auto const result = std::basic_string_view<typename policies::char_type>{str_.data(), str_.size()};
    parser.set_error(is_key_ ? backend.key(result) : backend.string_value(result));
    // Consume the closing quote character.
    parser.pop();
    return true;
  }

  // Remember this character.
  bool overflow = false;
  auto it = utf_32_to_8_(code_point, checked_back_insert_iterator{&str_, &overflow});
  utf_32_to_8_.end_cp(it);
  if (!utf_32_to_8_.well_formed()) {
    parser.set_error(error::bad_unicode_code_point);
    return true;
  }
  if (overflow) {
    parser.set_error(error::string_too_long);
    return true;
  }
  return true;
}

// escape
// ~~~~~~
template <backend Backend> void string_matcher<Backend>::escape(parser_type &parser, char32_t code_point) {
  switch (code_point) {
  case '"':
  case '/':
  case '\\':
    // code points are appended as-is.
    break;
  case 'b': code_point = '\b'; break;
  case 'f': code_point = '\f'; break;
  case 'n': code_point = '\n'; break;
  case 'r': code_point = '\r'; break;
  case 't': code_point = '\t'; break;
  case 'u': parser.set_state(state::string_hex1); return;
  default: parser.set_error(error::invalid_escape_char); return;
  }
  // We're adding this code point to the output string and returning to the "normal" state.
  bool overflow = false;
  utf_32_to_8_(code_point, checked_back_insert_iterator(&str_, &overflow));
  assert(utf_32_to_8_.well_formed());
  if (overflow) {
    parser.set_error(error::string_too_long);
  } else {
    parser.set_state(state::string_normal_char);
  }
}

template <backend Backend> void string_matcher<Backend>::hex(parser_type &parser, char32_t code_point) {
  auto &state = parser.stack_.top();
  assert(to_underlying(state) >= to_underlying(state::string_hex1) &&
         to_underlying(state) <= to_underlying(state::string_hex4) && "We must be in one of the hex states");
  if (state == state::string_hex1) {
    hex_ = 0;
  }
  auto offset = std::uint_least16_t{0};
  if (code_point >= '0' && code_point <= '9') {
    offset = static_cast<std::uint_least16_t>('0');
  } else if (code_point >= 'a' && code_point <= 'f') {
    offset = static_cast<std::uint_least16_t>('a' - 10U);
  } else if (code_point >= 'A' && code_point <= 'F') {
    offset = static_cast<std::uint_least16_t>('A' - 10U);
  } else {
    parser.set_error(error::invalid_hex_char);
    return;
  }
  hex_ = static_cast<std::uint_least16_t>(16U * hex_ + static_cast<std::uint_least16_t>(code_point) - offset);
  if (state < state::string_hex4) {
    // More hex characters to go.
    state = static_cast<enum state>(to_underlying(state) + 1);
    return;
  }
  // Convert the UTF-16 code unit to UTF-8.
  bool overflow = false;
  utf_16_to_8_(hex_, checked_back_insert_iterator(&str_, &overflow));
  if (!utf_16_to_8_.well_formed()) {
    parser.set_error(error::bad_unicode_code_point);
    return;
  }
  if (overflow) {
    parser.set_error(error::string_too_long);
    return;
  }
  state = state::string_normal_char;
}

// consume
// ~~~~~~~
template <backend Backend> bool string_matcher<Backend>::consume(parser_type &parser, std::optional<char32_t> ch) {
  if (!ch) {
    parser.set_error(error::expected_close_quote);
    return true;
  }

  auto const c = *ch;
  bool match = true;
  switch (parser.stack_.top()) {
  // Matches the opening quote.
  case state::string_start:
    str_.clear();
    if (c == '"') {
      parser.set_state(state::string_normal_char);
    } else {
      parser.set_error(error::expected_token);
    }
    break;
  case state::string_normal_char: match = this->normal(parser, c); break;
  case state::string_escape: this->escape(parser, c); break;

  case state::string_hex1:
  case state::string_hex2:
  case state::string_hex3:
  case state::string_hex4: this->hex(parser, c); break;

  default: unreachable(); break;
  }
  return match;
}

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS__STRING_HPP
