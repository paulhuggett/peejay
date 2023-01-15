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

using testing::StrictMock;

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
  p.input (u8"{\r\n}\n"sv);
  p.eof ();
  EXPECT_FALSE (p.has_error ());
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
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_object_member));
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
  EXPECT_FALSE (p.has_error ());
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
  EXPECT_FALSE (p.has_error ());
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
  EXPECT_FALSE (p.has_error ());
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
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Object, MisplacedCommaBeforeCloseBrace) {
  // An object with a trailing comma but with the extension disabled.
  parser p{null{}};
  p.input (u8R"({"a":1,})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, NoCommaBeforeProperty) {
  parser p{null{}};
  p.input (u8R"({"a":1 "b":1})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_object_member));
  EXPECT_EQ (p.pos (), (coord{column{8U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, TwoCommasBeforeProperty) {
  parser p{null{}};
  p.input (u8R"({"a":1,,"b":1})"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
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
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Object, KeyIsNotString) {
  parser p{null{}};
  p.input (u8"{{}:{}}"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_string));
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Object, BadNestedObject) {
  parser p{null{}};
  p.input (u8"{\"a\":nu}"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST_F (Object, TooDeeplyNested) {
  parser p{null{}};

  u8string input;
  for (auto ctr = 0U; ctr < 200U; ++ctr) {
    input += u8"{\"a\":";
  }
  p.input (input).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::nesting_too_deep));
}
