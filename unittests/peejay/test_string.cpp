//===- unittests/peejay/test_string.cpp -----------------------------------===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "callbacks.hpp"
#include "peejay/json/json.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

using testing::StrictMock;

using peejay::char8;
using peejay::char_set;
using column = peejay::coord::column;
using peejay::coord;
using peejay::error;
using peejay::extensions;
using line = peejay::coord::line;
using peejay::make_parser;
using peejay::u8string_view;

namespace {

class String : public testing::Test {
protected:
  StrictMock<mock_json_callbacks<std::uint64_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t>> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(String, EmptyDoubleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8""sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{2U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, EmptySingleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8""sv)).Times(1);

  auto p = make_parser(proxy_, extensions::single_quote_string);
  input(p, u8R"('')"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{2U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, EmptySingleQuoteExtensionDisabled) {
  auto p = make_parser(proxy_);
  input(p, u8R"('')"sv).eof();
  EXPECT_TRUE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{1U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, SimpleDoubleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8"hello"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("hello")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{7U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, SimpleSingleQuote) {
  EXPECT_CALL(callbacks_, string_value(u8"hello"sv)).Times(1);

  auto p = make_parser(proxy_, extensions::single_quote_string);
  input(p, u8R"('hello')"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{7U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, UnterminatedDoubleQuote) {
  auto p = make_parser(proxy_);
  input(p, u8R"("hello)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, UnterminatedSingleQuote) {
  auto p = make_parser(proxy_, extensions::single_quote_string);
  input(p, u8R"('hello)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{7U}, line{1U}}));
}

struct ml10_policy {
  static constexpr auto max_length = size_t{10};
  using integer_type = std::int64_t;
};

// NOLINTNEXTLINE
TEST_F(String, MaxLength) {
  EXPECT_CALL(callbacks_, string_value(u8"0123456789"sv)).Times(1);
  auto p = make_parser<ml10_policy>(proxy_);
  input(p, u8R"("0123456789")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, OnePastMaxLength) {
  auto p = make_parser<ml10_policy>(proxy_);
  input(p, u8R"("01234567890")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, OneUtf8HexPastMaxLength) {
  auto p = make_parser<ml10_policy>(proxy_);
  input(p, u8R"("0123456789\u0030")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, OneUtf16HexPastMaxLength) {
  auto p = make_parser<ml10_policy>(proxy_);
  input(p, u8R"("0123456789\uD834\uDD1E")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::string_too_long)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, EscapeN) {
  EXPECT_CALL(callbacks_, string_value(u8"a\n"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("a\n")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{5U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, BadEscape1) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\qb")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, BadEscape2) {
  auto p = make_parser(proxy_);
  input(p, u8"\"\\\xC3\xBF\""sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, XEscape) {
  EXPECT_CALL(callbacks_, string_value(u8"/"sv)).Times(1);

  auto p = make_parser(proxy_, extensions::string_escapes);
  // String contains just U+002F SOLIDUS ('/')
  input(p, u8R"("\x2f")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, BackslashQuoteUnterminated) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{5U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, TrailingBackslashUnterminated) {
  auto p = make_parser(proxy_);
  input(p, u8R"("a\)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, FiveWaysToWriteSolidus) {
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, string_value(u8"/"sv)).Times(5);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_, extensions::string_escapes);
  input(p, u8R"([ "\x2F", "\u002F", "\u002f", "\/", "/" ])"sv).eof();

  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(String, GCleffUtf8) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
  // Note that the 4 bytes making up the code point count as a single column.
  std::array<char8, 4> gclef{
      {static_cast<char8>(0xF0), static_cast<char8>(0x9D), static_cast<char8>(0x84), static_cast<char8>(0x9E)}};
  EXPECT_CALL(callbacks_, string_value(u8string_view{gclef.data(), gclef.size()})).Times(1);

  auto p = make_parser(proxy_);

  peejay::small_vector<std::byte, 9> src;
  src.push_back(std::byte{0xEF});  // three byte UTF-8 BOM.
  src.push_back(std::byte{0xBB});
  src.push_back(std::byte{0xBF});
  src.push_back(static_cast<std::byte>(char_set::quotation_mark));  // code point 1
  auto const op = [](char8 const c) { return static_cast<std::byte>(c); };
#if __cpp_lib_ranges
  std::ranges::transform(gclef, std::back_inserter(src), op);  // code point 2
#else
  std::transform(std::begin(gclef), std::end(gclef), std::back_inserter(src), op);
#endif
  src.push_back(static_cast<std::byte>(char_set::quotation_mark));  // code point 3
  p.input(std::begin(src), std::end(src)).eof();

  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{3U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, SlashUnicodeUpper) {
  EXPECT_CALL(callbacks_, string_value(u8"/"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u002F")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{8U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, SlashUnicodeLower) {
  std::array<char8 const, 2> const expected{{static_cast<char8>(0xC2), static_cast<char8>(0xAF)}};
  EXPECT_CALL(callbacks_, string_value(u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u00af")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{8U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, FourFs) {
  // Note that there is no unicode code-point at U+FFFF.
  std::array<char8 const, 3> const expected{
      {static_cast<char8>(0xEF), static_cast<char8>(0xBF), static_cast<char8>(0xBF)}};
  EXPECT_CALL(callbacks_, string_value(u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\uFFFF")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{8U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, TwoUtf16Chars) {
  // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A
  // (u+30A1) expressed as a pair of UTF-16 characters.
  std::array<char8 const, 6> const expected{{static_cast<char8>(0xE2), static_cast<char8>(0x85),
                                             static_cast<char8>(0x8B), static_cast<char8>(0xE3),
                                             static_cast<char8>(0x82), static_cast<char8>(0xA1)}};
  EXPECT_CALL(callbacks_, string_value(u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u214B\u30A1")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{14U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{15U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16Surrogates) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
  // surrogate pair.
  std::array<char8 const, 4> const expected{
      {static_cast<char8>(0xF0), static_cast<char8>(0x9D), static_cast<char8>(0x84), static_cast<char8>(0x9E)}};
  EXPECT_CALL(callbacks_, string_value(u8string_view{expected.data(), expected.size()})).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834\uDD1E")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{14U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{15U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighWithNoLowSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834\u30A1")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{13U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighFollowedByUtf8Char) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834!")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighWithMissingLowSurrogate) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\uDD1E\u30A1")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16HighSurrogateFollowedByHighSurrogate) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD800\uD800")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{13U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, ControlCharacter) {
  auto p = make_parser(proxy_);
  input(p, u8"\"\t\""sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, ControlCharacterUTF16) {
  EXPECT_CALL(callbacks_, string_value(u8"\t"sv)).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"("\u0009")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero";
  EXPECT_EQ(p.pos(), (coord{column{8U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{9U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Utf16LowWithNoHighSurrogate) {
  // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
  auto p = make_parser(proxy_);
  input(p, u8R"("\uD834")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, SlashBadHexChar) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\u00xf")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_hex_char));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, PartialHexChar) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\u00)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_close_quote));
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Escape0Disabled) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\0")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, Escape0Enabled) {
  peejay::u8string const str{'\0'};
  EXPECT_CALL(callbacks_, string_value(peejay::u8string_view{str})).Times(1);

  auto p = make_parser(proxy_, extensions::string_escapes);
  input(p, u8R"("\0")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{4U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{5U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, EscapeVDisabled) {
  auto p = make_parser(proxy_);
  input(p, u8R"("\v")"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{3U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(String, EscapeVEnabled) {
  peejay::u8string const str{'\x0B'};
  EXPECT_CALL(callbacks_, string_value(peejay::u8string_view{str})).Times(1);

  auto p = make_parser(proxy_, extensions::string_escapes);
  input(p, u8R"("\v")"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{column{4U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{5U}, line{1U}}));
}

namespace {

class StringContinuation : public String, public testing::WithParamInterface<std::vector<char32_t>> {
protected:
  static constexpr auto prefix = u8R"("Lorem ipsum dolor sit amet, \)"sv;
  static constexpr auto suffix = u8R"(consectetur adipiscing elit.")"sv;

  static constexpr auto expected = u8"Lorem ipsum dolor sit amet, consectetur adipiscing elit."sv;

  static peejay::u8string utf8_sequence(std::vector<char32_t> const& in) {
    peejay::u8string out;
    peejay::icubaby::t32_8 utf_32_to_8;
#if __cpp_lib_ranges
    std::ranges::copy(in, peejay::icubaby::iterator{&utf_32_to_8, std::back_inserter(out)});
#else
    std::copy(std::begin(in), std::end(in), peejay::icubaby::iterator{&utf_32_to_8, std::back_inserter(out)});
#endif
    return out;
  }
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_P(StringContinuation, ExtensionDisabled) {
  auto p = make_parser(proxy_);
  input(p, peejay::u8string{prefix} + utf8_sequence(GetParam()) + peejay::u8string{suffix}).eof();
  EXPECT_TRUE(p.has_error()) << "Expected the parse to fail";
  EXPECT_EQ(p.last_error(), make_error_code(error::invalid_escape_char)) << "Got error: " << p.last_error().message();
};

// NOLINTNEXTLINE
TEST_P(StringContinuation, ExtensionEnabled) {
  EXPECT_CALL(callbacks_, string_value(expected)).Times(1);

  auto p = make_parser(proxy_, extensions::string_escapes);
  input(p, peejay::u8string{prefix} + utf8_sequence(GetParam()) + peejay::u8string{suffix}).eof();
  EXPECT_FALSE(p.has_error()) << "Expected the parse to succeed";
  EXPECT_FALSE(p.last_error()) << "Expected the parse error to be zero but was: " << p.last_error().message();
};

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(StringContinuation, StringContinuation,
                         testing::Values(std::vector<char32_t>{char_set::line_feed},
                                         std::vector<char32_t>{char_set::carriage_return},
                                         std::vector<char32_t>{char_set::carriage_return, char_set::line_feed},
                                         std::vector<char32_t>{char_set::line_separator},
                                         std::vector<char32_t>{char_set::paragraph_separator}));
