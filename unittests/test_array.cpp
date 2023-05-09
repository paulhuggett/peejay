//===- unittests/test_array.cpp -------------------------------------------===//
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
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

using namespace std::string_literals;
using namespace std::string_view_literals;

using peejay::column;
using peejay::coord;
using peejay::error;
using peejay::extensions;
using peejay::line;
using peejay::make_parser;
using peejay::parser;
using peejay::u8string;

using testing::DoubleEq;
using testing::StrictMock;

namespace {

class JsonArray : public testing::Test {
protected:
  using mocks = mock_json_callbacks<std::uint64_t>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (JsonArray, Empty) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[\n]\n"sv);
  p.eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse to succeed";
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{2U}}));
  EXPECT_EQ (p.input_pos (), (coord{column{1U}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, BeginArrayReturnsError) {
  std::error_code const error = make_error_code (std::errc::io_error);
  using testing::Return;
  EXPECT_CALL (callbacks_, begin_array ()).WillOnce (Return (error));

  auto p = make_parser (proxy_);
  p.input (u8"[\n]\n"sv);
  EXPECT_EQ (p.last_error (), error);
  EXPECT_EQ (p.pos (), (coord{column{1U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, ArrayNoCloseBracket) {
  auto p = make_parser (json_out_callbacks{});
  p.input (u8"["sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_array_member));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, SingleElement) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  auto const input = u8"[ 1 ]"sv;
  p.input (input).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.pos (), (coord{column{5}, line{1U}}));
  EXPECT_EQ (
      p.input_pos (),
      (coord{column{static_cast<unsigned> (input.length ()) + 1U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, SingleStringElement) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, string_value (u8"a"sv)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[\"a\"]"sv);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

// NOLINTNEXTLINE
TEST_F (JsonArray, ZeroExpPlus1) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.0))).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[0e+1]"sv);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

// NOLINTNEXTLINE
TEST_F (JsonArray, SimpleFloat) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, double_value (DoubleEq (1.234))).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[1.234]"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

// NOLINTNEXTLINE
TEST_F (JsonArray, MinusZero) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[-0]"sv);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

// NOLINTNEXTLINE
TEST_F (JsonArray, TwoElements) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, string_value (u8"hello"sv)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (u8"[ 1 ,\n \"hello\" ]"sv);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.input_pos (), (coord{column{11U}, line{2U}}));
  EXPECT_EQ (p.pos (), (coord{column{10U}, line{2U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, MisplacedComma1) {
  parser p{json_out_callbacks{}};
  p.input (u8"[,"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}
// NOLINTNEXTLINE
TEST_F (JsonArray, MisplacedComma2) {
  parser p{json_out_callbacks{}};
  p.input (u8"[,1"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}
// NOLINTNEXTLINE
TEST_F (JsonArray, MisplacedComma3) {
  parser p{json_out_callbacks{}};
  p.input (u8"[1,,2]"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}
// NOLINTNEXTLINE
TEST_F (JsonArray, MisplacedComma4) {
  parser p{json_out_callbacks{}};
  p.input (u8"[1 true]"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_array_member));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, TrailingCommaEnabled) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::array_trailing_comma);
  p.input (u8"[1 , ]"sv).eof ();
  EXPECT_FALSE (p.last_error ());
}

// NOLINTNEXTLINE
TEST_F (JsonArray, EmptyTrailingCommaEnabled) {
  // The contents of an array must not consist of a comma alone, even with the
  // trailing-comma extension enabled.
  auto p = make_parser (json_out_callbacks{}, extensions::array_trailing_comma);
  p.input (u8"[,]"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, TrailingCommaDisabled1) {
  parser p{json_out_callbacks{}};
  p.input (u8"[,]"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
  EXPECT_EQ (p.pos (), (coord{column{2U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, TrailingCommaDisabled2) {
  parser p{json_out_callbacks{}};
  p.input (u8"[1,]"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
  EXPECT_EQ (p.pos (), (coord{column{4U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, NestedError1) {
  parser p{json_out_callbacks{}};
  p.input (u8"[[no"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST_F (JsonArray, NestedError2) {
  auto p = make_parser (json_out_callbacks{});
  p.input (u8"[[null"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_array_member))
      << "Actual error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (JsonArray, Nested) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (2);
  EXPECT_CALL (callbacks_, null_value ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (2);

  auto p = make_parser (proxy_);
  p.input (u8"[[null]]"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (JsonArray, Nested2) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (2);
  EXPECT_CALL (callbacks_, null_value ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (2);

  auto p = make_parser (proxy_);
  p.input (u8"[[null], [1]]"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (JsonArray, TooDeeplyNested) {
  parser p{json_out_callbacks{}};
  p.input (u8string (std::string::size_type{200}, '[')).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::nesting_too_deep));
}
