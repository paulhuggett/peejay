//===- unit_tests/test_json.cpp -------------------------------------------===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
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
#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;
using testing::StrictMock;

using coord = peejay::coord<true>;
using peejay::null;
using peejay::parser;

namespace {

struct Json : testing::Test {
  std::u8string const cr = u8"\r"s;
  std::u8string const lf = u8"\n"s;
  std::u8string const crlf = cr + lf;
  std::u8string const keyword = u8"null"s;
  unsigned const xord = static_cast<unsigned>(keyword.length()) + 1U;
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(Json, Empty) {
  parser p{json_out_callbacks{}};
  input(p, u8""sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(peejay::error::expected_token))
      << "Real error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 1U}));
}

// NOLINTNEXTLINE
TEST_F(Json, StringInput) {
  parser p{json_out_callbacks{}};
  std::u8string const res = input(p, keyword).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
  EXPECT_EQ(p.pos(), (coord{.line = 1U, .column = 5U}));
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U, .column = 5U}));
}

// NOLINTNEXTLINE
TEST_F(Json, LeadingWhitespace) {
  parser p{json_out_callbacks{}};
  std::u8string const res = input(p, u8"   \t    null"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, u8"null");
}

// NOLINTNEXTLINE
TEST_F(Json, POSIXLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, lf + lf + keyword);
  std::u8string const res = p.eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
}

// NOLINTNEXTLINE
TEST_F(Json, ClassicMacLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, cr + cr + keyword);  // MacOS Classic line endings
  std::u8string const res = p.eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
}

// NOLINTNEXTLINE
TEST_F(Json, CrLfLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  input(p, crlf + crlf + keyword);  // Windows-style CRLF
  std::u8string const res = p.eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
}

// NOLINTNEXTLINE
TEST_F(Json, BadLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // Nobody's line-endings. Each counts as a new line. Note that the middle
  // cr+lf pair will match a single Windows crlf.
  std::u8string const res = input(p, lf + cr + lf + cr + keyword).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
}

// NOLINTNEXTLINE
TEST_F(Json, MixedLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // A groovy mixture of line-ending characters.
  input(p, lf + lf + crlf + cr + keyword);
  std::u8string const res = p.eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(res, keyword);
}

// NOLINTNEXTLINE
TEST_F(Json, Null) {
  StrictMock<mock_json_callbacks<std::int64_t, double, char8_t>> callbacks;
  callbacks_proxy proxy{callbacks};
  EXPECT_CALL(callbacks, null_value()).Times(1);

  parser p{proxy};
  input(p, u8" null "sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Json, Move) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = parser<null<peejay::default_policies>>{};
  auto p2 = std::move(p1);
  input(p2, u8"null"sv).eof();
  EXPECT_FALSE(p2.has_error()) << "Real error was: " << p2.last_error().message();
  EXPECT_EQ(p2.pos(), (coord{.line = 1U, .column = 5U}));
  EXPECT_EQ(p2.input_pos(), (coord{.line = 1U, .column = 5U}));
}

// NOLINTNEXTLINE
TEST_F(Json, Move2) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = std::make_unique<parser<null<peejay::default_policies>>>();
  input(*p1, u8"[[1"sv);
  auto p2 = std::move(*p1);
  p1.reset();
  input(p2, u8"]]"sv).eof();
  EXPECT_FALSE(p2.has_error()) << "Real error was: " << p2.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Json, MoveAssign) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = parser<null<peejay::default_policies>>{};
  parser<null<peejay::default_policies>> p2;
  p2 = std::move(p1);
  input(p2, u8"null"sv).eof();
  EXPECT_FALSE(p2.has_error()) << "Real error was: " << p2.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Json, MoveAssign2) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = std::make_unique<parser<null<peejay::default_policies>>>();
  input(*p1, u8"[[1"sv);
  parser<null<peejay::default_policies>> p2;
  p2 = std::move(*p1);
  p1.reset();
  input(p2, u8"]]"sv).eof();
  EXPECT_FALSE(p2.has_error()) << "Real error was: " << p2.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Json, TwoKeywords) {
  parser p{json_out_callbacks{}};
  input(p, u8" true false "sv);
  EXPECT_EQ(p.last_error(), make_error_code(peejay::error::unexpected_extra_input))
      << "Real error was: " << p.last_error().message();
}

class BadKeyword : public testing::TestWithParam<std::tuple<std::u8string_view, peejay::error>> {};

TEST_P(BadKeyword, Fails) {
  auto const& [src, err] = GetParam();
  ASSERT_NE(err, peejay::error::none);
  parser p{json_out_callbacks{}};
  std::u8string const res = p.input(src).eof();
  EXPECT_EQ(p.last_error(), err) << "Real error was: " << p.last_error().message();
}

INSTANTIATE_TEST_SUITE_P(BadKeyword, BadKeyword,
                         testing::Values(std::make_tuple(u8"nu"sv, peejay::error::unrecognized_token),
                                         std::make_tuple(u8"bad"sv, peejay::error::expected_token),
                                         std::make_tuple(u8"fal"sv, peejay::error::unrecognized_token),
                                         std::make_tuple(u8"falsehood"sv, peejay::error::unexpected_extra_input)));
