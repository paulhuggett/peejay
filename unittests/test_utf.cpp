//===- unittests/test_utf.cpp ---------------------------------------------===//
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

#include "json/utf.hpp"

// Standard library includes
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>

// 3rd party includes
#include <gmock/gmock.h>

using namespace peejay;

namespace {

template <typename ResultType>
ResultType code_point_to_utf8_container (char32_t c) {
  ResultType result;
  peejay::code_point_to_utf8<typename ResultType::value_type> (
      c, std::back_inserter (result));
  return result;
}

}  // end anonymous namespace

TEST (CuToUtf8, All) {
  using namespace peejay;

  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x0001),
             utf8_string ({0x01}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x0024),
             utf8_string ({0x24}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x00A2),
             utf8_string ({0xC2, 0xA2}));

  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x007F),
             utf8_string ({0x7F}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x0080),
             utf8_string ({0b11000010, 0x80}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x07FF),
             utf8_string ({0b11011111, 0xBF}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x0800),
             utf8_string ({0xE0, 0xA0, 0x80}));

  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0xD7FF),
             utf8_string ({0xED, 0x9F, 0xBF}));

  // Since RFC 3629 (November 2003), the high and low surrogate halves used by
  // UTF-16 (U+D800 through U+DFFF) and code points not encodable by UTF-16
  // (those after U+10FFFF) are not legal Unicode values
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0xD800),
             utf8_string ({0xEF, 0xBF, 0xBD}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0xDFFF),
             utf8_string ({0xEF, 0xBF, 0xBD}));

  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0xE000),
             utf8_string ({0xEE, 0x80, 0x80}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0xFFFF),
             utf8_string ({0xEF, 0xBF, 0xBF}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x10000),
             utf8_string ({0xF0, 0x90, 0x80, 0x80}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x10FFFF),
             utf8_string ({0xF4, 0x8F, 0xBF, 0xBF}));
  EXPECT_EQ (code_point_to_utf8_container<utf8_string> (0x110000),
             utf8_string ({0xEF, 0xBF, 0xBD}));
}

namespace {

template <typename InputType>
char32_t utf16_to_code_point (InputType const& src) {
  auto const [end, cp] =
      peejay::utf16_to_code_point (std::begin (src), std::end (src));
  assert (end == std::end (src));
  return cp;
}

}  // end anonymous namespace

TEST (Utf16ToCodePoint, All) {
  EXPECT_EQ (utf16_to_code_point (utf16_string ({char16_t{'a'}})), 97U /*'a'*/);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDC00})), 0x010000U);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0x0000})),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDBFF})),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xDFFF})), 0xDFFFU);
}

namespace {

class Utf8Decode : public ::testing::Test {
protected:
  static utf32_string decode_good (std::initializer_list<uint8_t> input) {
    return decode (input, true);
  }
  static utf32_string decode_bad (std::initializer_list<uint8_t> input) {
    return decode (input, false);
  }

private:
  static utf32_string decode (std::initializer_list<uint8_t> input, bool good) {
    utf32_string result;
    utf8_decoder decoder;
    for (uint8_t b : input) {
      if (std::optional<char32_t> const code_point = decoder.get (b)) {
        result += *code_point;
      }
    }
    EXPECT_EQ (decoder.is_well_formed (), good);
    return result;
  }
};

}  // end anonymous namespace

TEST_F (Utf8Decode, Good) {
  EXPECT_EQ (decode_good ({
                 0xCE, 0xBA,  // GREEK SMALL LETTER KAPPA (U+03BA)
                 0xCF, 0x8C,  // GREEK SMALL LETTER OMICRON WITH TONOS (U+03CC)
                 0xCF, 0x83,  // GREEK SMALL LETTER SIGMA (U+03C3)
                 0xCE, 0xBC,  // GREEK SMALL LETTER MU (U+03BC)
                 0xCE, 0xB5,  // GREEK SMALL LETTER EPSILON (U+03B5)
             }),
             utf32_string ({0x03BA, 0x03CC, 0x03C3, 0x03BC, 0x03B5}));
}

TEST_F (Utf8Decode, FirstPossibleSequenceOfACertainLength) {
  EXPECT_EQ (decode_good ({0xC2, 0x80}), utf32_string ({0x00000080}));
  EXPECT_EQ (decode_good ({0xE0, 0xA0, 0x80}), utf32_string ({0x00000800}));
  EXPECT_EQ (decode_good ({0xF0, 0x90, 0x80, 0x80}),
             utf32_string ({0x00010000}));
}
TEST_F (Utf8Decode, LastPossibleSequenceOfACertainLength) {
  EXPECT_EQ (decode_good ({0x7F}), utf32_string ({0x0000007F}));
  EXPECT_EQ (decode_good ({0xDF, 0xBF}), utf32_string ({0x000007FF}));
  EXPECT_EQ (decode_good ({0xEF, 0xBF, 0xBF}), utf32_string ({0x0000FFFF}));
}
TEST_F (Utf8Decode, OtherBoundaryConditions) {
  EXPECT_EQ (decode_good ({0xED, 0x9F, 0xBF}), utf32_string ({0x0000D7FF}));
  EXPECT_EQ (decode_good ({0xEE, 0x80, 0x80}), utf32_string ({0x0000E000}));
  EXPECT_EQ (decode_good ({0xEF, 0xBF, 0xBD}), utf32_string ({0x0000FFFD}));
  EXPECT_EQ (decode_good ({0xF4, 0x8F, 0xBF, 0xBF}),
             utf32_string ({0x0010FFFF}));
}
TEST_F (Utf8Decode, UnexpectedContinuationBytes) {
  decode_bad ({0x80});                    // first continuation byte
  decode_bad ({0xbf});                    // last continuation byte
  decode_bad ({0x80, 0xbf});              // 2 continuation bytes
  decode_bad ({0x80, 0xbf, 0x80});        // 3 continuation bytes
  decode_bad ({0x80, 0xbf, 0x80, 0xbf});  // 4 continuation bytes
}
TEST_F (Utf8Decode, AllPossibleContinuationBytes) {
  for (std::uint8_t v = 0x80; v <= 0xBF; ++v) {
    decode_bad ({v});  // first continuation byte
  }
}
TEST_F (Utf8Decode, LonelyStartCharacters) {
  // All 32 first bytes of 2-byte sequences (0xC0-0xDF), each followed by a
  // space character.
  for (std::uint8_t v = 0xC0; v <= 0xDF; ++v) {
    decode_bad ({v, 0x20});
  }
  // All 16 first bytes of 3-byte sequences (0xE0-0xEF), each followed by a
  // space character.
  for (std::uint8_t v = 0xE0; v <= 0xEF; ++v) {
    decode_bad ({v, 0x20});
  }
  // All 8 first bytes of 4-byte sequences (0xF0-0xF7), each followed by a space
  // character.
  for (std::uint8_t v = 0xF0; v <= 0xF7; ++v) {
    decode_bad ({v, 0x20});
  }
}
TEST_F (Utf8Decode, SequencesWithLastContinuationByteMissing) {
  decode_bad ({0xC0});        // 2-byte sequence with last byte missing (U+0000)
  decode_bad ({0xE0, 0x80});  // 3-byte sequence with last byte missing (U+0000)
  decode_bad (
      {0xF0, 0x80, 0x80});  // 4-byte sequence with last byte missing (U+0000)
  decode_bad ({0xDF});  // 2-byte sequence with last byte missing (U+000007FF)
  decode_bad (
      {0xEF, 0xBF});  // 3-byte sequence with last byte missing (U-0000FFFF)
  decode_bad ({0xF7, 0xBF,
               0xBF});  // 4-byte sequence with last byte missing (U-001FFFFF)

  decode_bad (
      {0xC0, 0xE0, 0x80, 0xF0, 0x80, 0x80, 0xDF, 0xEF, 0xBF, 0xF7, 0xBF, 0xBF});
}
TEST_F (Utf8Decode, ImpossibleBytes) {
  decode_bad ({0xFE});
  decode_bad ({0xFF});
  decode_bad ({0xFE, 0xFE, 0xFF, 0xFF});
}
TEST_F (Utf8Decode, OverlongAscii) {
  decode_bad ({0xC0, 0xAF});              // U+002F
  decode_bad ({0xE0, 0x80, 0xAF});        // U+002F
  decode_bad ({0xF0, 0x80, 0x80, 0xAF});  // U+002F
}
TEST_F (Utf8Decode, MaximumOverlongSequences) {
  decode_bad ({0xC1, 0xBF});              // U-0000007F
  decode_bad ({0xE0, 0x9F, 0xBF});        // U-000007FF
  decode_bad ({0xF0, 0x8F, 0xBF, 0xBF});  // U-0000FFFF
}
TEST_F (Utf8Decode, OverlongNul) {
  decode_bad ({0xC0, 0x80});              // U+0000
  decode_bad ({0xE0, 0x80, 0x80});        // U+0000
  decode_bad ({0xF0, 0x80, 0x80, 0x80});  // U+0000
}
TEST_F (Utf8Decode, IllegalCodePositions) {
  // Single UTF-16 surrogates
  decode_bad ({0xED, 0xA0, 0x80});  // U+D800
  decode_bad ({0xED, 0xAD, 0xBF});  // U+DB7F
  decode_bad ({0xED, 0xAE, 0x80});  // U+DB80
  decode_bad ({0xED, 0xAF, 0xBF});  // U+DBFF
  decode_bad ({0xED, 0xB0, 0x80});  // U+DC00
  decode_bad ({0xED, 0xBE, 0x80});  // U+DF80
  decode_bad ({0xED, 0xBF, 0xBF});  // U+DFFF

  // Paired UTF-16 surrogates
  decode_bad ({0xED, 0xA0, 0x80, 0xED, 0xB0, 0x80});  // U+D800 U+DC00
  decode_bad ({0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF});  // U+D800 U+DFFF
  decode_bad ({0xED, 0xAD, 0xBF, 0xED, 0xB0, 0x80});  // U+DB7F U+DC00
  decode_bad ({0xED, 0xAD, 0xBF, 0xED, 0xBF, 0xBF});  // U+DB7F U+DFFF
  decode_bad ({0xED, 0xAE, 0x80, 0xED, 0xB0, 0x80});  // U+DB80 U+DC00
  decode_bad ({0xED, 0xAE, 0x80, 0xED, 0xBF, 0xBF});  // U+DB80 U+DFFF
  decode_bad ({0xED, 0xAF, 0xBF, 0xED, 0xB0, 0x80});  // U+DBFF U+DC00
  decode_bad ({0xED, 0xAF, 0xBF, 0xED, 0xBF, 0xBF});  // U+DBFF U+DFFF
}
