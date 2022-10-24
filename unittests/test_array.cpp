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
#include "json/json.hpp"

using namespace std::string_literals;
using testing::DoubleEq;
using testing::StrictMock;
using namespace peejay;

namespace {

class JsonArray : public testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

TEST_F (JsonArray, Empty) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input ("[\n]\n"s);
  p.eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse to succeed";
  EXPECT_EQ (p.coordinate (), (coord{column{1U}, row{3U}}));
}

TEST_F (JsonArray, BeginArrayReturnsError) {
  std::error_code const error = make_error_code (std::errc::io_error);
  using testing::Return;
  EXPECT_CALL (callbacks_, begin_array ()).WillOnce (Return (error));

  auto p = make_parser (proxy_);
  p.input ("[\n]\n"s);
  EXPECT_EQ (p.last_error (), error);
  EXPECT_EQ (p.coordinate (), (coord{column{1U}, row{1U}}));
}

TEST_F (JsonArray, ArrayNoCloseBracket) {
  auto p = make_parser (json_out_callbacks{});
  p.input ("["s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_array_member));
}

TEST_F (JsonArray, SingleElement) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  std::string const input = "[ 1 ]";
  p.input (input).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (
      p.coordinate (),
      (coord{column{static_cast<unsigned> (input.length ()) + 1U}, row{1U}}));
}

TEST_F (JsonArray, SingleStringElement) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, string_value (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"[\"a\"]"});
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, ZeroExpPlus1) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.0))).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input ("[0e+1]"s);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, SimpleFloat) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, double_value (DoubleEq (1.234))).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input ("[1.234]"s).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, MinusZero) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input ("[-0]"s);
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, TwoElements) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, string_value (std::string_view{"hello"})).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"[ 1 ,\n \"hello\" ]"});
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
  EXPECT_EQ (p.coordinate (), (coord{column{11U}, row{2U}}));
}

TEST_F (JsonArray, MisplacedComma1) {
  parser p{json_out_callbacks{}};
  p.input ("[,"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
}
TEST_F (JsonArray, MisplacedComma2) {
  parser p{json_out_callbacks{}};
  p.input ("[,1"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
}
TEST_F (JsonArray, MisplacedComma3) {
  parser p{json_out_callbacks{}};
  p.input ("[1,,2]"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
}
TEST_F (JsonArray, MisplacedComma4) {
  parser p{json_out_callbacks{}};
  p.input ("[1 true]"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_array_member));
}

TEST_F (JsonArray, TrailingCommaEnabled) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::array_trailing_comma);
  p.input ("[1 , ]"s).eof ();
  EXPECT_FALSE (p.last_error ());
}

TEST_F (JsonArray, EmptyTrailingCommaEnabled) {
  // The contents of an array must not consist of a comma alone, even with the
  // trailing-comma extension enabled.
  auto p = make_parser (json_out_callbacks{}, extensions::array_trailing_comma);
  p.input ("[,]"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p.coordinate (), (coord{column{2U}, row{1U}}));
}

TEST_F (JsonArray, TrailingCommaDisabled1) {
  parser p{json_out_callbacks{}};
  p.input ("[,]"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p.coordinate (), (coord{column{2U}, row{1U}}));
}

TEST_F (JsonArray, TrailingCommaDisabled2) {
  parser p{json_out_callbacks{}};
  p.input ("[1,]"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p.coordinate (), (coord{column{4U}, row{1U}}));
}

TEST_F (JsonArray, NestedError1) {
  parser p{json_out_callbacks{}};
  p.input ("[[no"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::unrecognized_token));
}

TEST_F (JsonArray, NestedError2) {
  auto p = make_parser (json_out_callbacks{});
  p.input ("[[null"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_array_member));
}

TEST_F (JsonArray, Nested) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (2);
  EXPECT_CALL (callbacks_, null_value ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (2);

  auto p = make_parser (proxy_);
  p.input ("[[null]]"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonArray, Nested2) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (2);
  EXPECT_CALL (callbacks_, null_value ()).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (2);

  auto p = make_parser (proxy_);
  p.input ("[[null], [1]]"s);
  p.eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonArray, TooDeeplyNested) {
  parser p{json_out_callbacks{}};
  p.input (std::string (std::string::size_type{200}, '[')).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::nesting_too_deep));
}
