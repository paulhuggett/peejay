//===- unit_tests/test_string.cpp -----------------------------------------===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
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
#include <vector>

#include "callbacks.hpp"
#include "peejay/json.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

using testing::StrictMock;

using coord = peejay::coord<true>;
using peejay::error;
using peejay::make_parser;

namespace {

class String : public testing::Test {
protected:
  StrictMock<mock_json_callbacks<std::uint64_t, double, char8_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t, double, char8_t>> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(String, EmptyDoubleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8""sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 3U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, SimpleDoubleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8"hello"sv)).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8R"("hello")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 8U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, UnterminatedDoubleQuote) {
  auto p = make_parser(proxy_);
  input(p, u8R"("hello)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 7U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, EscapeN) {
  EXPECT_CALL(callbacks_, string_value(u8"a\n"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("a\n")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 6U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, BadEscape1) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\qb")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 4U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, BadEscape2) {
  auto p = make_parser(proxy_);
  std::array const input{std::uint8_t{'"'}, std::uint8_t{'\\'}, std::uint8_t{0xC3}, std::uint8_t{0xBF},
                         std::uint8_t{'"'}};
  auto const *const b = std::bit_cast<char8_t const *>(input.data());
  p.input(std::ranges::subrange{b, b + input.size()}).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 3U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, BackslashQuoteUnterminated) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 5U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, TrailingBackslashUnterminated) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 4U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, FourWaysToWriteSolidus) {
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, string_value(u8"/"sv)).Times(4);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"([ "\u002F", "\u002f", "\/", "/" ])"sv).eof();

  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, GCleffUtf8) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
  // Note that the 4 bytes making up the code point count as a single column.
  std::array const gclef{static_cast<char8_t>(0xF0), static_cast<char8_t>(0x9D), static_cast<char8_t>(0x84),
                         static_cast<char8_t>(0x9E)};
  EXPECT_CALL(callbacks_, string_value(std::u8string_view{gclef.data(), gclef.size()})).Times(1);

  auto p = make_parser(proxy_);

  std::vector<char8_t> src;
  src.push_back(u8'"');  // code point 1
  std::ranges::copy(gclef, std::back_inserter(src));
  src.push_back(u8'"');  // code point 3
  p.input(src).eof();

  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 4U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, SlashUnicodeUpper) {
  EXPECT_CALL(callbacks_, string_value(u8"/"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u002F")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 9U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, SlashUnicodeLower) {
  std::array const expected{static_cast<char8_t>(0xC2), static_cast<char8_t>(0xAF)};
  EXPECT_CALL(callbacks_, string_value(std::u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u00af")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 9U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, FourFs) {
  // Note that there is no unicode code-point at U+FFFF.
  std::array const expected{static_cast<char8_t>(0xEF), static_cast<char8_t>(0xBF), static_cast<char8_t>(0xBF)};
  EXPECT_CALL(callbacks_, string_value(std::u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\uFFFF")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 9U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, TwoUtf16Chars) {
  // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A
  // (u+30A1) expressed as a pair of UTF-16 characters.
  std::array const expected{static_cast<char8_t>(0xE2), static_cast<char8_t>(0x85), static_cast<char8_t>(0x8B),
                            static_cast<char8_t>(0xE3), static_cast<char8_t>(0x82), static_cast<char8_t>(0xA1)};
  EXPECT_CALL(callbacks_, string_value(std::u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u214B\u30A1")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 15U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16Surrogates) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
  // surrogate pair.
  std::array const expected{static_cast<char8_t>(0xF0), static_cast<char8_t>(0x9D), static_cast<char8_t>(0x84),
                            static_cast<char8_t>(0x9E)};
  EXPECT_CALL(callbacks_, string_value(std::u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834\uDD1E")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 15U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighWithNoLowSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834\u30A1")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 13U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighFollowedByUtf8Char) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834!")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 8U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighWithMissingLowSurrogate) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\uDD1E\u30A1")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 7U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighSurrogateFollowedByHighSurrogate) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD800\uD800")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 13U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, ControlCharacter) {
  auto p = make_parser(proxy_);
  input(p, u8"\"\t\""sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 2U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, ControlCharacterUTF16) {
  EXPECT_CALL(callbacks_, string_value(u8"\t"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u0009")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 9U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Utf16LowWithNoHighSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 8U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, SlashBadHexChar) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\u00xf")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_hex_char));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 6U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, PartialHexChar) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\u00)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 6U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, Escape0Disabled) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\0")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Error was: " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 3U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, EscapeVDisabled) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\v")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Error was: " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 3U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(String, StringValueReturnsAnError) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::io_error);
  EXPECT_CALL(callbacks_, string_value(u8"hello"sv)).Times(1).WillOnce(Return(erc));

  auto p = make_parser(proxy_);
  p.input(u8R"("hello")"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

struct ml10_policy : public peejay::default_policies {
  static constexpr auto max_length = std::size_t{10};
};

class StringLength10 : public testing::Test {
protected:
  StrictMock<mock_json_callbacks<std::uint64_t, double, char8_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t, double, char8_t>, ml10_policy> proxy_{callbacks_};
};

// NOLINTNEXTLINE
TEST_F(StringLength10, MaxLength) {
  EXPECT_CALL(callbacks_, string_value(u8"0123456789"sv)).Times(1);
  auto p = make_parser(proxy_);
  input(p, u8R"("0123456789")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(StringLength10, OnePastMaxLength) {
  auto p = make_parser(proxy_);
  input(p, u8R"("01234567890")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(StringLength10, OneUtf8HexPastMaxLength) {
  auto p = make_parser(proxy_);
  input(p, u8R"("0123456789\u0030")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(StringLength10, OneUtf16HexPastMaxLength) {
  auto p = make_parser(proxy_);
  input(p, u8R"("0123456789\uD834\uDD1E")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TEST_F(StringLength10, OneEscapePastMaxLength) {
  auto p = make_parser(proxy_);
  input(p, u8R"("0123456789\n")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(StringLength10, UTF8TooLong) {
  auto p = make_parser(proxy_);
  // smiling face witth sunglasses (U+1F60E)
  std::array const str{std::byte{0xF0}, std::byte{0x9F}, std::byte{0x98}, std::byte{0x8E}};

  auto b = std::bit_cast<char8_t const *>(str.data());
  std::ranges::subrange str2{b, b + str.size()};
  p.input(u8"\""sv);
  p.input(str2);
  p.input(str2);
  p.input(str2);
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// TODO: enumerate the various character types.
class StringCharType : public testing::Test {
protected:
  using char_type = char;
  struct string_view_policy : public peejay::default_policies {
    using char_type = char;
  };

  using mocks = mock_json_callbacks<std::uint64_t, double, char_type>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks, string_view_policy> proxy_{callbacks_};
};

TEST_F(StringCharType, StringView) {
  EXPECT_CALL(callbacks_, string_value("hello"sv)).Times(1);
  auto p = make_parser(proxy_);
  p.input(R"("hello")"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Error was: " << p.last_error().message();
}
TEST_F(StringCharType, BadEscape) {
  auto p = make_parser(proxy_);
  p.input(R"("\v")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Error was: " << p.last_error().message();
}
