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
#include "json/json.hpp"

using namespace std::string_literals;
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

TEST_F (JsonString, Empty) {
  EXPECT_CALL (callbacks_, string_value (std::string_view{""})).Times (1);

  parser<decltype (proxy_)> p = make_parser (proxy_);
  p.input (R"("")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{3U}, row{1U}}));
}

TEST_F (JsonString, Simple) {
  EXPECT_CALL (callbacks_, string_value (std::string_view{"hello"})).Times (1);

  parser<decltype (proxy_)> p = make_parser (proxy_);
  p.input (R"("hello")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonString, Unterminated) {
  auto p = make_parser (proxy_);
  p.input (R"("hello)"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_close_quote));
  EXPECT_EQ (p.coordinate (), (coord{column{7U}, row{1U}}));
}

TEST_F (JsonString, EscapeN) {
  EXPECT_CALL (callbacks_, string_value (std::string_view{"a\n"})).Times (1);

  auto p = make_parser (proxy_);
  p.input (R"("a\n")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{6U}, row{1U}}));
}

TEST_F (JsonString, BadEscape1) {
  auto p = make_parser (proxy_);
  p.input (R"("a\qb")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::invalid_escape_char));
  EXPECT_EQ (p.coordinate (), (coord{column{4U}, row{1U}}));
}

TEST_F (JsonString, BadEscape2) {
  auto p = make_parser (proxy_);
  p.input ("\"\\\xC3\xBF\""s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::invalid_escape_char));
  EXPECT_EQ (p.coordinate (), (coord{column{4U}, row{1U}}));
}

TEST_F (JsonString, BackslashQuoteUnterminated) {
  auto p = make_parser (proxy_);
  p.input (R"("a\")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_close_quote));
  EXPECT_EQ (p.coordinate (), (coord{column{5U}, row{1U}}));
}

TEST_F (JsonString, TrailingBackslashUnterminated) {
  auto p = make_parser (proxy_);
  p.input (R"("a\)"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_close_quote));
  EXPECT_EQ (p.coordinate (), (coord{column{4U}, row{1U}}));
}

TEST_F (JsonString, GCleffUtf8) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
  // Note that the 4 bytes making up the code point count as a single column.
  EXPECT_CALL (callbacks_, string_value (std::string_view{"\xF0\x9D\x84\x9E"}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input ("\"\xF0\x9D\x84\x9E\""s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{4U}, row{1U}}));
}

TEST_F (JsonString, SlashUnicodeUpper) {
  EXPECT_CALL (callbacks_, string_value (std::string_view{"/"})).Times (1);

  auto p = make_parser (proxy_);
  p.input ("\"\\u002F\""s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{9U}, row{1U}}));
}

TEST_F (JsonString, FourFs) {
  // Note that there is no unicode code-point at U+FFFF.
  EXPECT_CALL (callbacks_, string_value (std::string_view{"\xEF\xBF\xBF"}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input ("\"\\uFFFF\""s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{9U}, row{1U}}));
}

TEST_F (JsonString, TwoUtf16Chars) {
  // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A
  // (u+30A1) expressed as a pair of UTF-16 characters.
  EXPECT_CALL (callbacks_,
               string_value (std::string_view{"\xE2\x85\x8B\xE3\x82\xA1"}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (R"("\u214B\u30A1")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{15U}, row{1U}}));
}

TEST_F (JsonString, Utf16Surrogates) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
  // surrogate pair.
  EXPECT_CALL (callbacks_, string_value (std::string_view{"\xF0\x9D\x84\x9E"}))
      .Times (1);

  auto p = make_parser (proxy_);
  p.input (R"("\uD834\uDD1E")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{15U}, row{1U}}));
}

TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (R"("\uD834\u30A1")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::bad_unicode_code_point));
  EXPECT_EQ (p.coordinate (), (coord{column{13U}, row{1U}}));
}

TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (R"("\uD834!")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::bad_unicode_code_point));
  EXPECT_EQ (p.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
  // surrogate pair.
  auto p = make_parser (proxy_);
  p.input (R"("\uDD1E\u30A1")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::bad_unicode_code_point));
  EXPECT_EQ (p.coordinate (), (coord{column{7U}, row{1U}}));
}

TEST_F (JsonString, ControlCharacter) {
  auto p = make_parser (proxy_);
  p.input ("\"\t\""s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::bad_unicode_code_point));
  EXPECT_EQ (p.coordinate (), (coord{column{2U}, row{1U}}));
}

TEST_F (JsonString, ControlCharacterUTF16) {
  EXPECT_CALL (callbacks_, string_value (std::string_view{"\t"})).Times (1);

  auto p = make_parser (proxy_);
  p.input (R"("\u0009")"s).eof ();
  EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{9U}, row{1U}}));
}

TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser (proxy_);
  p.input (R"("\uD834")"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::bad_unicode_code_point));
  EXPECT_EQ (p.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonString, SlashBadHexChar) {
  auto p = make_parser (proxy_);
  p.input ("\"\\u00xF\""s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::invalid_hex_char));
  EXPECT_EQ (p.coordinate (), (coord{column{6U}, row{1U}}));
}

TEST_F (JsonString, PartialHexChar) {
  auto p = make_parser (proxy_);
  p.input (R"("\u00)"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_close_quote));
  EXPECT_EQ (p.coordinate (), (coord{column{6U}, row{1U}}));
}
