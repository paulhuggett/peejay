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
#include <limits>

// 3rd party includes
#include <gmock/gmock.h>

TEST (CuToUtf8, All) {
  using namespace peejay;

  EXPECT_EQ (code_point_to_utf8<peejay::utf8_string> (0x0001),
             utf8_string ({0x01}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0024), utf8_string ({0x24}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x00A2),
             utf8_string ({0xC2, 0xA2}));

  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x007F), utf8_string ({0x7F}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0080),
             utf8_string ({0b11000010, 0x80}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x07FF),
             utf8_string ({0b11011111, 0xBF}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0800),
             utf8_string ({0xE0, 0xA0, 0x80}));

  EXPECT_EQ (code_point_to_utf8<utf8_string> (0xD7FF),
             utf8_string ({0xED, 0x9F, 0xBF}));

  // Since RFC 3629 (November 2003), the high and low surrogate halves used by
  // UTF-16 (U+D800 through U+DFFF) and code points not encodable by UTF-16
  // (those after U+10FFFF) are not legal Unicode values
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0xD800),
             utf8_string ({0xEF, 0xBF, 0xBD}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0xDFFF),
             utf8_string ({0xEF, 0xBF, 0xBD}));

  EXPECT_EQ (code_point_to_utf8<utf8_string> (0xE000),
             utf8_string ({0xEE, 0x80, 0x80}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0xFFFF),
             utf8_string ({0xEF, 0xBF, 0xBF}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x10000),
             utf8_string ({0xF0, 0x90, 0x80, 0x80}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x10FFFF),
             utf8_string ({0xF4, 0x8F, 0xBF, 0xBF}));
  EXPECT_EQ (code_point_to_utf8<utf8_string> (0x110000),
             utf8_string ({0xEF, 0xBF, 0xBD}));
}

TEST (Utf16ToUtf8, All) {
  using namespace peejay;

  EXPECT_EQ (utf16_to_code_point (utf16_string ({char16_t{'a'}}), nop_swapper),
             97U /*'a'*/);
  EXPECT_EQ (
      utf16_to_code_point (utf16_string ({char16_t{'a'} << 8}), byte_swapper),
      97U /*'a'*/);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0x00E0}), byte_swapper),
             0xE000U);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDC00}), nop_swapper),
             0x010000U);
  EXPECT_EQ (
      utf16_to_code_point (utf16_string ({0x00D8, 0x00DC}), byte_swapper),
      0x010000U);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0x0000}), nop_swapper),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDBFF}), nop_swapper),
             replacement_char_code_point);
  EXPECT_EQ (utf16_to_code_point (utf16_string ({0xDFFF}), nop_swapper),
             0xDFFFU);
}

namespace {
class Utf8Decode : public ::testing::Test {
protected:
  using cpstring = std::basic_string<char32_t>;
  using bytes = std::vector<std::uint8_t>;

  static cpstring decode (peejay::utf8_decoder &decoder, char const *src) {
    cpstring result;
    for (; *src != '\0'; ++src) {
      if (std::optional<char32_t> const code_point =
              decoder.get (static_cast<std::uint8_t> (*src))) {
        result += *code_point;
      }
    }
    return result;
  }

  static cpstring decode (bytes &&input, bool good) {
    input.push_back (0x00);  // add terminating NUL
    peejay::utf8_decoder decoder;
    cpstring result =
        decode (decoder, reinterpret_cast<char const *> (input.data ()));
    EXPECT_EQ (decoder.is_well_formed (), good);
    return result;
  }
  static cpstring decode_good (bytes &&input) {
    return decode (std::move (input), true);
  }
  static cpstring decode_bad (bytes &&input) {
    return decode (std::move (input), false);
  }
};
}  // namespace

TEST_F (Utf8Decode, Good) {
  std::vector<std::uint8_t> test{
      0xCE, 0xBA,  // GREEK SMALL LETTER KAPPA (U+03BA)
      0xCF, 0x8C,  // GREEK SMALL LETTER OMICRON WITH TONOS (U+03CC)
      0xCF, 0x83,  // GREEK SMALL LETTER SIGMA (U+03C3)
      0xCE, 0xBC,  // GREEK SMALL LETTER MU (U+03BC)
      0xCE, 0xB5,  // GREEK SMALL LETTER EPSILON (U+03B5)
  };
  EXPECT_EQ (decode_good (std::move (test)),
             cpstring ({0x03BA, 0x03CC, 0x03C3, 0x03BC, 0x03B5}));
}

TEST_F (Utf8Decode, FirstPossibleSequenceOfACertainLength) {
  EXPECT_EQ (
      decode_good (bytes ({0x00})),
      cpstring ());  // We treat the NUL character as the end of sequence.
  EXPECT_EQ (decode_good (bytes ({0xC2, 0x80})), cpstring ({0x00000080}));
  EXPECT_EQ (decode_good (bytes ({0xE0, 0xA0, 0x80})), cpstring ({0x00000800}));
  EXPECT_EQ (decode_good (bytes ({0xF0, 0x90, 0x80, 0x80})),
             cpstring ({0x00010000}));
}
TEST_F (Utf8Decode, LastPossibleSequenceOfACertainLength) {
  EXPECT_EQ (decode_good (bytes ({0x7F})), cpstring ({0x0000007F}));
  EXPECT_EQ (decode_good (bytes ({0xDF, 0xBF})), cpstring ({0x000007FF}));
  EXPECT_EQ (decode_good (bytes ({0xEF, 0xBF, 0xBF})), cpstring ({0x0000FFFF}));
}
TEST_F (Utf8Decode, OtherBoundaryConditions) {
  EXPECT_EQ (decode_good (bytes ({0xED, 0x9F, 0xBF})), cpstring ({0x0000D7FF}));
  EXPECT_EQ (decode_good (bytes ({0xEE, 0x80, 0x80})), cpstring ({0x0000E000}));
  EXPECT_EQ (decode_good (bytes ({0xEF, 0xBF, 0xBD})), cpstring ({0x0000FFFD}));
  EXPECT_EQ (decode_good (bytes ({0xF4, 0x8F, 0xBF, 0xBF})),
             cpstring ({0x0010FFFF}));
}
TEST_F (Utf8Decode, UnexpectedContinuationBytes) {
  decode_bad (bytes ({0x80}));                    // first continuation byte
  decode_bad (bytes ({0xbf}));                    // last continuation byte
  decode_bad (bytes ({0x80, 0xbf}));              // 2 continuation bytes
  decode_bad (bytes ({0x80, 0xbf, 0x80}));        // 3 continuation bytes
  decode_bad (bytes ({0x80, 0xbf, 0x80, 0xbf}));  // 4 continuation bytes
}
TEST_F (Utf8Decode, AllPossibleContinuationBytes) {
  for (std::uint8_t v = 0x80; v <= 0xBF; ++v) {
    decode_bad (bytes ({v}));  // first continuation byte
  }
}
TEST_F (Utf8Decode, LonelyStartCharacters) {
  // All 32 first bytes of 2-byte sequences (0xC0-0xDF), each followed by a
  // space character.
  for (std::uint8_t v = 0xC0; v <= 0xDF; ++v) {
    decode_bad (bytes ({v, 0x20}));
  }
  // All 16 first bytes of 3-byte sequences (0xE0-0xEF), each followed by a
  // space character.
  for (std::uint8_t v = 0xE0; v <= 0xEF; ++v) {
    decode_bad (bytes ({v, 0x20}));
  }
  // All 8 first bytes of 4-byte sequences (0xF0-0xF7), each followed by a space
  // character.
  for (std::uint8_t v = 0xF0; v <= 0xF7; ++v) {
    decode_bad (bytes ({v, 0x20}));
  }
}
TEST_F (Utf8Decode, SequencesWithLastContinuationByteMissing) {
  decode_bad (
      bytes ({0xC0}));  // 2-byte sequence with last byte missing (U+0000)
  decode_bad (
      bytes ({0xE0, 0x80}));  // 3-byte sequence with last byte missing (U+0000)
  decode_bad (bytes (
      {0xF0, 0x80, 0x80}));  // 4-byte sequence with last byte missing (U+0000)
  decode_bad (
      bytes ({0xDF}));  // 2-byte sequence with last byte missing (U+000007FF)
  decode_bad (bytes (
      {0xEF, 0xBF}));  // 3-byte sequence with last byte missing (U-0000FFFF)
  decode_bad (
      bytes ({0xF7, 0xBF,
              0xBF}));  // 4-byte sequence with last byte missing (U-001FFFFF)

  decode_bad (bytes ({0xC0, 0xE0, 0x80, 0xF0, 0x80, 0x80, 0xDF, 0xEF, 0xBF,
                      0xF7, 0xBF, 0xBF}));
}
TEST_F (Utf8Decode, ImpossibleBytes) {
  decode_bad (bytes ({0xFE}));
  decode_bad (bytes ({0xFF}));
  decode_bad (bytes ({0xFE, 0xFE, 0xFF, 0xFF}));
}
TEST_F (Utf8Decode, OverlongAscii) {
  decode_bad (bytes ({0xC0, 0xAF}));              // U+002F
  decode_bad (bytes ({0xE0, 0x80, 0xAF}));        // U+002F
  decode_bad (bytes ({0xF0, 0x80, 0x80, 0xAF}));  // U+002F
}
TEST_F (Utf8Decode, MaximumOverlongSequences) {
  decode_bad (bytes ({0xC1, 0xBF}));              // U-0000007F
  decode_bad (bytes ({0xE0, 0x9F, 0xBF}));        // U-000007FF
  decode_bad (bytes ({0xF0, 0x8F, 0xBF, 0xBF}));  // U-0000FFFF
}
TEST_F (Utf8Decode, OverlongNul) {
  decode_bad (bytes ({0xC0, 0x80}));              // U+0000
  decode_bad (bytes ({0xE0, 0x80, 0x80}));        // U+0000
  decode_bad (bytes ({0xF0, 0x80, 0x80, 0x80}));  // U+0000
}
TEST_F (Utf8Decode, IllegalCodePositions) {
  // Single UTF-16 surrogates
  decode_bad (bytes ({0xED, 0xA0, 0x80}));  // U+D800
  decode_bad (bytes ({0xED, 0xAD, 0xBF}));  // U+DB7F
  decode_bad (bytes ({0xED, 0xAE, 0x80}));  // U+DB80
  decode_bad (bytes ({0xED, 0xAF, 0xBF}));  // U+DBFF
  decode_bad (bytes ({0xED, 0xB0, 0x80}));  // U+DC00
  decode_bad (bytes ({0xED, 0xBE, 0x80}));  // U+DF80
  decode_bad (bytes ({0xED, 0xBF, 0xBF}));  // U+DFFF

  // Paired UTF-16 surrogates
  decode_bad (bytes ({0xED, 0xA0, 0x80, 0xED, 0xB0, 0x80}));  // U+D800 U+DC00
  decode_bad (bytes ({0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF}));  // U+D800 U+DFFF
  decode_bad (bytes ({0xED, 0xAD, 0xBF, 0xED, 0xB0, 0x80}));  // U+DB7F U+DC00
  decode_bad (bytes ({0xED, 0xAD, 0xBF, 0xED, 0xBF, 0xBF}));  // U+DB7F U+DFFF
  decode_bad (bytes ({0xED, 0xAE, 0x80, 0xED, 0xB0, 0x80}));  // U+DB80 U+DC00
  decode_bad (bytes ({0xED, 0xAE, 0x80, 0xED, 0xBF, 0xBF}));  // U+DB80 U+DFFF
  decode_bad (bytes ({0xED, 0xAF, 0xBF, 0xED, 0xB0, 0x80}));  // U+DBFF U+DC00
  decode_bad (bytes ({0xED, 0xAF, 0xBF, 0xED, 0xBF, 0xBF}));  // U+DBFF U+DFFF
}
