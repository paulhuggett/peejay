//===- unit_tests/test_transcoder_input.cpp -------------------------------===//
//*  _                                _           _                _    *
//* | |_ _ _ __ _ _ _  ___ __ ___  __| |___ _ _  (_)_ _  _ __ _  _| |_  *
//* |  _| '_/ _` | ' \(_-</ _/ _ \/ _` / -_) '_| | | ' \| '_ \ || |  _| *
//*  \__|_| \__,_|_||_/__/\__\___/\__,_\___|_|   |_|_||_| .__/\_,_|\__| *
//*                                                     |_|             *
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
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "callbacks.hpp"
#include "icubaby/icubaby.hpp"
#include "peejay/json.hpp"

using namespace std::string_view_literals;

namespace {

using peejay::error;
using peejay::parser;

TEST(TranscoderInput, Utf8WellFormed) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(utf8, u8"[\"caf\u00E9\"]"sv, true).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_TRUE(utf8.well_formed());
  EXPECT_EQ(res, u8"[ \"caf\u00E9\" ]");
}

TEST(TranscoderInput, Utf16WellFormed) {
  icubaby::t16_8 utf16;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(utf16, u"[\"caf\u00E9\"]"sv, true).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"[ \"caf\u00E9\" ]");
}

TEST(TranscoderInput, Utf16SurrogatePair) {
  icubaby::t16_8 utf16;
  parser p{json_out_callbacks{}};
  // U+1F600 GRINNING FACE is encoded as the surrogate pair D83D DE00.
  std::u8string const res = p.input(utf16, u"\"\xD83D\xDE00\""sv, true).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"\"\U0001F600\"");
}

TEST(TranscoderInput, Utf32WellFormed) {
  icubaby::t32_8 utf32;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(utf32, U"[\"\U0001F600\",1]"sv, true).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"[ \"\U0001F600\" 1 ]");
}

TEST(TranscoderInput, Bytes) {
  // A UTF-16 BE byte order mark followed by "[1]".
  std::array const src{std::byte{0xFE}, std::byte{0xFF}, std::byte{0x00}, std::byte{'['},
                       std::byte{0x00}, std::byte{'1'},  std::byte{0x00}, std::byte{']'}};
  icubaby::tx_8 bytes;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(bytes, src, true).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"[ 1 ]");
}

TEST(TranscoderInput, SequenceSplitBetweenCalls) {
  // U+00E9 is C3 A9 in UTF-8. The two bytes are passed in separate calls.
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  p.input(utf8, std::array{u8'"', static_cast<char8_t>(0xC3)}, true);
  EXPECT_TRUE(utf8.partial());
  p.input(utf8, std::array{static_cast<char8_t>(0xA9), u8'"'}, true);
  EXPECT_FALSE(utf8.partial());
  std::u8string const res = p.eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"\"\u00E9\"");
}

TEST(TranscoderInput, IllFormedStop) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::array const src{u8'[', u8'"', u8'a', static_cast<char8_t>(0xFF), u8'b', u8'"', u8']'};
  std::u8string const res = p.input(utf8, src, true).eof();
  EXPECT_FALSE(utf8.well_formed());
  EXPECT_EQ(p.last_error(), make_error_code(error::ill_formed_input)) << "Real error was: " << p.last_error().message();
  // The parse stopped inside the string, so the backend saw only the start of the array.
  EXPECT_EQ(res, u8"[");
}

TEST(TranscoderInput, IllFormedContinue) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::array const src{u8'[', u8'"', u8'a', static_cast<char8_t>(0xFF), u8'b', u8'"', u8']'};
  std::u8string const res = p.input(utf8, src, false).eof();
  EXPECT_FALSE(utf8.well_formed());
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"[ \"a\uFFFDb\" ]");
}

TEST(TranscoderInput, IllFormedTruncatedSequenceContinue) {
  // E2 82 is the start of a three-byte sequence (such as E2 82 AC, U+20AC EURO SIGN) but '"' follows it.
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::array const src{u8'"', static_cast<char8_t>(0xE2), static_cast<char8_t>(0x82), u8'"'};
  std::u8string const res = p.input(utf8, src, false).eof();
  EXPECT_FALSE(utf8.well_formed());
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"\"\uFFFD\"");
}

TEST(TranscoderInput, IllFormedUtf16Stop) {
  // An unpaired high surrogate.
  icubaby::t16_8 utf16;
  parser p{json_out_callbacks{}};
  p.input(utf16, u"\"\xD800\""sv, true).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::ill_formed_input)) << "Real error was: " << p.last_error().message();
}

TEST(TranscoderInput, StopAfterEarlierIllFormedInput) {
  // well_formed() stays false once ill-formed input has been seen, so a later call that asks to stop does so at once.
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  p.input(utf8, std::array{u8'"', static_cast<char8_t>(0xFF)}, false);
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  p.input(utf8, u8"\""sv, true);
  EXPECT_EQ(p.last_error(), make_error_code(error::ill_formed_input)) << "Real error was: " << p.last_error().message();
}

TEST(TranscoderInput, JsonErrorStopsInput) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(utf8, u8"[1 2]"sv, true).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_array_member))
      << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"[ 1");
}

TEST(TranscoderInput, JsonErrorPartWayThroughCodePoint) {
  // U+00E9 is not a valid token. Its first UTF-8 byte (C3) raises the error and the second (A9) is ignored.
  icubaby::t32_8 utf32;
  parser p{json_out_callbacks{}};
  p.input(utf32, U"\u00E9"sv, true).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token)) << "Real error was: " << p.last_error().message();
}

TEST(TranscoderInput, IgnoredAfterError) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  p.input(u8"]"sv);
  ASSERT_EQ(p.last_error(), make_error_code(error::expected_token));
  // The parser is already in an error state, so the transcoder isn't given anything.
  p.input(utf8, std::array{static_cast<char8_t>(0xFF)}, true);
  EXPECT_TRUE(utf8.well_formed());
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
}

TEST(TranscoderEof, WellFormed) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(utf8, u8"[\"caf\u00E9\"]"sv, true).eof(utf8, true);
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_TRUE(utf8.well_formed());
  EXPECT_EQ(res, u8"[ \"caf\u00E9\" ]");
}

TEST(TranscoderEof, TruncatedStop) {
  // E2 82 is the start of a three-byte sequence but the input ends after it.
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::u8string const res =
      p.input(utf8, std::array{u8'1', static_cast<char8_t>(0xE2), static_cast<char8_t>(0x82)}, true).eof(utf8, true);
  EXPECT_EQ(p.last_error(), make_error_code(error::ill_formed_input)) << "Real error was: " << p.last_error().message();
  EXPECT_FALSE(utf8.well_formed());
  EXPECT_FALSE(utf8.partial());
  EXPECT_EQ(res, u8"");
}

TEST(TranscoderEof, TruncatedContinue) {
  // The U+FFFD that replaces the incomplete sequence follows the top-level value. (Without the flush, the parser
  // would never see the incomplete sequence and "1" would be accepted.)
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  std::u8string const res =
      p.input(utf8, std::array{u8'1', static_cast<char8_t>(0xE2), static_cast<char8_t>(0x82)}).eof(utf8);
  EXPECT_EQ(p.last_error(), make_error_code(error::unexpected_extra_input))
      << "Real error was: " << p.last_error().message();
  EXPECT_FALSE(utf8.partial());
  EXPECT_EQ(res, u8"1");
}

TEST(TranscoderEof, Utf16UnpairedHighSurrogateAtEnd) {
  icubaby::t16_8 utf16;
  parser p{json_out_callbacks{}};
  p.input(utf16, u"\"a\xD800"sv, true).eof(utf16, true);
  EXPECT_EQ(p.last_error(), make_error_code(error::ill_formed_input)) << "Real error was: " << p.last_error().message();
}

TEST(TranscoderEof, FlushesTranscoderAfterError) {
  icubaby::t8_8 utf8;
  parser p{json_out_callbacks{}};
  p.input(utf8, std::array{u8'"', static_cast<char8_t>(0xE2)}, true);
  EXPECT_TRUE(utf8.partial());
  // An unescaped control character in a string is a JSON error.
  p.input(std::array{static_cast<char8_t>(0x01)});
  ASSERT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  p.eof(utf8, true);
  // The transcoder is flushed, but the parser's original error is kept.
  EXPECT_FALSE(utf8.partial());
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
}

}  // end anonymous namespace
