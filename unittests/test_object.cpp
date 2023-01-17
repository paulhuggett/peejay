//===- unittests/test_object.cpp ------------------------------------------===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <cerrno>

#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

using testing::StrictMock;

using peejay::char8;
using peejay::column;
using peejay::coord;
using peejay::error;
using peejay::extensions;
using peejay::line;
using peejay::make_parser;
using peejay::null;
using peejay::parser;
using peejay::u8string;

namespace {

class Object : public testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (Object, Empty) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"{\r\n}\n"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{2U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{1U}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, OpeningBraceOnly) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"{"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_object_member))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, SingleKvp) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"({ "a":1 })"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{line{1U}, column{9U}}));
  EXPECT_EQ (p.input_pos (), (coord{line{1U}, column{10U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, BadBeginObject) {
  std::error_code const error{EDOM, std::generic_category ()};

  using testing::_;
  using testing::Return;
  EXPECT_CALL (callbacks_, begin_object ()).WillOnce (Return (error));

  auto p = make_parser (proxy_);
  p.input (u8R"({ "a":1 })"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), error)
      << "Expected the error to be propagated from the begin_object() callback";
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, SingleKvpBadEndObject) {
  std::error_code const end_object_error{EDOM, std::generic_category ()};

  using testing::_;
  using testing::Return;
  EXPECT_CALL (callbacks_, begin_object ());
  EXPECT_CALL (callbacks_, key (_));
  EXPECT_CALL (callbacks_, uint64_value (_));
  EXPECT_CALL (callbacks_, end_object ()).WillOnce (Return (end_object_error));

  auto p = make_parser (proxy_);
  p.input (u8"{\n\"a\" : 1\n}"sv);
  p.eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), end_object_error)
      << "Expected the error to be propagated from the end_object() callback";
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, TwoKvps) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, key (u8"b"sv)).Times (1);
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"({"a":1, "b" : true })"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, DuplicateKeys) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8R"({"a":1, "a":true})"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, ArrayValue) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (2)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"{\"a\": [1,2]}"sv);
  p.eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, MisplacedCommaBeforeCloseBrace) {
  // An object with a trailing comma but with the extension disabled.
  parser p{null{}};
  p.input (u8R"({"a":1,})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_string))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, NoCommaBeforeProperty) {
  parser p{null{}};
  p.input (u8R"({"a":1 "b":1})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_object_member))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, TwoCommasBeforeProperty) {
  parser p{null{}};
  p.input (u8R"({"a":1,,"b":1})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_string))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, TrailingCommaExtensionEnabled) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (16)).Times (1);
  EXPECT_CALL (callbacks_, key (u8"b"sv)).Times (1);
  EXPECT_CALL (callbacks_, string_value (u8"c"sv)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  // An object with a trailing comma but with the extension _enabled_. Note that
  // there is deliberate whitespace around the final comma.
  auto p = make_parser (proxy_, extensions::object_trailing_comma);
  p.input (u8R"({ "a":16, "b":"c" , })"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, BadNestedObject) {
  parser p{null{}};
  p.input (u8"{\"a\":nu}"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token))
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, TooDeeplyNested) {
  parser p{null{}};

  u8string input;
  for (auto ctr = 0U; ctr < 200U; ++ctr) {
    input += u8"{\"a\":";
  }
  p.input (input).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::nesting_too_deep))
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Object, KeyIsNotString) {
  parser p{null{}};
  p.input (u8"{{}:{}}"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_string))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, KeyIsIdentifierWithoutExtensionEnabled) {
  parser p{null{}};
  p.input (u8"{foo:1}"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_string))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, IdentifierKey) {
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"key"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8"{key:1}"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Object, IdentifierKeyWhitespaceSurrounding) {
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (u8"$key"sv)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8"{ $key : 1 }"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Object, IdentifierKeyEmpty) {
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8"{ : 1 }"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_identifier))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{3U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{3U}, line{1U}}));
}

TEST_F (Object, IdentifierKeyExtendedChars) {
  u8string const mathematical_bold_capital_a{
      static_cast<char8> (0xF0), static_cast<char8> (0x9D),
      static_cast<char8> (0x90),
      static_cast<char8> (0x80)};  // U+1d400 MATHEMATICAL BOLD CAPITAL A
  u8string const zero_width_non_joiner{
      static_cast<char8> (0xE2), static_cast<char8> (0x80),
      static_cast<char8> (0x8C)};  // U+200C ZERO WIDTH NON-JOINER
  u8string const key = mathematical_bold_capital_a + zero_width_non_joiner;

  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (peejay::u8string_view (key))).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8"{ " + key + u8":1}").eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Object, IdentifierKeyHexEscape) {
  u8string const greek_capital_letter_sigma{static_cast<char8> (0xCE),
                                            static_cast<char8> (0xA3)};
  u8string const key = u8"sig"s + greek_capital_letter_sigma + u8"ma"s;

  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (peejay::u8string_view (key))).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8R"({ sig\u03A3ma: 1 })"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Object, IdentifierKeyHexEscapeHighLowSurrogatePair) {
  // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as UTF-8.
  u8string const gclef8{static_cast<char8> (0xF0), static_cast<char8> (0x9D),
                        static_cast<char8> (0x84), static_cast<char8> (0x9E)};
  // The same U+1D11E as a UTF-16 surrogate pair.
  u8string const gclef16 = u8R"(\uD834\uDD1E)";

  auto const prefix = u8"key"s;
  auto const suffix = u8"G"s;
  u8string const expected_key = prefix + gclef8 + suffix;

  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (peejay::u8string_view (expected_key)))
      .Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8"{ "s + prefix + gclef16 + suffix + u8" : 1 }"s).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Object, IdentifierKeyHexEscapeHighSurrogateMissingLow) {
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8R"({ key\uD834g: 1 })"s).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{3U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{12U}, line{1U}}));
}

TEST_F (Object, IdentifierKeyHexEscapeLowSurrogateOnly) {
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);

  auto p = make_parser (proxy_, extensions::identifier_object_key);
  p.input (u8R"({ key\uDD1E: 1 })"s).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::bad_unicode_code_point))
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{column{3U}, line{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{11U}, line{1U}}));
}
