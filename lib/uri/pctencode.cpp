//===- lib/uri/pctencode.cpp ----------------------------------------------===//
//*             _                           _       *
//*  _ __   ___| |_ ___ _ __   ___ ___   __| | ___  *
//* | '_ \ / __| __/ _ \ '_ \ / __/ _ \ / _` |/ _ \ *
//* | |_) | (__| ||  __/ | | | (_| (_) | (_| |  __/ *
//* | .__/ \___|\__\___|_| |_|\___\___/ \__,_|\___| *
//* |_|                                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "uri/pctencode.hpp"

#include "uri/uri.hpp"

namespace {

constexpr bool operator<= (unsigned const x, uri::code_point const y) noexcept {
  return x <= static_cast<std::underlying_type_t<uri::code_point>> (y);
}
constexpr bool operator> (unsigned const x, uri::code_point const y) noexcept {
  return x > static_cast<std::underlying_type_t<uri::code_point>> (y);
}
constexpr bool operator>= (unsigned const x, uri::code_point const y) noexcept {
  return x >= static_cast<std::underlying_type_t<uri::code_point>> (y);
}

constexpr uri::code_point operator+ (uri::code_point const x,
                                     unsigned const y) noexcept {
  return static_cast<uri::code_point> (
    static_cast<std::underlying_type_t<uri::code_point>> (x) + y);
}
constexpr uri::code_point operator- (uri::code_point const x,
                                     uri::code_point const y) noexcept {
  using ut = std::underlying_type_t<uri::code_point>;
  assert (static_cast<ut> (x) >= static_cast<ut> (y));
  return static_cast<uri::code_point> (static_cast<ut> (x) -
                                       static_cast<ut> (y));
}
constexpr uri::code_point operator- (unsigned const x,
                                     uri::code_point const y) noexcept {
  using ut = std::underlying_type_t<uri::code_point>;
  assert (x >= static_cast<ut> (y));
  return static_cast<uri::code_point> (x - static_cast<ut> (y));
}
std::uint_least8_t& operator-= (std::uint_least8_t& x,
                                uri::code_point const y) {
  x = static_cast<std::uint_least8_t> (x - y);
  return x;
}

}  // end anonymous namespace

namespace uri {

bool needs_pctencode (std::uint_least8_t c, pctencode_set es) noexcept {
  static std::array<std::uint8_t const, 32> const encode{{
    0b0100'0000,  // U+0021 EXCLAMATION MARK
    0b0111'1111,  // U+0022 QUOTATION MARK
    0b0111'1110,  // U+0023 NUMBER SIGN
    0b0110'0000,  // U+0024 DOLLAR SIGN
    0b1111'1111,  // U+0025 PERCENT SIGN
    0b0110'0000,  // U+0026 AMPERSAND
    0b0100'0100,  // U+0027 APOSTROPHE
    0b0100'0000,  // U+0028 LEFT PARENTHESIS
    0b0100'0000,  // U+0029 RIGHT PARENTHESIS
    0b0000'0000,  // U+002A ASTERISK
    0b0110'0000,  // U+002B PLUS SIGN
    0b0110'0000,  // U+002C COMMA
    0b0000'0000,  // U+002D HYPHEN MINUS
    0b0000'0000,  // U+002E FULL STOP
    0b0111'0000,  // U+002F SOLIDUS
    // U+0030 DIGIT ZERO to U+0039 DIGIT NINE removed.
    0b0111'0000,  // U+003A (:)
    0b0111'0000,  // U+003B (;)
    0b0111'1111,  // U+003C (<)
    0b0111'0000,  // U+003D (=)
    0b0111'1111,  // U+003E (>)
    0b0111'1000,  // U+003F (?)
    0b0111'0000,  // U+0040 (@)
    // U+0041 LATIN CAPITAL LETTER A to U+005A LATIN CAPITAL LETTER Z removed.
    0b0111'0000,  // U+005B ([ LEFT SQUARE BRACKET)
    0b0111'0000,  // U+005C (\ REVERSE SOLIDUS)
    0b0111'0000,  // U+005D (] RIGHT SWUARE BRACKET)
    0b0111'0000,  // U+005E (^ CIRCUMFLEX ACCENT)
    0b0000'0000,  // U+005F LOW LINE
    0b0111'1000,  // U+0060 GRAVE ACCENT
    // U+0061 LATIN SMALL LETTER A to U+007A LATIN SMALL LETTER Z removed.
    0b0111'1000,  // U+007B LEFT CURLY BRACKET
    0b0111'0000,  // U+007C VERTICAL LINE
    0b0111'1000,  // U+007D RIGHT CURLY BRACKET
    0b0100'0000,  // U+007E TILDE
  }};
  constexpr auto num_digits = 10U;
  constexpr auto num_alpha = 26U;
  // Code point of the first entry in the table.
  constexpr auto table_first = code_point::exclamation_mark;
  constexpr auto capital_letter_adjustment = table_first + num_digits;
  constexpr auto small_letter_adjustment = table_first + num_digits + num_alpha;

  if (c <= code_point::space || c > code_point::tilde) {
    return true;
  }
  c -= table_first;  // C0 control codes were removed.
  if (c >= code_point::digit_zero - table_first) {
    if (c <= code_point::digit_nine - table_first) {
      return false;
    }
    c -= num_digits;  // The digits are missing from the table.
  }
  if (c >= code_point::latin_capital_letter_a - capital_letter_adjustment) {
    if (c <= code_point::latin_capital_letter_z - capital_letter_adjustment) {
      return false;  // A latin capital letter.
    }
    c -= num_alpha;  // Upper-case letters are missing from the table.
  }
  if (c >= code_point::latin_small_letter_a - small_letter_adjustment) {
    if (c <= code_point::latin_small_letter_z - small_letter_adjustment) {
      return false;  // A latin lower-case letter.
    }
    c -= num_alpha;  // Lower-case letters are missing.
  }
  if (c >= encode.size ()) {
    return false;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return (encode[c] &
          static_cast<std::underlying_type_t<pctencode_set>> (es)) != 0U;
}

bool needs_pctencode (std::string_view s, pctencode_set es) {
  return std::any_of (std::begin (s), std::end (s), [es] (auto c) {
    return needs_pctencode (static_cast<std::uint_least8_t> (c), es);
  });
}

}  // end namespace uri
