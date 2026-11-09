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

class ImportStdString : public testing::Test,
                        public TerminalTest<peejay::rules::string_rule<peejay::default_policies, std::string>> {};
// NOLINTNEXTLINE
TEST_F(ImportStdString, EmptyString) {
  value = "x";
  parser.input(u8R"("")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, "");
}
// NOLINTNEXTLINE
TEST_F(ImportStdString, MultiByte) {
  parser.input(u8"\"caf\u00E9\""sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, "caf\xC3\xA9") << "the UTF-8 code units are stored unchanged";
}

static_assert(std::is_same_v<peejay::rules::type_to_terminal<std::string, peejay::default_policies>::type,
                             peejay::rules::string_rule<peejay::default_policies, std::string>>);
static_assert(peejay::rules::string_target<std::u8string>);
static_assert(peejay::rules::string_target<peejay::arrayvec<char8_t, 4>>);
static_assert(peejay::rules::string_target<std::string>);
static_assert(!peejay::rules::string_target<std::wstring>);
static_assert(!peejay::rules::string_target<peejay::arrayvec<char, 4>>);
static_assert(!peejay::rules::string_target<peejay::arrayvec<char8_t, 4> const>);
static_assert(std::is_same_v<peejay::rules::string_rule<peejay::default_policies>::value_type, std::u8string>);
static_assert(
    std::is_same_v<peejay::rules::type_to_terminal<peejay::arrayvec<char8_t, 4>, peejay::default_policies>::type,
                   peejay::rules::string_rule<peejay::default_policies, peejay::arrayvec<char8_t, 4>>>);

/// \returns A copy of the characters held by \p av.
template <std::size_t Size> std::u8string to_u8string(peejay::arrayvec<char8_t, Size> const& av) {
  return {av.data(), av.size()};
}

// A string with room for four UTF-8 code units.
class ImportArrayvecString
    : public testing::Test,
      public TerminalTest<peejay::rules::string_rule<peejay::default_policies, peejay::arrayvec<char8_t, 4>>> {};
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, EmptyString) {
  parser.input(u8R"("")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(to_u8string(value), u8"");
}
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, Full) {
  parser.input(u8R"("abcd")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(to_u8string(value), u8"abcd");
}
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, ReplacesContents) {
  value.assign({u8'x', u8'y', u8'z'});
  parser.input(u8R"("ab")"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(to_u8string(value), u8"ab");
}
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, MultiByteFull) {
  // Each U+00E9 is two UTF-8 code units.
  parser.input(u8"\"\u00E9\u00E9\""sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(to_u8string(value), u8"\u00E9\u00E9");
}
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, TooLong) {
  value.assign({u8'x'});
  parser.input(u8R"("abcde")"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(peejay::error::string_too_long));
  EXPECT_EQ(to_u8string(value), u8"x");
}
// NOLINTNEXTLINE
TEST_F(ImportArrayvecString, MultiByteTooLong) {
  parser.input(u8"\"ab\u00E9c\""sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(peejay::error::string_too_long));
  EXPECT_EQ(to_u8string(value), u8"");
}

//-MARK: float

class ImportFloat : public testing::Test, public TerminalTest<peejay::rules::float_rule<peejay::default_policies>> {};
// NOLINTNEXTLINE
TEST_F(ImportFloat, Fraction) {
  parser.input(u8"1.5"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 1.5);
}
// NOLINTNEXTLINE
TEST_F(ImportFloat, Integer) {
  parser.input(u8"2"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 2.0);
}
// NOLINTNEXTLINE
TEST_F(ImportFloat, NegativeInteger) {
  parser.input(u8"-3"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, -3.0);
}

// A policy which disables floating-point support.
struct no_float_policies : peejay::default_policies {
  using float_type = peejay::no_float_type;
};

class ImportNoFloat : public testing::Test, public TerminalTest<peejay::rules::float_rule<no_float_policies>> {};
// NOLINTNEXTLINE
TEST_F(ImportNoFloat, IntegerRejected) {
  parser.input(u8"2"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(peejay::rules::error::unexpected_number));
}

//-MARK: integer types

static_assert(peejay::rules::integer_target<signed char>);
static_assert(peejay::rules::integer_target<unsigned long long>);
static_assert(!peejay::rules::integer_target<bool>);
static_assert(!peejay::rules::integer_target<char>);
static_assert(!peejay::rules::integer_target<char8_t>);
static_assert(!peejay::rules::integer_target<int const>);

using default_policies = peejay::default_policies;
static_assert(std::is_same_v<peejay::rules::uinteger_rule<default_policies>,
                             peejay::rules::integer_rule<default_policies, unsigned>>);
static_assert(std::is_same_v<peejay::rules::type_to_terminal<short, default_policies>::type,
                             peejay::rules::integer_rule<default_policies, short>>);
static_assert(std::is_same_v<peejay::rules::type_to_terminal<unsigned char, default_policies>::type,
                             peejay::rules::integer_rule<default_policies, unsigned char>>);
static_assert(std::is_same_v<peejay::rules::type_to_terminal<bool, default_policies>::type,
                             peejay::rules::bool_rule<default_policies>>);
// The policy's integer_type (int) is a standard integer type so it is not repeated.
static_assert(peejay::type_list::index_of_v<peejay::rules::builtin_integers<default_policies>, unsigned long long> ==
              9);

template <typename T> using int_rule = peejay::rules::integer_rule<default_policies, T>;
auto const out_of_range = make_error_code(peejay::error::number_out_of_range);

class ImportInt8 : public testing::Test, public TerminalTest<int_rule<signed char>> {};
// NOLINTNEXTLINE
TEST_F(ImportInt8, Min) {
  parser.input(u8"-128"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, -128);
}
// NOLINTNEXTLINE
TEST_F(ImportInt8, Max) {
  parser.input(u8"127"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 127);
}
// NOLINTNEXTLINE
TEST_F(ImportInt8, BelowMin) {
  parser.input(u8"-129"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}
// NOLINTNEXTLINE
TEST_F(ImportInt8, AboveMax) {
  parser.input(u8"128"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

class ImportUInt8 : public testing::Test, public TerminalTest<int_rule<unsigned char>> {};
// NOLINTNEXTLINE
TEST_F(ImportUInt8, Min) {
  parser.input(u8"0"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 0U);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt8, Max) {
  parser.input(u8"255"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 255U);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt8, BelowMin) {
  parser.input(u8"-1"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt8, AboveMax) {
  parser.input(u8"256"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

class ImportInt16 : public testing::Test, public TerminalTest<int_rule<short>> {};
// NOLINTNEXTLINE
TEST_F(ImportInt16, Min) {
  parser.input(u8"-32768"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, -32768);
}
// NOLINTNEXTLINE
TEST_F(ImportInt16, Max) {
  parser.input(u8"32767"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 32767);
}
// NOLINTNEXTLINE
TEST_F(ImportInt16, BelowMin) {
  parser.input(u8"-32769"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}
// NOLINTNEXTLINE
TEST_F(ImportInt16, AboveMax) {
  parser.input(u8"32768"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

class ImportUInt16 : public testing::Test, public TerminalTest<int_rule<unsigned short>> {};
// NOLINTNEXTLINE
TEST_F(ImportUInt16, Max) {
  parser.input(u8"65535"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 65535U);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt16, BelowMin) {
  parser.input(u8"-1"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt16, AboveMax) {
  parser.input(u8"65536"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

// A type wider than the policy's integer_type (int) receives any value that the parser produces.
class ImportInt64 : public testing::Test, public TerminalTest<int_rule<long long>> {};
// NOLINTNEXTLINE
TEST_F(ImportInt64, IntMin) {
  parser.input(u8"-2147483648"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, -2147483648LL);
}

class ImportUInt64 : public testing::Test, public TerminalTest<int_rule<unsigned long long>> {};
// NOLINTNEXTLINE
TEST_F(ImportUInt64, IntMax) {
  parser.input(u8"2147483647"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 2147483647ULL);
}
// NOLINTNEXTLINE
TEST_F(ImportUInt64, Negative) {
  parser.input(u8"-1"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

// A policy whose integer_type is wider than int, so that values beyond the range of int reach the rules.
struct wide_policies : peejay::default_policies {
  using integer_type = long long;
};
static_assert(peejay::type_list::index_of_v<peejay::rules::builtin_integers<wide_policies>, unsigned long long> == 9,
              "long long is a standard integer type so it is not repeated");

class ImportWideInt : public testing::Test, public TerminalTest<peejay::rules::integer_rule<wide_policies, int>> {};
// NOLINTNEXTLINE
TEST_F(ImportWideInt, Max) {
  parser.input(u8"2147483647"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 2147483647);
}
// NOLINTNEXTLINE
TEST_F(ImportWideInt, AboveMax) {
  parser.input(u8"2147483648"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

class ImportWideUInt : public testing::Test,
                       public TerminalTest<peejay::rules::integer_rule<wide_policies, unsigned>> {};
// NOLINTNEXTLINE
TEST_F(ImportWideUInt, Max) {
  parser.input(u8"4294967295"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 4294967295U);
}
// NOLINTNEXTLINE
TEST_F(ImportWideUInt, AboveMax) {
  parser.input(u8"4294967296"sv).eof();
  EXPECT_EQ(parser.last_error(), out_of_range);
}

class ImportWideInt64 : public testing::Test,
                        public TerminalTest<peejay::rules::integer_rule<wide_policies, long long>> {};
// NOLINTNEXTLINE
TEST_F(ImportWideInt64, Large) {
  parser.input(u8"1099511627776"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "JSON error was: " << parser.last_error().message();
  EXPECT_EQ(value, 1099511627776LL);
}

}  // end anonymous namespace
