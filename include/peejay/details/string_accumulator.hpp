//===- include/peejay/details/string_accumulator.hpp ------*- mode: C++ -*-===//
//*     _       _                                  *
//*  __| |_ _ _(_)_ _  __ _                        *
//* (_-<  _| '_| | ' \/ _` |                       *
//* /__/\__|_| |_|_||_\__, |                       *
//*                   |___/                        *
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
/// \file string_accumulator.hpp
/// \brief Provides peejay::details::string_accumulator, which accumulates the characters of a JSON string and
///   decodes its escape sequences. Nothing here depends on the parser's backend.
#ifndef PEEJAY_DETAILS_STRING_ACCUMULATOR_HPP
#define PEEJAY_DETAILS_STRING_ACCUMULATOR_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

// icubaby
#include "icubaby/icubaby.hpp"

// Local includes
#include "peejay/details/arrayvec.hpp"
#include "peejay/details/cbii.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/details/states.hpp"
#include "peejay/error.hpp"

namespace peejay::details {

/// \brief Accumulates the characters of a JSON string, decoding its escape sequences.
///
/// The opening quote has already been consumed before the first call to consume(). The state of the string grammar
/// is held by the caller (it lives on the parser's stack) and passed to consume().
///
/// This class depends only on the maximum length of a string, not on the parser's backend. Parsers whose backends
/// have the same policies therefore share a single copy of its code.
///
/// \tparam MaxLength  The maximum length, in UTF-8 code units, of the decoded string.
template <std::size_t MaxLength> class string_accumulator {
public:
  /// \brief Feeds one character of the input to the accumulator.
  ///
  /// Every code unit is consumed.
  ///
  /// \param current  The current state of the string grammar. Must be one of the string states.
  /// \param code_unit  A UTF-8 code unit from the input.
  /// \returns If the string continues, the underlying value of its next state. Otherwise the underlying value of
  ///   an error: error::none if \p code_unit was the closing quote (the string is then available from str()), or
  ///   the error to be reported. Use is_state() to tell the two apart.
  std::uint8_t consume(state current, char8_t code_unit) noexcept;

  /// \brief The fast path for the common case: an ordinary character in the body of a string.
  ///
  /// If \p code_unit is not a quote, backslash, or control character, no UTF-16 surrogate pair is incomplete, and
  /// the string has room for it, then \p code_unit is appended to the string. This has exactly the same effect as
  /// consume() would in state::string_normal_char but is small enough to be inlined into the caller.
  ///
  /// \param code_unit  A UTF-8 code unit from the input.
  /// \returns True if \p code_unit was appended (the state remains state::string_normal_char), false if it must
  ///   instead be passed to consume().
  PEEJAY_ALWAYS_INLINE bool append_plain(char8_t const code_unit) noexcept {
    if (code_unit <= 0x1F || code_unit == '"' || code_unit == '\\' || utf_16_to_8_.partial() ||
        str_.size() == str_.capacity()) {
      return false;
    }
    str_.push_back(code_unit);
    return true;
  }

  /// \param code  A value returned by consume().
  /// \returns True if \p code is a state, false if it is an error (error::none if the string ended).
  static constexpr bool is_state(std::uint8_t const code) noexcept {
    return code >= to_underlying(state::string_start);
  }

  /// \returns The decoded string. Complete once consume() has reported error::none.
  [[nodiscard]] constexpr std::u8string_view str() const noexcept { return {str_.data(), str_.size()}; }

private:
  // The result of consume() is a single byte which holds either a state or an error. This works because every
  // error value is smaller than every string state. error::string_too_long is the last enumerator of peejay::error.
  static_assert(to_underlying(error::string_too_long) < to_underlying(state::string_start),
                "error values and string states must not overlap");

  /// \param s  A string state.
  /// \returns The value returned by consume() for state \p s.
  static constexpr std::uint8_t result(state const s) noexcept { return to_underlying(s); }
  /// \param erc  An error.
  /// \returns The value returned by consume() for error \p erc.
  static constexpr std::uint8_t result(error const erc) noexcept {
    auto const value = to_underlying(erc);
    assert(value >= 0 && static_cast<unsigned>(value) < std::numeric_limits<std::uint8_t>::max());
    return static_cast<std::uint8_t>(value);
  }

  /// Processes a single "normal" (i.e. not part of an escape or hex sequence) character.
  ///
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  /// \returns The value to be returned by consume().
  std::uint8_t normal(char8_t code_unit) noexcept;

  /// Processes the character following a backslash in a string.
  ///
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  /// \returns The value to be returned by consume().
  std::uint8_t escape(char8_t code_unit) noexcept;

  /// Processes one of the hex digits of a \\uXXXX escape sequence for a string.
  ///
  /// \param current  The current state: one of state::string_hex1 to state::string_hex4.
  /// \param code_unit  A UTF-8 code unit for the character being processed.
  /// \returns The value to be returned by consume().
  std::uint8_t hex(state current, char8_t code_unit) noexcept;

  /// UTF-16 to UTF-8 converter.
  icubaby::t16_8 utf_16_to_8_;
  /// Used to accumulate the code unit value from the four hex digits. After
  /// the four digits have been consumed, this UTF-16 code unit is
  /// converted to UTF-8 and added to the output.
  uint_least16_t hex_ = 0U;
  /// The accumulated string contents.
  arrayvec<char8_t, MaxLength> str_;
};

// normal
// ~~~~~~
template <std::size_t MaxLength> std::uint8_t string_accumulator<MaxLength>::normal(char8_t const code_unit) noexcept {
  if (this->append_plain(code_unit)) {
    return result(state::string_normal_char);
  }
  if (code_unit == '\\') {
    return result(state::string_escape);
  }
  // Check whether:
  // a) We processed part of a Unicode UTF-16 code point (in which case the rest needs to be expressed using the '\u'
  //   escape).
  // b) Control characters U+0000 through U+001F MUST be escaped.
  if (utf_16_to_8_.partial() || code_unit <= 0x1F) {
    return result(error::bad_unicode_code_point);
  }
  // The end of the string?
  if (code_unit == '"') {
    return result(error::none);
  }
  // An ordinary character for which there is no room.
  assert(str_.size() == str_.capacity());
  return result(error::string_too_long);
}

// escape
// ~~~~~~
template <std::size_t MaxLength> std::uint8_t string_accumulator<MaxLength>::escape(char8_t code_unit) noexcept {
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
  case 'u': hex_ = 0; return result(state::string_hex1);
  default: return result(error::invalid_escape_char);
  }
  // We're adding this code point to the output string and returning to the "normal" state.
  if (str_.size() == str_.capacity()) {
    return result(error::string_too_long);
  }
  str_.push_back(code_unit);
  return result(state::string_normal_char);
}

// hex
// ~~~
template <std::size_t MaxLength>
std::uint8_t string_accumulator<MaxLength>::hex(state const current, char8_t const code_unit) noexcept {
  assert(to_underlying(current) >= to_underlying(state::string_hex1) &&
         to_underlying(current) <= to_underlying(state::string_hex4) && "We must be in one of the hex states");
  assert((current != state::string_hex1 || hex_ == 0) && "hex_ must be zero in the hex1 state");
  auto offset = std::uint_least16_t{0};
  if (code_unit >= '0' && code_unit <= '9') {
    offset = static_cast<std::uint_least16_t>('0');
  } else if (code_unit >= 'a' && code_unit <= 'f') {
    offset = static_cast<std::uint_least16_t>('a' - 10U);
  } else if (code_unit >= 'A' && code_unit <= 'F') {
    offset = static_cast<std::uint_least16_t>('A' - 10U);
  } else {
    return result(error::invalid_hex_char);
  }
  hex_ = static_cast<std::uint_least16_t>((16U * hex_) + static_cast<std::uint_least16_t>(code_unit) - offset);
  if (current < state::string_hex4) {
    // More hex characters to go.
    return static_cast<std::uint8_t>(to_underlying(current) + 1);
  }
  // Convert the UTF-16 code unit to UTF-8.
  bool overflow = false;
  utf_16_to_8_(hex_, checked_back_insert_iterator<decltype(str_), char8_t>(&str_, &overflow));
  if (!utf_16_to_8_.well_formed()) {
    return result(error::bad_unicode_code_point);
  }
  if (overflow) {
    return result(error::string_too_long);
  }
  return result(state::string_normal_char);
}

// consume
// ~~~~~~~
template <std::size_t MaxLength>
std::uint8_t string_accumulator<MaxLength>::consume(state const current, char8_t const code_unit) noexcept {
  switch (current) {
  case state::string_start: str_.clear(); return this->normal(code_unit);
  case state::string_normal_char: return this->normal(code_unit);
  case state::string_escape: return this->escape(code_unit);
  case state::string_hex1:
  case state::string_hex2:
  case state::string_hex3:
  case state::string_hex4: return this->hex(current, code_unit);
  default: unreachable();  // LCOV_EXCL_LINE: current is always one of the string states.
  }
}

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_STRING_ACCUMULATOR_HPP
