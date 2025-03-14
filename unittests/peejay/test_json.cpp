//===- unittests/peejay/test_json.cpp -------------------------------------===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "peejay/json/json.hpp"
#include "peejay/json/stack.hpp"
// standard library
#include <stack>

#include "callbacks.hpp"
#include "peejay/json/null.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;
using testing::StrictMock;

using column = peejay::coord::column;
using line = peejay::coord::line;
using peejay::coord;
using peejay::error;
using peejay::null;
using peejay::parser;
using peejay::u8string;
using peejay::u8string_view;

namespace {

struct Json : testing::Test {
  static void check_error(u8string_view const& src, error err) {
    ASSERT_NE(err, error::none);
    parser p{json_out_callbacks{}};
    u8string const res = input(p, src).eof();
    EXPECT_EQ(res, u8"");
    EXPECT_NE(p.last_error(), make_error_code(error::none));
  }

  u8string const cr = u8"\r"s;
  u8string const lf = u8"\n"s;
  u8string const crlf = cr + lf;
  u8string const keyword = u8"null"s;
  unsigned const xord = static_cast<unsigned>(keyword.length()) + 1U;
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(Json, Empty) {
  parser p{json_out_callbacks{}};
  input(p, u8""sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
  EXPECT_EQ(p.pos(), (coord{line{1U}, column{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, StringInput) {
  parser p{json_out_callbacks{}};
  u8string const res = input(p, keyword).eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{line{1U}, column{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{line{1U}, column{5U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, LeadingWhitespace) {
  parser p{json_out_callbacks{}};
  u8string const res = input(p, u8"   \t    null"sv).eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, u8"null");
  EXPECT_EQ(p.pos(), (coord{line{1U}, column{9U}}));
  EXPECT_EQ(p.input_pos(), (coord{line{1U}, column{13U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, POSIXLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, lf + lf + keyword);
  u8string const res = p.eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{column{1}, line{3U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{xord}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, ClassicMacLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, cr + cr + keyword);  // MacOS Classic line endings
  u8string const res = p.eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{column{1}, line{3U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{xord}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, CrLfLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, crlf + crlf + keyword);  // Windows-style CRLF
  u8string const res = p.eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{column{1}, line{3U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{xord}, line{3U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, BadLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // Nobody's line-endings. Each counts as a new line. Note that the middle
  // cr+lf pair will match a single Windows crlf.
  u8string const res = input(p, lf + cr + lf + cr + keyword).eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{column{1}, line{4U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{xord}, line{4U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, MixedLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // A groovy mixture of line-ending characters.
  input(p, lf + lf + crlf + cr + keyword);
  u8string const res = p.eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{column{1}, line{5U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{xord}, line{5U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, Null) {
  StrictMock<mock_json_callbacks<std::int64_t>> callbacks;
  callbacks_proxy proxy{callbacks};
  EXPECT_CALL(callbacks, null_value()).Times(1);

  parser p{proxy};
  input(p, u8" null "sv).eof();
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(p.pos(), (coord{column{6U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, Move) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = parser<null>{};
  auto p2 = std::move(p1);
  input(p2, u8"null"sv).eof();
  EXPECT_FALSE(p2.has_error());
  EXPECT_EQ(p2.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p2.input_pos(), (coord{column{5U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, Move2) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = std::make_unique<parser<null>>();
  input(*p1, u8"[[1"sv);
  auto p2 = std::move(*p1);
  p1.reset();
  input(p2, u8"]]"sv).eof();
  EXPECT_FALSE(p2.has_error());
  EXPECT_EQ(p2.pos(), (coord{column{5U}, line{1U}}));
  EXPECT_EQ(p2.input_pos(), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, MoveAssign) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = parser<null>{};
  parser<null> p2;
  p2 = std::move(p1);
  input(p2, u8"null"sv).eof();
  EXPECT_FALSE(p2.has_error());
  EXPECT_EQ(p2.pos(), (coord{column{1U}, line{1U}}));
  EXPECT_EQ(p2.input_pos(), (coord{column{5U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, MoveAssign2) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = std::make_unique<parser<null>>();
  input(*p1, u8"[[1"sv);
  parser<null> p2;
  p2 = std::move(*p1);
  p1.reset();
  input(p2, u8"]]"sv).eof();
  EXPECT_FALSE(p2.has_error());
  EXPECT_EQ(p2.pos(), (coord{column{5U}, line{1U}}));
  EXPECT_EQ(p2.input_pos(), (coord{column{6U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, TwoKeywords) {
  parser p{json_out_callbacks{}};
  input(p, u8" true false "sv);
  EXPECT_EQ(p.last_error(), make_error_code(error::unexpected_extra_input));
  EXPECT_EQ(p.pos(), (coord{column{7U}, line{1U}}));
  EXPECT_EQ(p.input_pos(), (coord{column{7U}, line{1U}}));
}

// NOLINTNEXTLINE
TEST_F(Json, BadKeyword) {
  check_error(u8"nu"sv, error::expected_token);
  check_error(u8"bad"sv, error::expected_token);
  check_error(u8"fal"sv, error::expected_token);
  check_error(u8"falsehood"sv, error::unexpected_extra_input);
}
