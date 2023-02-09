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

#include <limits>
#include <string_view>

#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using peejay::error;
using peejay::extensions;
using peejay::make_parser;
using peejay::parser;
using peejay::u8string_view;

using testing::DoubleEq;
using testing::IsNan;
using testing::StrictMock;

namespace {

class Number : public ::testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end of anonymous namespace

// NOLINTNEXTLINE
TEST_F (Number, Zero) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  parser p{proxy_};
  p.input (u8"0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, NegativeZero) {
  EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
  parser p{proxy_};
  p.input (u8"-0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, One) {
  EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
  parser p{proxy_};
  p.input (u8" 1 "sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, LeadingZero) {
  parser p{proxy_};
  p.input (u8"01"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, MinusOne) {
  EXPECT_CALL (callbacks_, int64_value (-1)).Times (1);
  parser p{proxy_};
  p.input (u8"-1"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneWithLeadingPlus) {
  EXPECT_CALL (callbacks_, uint64_value (1U)).Times (1);
  auto p = make_parser (proxy_, extensions::leading_plus);
  p.input (u8"+1"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Number, LeadingPlusExtensionDisabled) {
  parser p{proxy_};
  p.input (u8"+1"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}

// NOLINTNEXTLINE
TEST_F (Number, MinusOneLeadingZero) {
  parser p{proxy_};
  p.input (u8"-01"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, MinusOnly) {
  parser p{proxy_};
  p.input (u8"-"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits));
}
// NOLINTNEXTLINE
TEST_F (Number, MinusMinus) {
  parser p{proxy_};
  p.input (u8"--"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST_F (Number, AllDigits) {
  EXPECT_CALL (callbacks_, uint64_value (1234567890UL)).Times (1);
  parser p{proxy_};
  p.input (u8"1234567890"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, PositivePi) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (3.1415))).Times (1);
  parser p{proxy_};
  p.input (u8"3.1415"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, NegativePi) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (-3.1415))).Times (1);
  parser p{proxy_};
  p.input (u8"-3.1415"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, PositiveZeroPoint45) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.45))).Times (1);
  parser p{proxy_};
  p.input (u8"0.45"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, NegativeZeroPoint45) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (-0.45))).Times (1);
  parser p{proxy_};
  p.input (u8"-0.45"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, ZeroExp2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0))).Times (1);
  parser p{proxy_};
  p.input (u8"0e2"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneExp2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
  parser p{proxy_};
  p.input (u8"1e2"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneExpPlus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
  parser p{proxy_};
  p.input (u8"1e+2"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, ZeroPointZeroOne) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input (u8"0.01"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneExpMinus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input (u8"1e-2"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneCapitalExpMinus2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input (u8"1E-2"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneExpMinusZero2) {
  EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
  parser p{proxy_};
  p.input (u8"1E-02"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, IntegerMax) {
  constexpr auto long_max = std::numeric_limits<std::int64_t>::max ();
  auto const str_max = to_u8string (long_max);

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
    u8"18446744073709551615";  // string equivalent of uint64_max.
constexpr auto uint64_overflow =
    u8"18446744073709551616";  // uint64_max plus 1.

// The literal "most negative int" cannot be written in C++. Integer constants
// are formed by building an unsigned integer and then applying unary minus.
constexpr auto int64_min = -(INT64_C (9223372036854775807)) - 1;
static_assert (int64_min == std::numeric_limits<std::int64_t>::min (),
               "Hard-wired signed 64-bit min value seems to be incorrect");
constexpr auto int64_min_str = u8"-9223372036854775808";
constexpr auto int64_overflow = u8"-9223372036854775809";  // int64_min minus 1.

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (Number, Uint64Max) {
  assert (uint64_max_str == to_u8string (uint64_max) &&
          "The hard-wired unsigned 64-bit max string seems to be incorrect");
  EXPECT_CALL (callbacks_, uint64_value (uint64_max)).Times (1);
  parser p{proxy_};
  p.input (u8string_view{uint64_max_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, Int64Min) {
  assert (int64_min_str == to_u8string (int64_min) &&
          "The hard-wired signed 64-bit min string seems to be incorrect");
  EXPECT_CALL (callbacks_, int64_value (int64_min)).Times (1);
  parser p{proxy_};
  p.input (u8string_view{int64_min_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, IntegerPositiveOverflow) {
  parser p{proxy_};
  p.input (u8string_view{uint64_overflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, IntegerNegativeOverflow1) {
  parser p{proxy_};
  p.input (u8"-123123123123123123123123123123"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, IntegerNegativeOverflow2) {
  parser p{proxy_};
  p.input (u8string_view{int64_overflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, RealPositiveOverflow) {
  parser p{proxy_};
  p.input (u8"123123e100000"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, RealPositiveOverflow2) {
  parser p{proxy_};
  p.input (u8"9999E999"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, RealUnderflow) {
  parser p = make_parser (proxy_);
  p.input (u8"123e-10000000"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST_F (Number, BadExponentDigit) {
  parser p{proxy_};
  p.input (u8"1Ex"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST_F (Number, BadFractionDigit) {
  parser p{proxy_};
  p.input (u8"1.."sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}
// NOLINTNEXTLINE
TEST_F (Number, BadExponentAfterPoint) {
  parser p{proxy_};
  p.input (u8"1.E"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token));
}
// NOLINTNEXTLINE
TEST_F (Number, Hex) {
  EXPECT_CALL (callbacks_, uint64_value (uint64_t{0x10})).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"0x10"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, HexArray) {
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (uint64_t{0x10})).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"[0x10,0x10]"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, NegativeHex) {
  EXPECT_CALL (callbacks_, int64_value (int64_t{-31})).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"-0x1f"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, HexExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"0x10"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range))
      << "Error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, BadLetterAfterX) {
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"0xt"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits))
      << "Error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, EndAfterX) {
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"0x"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits))
      << "Error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, Infinity) {
  EXPECT_CALL (callbacks_,
               double_value (std::numeric_limits<double>::infinity ()))
      .Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"Infinity"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, InfinityExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"Infinity"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token))
      << "Error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, NaN) {
  EXPECT_CALL (callbacks_, double_value (IsNan ())).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"NaN"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, NaNExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"NaN"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token))
      << "Error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, PlusInfinity) {
  EXPECT_CALL (
      callbacks_,
      double_value (DoubleEq (std::numeric_limits<double>::infinity ())))
      .Times (1);
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"+Infinity"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, PlusInfinityExtraCharacters) {
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"+InfinityX"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token))
      << "Parse error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, PlusInfinityPartial) {
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"+Inf"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token))
      << "Parse error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, MinusInfinity) {
  EXPECT_CALL (
      callbacks_,
      double_value (DoubleEq (-1.0 * std::numeric_limits<double>::infinity ())))
      .Times (1);
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"-Infinity"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, PlusNan) {
  EXPECT_CALL (callbacks_, double_value (IsNan ())).Times (1);
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"+NaN"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, MinusNan) {
  EXPECT_CALL (callbacks_, double_value (IsNan ())).Times (1);
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"-NaN"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, ArrayOfNaNAndInfinity) {
  {
    testing::InSequence _;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (
        callbacks_,
        double_value (DoubleEq (std::numeric_limits<double>::infinity ())))
        .Times (1);
    EXPECT_CALL (callbacks_, double_value (IsNan ())).Times (1);
    EXPECT_CALL (
        callbacks_,
        double_value (DoubleEq (std::numeric_limits<double>::infinity ())))
        .Times (1);
    EXPECT_CALL (callbacks_,
                 double_value (DoubleEq (
                     -1.0 * std::numeric_limits<double>::infinity ())))
        .Times (1);
    EXPECT_CALL (callbacks_, double_value (IsNan ())).Times (1);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);
  }
  auto p = make_parser (proxy_, extensions::all);
  p.input (u8"[Infinity,NaN,+Infinity,-Infinity,-NaN]"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, LeadingDot) {
  EXPECT_CALL (callbacks_, double_value (0.1234)).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8".1234"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, LeadingDotExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8".1234"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token))
      << "Real error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, TrailingDot) {
  EXPECT_CALL (callbacks_, double_value (1234.0)).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"1234."sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, TrailingDotExtensionDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"1234."sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits))
      << "Real error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, ArrayOfLeadingAndTrailingDot) {
  testing::InSequence _;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, double_value (0.1)).Times (1);
  EXPECT_CALL (callbacks_, double_value (1.0)).Times (1);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"[.1,1.]"sv).eof ();
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, LoneDecimalPointThenEOF) {
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"."sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_digits))
      << "Real error was: " << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, LoneDecimalPointThenWhitespace) {
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8". "sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::unrecognized_token))
      << "Real error was: " << p.last_error ().message ();
}
