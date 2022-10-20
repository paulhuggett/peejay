//===- include/json/utf.hpp -------------------------------*- mode: C++ -*-===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_UTF_HPP
#define PEEJAY_UTF_HPP

#include <cassert>
#if __cplusplus >= 202002L
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

#if __cplusplus >= 202002L
#define CXX20REQUIRES(x) requires x
#else
#define CXX20REQUIRES(x)
#endif  // __cplusplus >= 202002L

namespace peejay {

#if __cplusplus >= 202002L
using utf8_string = std::basic_string<char8_t>;
#else
using utf8_string = std::basic_string<std::uint8_t>;
#endif  // __cplusplus >= 202002L
using utf16_string = std::basic_string<char16_t>;
using utf32_string = std::basic_string<char32_t>;

std::ostream& operator<< (std::ostream& os, utf8_string const& s);

/// If the top two bits are 0b10, then this is a UTF-8 continuation byte
/// and is skipped; other patterns in these top two bits represent the
/// start of a character.
template <typename CharType>
constexpr bool is_utf_char_start (CharType c) noexcept {
  return (static_cast<std::make_unsigned_t<CharType>> (c) & 0xC0U) != 0x80U;
}

class utf8_decoder {
public:
  std::optional<char32_t> get (std::uint8_t c) noexcept;
  bool is_well_formed () const { return well_formed_; }

private:
  enum state { accept, reject };

  static std::uint8_t decode (std::uint8_t* const state, char32_t* const codep,
                              std::uint32_t const byte);

  static std::uint8_t const utf8d_[];
  char32_t codepoint_ = 0;
  std::uint8_t state_ = accept;
  bool well_formed_ = true;
};

extern char32_t const replacement_char_code_point;

template <typename CharType = char, typename OutputIt>
CXX20REQUIRES ((std::output_iterator<OutputIt, CharType>))
OutputIt replacement_char (OutputIt out) {
  *(out++) = static_cast<CharType> (0xEF);
  *(out++) = static_cast<CharType> (0xBF);
  *(out++) = static_cast<CharType> (0xBD);
  return out;
}

// code point to utf8
// ~~~~~~~~~~~~~~~~~~
template <typename CharType = char, typename OutputIt>
CXX20REQUIRES ((std::output_iterator<OutputIt, CharType>))
OutputIt code_point_to_utf8 (char32_t c, OutputIt out) {
  if (c < 0x80) {
    *(out++) = static_cast<CharType> (c);
  } else {
    if (c < 0x800) {
      *(out++) = static_cast<CharType> (c / 64 + 0xC0);
      *(out++) = static_cast<CharType> (c % 64 + 0x80);
    } else if (c >= 0xD800 && c < 0xE000) {
      out = replacement_char<CharType> (out);
    } else if (c < 0x10000) {
      *(out++) = static_cast<CharType> ((c / 0x1000) | 0xE0);
      *(out++) = static_cast<CharType> ((c / 64 % 64) | 0x80);
      *(out++) = static_cast<CharType> ((c % 64) | 0x80);
    } else if (c < 0x110000) {
      *(out++) = static_cast<CharType> ((c / 0x40000) | 0xF0);
      *(out++) = static_cast<CharType> ((c / 0x1000 % 64) | 0x80);
      *(out++) = static_cast<CharType> ((c / 64 % 64) | 0x80);
      *(out++) = static_cast<CharType> ((c % 64) | 0x80);
    } else {
      out = replacement_char<CharType> (out);
    }
  }
  return out;
}

constexpr bool is_utf16_high_surrogate (char16_t code_unit) {
  return code_unit >= 0xD800 && code_unit <= 0xDBFF;
}
constexpr bool is_utf16_low_surrogate (char16_t code_unit) {
  return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
}

// utf16 to code point
// ~~~~~~~~~~~~~~~~~~~
/// \tparam InputIterator  An input iterator which must produce (possible const/volatile qualified) char16_t.
template <typename InputIterator>
CXX20REQUIRES ((std::input_iterator<InputIterator> &&
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
