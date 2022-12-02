//===- unittests/test_string.cpp ------------------------------------------===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "callbacks.hpp"
#include "peejay/json.hpp"

using namespace std::string_view_literals;

using testing::DoubleEq;
using testing::StrictMock;

using namespace peejay;

namespace {

class JsonString : public testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (JsonString, EmptyDoubleQuote) {
  EXPECT_CALL (callbacks_, string_value (u8""sv)).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, EmptySingleQuote) {
  EXPECT_CALL (callbacks_, string_value (u8""sv)).Times (1);

  auto p = make_parser (proxy_, extensions::single_quote_string);
  p.input (u8R"('')"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, EmptySingleQuoteExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8R"('')"sv).eof ();
  EXPECT_TRUE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{1U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, SimpleDoubleQuote) {
  EXPECT_CALL (callbacks_, string_value (u8"hello"sv)).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("hello")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{7U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, SimpleSingleQuote) {
  EXPECT_CALL (callbacks_, string_value (u8"hello"sv)).Times (1);

  auto p = make_parser (proxy_, extensions::single_quote_string);
  p.input (u8R"('hello')"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{7U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, UnterminatedDoubleQuote) {
  auto p = make_parser (proxy_);
  p.input (u8R"("hello)"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_close_quote));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, UnterminatedSingleQuote) {
  auto p = make_parser (proxy_, extensions::single_quote_string);
  p.input (u8R"('hello)"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_close_quote));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, EscapeN) {
  EXPECT_CALL (callbacks_, string_value (u8"a\n"sv)).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("a\n")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{5U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, BadEscape1) {
  auto p = make_parser (proxy_);
  p.input (u8R"("a\qb")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::invalid_escape_char));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, BadEscape2) {
  auto p = make_parser (proxy_);
  p.input (u8"\"\\\xC3\xBF\""sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::invalid_escape_char));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, BackslashQuoteUnterminated) {
  auto p = make_parser (proxy_);
  p.input (u8R"("a\")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_close_quote));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{5U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, TrailingBackslashUnterminated) {
  auto p = make_parser (proxy_);
  p.input (u8R"("a\)"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_close_quote));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, GCleffUtf8) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
  // Note that the 4 bytes making up the code point count as a single column.
  std::array<char8, 4> gclef{
      {static_cast<char8> (0xF0), static_cast<char8> (0x9D),
       static_cast<char8> (0x84), static_cast<char8> (0x9E)}};
  EXPECT_CALL (callbacks_,
               string_value (u8string_view{gclef.data (), gclef.size ()}))
      .Times (1);

  auto p = make_parser (proxy_);

  std::array<char8 const, 6> const input{
      {'"', static_cast<char8> (0xF0), static_cast<char8> (0x9D),
       static_cast<char8> (0x84), static_cast<char8> (0x9E), '"'}};
  p.input (std::begin (input), std::end (input)).eof ();

  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, SlashUnicodeUpper) {
  EXPECT_CALL (callbacks_, string_value (u8"/"sv)).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\u002F")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, SlashUnicodeLower) {
  std::array<char8 const, 2> const expected{
      {static_cast<char8> (0xC2), static_cast<char8> (0xAF)}};
  EXPECT_CALL (callbacks_,
               string_value (u8string_view{expected.data (), expected.size ()}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\u00af")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, FourFs) {
  // Note that there is no unicode code-point at U+FFFF.
  std::array<char8 const, 3> const expected{{static_cast<char8> (0xEF),
                                             static_cast<char8> (0xBF),
                                             static_cast<char8> (0xBF)}};
  EXPECT_CALL (callbacks_,
               string_value (u8string_view{expected.data (), expected.size ()}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\uFFFF")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, TwoUtf16Chars) {
  // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A
  // (u+30A1) expressed as a pair of UTF-16 characters.
  std::array<char8 const, 6> const expected{
      {static_cast<char8> (0xE2), static_cast<char8> (0x85),
       static_cast<char8> (0x8B), static_cast<char8> (0xE3),
       static_cast<char8> (0x82), static_cast<char8> (0xA1)}};
  EXPECT_CALL (callbacks_,
               string_value (u8string_view{expected.data (), expected.size ()}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\u214B\u30A1")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{14U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{15U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16Surrogates) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
  // surrogate pair.
  std::array<char8 const, 4> const expected{
      {static_cast<char8> (0xF0), static_cast<char8> (0x9D),
       static_cast<char8> (0x84), static_cast<char8> (0x9E)}};
  EXPECT_CALL (callbacks_,
               string_value (u8string_view{expected.data (), expected.size ()}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\uD834\uDD1E")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{14U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{15U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (u8R"("\uD834\u30A1")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{13U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (u8R"("\uD834!")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
  auto p = make_parser (proxy_);
  p.input (u8R"("\uDD1E\u30A1")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16HighSurrogateFollowedByHighSurrogate) {
  auto p = make_parser (proxy_);
  p.input (u8R"("\uD800\uD800")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{13U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, ControlCharacter) {
  auto p = make_parser (proxy_);
  p.input (u8"\"\t\""sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, ControlCharacterUTF16) {
  EXPECT_CALL (callbacks_, string_value (u8"\t"sv)).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"("\u0009")"sv).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (u8R"("\uD834")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, SlashBadHexChar) {
  auto p = make_parser (proxy_);
  p.input (u8R"("\u00xf")"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::invalid_hex_char));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonString, PartialHexChar) {
  auto p = make_parser (proxy_);
  p.input (u8R"("\u00)"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_close_quote));
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{6U}, line{1U}}));
}
