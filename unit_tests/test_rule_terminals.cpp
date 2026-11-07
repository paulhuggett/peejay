//===- unit_tests/test_rule_terminals.cpp ---------------------------------===//
//*             _        _                      _             _      *
//*  _ __ _   _| | ___  | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | '__| | | | |/ _ \ | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | |  | |_| | |  __/ | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//* |_|   \__,_|_|\___|  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*                                                                  *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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

// DUT
#include "peejay/json.hpp"
#include "peejay/rules/terminals.hpp"

// Standard Library
#include <limits>

// 3rd party includes
#include <gmock/gmock.h>

using namespace std::string_view_literals;

namespace {

template <typename Rule> struct TerminalTest {
  using backend = peejay::rules::backend<typename Rule::policies>;
  using context_type = peejay::rules::context<typename Rule::policies, Rule>;

  Rule::value_type value{};
  context_type context{&value};
  peejay::parser<backend> parser = context.make_parser();
};

class ImportBool : public testing::Test, public TerminalTest<peejay::rules::bool_rule<peejay::default_policies>> {};
// NOLINTNEXTLINE
TEST_F(ImportBool, True) {
  parser.input(u8"true"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, true);
}
// NOLINTNEXTLINE
TEST_F(ImportBool, False) {
  parser.input(u8"false"sv);
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, false);
}

class ImportInt : public testing::TestWithParam<std::pair<int, std::u8string_view>>,
                  public TerminalTest<peejay::rules::integer_rule<peejay::default_policies>> {};
// NOLINTNEXTLINE
TEST_P(ImportInt, Check) {
  value = std::numeric_limits<int>::max();
  auto const& [expected, input] = GetParam();
  parser.input(input).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, expected);
}
// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(ImportInt, ImportInt,
                         testing::Values(std::make_pair(0, u8"0"sv), std::make_pair(1, u8"1"sv),
                                         std::make_pair(1234, u8"1234"sv), std::make_pair(-1, u8"-1"sv)));

class ImportUInt : public testing::Test, public TerminalTest<peejay::rules::uinteger_rule<peejay::default_policies>> {};
// NOLINTNEXTLINE
TEST_F(ImportUInt, Positive) {
  parser.input(u8"1234"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 1234);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt, Negative) {
  parser.input(u8"-1234"sv).eof();
  EXPECT_TRUE(parser.has_error());
  EXPECT_EQ(parser.last_error().message(), "number out of range")
      << "JSON error was: " << parser.last_error().message();
}

class ImportString : public testing::Test, public TerminalTest<peejay::rules::string_rule<peejay::default_policies>> {};
// NOLINTNEXTLINE
TEST_F(ImportString, EmptyString) {
  parser.input(u8R"("")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, u8"");
}
TEST_F(ImportString, String) {
  parser.input(u8R"("string")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, u8"string");
}

}  // end anonymous namespace
