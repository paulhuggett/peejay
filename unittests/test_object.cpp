//===- unittests/test_object.cpp ------------------------------------------===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "callbacks.hpp"
#include "json/dom_types.hpp"
#include "json/json.hpp"

using namespace std::string_literals;
using testing::DoubleEq;
using testing::StrictMock;

using namespace peejay;

namespace {

class JsonObject : public testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

TEST_F (JsonObject, Empty) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input ("{\r\n}\n"s);
  p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (p.coordinate (), (coord{column{1U}, row{3U}}));
}

TEST_F (JsonObject, SingleKvp) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"{\n\"a\" : 1\n}"});
  p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (p.coordinate (), (coord{column{2U}, row{3U}}));
}

TEST_F (JsonObject, SingleKvpBadEndObject) {
  std::error_code const end_object_error =
      make_error_code (std::errc::io_error);

  using testing::_;
  using testing::Return;
  EXPECT_CALL (callbacks_, begin_object ());
  EXPECT_CALL (callbacks_, key (_));
  EXPECT_CALL (callbacks_, uint64_value (_));
  EXPECT_CALL (callbacks_, end_object ()).WillOnce (Return (end_object_error));

  auto p = make_parser (proxy_);
  p.input (std::string{"{\n\"a\" : 1\n}"});
  p.eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), end_object_error)
      << "Expected the error to be propagated from the end_object() callback";
  EXPECT_EQ (p.coordinate (), (coord{column{1U}, row{3U}}));
}

TEST_F (JsonObject, TwoKvps) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"b"})).Times (1);
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"{\"a\":1, \"b\" : true }"});
  p.eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, DuplicateKeys) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"{\"a\":1, \"a\" : true }"});
  p.eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, ArrayValue) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (2)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  auto p = make_parser (proxy_);
  p.input (std::string{"{\"a\": [1,2]}"});
  p.eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, MisplacedCommaBeforeCloseBrace) {
  // An object with a trailing comma but with the extension disabled.
  parser<null_output> p;
  p.input (R"({"a":1,})"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonObject, NoCommaBeforeProperty) {
  parser<null_output> p;
  p.input (R"({"a":1 "b":1})"s).eof ();
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::expected_object_member));
  EXPECT_EQ (p.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonObject, TwoCommasBeforeProperty) {
  parser<null_output> p3;
  p3.input (R"({"a":1,,"b":1})"s).eof ();
  EXPECT_EQ (p3.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p3.coordinate (), (coord{column{8U}, row{1U}}));
}

TEST_F (JsonObject, TrailingCommaExtensionEnabled) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_object ()).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"a"})).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (16)).Times (1);
  EXPECT_CALL (callbacks_, key (std::string_view{"b"})).Times (1);
  EXPECT_CALL (callbacks_, string_value (std::string_view{"c"})).Times (1);
  EXPECT_CALL (callbacks_, end_object ()).Times (1);

  // An object with a trailing comma but with the extension _enabled_. Note that
  // there is deliberate whitespace around the final comma.
  auto p = make_parser (proxy_, extensions::object_trailing_comma);
  p.input (R"({ "a":16, "b":"c" , })"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, KeyIsNotString) {
  parser<null_output> p;
  p.input ("{{}:{}}"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_string));
  EXPECT_EQ (p.coordinate (), (coord{column{2U}, row{1U}}));
}

TEST_F (JsonObject, BadNestedObject) {
  parser<null_output> p;
  p.input (std::string{"{\"a\":nu}"}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::unrecognized_token));
}

TEST_F (JsonObject, TooDeeplyNested) {
  parser<null_output> p;

  std::string input;
  for (auto ctr = 0U; ctr < 200U; ++ctr) {
    input += "{\"a\":";
  }
  p.input (input).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::nesting_too_deep));
}
