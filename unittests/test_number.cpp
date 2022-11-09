//===- unittests/test_number.cpp ------------------------------------------===//
//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include <string_view>

#include "callbacks.hpp"
#include "json/json.hpp"
#include "json/null.hpp"

using namespace std::string_literals;
using namespace peejay;

using testing::DoubleEq;
using testing::StrictMock;

namespace {

class Number : public ::testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end of anonymous namespace

TEST_F (Number, Zero) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  parser p{proxy_};
  p.input ("0"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, NegativeZero) {
  EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
  parser p{proxy_};
  p.input ("-0"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, One) {
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  parser p{proxy_};
  p.input (" 1 "s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, LeadingZero) {
  parser p{proxy_};
  p.input ("01"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, MinusOne) {
  EXPECT_CALL (callbacks_, int64_value (-1)).Times (1);
  parser p{proxy_};
  p.input ("-1"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, MinusOneLeadingZero) {
  parser p{proxy_};
  p.input ("-01"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, MinusOnly) {
  parser p{proxy_};
  p.input ("-"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits));
}
TEST_F (Number, MinusMinus) {
  parser p{proxy_};
  p.input ("--"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

TEST_F (Number, AllDigits) {
  EXPECT_CALL (callbacks_, uint64_value (1234567890UL)).Times (1);
  parser p{proxy_};
  p.input ("1234567890"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, PositivePi) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (3.1415))).Times (1);
  parser p{proxy_};
  p.input ("3.1415"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, NegativePi) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (-3.1415))).Times (1);
  parser p{proxy_};
  p.input ("-3.1415"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, PositiveZeroPoint45) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.45))).Times (1);
  parser p{proxy_};
  p.input ("0.45"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, NegativeZeroPoint45) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (-0.45))).Times (1);
  parser p{proxy_};
  p.input ("-0.45"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, ZeroExp2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0))).Times (1);
  parser p{proxy_};
  p.input ("0e2"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, OneExp2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
  parser p{proxy_};
  p.input ("1e2"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, OneExpPlus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
  parser p{proxy_};
  p.input ("1e+2"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, ZeroPointZeroOne) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input ("0.01"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, OneExpMinus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input ("1e-2"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, OneCapitalExpMinus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input ("1E-2"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, OneExpMinusZero2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input ("1E-02"s).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, IntegerMax) {
  constexpr auto long_max = std::numeric_limits<std::int64_t>::max ();
  auto const str_max = std::to_string (long_max);

  EXPECT_CALL (callbacks_, uint64_value (long_max)).Times (1);
  parser p{proxy_};
  p.input (str_max).eof ();
  EXPECT_FALSE (p.has_error ());
}

namespace {

// Note that I hard-wire the numbers here rather than just using
// numeric_limits<> so that  we've got a reference for the string constants
// below.
constexpr auto uint64_max = UINT64_C (18446744073709551615);
static_assert (uint64_max == std::numeric_limits<std::uint64_t>::max (),
               "Hard-wired unsigned 64-bit max value seems to be incorrect");
constexpr auto uint64_max_str =
    "18446744073709551615";  // string equivalent of uint64_max.
constexpr auto uint64_overflow = "18446744073709551616";  // uint64_max plus 1.

// The literal "most negative int" cannot be written in C++. Integer constants
// are formed by building an unsigned integer and then applying unary minus.
constexpr auto int64_min = -(INT64_C (9223372036854775807)) - 1;
static_assert (int64_min == std::numeric_limits<std::int64_t>::min (),
               "Hard-wired signed 64-bit min value seems to be incorrect");
constexpr auto int64_min_str = "-9223372036854775808";
constexpr auto int64_overflow = "-9223372036854775809";  // int64_min minus 1.

}  // end anonymous namespace

TEST_F (Number, Uint64Max) {
  assert (uint64_max_str == std::to_string (uint64_max) &&
          "The hard-wired unsigned 64-bit max string seems to be incorrect");
  EXPECT_CALL (callbacks_, uint64_value (uint64_max)).Times (1);
  parser p{proxy_};
  p.input (std::string_view{uint64_max_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, Int64Min) {
  assert (int64_min_str == std::to_string (int64_min) &&
          "The hard-wired signed 64-bit min string seems to be incorrect");
  EXPECT_CALL (callbacks_, int64_value (int64_min)).Times (1);
  parser p{proxy_};
  p.input (std::string_view{int64_min_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (Number, IntegerPositiveOverflow) {
  parser p{proxy_};
  p.input (std::string_view{uint64_overflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, IntegerNegativeOverflow1) {
  parser p{proxy_};
  p.input ("-123123123123123123123123123123"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, IntegerNegativeOverflow2) {
  parser p{proxy_};
  p.input (std::string_view{int64_overflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, RealPositiveOverflow) {
  parser p{proxy_};
  p.input ("123123e100000"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, RealPositiveOverflow2) {
  parser p{proxy_};
  p.input ("9999E999"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, RealUnderflow) {
  parser p = make_parser (proxy_);
  p.input ("123e-10000000"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

TEST_F (Number, BadExponentDigit) {
  parser p{proxy_};
  p.input ("1Ex"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

TEST_F (Number, BadFractionDigit) {
  parser p{proxy_};
  p.input ("1.."s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}
TEST_F (Number, BadExponentAfterPoint) {
  parser p{proxy_};
  p.input ("1.E"s).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}
