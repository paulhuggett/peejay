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
using testing::InSequence;
using testing::IsNan;
using testing::StrictMock;

namespace {

class Number : public ::testing::Test {
protected:
  using mocks = mock_json_callbacks<std::uint64_t>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks> proxy_{callbacks_};
};

}  // end of anonymous namespace

// NOLINTNEXTLINE
TEST_F (Number, Zero) {
  EXPECT_CALL (callbacks_, integer_value (0)).Times (1);
  parser p{proxy_};
  p.input (u8"0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, NegativeZero) {
  EXPECT_CALL (callbacks_, integer_value (0)).Times (1);
  parser p{proxy_};
  p.input (u8"-0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, One) {
  EXPECT_CALL (callbacks_, integer_value (1)).Times (1);
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
  EXPECT_CALL (callbacks_, integer_value (-1)).Times (1);
  parser p{proxy_};
  p.input (u8"-1"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Number, OneWithLeadingPlus) {
  EXPECT_CALL (callbacks_, integer_value (1)).Times (1);
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
  EXPECT_CALL (callbacks_, integer_value (INT64_C (1234567890))).Times (1);
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

  EXPECT_CALL (callbacks_, integer_value (long_max)).Times (1);
  parser p{proxy_};
  p.input (str_max).eof ();
  EXPECT_FALSE (p.has_error ());
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
  EXPECT_CALL (callbacks_, integer_value (0x10)).Times (1);
  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"0x10"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, HexArray) {
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, integer_value (0x10)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::numbers);
  p.input (u8"[0x10,0x10]"sv).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero. Was: "
                                 << p.last_error ().message ();
}
// NOLINTNEXTLINE
TEST_F (Number, NegativeHex) {
  EXPECT_CALL (callbacks_, integer_value (-31)).Times (1);
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
    InSequence const _;
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
  InSequence const _;
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

namespace {

template <int Bits>
struct limits {};
template <>
struct limits<64> {
  struct policy {
    static constexpr std::size_t max_length = 20;
    using integer_type = std::int64_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = INT64_C (9223372036854775807);
  static_assert (int_max == std::numeric_limits<std::int64_t>::max (),
                 "Hard-wired signed 64-bit max value seems to be incorrect");
  static constexpr auto int_max_str =
      u8"9223372036854775807";  // string equivalent of int_max.
  static constexpr auto int_overflow =
      u8"9223372036854775808";  // uint_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT64_C (9'223'372'036'854'775'807)) - 1;
  static_assert (int_min == std::numeric_limits<std::int64_t>::min (),
                 "Hard-wired signed 64-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-9223372036854775808";
  static constexpr auto int_underflow =
      u8"-9223372036854775809";  // int_min minus 1.
};

template <>
struct limits<32> {
  struct policy {
    static constexpr std::size_t max_length = 20;
    using integer_type = std::int32_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = INT32_C (2'147'483'647);
  static_assert (int_max == std::numeric_limits<std::int32_t>::max (),
                 "Hard-wired signed 32-bit max value seems to be incorrect");
  static constexpr auto int_max_str =
      u8"2147483647";  // string equivalent of int_max.
  static constexpr auto int_overflow = u8"2147483648";  // int_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT32_C (2'147'483'647)) - 1;
  static_assert (int_min == std::numeric_limits<std::int32_t>::min (),
                 "Hard-wired signed 32-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-2147483648";
  static constexpr auto int_underflow = u8"-2147483649";  // int_min minus 1.
};

template <>
struct limits<16> {
  struct policy {
    static constexpr std::size_t max_length = 20;
    using integer_type = std::int16_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = UINT16_C (32767);
  static_assert (int_max == std::numeric_limits<std::int16_t>::max (),
                 "Hard-wired signed 16-bit max value seems to be incorrect");
  static constexpr auto int_max_str =
      u8"32767";  // string equivalent of uint_max.
  static constexpr auto int_overflow = u8"32768";  // uint_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT16_C (32767)) - 1;
  static_assert (int_min == std::numeric_limits<std::int16_t>::min (),
                 "Hard-wired signed 16-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-32768";
  static constexpr auto int_underflow = u8"-32769";  // int_min minus 1.
};

}  // end anonymous namespace

template <typename TypeParam>
class NumberLimits : public testing::Test {
public:
  static constexpr int bits_param = TypeParam ();
  using policy = typename limits<bits_param>::policy;

  using mocks = mock_json_callbacks<typename policy::integer_type>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks> proxy_{callbacks_};
};

using Sizes = testing::Types<std::integral_constant<int, 16>,
                             std::integral_constant<int, 32>,
                             std::integral_constant<int, 64>>;
TYPED_TEST_SUITE (NumberLimits, Sizes, );

// NOLINTNEXTLINE
TYPED_TEST (NumberLimits, IntMax) {
  constexpr auto bits = TypeParam ();
  assert (limits<bits>::int_max_str == to_u8string (limits<bits>::int_max) &&
          "The hard-wired unsigned max string seems to be incorrect");
  EXPECT_CALL (TestFixture::callbacks_, integer_value (limits<bits>::int_max))
      .Times (1);
  auto p = make_parser<typename TestFixture::policy> (TestFixture::proxy_);
  p.input (u8string_view{limits<bits>::int_max_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}
// NOLINTNEXTLINE
TYPED_TEST (NumberLimits, IntMin) {
  constexpr auto bits = TypeParam ();
  assert (limits<bits>::int_min_str == to_u8string (limits<bits>::int_min) &&
          "The hard-wired signed min string seems to be incorrect");
  EXPECT_CALL (TestFixture::callbacks_, integer_value (limits<bits>::int_min))
      .Times (1);
  auto p = make_parser<typename TestFixture::policy> (TestFixture::proxy_);
  p.input (u8string_view{limits<bits>::int_min_str}).eof ();
  EXPECT_FALSE (p.has_error ());
}
// NOLINTNEXTLINE
TYPED_TEST (NumberLimits, IntegerPositiveOverflow) {
  constexpr auto bits = TypeParam ();
  auto p = make_parser<typename TestFixture::policy> (TestFixture::proxy_);
  p.input (u8string_view{limits<bits>::int_overflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}
// NOLINTNEXTLINE
TYPED_TEST (NumberLimits, IntegerNegativeOverflow1) {
  auto p = make_parser<typename TestFixture::policy> (TestFixture::proxy_);
  p.input (u8"-123123123123123123123123123123"sv).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}
// NOLINTNEXTLINE
TYPED_TEST (NumberLimits, IntegerNegativeOverflow2) {
  constexpr auto bits = TypeParam ();
  auto p = make_parser<typename TestFixture::policy> (TestFixture::proxy_);
  p.input (u8string_view{limits<bits>::int_underflow}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error::number_out_of_range));
}
