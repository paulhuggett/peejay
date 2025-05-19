//===- unit_tests/test_object.cpp -----------------------------------------===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
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
#include <cerrno>

#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

using testing::InSequence;
using testing::StrictMock;

using coord = peejay::coord<true>;
using peejay::error;
using peejay::make_parser;
using peejay::null;
using peejay::parser;

namespace {

class Object : public testing::Test {
protected:
  StrictMock<mock_json_callbacks<std::uint64_t, double, char8_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t, double, char8_t>> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(Object, Empty) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8"{\r\n}\n"sv).eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, EmptyNoWhitespace) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"{}"sv).eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, OpeningBraceOnly) {
  auto p = make_parser(proxy_);
  p.input(u8"{"sv).eof();
  EXPECT_TRUE(p.has_error());
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_object_member))
      << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, SingleKvp) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8R"({ "a":1 })"sv).eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, BadBeginObject) {
  auto const error = make_error_code(std::errc::argument_out_of_domain);

  using testing::Return;
  EXPECT_CALL(callbacks_, begin_object()).WillOnce(Return(error));

  auto p = make_parser(proxy_);
  p.input(u8R"({ "a":1 })"sv).eof();
  EXPECT_TRUE(p.has_error());
  EXPECT_EQ(p.last_error(), error) << "Expected the error to be propagated from the begin_object() callback";
}

// NOLINTNEXTLINE
TEST_F(Object, KeyReturnsError) {
  auto const error = make_error_code(std::errc::argument_out_of_domain);

  using testing::Return;
  EXPECT_CALL(callbacks_, begin_object());
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1).WillOnce(Return(error));

  auto p = make_parser(proxy_);
  p.input(u8R"({ "a":1 })"sv).eof();
  EXPECT_EQ(p.last_error(), error) << "Error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, SingleKvpBadEndObject) {
  std::error_code const end_object_error{EDOM, std::generic_category()};

  using testing::_;
  using testing::Return;
  EXPECT_CALL(callbacks_, begin_object());
  EXPECT_CALL(callbacks_, key(_));
  EXPECT_CALL(callbacks_, integer_value(_));
  EXPECT_CALL(callbacks_, end_object()).WillOnce(Return(end_object_error));

  auto p = make_parser(proxy_);
  input(p, u8"{\n\"a\" : 1\n}"sv);
  p.eof();
  EXPECT_TRUE(p.has_error());
  EXPECT_EQ(p.last_error(), end_object_error) << "Expected the error to be propagated from the end_object() callback";
  EXPECT_EQ(p.pos(), (coord{.line = 3U, .column = 1U}));
}

// NOLINTNEXTLINE
TEST_F(Object, TwoKvps) {
  testing::InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, key(u8"b"sv)).Times(1);
  EXPECT_CALL(callbacks_, boolean_value(true)).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"({"a":1, "b" : true })"sv).eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, DuplicateKeys) {
  testing::InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1);
  EXPECT_CALL(callbacks_, boolean_value(true)).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8R"({"a":1, "a":true})"sv).eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, ArrayValue) {
  testing::InSequence const _;
  EXPECT_CALL(callbacks_, begin_object()).Times(1);
  EXPECT_CALL(callbacks_, key(u8"a"sv)).Times(1);
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, integer_value(2)).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);
  EXPECT_CALL(callbacks_, end_object()).Times(1);

  auto p = make_parser(proxy_);
  input(p, u8"{\"a\": [1,2]}"sv);
  p.eof();
  EXPECT_FALSE(p.has_error()) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, MisplacedCommaBeforeCloseBrace) {
  // An object with a trailing comma but with the extension disabled.
  parser p{null{}};
  input(p, u8R"({"a":1,})"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_object_key))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 8U}));
}

// NOLINTNEXTLINE
TEST_F(Object, NoCommaBeforeProperty) {
  parser p{null{}};
  input(p, u8R"({"a":1 "b":1})"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_object_member))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 8U}));
}

// NOLINTNEXTLINE
TEST_F(Object, TwoCommasBeforeProperty) {
  parser p{null{}};
  input(p, u8R"({"a":1,,"b":1})"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_object_key))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 8U}));
}

// NOLINTNEXTLINE
TEST_F(Object, MissingColon) {
  parser p{null{}};
  p.input(u8R"({"a" 1)"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_colon)) << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 6U}));
}

// NOLINTNEXTLINE
TEST_F(Object, BadNestedObject) {
  parser p{null{}};
  input(p, u8"{\"a\":nu}"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token))
      << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, TooDeeplyNested) {
  parser p{null{}};

  std::u8string src;
  for (auto ctr = 0U; ctr < 200U; ++ctr) {
    src += u8"{\"a\":";
  }
  input(p, src).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::nesting_too_deep)) << "JSON error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Object, KeyIsNotString) {
  parser p{null{}};
  input(p, u8"{{}:{}}"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_object_key))
      << "JSON error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 2U}));
}

#if 0
struct depth_two_policies : public peejay::default_policies {
  static constexpr std::size_t max_stack_depth = 2;
};
// NOLINTNEXTLINE
TEST(ObjectWithLimitedStack, Two) {
  auto parser = make_parser<depth_two_policies>(null{});

  input(parser, u8"123"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::expected_object_key))
  << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(parser.pos(), (coord{.line = 1U, .column = 2U}));

  input(parser, u8R"({"key":"value"})"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::expected_object_key))
  << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(parser.pos(), (coord{.line = 1U, .column = 2U}));
}
#endif
