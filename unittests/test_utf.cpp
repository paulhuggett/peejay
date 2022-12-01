//===- unittests/test_utf.cpp ---------------------------------------------===//
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

#include "peejay/utf.hpp"

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
  code_point_to_utf8 (c, std::back_inserter (result));
  return result;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (CuToUtf8, All) {
  using namespace peejay;

  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x0001),
             u8string ({0x01}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x0024),
             u8string ({0x24}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x00A2),
             u8string ({static_cast<char8> (0xC2), static_cast<char8> (0xA2)}));

  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x007F),
             u8string ({0x7F}));
  EXPECT_EQ (
      code_point_to_utf8_container<u8string> (0x0080),
      u8string ({static_cast<char8> (0b11000010), static_cast<char8> (0x80)}));
  EXPECT_EQ (
      code_point_to_utf8_container<u8string> (0x07FF),
      u8string ({static_cast<char8> (0b11011111), static_cast<char8> (0xBF)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x0800),
             u8string ({static_cast<char8> (0xE0), static_cast<char8> (0xA0),
                        static_cast<char8> (0x80)}));

  EXPECT_EQ (code_point_to_utf8_container<u8string> (0xD7FF),
             u8string ({static_cast<char8> (0xED), static_cast<char8> (0x9F),
                        static_cast<char8> (0xBF)}));

  // Since RFC 3629 (November 2003), the high and low surrogate halves used by
  // UTF-16 (U+D800 through U+DFFF) and code points not encodable by UTF-16
  // (those after U+10FFFF) are not legal Unicode values
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0xD800),
             u8string ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                        static_cast<char8> (0xBD)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0xDFFF),
             u8string ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                        static_cast<char8> (0xBD)}));

  EXPECT_EQ (code_point_to_utf8_container<u8string> (0xE000),
             u8string ({static_cast<char8> (0xEE), static_cast<char8> (0x80),
                        static_cast<char8> (0x80)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0xFFFF),
             u8string ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                        static_cast<char8> (0xBF)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x10000),
             u8string ({static_cast<char8> (0xF0), static_cast<char8> (0x90),
                        static_cast<char8> (0x80), static_cast<char8> (0x80)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x10FFFF),
             u8string ({static_cast<char8> (0xF4), static_cast<char8> (0x8F),
                        static_cast<char8> (0xBF), static_cast<char8> (0xBF)}));
  EXPECT_EQ (code_point_to_utf8_container<u8string> (0x110000),
             u8string ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                        static_cast<char8> (0xBD)}));
}

namespace {

char32_t utf16_to_code_point (std::u16string const& src) {
  auto const [end, cp] =
      peejay::utf16_to_code_point (std::begin (src), std::end (src));
  assert (end == std::end (src));
  return cp;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (Utf16ToCodePoint, All) {
  EXPECT_EQ (utf16_to_code_point (std::u16string ({char16_t{'a'}})),
             97U /*'a'*/);
  EXPECT_EQ (utf16_to_code_point (std::u16string ({0xD800, 0xDC00})),
             0x010000U);
  EXPECT_EQ (utf16_to_code_point (std::u16string ({0xD800, 0x0000})),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (std::u16string ({0xD800, 0xDBFF})),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (std::u16string ({0xDFFF})), 0xDFFFU);
}

namespace {

class Utf8Decode : public ::testing::Test {
protected:
  static std::u32string decode_good (std::initializer_list<char8> input) {
    return decode (input, true);
  }
  static std::u32string decode_bad (std::initializer_list<char8> input) {
    return decode (input, false);
  }

private:
  static std::u32string decode (std::initializer_list<char8> input, bool good) {
    std::u32string result;
    utf8_decoder decoder;
    for (char8 b : input) {
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
  EXPECT_EQ (
      decode_good ({
          static_cast<char8> (0xCE),
          static_cast<char8> (0xBA),  // GREEK SMALL LETTER KAPPA (U+03BA)
          static_cast<char8> (0xCF),
          static_cast<char8> (
              0x8C),  // GREEK SMALL LETTER OMICRON WITH TONOS (U+03CC)
          static_cast<char8> (0xCF),
          static_cast<char8> (0x83),  // GREEK SMALL LETTER SIGMA (U+03C3)
          static_cast<char8> (0xCE),
          static_cast<char8> (0xBC),  // GREEK SMALL LETTER MU (U+03BC)
          static_cast<char8> (0xCE),
          static_cast<char8> (0xB5),  // GREEK SMALL LETTER EPSILON (U+03B5)
      }),
      std::u32string ({0x03BA, 0x03CC, 0x03C3, 0x03BC, 0x03B5}));
}

TEST_F (Utf8Decode, FirstPossibleSequenceOfACertainLength) {
  EXPECT_EQ (
      decode_good ({static_cast<char8> (0xC2), static_cast<char8> (0x80)}),
      std::u32string ({0x00000080}));
  EXPECT_EQ (decode_good ({static_cast<char8> (0xE0), static_cast<char8> (0xA0),
                           static_cast<char8> (0x80)}),
             std::u32string ({0x00000800}));
  EXPECT_EQ (
      decode_good ({static_cast<char8> (0xF0), static_cast<char8> (0x90),
                    static_cast<char8> (0x80), static_cast<char8> (0x80)}),
      std::u32string ({0x00010000}));
}
TEST_F (Utf8Decode, LastPossibleSequenceOfACertainLength) {
  EXPECT_EQ (decode_good ({0x7F}), std::u32string ({0x0000007F}));
  EXPECT_EQ (
      decode_good ({static_cast<char8> (0xDF), static_cast<char8> (0xBF)}),
      std::u32string ({0x000007FF}));
  EXPECT_EQ (decode_good ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                           static_cast<char8> (0xBF)}),
             std::u32string ({0x0000FFFF}));
}
TEST_F (Utf8Decode, OtherBoundaryConditions) {
  EXPECT_EQ (decode_good ({static_cast<char8> (0xED), static_cast<char8> (0x9F),
                           static_cast<char8> (0xBF)}),
             std::u32string ({0x0000D7FF}));
  EXPECT_EQ (decode_good ({static_cast<char8> (0xEE), static_cast<char8> (0x80),
                           static_cast<char8> (0x80)}),
             std::u32string ({0x0000E000}));
  EXPECT_EQ (decode_good ({static_cast<char8> (0xEF), static_cast<char8> (0xBF),
                           static_cast<char8> (0xBD)}),
             std::u32string ({0x0000FFFD}));
  EXPECT_EQ (
      decode_good ({static_cast<char8> (0xF4), static_cast<char8> (0x8F),
                    static_cast<char8> (0xBF), static_cast<char8> (0xBF)}),
      std::u32string ({0x0010FFFF}));
}
TEST_F (Utf8Decode, UnexpectedContinuationBytes) {
  decode_bad ({static_cast<char8> (0x80)});  // first continuation byte
  decode_bad ({static_cast<char8> (0xbf)});  // last continuation byte
  decode_bad ({static_cast<char8> (0x80),
               static_cast<char8> (0xbf)});  // 2 continuation bytes
  decode_bad ({static_cast<char8> (0x80), static_cast<char8> (0xbf),
               static_cast<char8> (0x80)});  // 3 continuation bytes
  decode_bad ({static_cast<char8> (0x80), static_cast<char8> (0xbf),
               static_cast<char8> (0x80),
               static_cast<char8> (0xbf)});  // 4 continuation bytes
}
TEST_F (Utf8Decode, AllPossibleContinuationBytes) {
  for (std::uint8_t v = 0x80; v <= 0xBF; ++v) {
    decode_bad ({static_cast<char8> (v)});  // first continuation byte
  }
}
TEST_F (Utf8Decode, LonelyStartCharacters) {
  // All 32 first bytes of 2-byte sequences (0xC0-0xDF), each followed by a
  // space character.
  for (std::uint8_t v = 0xC0; v <= 0xDF; ++v) {
    decode_bad ({static_cast<char8> (v), 0x20});
  }
  // All 16 first bytes of 3-byte sequences (0xE0-0xEF), each followed by a
  // space character.
  for (std::uint8_t v = 0xE0; v <= 0xEF; ++v) {
    decode_bad ({static_cast<char8> (v), 0x20});
  }
  // All 8 first bytes of 4-byte sequences (0xF0-0xF7), each followed by a space
  // character.
  for (std::uint8_t v = 0xF0; v <= 0xF7; ++v) {
    decode_bad ({static_cast<char8> (v), 0x20});
  }
}
TEST_F (Utf8Decode, SequencesWithLastContinuationByteMissing) {
  decode_bad ({static_cast<char8> (
      0xC0)});  // 2-byte sequence with last byte missing (U+0000)
  decode_bad ({static_cast<char8> (0xE0),
               static_cast<char8> (
                   0x80)});  // 3-byte sequence with last byte missing (U+0000)
  decode_bad ({static_cast<char8> (0xF0), static_cast<char8> (0x80),
               static_cast<char8> (
                   0x80)});  // 4-byte sequence with last byte missing (U+0000)
  decode_bad ({static_cast<char8> (
      0xDF)});  // 2-byte sequence with last byte missing (U+000007FF)
  decode_bad (
      {static_cast<char8> (0xEF),
       static_cast<char8> (
           0xBF)});  // 3-byte sequence with last byte missing (U-0000FFFF)
  decode_bad (
      {static_cast<char8> (0xF7), static_cast<char8> (0xBF),
       static_cast<char8> (
           0xBF)});  // 4-byte sequence with last byte missing (U-001FFFFF)

  decode_bad ({static_cast<char8> (0xC0), static_cast<char8> (0xE0),
               static_cast<char8> (0x80), static_cast<char8> (0xF0),
               static_cast<char8> (0x80), static_cast<char8> (0x80),
               static_cast<char8> (0xDF), static_cast<char8> (0xEF),
               static_cast<char8> (0xBF), static_cast<char8> (0xF7),
               static_cast<char8> (0xBF), static_cast<char8> (0xBF)});
}
TEST_F (Utf8Decode, ImpossibleBytes) {
  decode_bad ({static_cast<char8> (0xFE)});
  decode_bad ({static_cast<char8> (0xFF)});
  decode_bad ({static_cast<char8> (0xFE), static_cast<char8> (0xFE),
               static_cast<char8> (0xFF), static_cast<char8> (0xFF)});
}
TEST_F (Utf8Decode, OverlongAscii) {
  decode_bad (
      {static_cast<char8> (0xC0), static_cast<char8> (0xAF)});  // U+002F
  decode_bad ({static_cast<char8> (0xE0), static_cast<char8> (0x80),
               static_cast<char8> (0xAF)});  // U+002F
  decode_bad ({static_cast<char8> (0xF0), static_cast<char8> (0x80),
               static_cast<char8> (0x80),
               static_cast<char8> (0xAF)});  // U+002F
}
TEST_F (Utf8Decode, MaximumOverlongSequences) {
  decode_bad (
      {static_cast<char8> (0xC1), static_cast<char8> (0xBF)});  // U-0000007F
  decode_bad ({static_cast<char8> (0xE0), static_cast<char8> (0x9F),
               static_cast<char8> (0xBF)});  // U-000007FF
  decode_bad ({static_cast<char8> (0xF0), static_cast<char8> (0x8F),
               static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U-0000FFFF
}
TEST_F (Utf8Decode, OverlongNul) {
  decode_bad (
      {static_cast<char8> (0xC0), static_cast<char8> (0x80)});  // U+0000
  decode_bad ({static_cast<char8> (0xE0), static_cast<char8> (0x80),
               static_cast<char8> (0x80)});  // U+0000
  decode_bad ({static_cast<char8> (0xF0), static_cast<char8> (0x80),
               static_cast<char8> (0x80),
               static_cast<char8> (0x80)});  // U+0000
}
TEST_F (Utf8Decode, IllegalCodePositions) {
  // Single UTF-16 surrogates
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xA0),
               static_cast<char8> (0x80)});  // U+D800
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAD),
               static_cast<char8> (0xBF)});  // U+DB7F
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAE),
               static_cast<char8> (0x80)});  // U+DB80
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAF),
               static_cast<char8> (0xBF)});  // U+DBFF
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xB0),
               static_cast<char8> (0x80)});  // U+DC00
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xBE),
               static_cast<char8> (0x80)});  // U+DF80
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U+DFFF

  // Paired UTF-16 surrogates
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xA0),
               static_cast<char8> (0x80), static_cast<char8> (0xED),
               static_cast<char8> (0xB0),
               static_cast<char8> (0x80)});  // U+D800 U+DC00
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xA0),
               static_cast<char8> (0x80), static_cast<char8> (0xED),
               static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U+D800 U+DFFF
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAD),
               static_cast<char8> (0xBF), static_cast<char8> (0xED),
               static_cast<char8> (0xB0),
               static_cast<char8> (0x80)});  // U+DB7F U+DC00
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAD),
               static_cast<char8> (0xBF), static_cast<char8> (0xED),
               static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U+DB7F U+DFFF
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAE),
               static_cast<char8> (0x80), static_cast<char8> (0xED),
               static_cast<char8> (0xB0),
               static_cast<char8> (0x80)});  // U+DB80 U+DC00
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAE),
               static_cast<char8> (0x80), static_cast<char8> (0xED),
               static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U+DB80 U+DFFF
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAF),
               static_cast<char8> (0xBF), static_cast<char8> (0xED),
               static_cast<char8> (0xB0),
               static_cast<char8> (0x80)});  // U+DBFF U+DC00
  decode_bad ({static_cast<char8> (0xED), static_cast<char8> (0xAF),
               static_cast<char8> (0xBF), static_cast<char8> (0xED),
               static_cast<char8> (0xBF),
               static_cast<char8> (0xBF)});  // U+DBFF U+DFFF
}
