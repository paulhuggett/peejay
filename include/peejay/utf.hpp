//===- include/peejay/utf.hpp -----------------------------*- mode: C++ -*-===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_UTF_HPP
#define PEEJAY_UTF_HPP

#include <cassert>

#include "peejay/portab.hpp"
#if PEEJAY_CXX20
#include <concepts>
#endif
#include <cstdint>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace peejay {

#if PEEJAY_CXX20
using char8 = char8_t;
using u8string = std::u8string;
using u8string_view = std::u8string_view;
#else
using char8 = char;
using u8string = std::basic_string<char8>;
using u8string_view = std::basic_string_view<char8>;
#endif

/// If the top two bits are 0b10, then this is a UTF-8 continuation byte
/// and is skipped; other patterns in these top two bits represent the
/// start of a character.
template <typename CharType>
constexpr bool is_utf_char_start (CharType c) noexcept {
  return (static_cast<std::make_unsigned_t<CharType>> (c) & 0xC0U) != 0x80U;
}

class utf8_decoder {
public:
  std::optional<char32_t> get (char8 c) noexcept;
  bool is_well_formed () const { return well_formed_; }

private:
  enum state { accept, reject };

  static uint8_t decode (uint8_t* const state, char32_t* const codep,
                         uint32_t const byte);

  static uint8_t const utf8d_[];
  char32_t codepoint_ = 0;
  uint8_t state_ = accept;
  bool well_formed_ = true;
};

inline constexpr char32_t replacement_char_code_point = 0xFFFD;

// code point to utf8
// ~~~~~~~~~~~~~~~~~~
template <typename OutputIt>
PEEJAY_CXX20REQUIRES ((std::output_iterator<OutputIt, char8_t>))
OutputIt code_point_to_utf8 (char32_t c, OutputIt out) {
  if (c < 0x80) {
    *(out++) = static_cast<char8> (c);
  } else {
    if (c < 0x800) {
      *(out++) = static_cast<char8> (c / 64 + 0xC0);
      *(out++) = static_cast<char8> (c % 64 + 0x80);
    } else if (c >= 0xD800 && c < 0xE000) {
      out = code_point_to_utf8 (replacement_char_code_point, out);
    } else if (c < 0x10000) {
      *(out++) = static_cast<char8> ((c / 0x1000) | 0xE0);
      *(out++) = static_cast<char8> ((c / 64 % 64) | 0x80);
      *(out++) = static_cast<char8> ((c % 64) | 0x80);
    } else if (c < 0x110000) {
      *(out++) = static_cast<char8> ((c / 0x40000) | 0xF0);
      *(out++) = static_cast<char8> ((c / 0x1000 % 64) | 0x80);
      *(out++) = static_cast<char8> ((c / 64 % 64) | 0x80);
      *(out++) = static_cast<char8> ((c % 64) | 0x80);
    } else {
      out = code_point_to_utf8 (replacement_char_code_point, out);
    }
  }
  return out;
}

// is utf16 high surrogate
// ~~~~~~~~~~~~~~~~~~~~~~~
constexpr bool is_utf16_high_surrogate (char16_t code_unit) {
  return code_unit >= 0xD800 && code_unit <= 0xDBFF;
}

// is utf16 low surrogate
// ~~~~~~~~~~~~~~~~~~~~~~
constexpr bool is_utf16_low_surrogate (char16_t code_unit) {
  return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
}

// utf16 to code point
// ~~~~~~~~~~~~~~~~~~~
/// \tparam InputIterator  An input iterator which must produce (possible const/volatile qualified) char16_t.
template <typename InputIterator>
PEEJAY_CXX20REQUIRES (
    (std::input_iterator<InputIterator> &&
     std::is_same_v<std::remove_cv_t<typename std::iterator_traits<
                        InputIterator>::value_type>,
                    char16_t>))
std::pair<InputIterator, char32_t> utf16_to_code_point (InputIterator first,
                                                        InputIterator last) {
  if (first == last) {
    return {first, replacement_char_code_point};
  }
  char16_t const code_unit = *(first++);
  if (!is_utf16_high_surrogate (code_unit)) {
    return {first, static_cast<char32_t> (code_unit)};
  }
  if (first == last) {
    return {first, replacement_char_code_point};
  }

  auto const high = code_unit;
  auto const low = *(first++);
  if (!is_utf16_low_surrogate (low)) {
    return {first, replacement_char_code_point};
  }

  uint32_t code_point = 0x10000 +
                        ((static_cast<uint16_t> (high) & 0x03FFU) << 10U) +
                        (static_cast<uint16_t> (low) & 0x03FFU);
  return {first, static_cast<char32_t> (code_point)};
}

}  // namespace peejay

#endif  // PEEJAY_UTF_HPP
