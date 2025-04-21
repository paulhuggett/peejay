//===- unit_tests/test_number.cpp -----------------------------------------===//
//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
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
#include <gtest/gtest.h>

#include <limits>
#include <string_view>

#include "callbacks.hpp"
#include "config.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using peejay::error;
using peejay::make_parser;
using peejay::parser;

using testing::DoubleEq;
using testing::InSequence;
using testing::IsNan;
using testing::StrictMock;

namespace {

class Number : public ::testing::Test {
protected:
  using mocks = mock_json_callbacks<std::uint64_t, double, char8_t>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks> proxy_{callbacks_};
};

}  // end of anonymous namespace

// NOLINTNEXTLINE
TEST_F(Number, Zero) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  parser p{proxy_};
  input(p, u8"0"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, NegativeZero) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  parser p{proxy_};
  input(p, u8"-0"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, One) {
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  parser p{proxy_};
  input(p, u8" 1 "sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, LeadingZero) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  parser p{proxy_};
  input(p, u8"01"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unexpected_extra_input))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, MinusOne) {
  EXPECT_CALL(callbacks_, integer_value(-1)).Times(1);
  parser p{proxy_};
  input(p, u8"-1"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, LeadingPlusExtensionDisabled) {
  parser p{proxy_};
  input(p, u8"+1"sv).eof();
  EXPECT_TRUE(p.has_error());
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token)) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, MinusOneLeadingZero) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  parser p{proxy_};
  input(p, u8"-01"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unexpected_extra_input))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, MinusOnly) {
  parser p{proxy_};
  input(p, u8"-"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_digits)) << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TEST_F(Number, MinusMinus) {
  parser p{proxy_};
  input(p, u8"--"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, AllDigits) {
  EXPECT_CALL(callbacks_, integer_value(INT64_C(1234567890))).Times(1);
  parser p{proxy_};
  input(p, u8"1234567890"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, PositivePi) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(3.1415))).Times(1);
  parser p{proxy_};
  input(p, u8"3.1415"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, NegativePi) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(-3.1415))).Times(1);
  parser p{proxy_};
  input(p, u8"-3.1415"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, PositiveZeroPoint45) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(0.45))).Times(1);
  parser p{proxy_};
  input(p, u8"0.45"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, NegativeZeroPoint45) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(-0.45))).Times(1);
  parser p{proxy_};
  input(p, u8"-0.45"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, ZeroExp2) {
  EXPECT_CALL(callbacks_, integer_value(0)).Times(1);
  parser p{proxy_};
  input(p, u8"0e2"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, OneExp2) {
  EXPECT_CALL(callbacks_, integer_value(100)).Times(1);
  parser p{proxy_};
  input(p, u8"1e2"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, OneExpPlus2) {
  EXPECT_CALL(callbacks_, integer_value(100)).Times(1);
  parser p{proxy_};
  input(p, u8"1e+2"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, ZeroPointZeroOne) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(0.01))).Times(1);
  parser p{proxy_};
  input(p, u8"0.01"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, OneExpMinus2) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(0.01))).Times(1);
  parser p{proxy_};
  input(p, u8"1e-2"sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(Number, OneCapitalExpMinus2) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(0.01))).Times(1);
  parser p{proxy_};
  input(p, u8"1E-2"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, OneExpMinusZero2) {
  EXPECT_CALL(callbacks_, float_value(DoubleEq(0.01))).Times(1);
  parser p{proxy_};
  input(p, u8"1E-02"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, IntegerMax) {
  constexpr auto long_max = std::numeric_limits<std::int64_t>::max();
  auto const str_max = to_u8string(long_max);

  EXPECT_CALL(callbacks_, integer_value(long_max)).Times(1);
  parser p{proxy_};
  input(p, str_max).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, RealPositiveOverflow) {
  parser p{proxy_};
  input(p, u8"123123e100000"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, RealPositiveOverflow2) {
  parser p{proxy_};
  input(p, u8"9999E999"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, RealUnderflow) {
  parser p = make_parser(proxy_);
  input(p, u8"123e-10000000"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, BadExponentDigit) {
  parser p{proxy_};
  input(p, u8"1Ex"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token))
      << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(Number, BadFractionDigit) {
  parser p{proxy_};
  input(p, u8"1.."sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token))
      << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TEST_F(Number, BadExponentAfterPoint) {
  parser p{proxy_};
  input(p, u8"1.E"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token))
      << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TEST_F(Number, LeadingDotExtensionDisabled) {
  auto p = make_parser(proxy_);
  input(p, u8".1234"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token)) << "Real error was: " << p.last_error().message();
}

namespace {

struct policy_common : public peejay::default_policies {
  static constexpr auto max_length = std::size_t{20};
  static constexpr auto max_stack_depth = std::size_t{8};
};

template <int Bits> struct limits {};
template <> struct limits<64> {
  struct policy : policy_common {
    using integer_type = std::int64_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = INT64_C(9223372036854775807);
  static_assert(int_max == std::numeric_limits<std::int64_t>::max(),
                "Hard-wired signed 64-bit max value seems to be incorrect");
  static constexpr auto int_max_str = u8"9223372036854775807";   // string equivalent of int_max.
  static constexpr auto int_overflow = u8"9223372036854775808";  // uint_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT64_C(9'223'372'036'854'775'807)) - 1;
  static_assert(int_min == std::numeric_limits<std::int64_t>::min(),
                "Hard-wired signed 64-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-9223372036854775808";
  static constexpr auto int_underflow = u8"-9223372036854775809";  // int_min minus 1.
};

template <> struct limits<32> {
  struct policy : policy_common {
    using integer_type = std::int32_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = INT32_C(2'147'483'647);
  static_assert(int_max == std::numeric_limits<std::int32_t>::max(),
                "Hard-wired signed 32-bit max value seems to be incorrect");
  static constexpr auto int_max_str = u8"2147483647";   // string equivalent of int_max.
  static constexpr auto int_overflow = u8"2147483648";  // int_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT32_C(2'147'483'647)) - 1;
  static_assert(int_min == std::numeric_limits<std::int32_t>::min(),
                "Hard-wired signed 32-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-2147483648";
  static constexpr auto int_underflow = u8"-2147483649";  // int_min minus 1.
};

template <> struct limits<16> {
  struct policy : policy_common {
    using integer_type = std::int16_t;
  };

  // Note that I hard-wire the numbers here rather than just using
  // numeric_limits<> so that  we've got a reference for the string constants
  // below.
  static constexpr auto int_max = UINT16_C(32767);
  static_assert(int_max == std::numeric_limits<std::int16_t>::max(),
                "Hard-wired signed 16-bit max value seems to be incorrect");
  static constexpr auto int_max_str = u8"32767";   // string equivalent of uint_max.
  static constexpr auto int_overflow = u8"32768";  // uint_max plus 1.

  // The literal "most negative int" cannot be written in C++. Integer constants
  // are formed by building an unsigned integer and then applying unary minus.
  static constexpr auto int_min = -(INT16_C(32767)) - 1;
  static_assert(int_min == std::numeric_limits<std::int16_t>::min(),
                "Hard-wired signed 16-bit min value seems to be incorrect");
  static constexpr auto int_min_str = u8"-32768";
  static constexpr auto int_underflow = u8"-32769";  // int_min minus 1.
};

}  // end anonymous namespace

template <typename TypeParam> class NumberLimits : public testing::Test {
public:
  static constexpr int bits_param = TypeParam();
  using policy = typename limits<bits_param>::policy;

  using mocks = mock_json_callbacks<typename policy::integer_type, double, char8_t>;
  StrictMock<mocks> callbacks_;
  callbacks_proxy<mocks, policy> proxy_{callbacks_};
};

using Sizes =
    testing::Types<std::integral_constant<int, 16>, std::integral_constant<int, 32>, std::integral_constant<int, 64>>;
TYPED_TEST_SUITE(NumberLimits, Sizes, );

// NOLINTNEXTLINE
TYPED_TEST(NumberLimits, IntMax) {
  constexpr auto bits = TypeParam();
  assert(limits<bits>::int_max_str == to_u8string(limits<bits>::int_max) &&
         "The hard-wired unsigned max string seems to be incorrect");
  EXPECT_CALL(TestFixture::callbacks_, integer_value(limits<bits>::int_max)).Times(1);
  auto p = make_parser(TestFixture::proxy_);
  input(p, std::u8string_view{limits<bits>::int_max_str}).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TYPED_TEST(NumberLimits, IntMin) {
  constexpr auto bits = TypeParam();
  assert(limits<bits>::int_min_str == to_u8string(limits<bits>::int_min) &&
         "The hard-wired signed min string seems to be incorrect");
  EXPECT_CALL(TestFixture::callbacks_, integer_value(limits<bits>::int_min)).Times(1);
  auto p = make_parser(TestFixture::proxy_);
  input(p, std::u8string_view{limits<bits>::int_min_str}).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TYPED_TEST(NumberLimits, IntegerPositiveOverflow) {
  constexpr auto bits = TypeParam();
  auto p = make_parser(TestFixture::proxy_);
  input(p, std::u8string_view{limits<bits>::int_overflow}).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TYPED_TEST(NumberLimits, IntegerNegativeOverflow1) {
  auto p = make_parser(TestFixture::proxy_);
  input(p, u8"-123123123123123123123123123123"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}
// NOLINTNEXTLINE
TYPED_TEST(NumberLimits, IntegerNegativeOverflow2) {
  constexpr auto bits = TypeParam();
  auto p = make_parser(TestFixture::proxy_);
  input(p, std::u8string_view{limits<bits>::int_underflow}).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}

struct no_float_policy : public peejay::default_policies {
  using float_type = peejay::no_float_type;
};

TEST(NumberFloat, NoFloat) {
  using mocks = mock_json_callbacks<std::uint64_t, double, char8_t>;
  StrictMock<mocks> callbacks;
  callbacks_proxy<mocks, no_float_policy> proxy{callbacks};

  auto p = make_parser(proxy);
  input(p, u8"1.2"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::number_out_of_range))
      << "Real error was: " << p.last_error().message();
}

struct long_double_policy : public peejay::default_policies {
  using float_type = long double;
};

TEST(NumberFloat, LongDouble) {
  using mocks = mock_json_callbacks<std::uint64_t, long double, char8_t>;
  StrictMock<mocks> callbacks;
  callbacks_proxy<mocks, long_double_policy> proxy{callbacks};
  EXPECT_CALL(callbacks, float_value(1.2L)).Times(1);

  auto p = make_parser(proxy);
  input(p, u8"1.2"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

#if PEEJAY_HAVE_INT128
struct int128_policy : public peejay::default_policies {
  using integer_type = __int128;
};

TEST(NumberInt128, LongDouble) {
  using mocks = mock_json_callbacks<int128_policy::integer_type, int128_policy::float_type, int128_policy::char_type>;
  StrictMock<mocks> callbacks;
  callbacks_proxy<mocks, int128_policy> proxy{callbacks};

  constexpr auto expected = []() -> __int128 {
    __int128 v = 1234567890;
    v *= 10000000000;
    v += 1234567890;
    v *= 10000000000;
    v += 1234567890;
    return v;
  }();
  EXPECT_CALL(callbacks, integer_value(expected)).Times(1);

  auto p = make_parser(proxy);
  p.input(u8"123456789012345678901234567890"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}
#endif  // PEEJAY_HAVE_INT128
