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
#include <cstdint>
#include <iosfwd>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace peejay {

using utf8_string = std::basic_string<std::uint8_t>;
using utf16_string = std::basic_string<char16_t>;

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
OutputIt replacement_char (OutputIt out) {
  *(out++) = static_cast<CharType> (0xEF);
  *(out++) = static_cast<CharType> (0xBF);
  *(out++) = static_cast<CharType> (0xBD);
  return out;
}

template <typename ResultType>
ResultType replacement_char () {
  ResultType result;
  replacement_char (std::back_inserter (result));
  return result;
}

template <typename CharType = char, typename OutputIt>
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

template <typename ResultType>
ResultType code_point_to_utf8 (char32_t c) {
  ResultType result;
  code_point_to_utf8<typename ResultType::value_type> (
      c, std::back_inserter (result));
  return result;
}

inline std::uint16_t nop_swapper (std::uint16_t v) {
  return v;
}
inline std::uint16_t byte_swapper (std::uint16_t v) {
  return static_cast<std::uint16_t> (((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8));
}

inline bool is_utf16_high_surrogate (std::uint16_t code_unit) {
  return code_unit >= 0xD800 && code_unit <= 0xDBFF;
}
inline bool is_utf16_low_surrogate (std::uint16_t code_unit) {
  return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
}

// utf16_to_code_point
// ~~~~~~~~~~~~~~~~~~~
template <typename InputIterator, typename SwapperFunction>
std::pair<InputIterator, char32_t> utf16_to_code_point (
    InputIterator first, InputIterator last, SwapperFunction swapper) {
  using value_type = typename std::remove_cv_t<
      typename std::iterator_traits<InputIterator>::value_type>;
  static_assert (std::is_same_v<value_type, char16_t>,
                 "iterator must produce char16_t");

  assert (first != last);
  char32_t code_point = 0;
  char16_t code_unit = swapper (*(first++));
  if (!is_utf16_high_surrogate (code_unit)) {
    code_point = code_unit;
  } else if (first == last) {
    code_point = replacement_char_code_point;
  } else {
    auto const high = code_unit;
    auto const low = swapper (*(first++));

    if (low < 0xDC00 || low > 0xDFFF) {
      code_point = replacement_char_code_point;
    } else {
      code_point = 0x10000;
      code_point += (high & 0x03FFU) << 10U;
      code_point += (low & 0x03FFU);
    }
  }
  return {first, code_point};
}

// utf16_to_code_points
// ~~~~~~~~~~~~~~~~~~~~
template <typename InputIt, typename OutputIt, typename Swapper>
OutputIt utf16_to_code_points (InputIt first, InputIt last, OutputIt out,
                               Swapper swapper) {
  while (first != last) {
    char32_t code_point;
    std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
    *(out++) = code_point;
  }
  return out;
}

template <typename ResultType, typename InputIt, typename Swapper>
ResultType utf16_to_code_points (InputIt first, InputIt last, Swapper swapper) {
  ResultType result;
  utf16_to_code_points (first, last, std::back_inserter (result), swapper);
  return result;
}

template <typename ResultType, typename InputType, typename Swapper>
ResultType utf16_to_code_points (InputType const& src, Swapper swapper) {
  return utf16_to_code_points<ResultType> (std::begin (src), std::end (src),
                                           swapper);
}

// utf16_to_code_point
// ~~~~~~~~~~~~~~~~~~~
template <typename InputType, typename Swapper>
char32_t utf16_to_code_point (InputType const& src, Swapper swapper) {
  auto end = std::end (src);
  char32_t cp;
  std::tie (end, cp) = utf16_to_code_point (std::begin (src), end, swapper);
  assert (end == std::end (src));
  return cp;
}

// utf16_to_utf8
// ~~~~~~~~~~~~~
template <typename InputIt, typename OutputIt, typename Swapper>
OutputIt utf16_to_utf8 (InputIt first, InputIt last, OutputIt out,
                        Swapper swapper) {
  while (first != last) {
    char32_t code_point;
    std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
    out = code_unit_to_utf8 (code_point, out);
  }
  return out;
}

template <typename ResultType, typename InputIt, typename Swapper>
ResultType utf16_to_utf8 (InputIt first, InputIt last, Swapper swapper) {
  ResultType result;
  utf16_to_utf8 (first, last, std::back_inserter (result), swapper);
  return result;
}

template <typename ResultType, typename InputType, typename Swapper>
ResultType utf16_to_utf8 (InputType const& src, Swapper swapper) {
  return utf16_to_utf8<ResultType> (std::begin (src), std::end (src), swapper);
}

}  // namespace peejay

#endif  // PEEJAY_UTF_HPP
