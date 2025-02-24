//===- unittests/test_whitespace.cpp --------------------------------------===//
//*           _     _ _                                  *
//* __      _| |__ (_) |_ ___  ___ _ __   __ _  ___ ___  *
//* \ \ /\ / / '_ \| | __/ _ \/ __| '_ \ / _` |/ __/ _ \ *
//*  \ V  V /| | | | | ||  __/\__ \ |_) | (_| | (_|  __/ *
//*   \_/\_/ |_| |_|_|\__\___||___/ .__/ \__,_|\___\___| *
//*                               |_|                    *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "callbacks.hpp"
#include "peejay/json/json.hpp"

using namespace std::string_view_literals;
using peejay::char_set;
using peejay::error;
using peejay::extensions;
using peejay::make_parser;

namespace {

class Whitespace : public testing::Test {
protected:
  using mocks = mock_json_callbacks<peejay::default_policies::integer_type>;
  testing::StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks> proxy_{callbacks_};
};

}  // end of anonymous namespace

// NOLINTNEXTLINE
TEST_F(Whitespace, Empty) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  auto p = make_parser(proxy_);
  input(p, u8"0"sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(Whitespace, MultipleLeadingSpaces) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  auto p = make_parser(proxy_);
  input(p, u8"    0"sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(Whitespace, MultipleTrailingSpaces) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  auto p = make_parser(proxy_);
  input(p, u8"0    "sv).eof();
  EXPECT_FALSE(p.has_error());
}

namespace {

constexpr decltype(auto) code_point_as_utf32be(char32_t code_point) {
  return std::array{
      static_cast<std::byte>((static_cast<std::uint_least32_t>(code_point) >> 24U) & 0xFFU),
      static_cast<std::byte>((static_cast<std::uint_least32_t>(code_point) >> 16U) & 0xFFU),
      static_cast<std::byte>((static_cast<std::uint_least32_t>(code_point) >> 8U) & 0xFFU),
      static_cast<std::byte>((static_cast<std::uint_least32_t>(code_point) >> 0U) & 0xFFU),
  };
}

template <typename Parser> Parser& input_whitespace_code_points(Parser& parser) {
  for (auto const code_point : {
           char_set::byte_order_mark,
           char_set::character_tabulation,
           char_set::vertical_tabulation,
           char_set::space,
           char_set::no_break_space,
           char_set::en_quad,
           char_set::digit_zero,
       }) {
#if PEEJAY_HAVE_CONCEPTS && PEEJAY_HAVE_RANGES
    parser.input(code_point_as_utf32be(code_point));
#else
    auto const& bytes = code_point_as_utf32be(code_point);
    parser.input(std::begin(bytes), std::end(bytes));
#endif
  }
  return parser;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(Whitespace, ExtendedWhitespaceCharactersEnabled) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  auto p = make_parser(proxy_, extensions::extra_whitespace);
  input_whitespace_code_points(p).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(Whitespace, ExtendedWhitespaceCharactersDisabled) {
  auto p = make_parser(proxy_);
  input_whitespace_code_points(p).eof();
  EXPECT_TRUE(p.has_error());
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
}
