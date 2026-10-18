//===- unit_tests/test_big_uint.cpp ---------------------------------------===//
//*  _    _             _     _    *
//* | |__(_)__ _   _  _(_)_ _| |_  *
//* | '_ \ / _` | | || | | ' \  _| *
//* |_.__/_\__, |  \_,_|_|_||_\__| *
//*        |___/                   *
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
#include "peejay/details/big_uint.hpp"

// Standard library
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <type_traits>

// Google test
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#if defined(PEEJAY_FUZZTEST) && PEEJAY_FUZZTEST
#include <fuzztest/fuzztest.h>
#endif

namespace {

#if defined(__SIZEOF_INT128__)
using u128 = unsigned __int128;
#endif  // defined(__SIZEOF_INT128__)

constexpr std::uint32_t max32 = std::numeric_limits<std::uint32_t>::max();
constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

template <std::size_t Limbs> using big_uint = peejay::details::big_uint<Limbs>;

using big2 = big_uint<2>;
using big4 = big_uint<4>;
using limbs4 = std::array<std::uint32_t, 4>;

template <std::size_t N> std::array<std::uint32_t, N> limbs_of(big_uint<N> const& b) {
  std::array<std::uint32_t, N> out{};
  for (auto index = std::size_t{0}; index < N; ++index) {
    out.at(index) = b.limb(index);
  }
  return out;
}

// A value whose every limb is \p v.
template <std::size_t N> big_uint<N> filled(std::uint32_t const v) {
  big_uint<N> b;
  for (auto index = std::size_t{0}; index < N; ++index) {
    b.set_limb(index, v);
  }
  return b;
}

template <std::size_t N> big_uint<N> make_limbs(std::initializer_list<std::uint32_t> const values) {
  big_uint<N> b;
  std::size_t index = 0;
  for (std::uint32_t const v : values) {
    b.set_limb(index, v);
    ++index;
  }
  return b;
}

#if defined(__SIZEOF_INT128__)
big4 make_big(u128 const v) {
  big4 b;
  for (auto index = std::size_t{0}; index < 4; ++index) {
    b.set_limb(index, static_cast<std::uint32_t>(v >> (32U * index)));
  }
  return b;
}
#endif  // defined(__SIZEOF_INT128__)

big4 from_limbs(limbs4 const& limbs) {
  big4 b;
  for (auto index = std::size_t{0}; index < limbs.size(); ++index) {
    b.set_limb(index, limbs[index]);
  }
  return b;
}

// A value with a different pattern in every limb, used as a "typical" multi-limb operand.
constexpr limbs4 pattern4{0x89ABCDEFU, 0x01234567U, 0xFEDCBA98U, 0x76543210U};

// ============================================================ constraints ===

template <std::size_t N>
concept big_uint_instantiable = requires { typename big_uint<N>; };

static_assert(!big_uint_instantiable<0>);
static_assert(!big_uint_instantiable<1>);
static_assert(big_uint_instantiable<2>);
static_assert(big_uint_instantiable<3>);
static_assert(big_uint_instantiable<32>);

TEST(BigUintConstraints, RequiresAtLeastTwoLimbs) {
  EXPECT_FALSE(big_uint_instantiable<0>);
  EXPECT_FALSE(big_uint_instantiable<1>);
  EXPECT_TRUE(big_uint_instantiable<2>);
  EXPECT_TRUE(big_uint_instantiable<64>);
}

TEST(BigUintBasics, StartsAtZero) {
  big4 const b;
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
  EXPECT_EQ(b.bit_length(), 0U);
  EXPECT_EQ(b.low64(), 0U);
  EXPECT_TRUE(b.fits_u64());
  for (std::size_t i = 0; i < 140; ++i) {
    EXPECT_FALSE(b.bit(i)) << i;
    EXPECT_FALSE(b.any_below(i)) << i;
  }
}
TEST(BigUintBasics, SetLimbAndLimbRoundTrip) {
  big4 b;
  b.set_limb(0, 0x01234567U);
  b.set_limb(1, 0x89ABCDEFU);
  b.set_limb(2, max32);
  b.set_limb(3, 1U);
  EXPECT_EQ(b.limb(0), 0x01234567U);
  EXPECT_EQ(b.limb(1), 0x89ABCDEFU);
  EXPECT_EQ(b.limb(2), max32);
  EXPECT_EQ(b.limb(3), 1U);
}
TEST(BigUintBasics, SetLimbOverwritesOnlyThatLimb) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.set_limb(2, 0);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 2, 0, 4}));
}
TEST(BigUintBasics, CopiesAreIndependent) {
  big4 const a = make_limbs<4>({1, 2, 3, 4});
  big4 b = a;
  b.set_limb(0, 99);
  EXPECT_EQ(a.limb(0), 1U);
  EXPECT_EQ(b.limb(0), 99U);
}
TEST(BigUintBasics, MinimumSizeIsTwoLimbs) {
  big2 b;
  b.set_limb(0, max32);
  b.set_limb(1, max32);
  EXPECT_EQ(b.low64(), std::numeric_limits<std::uint64_t>::max());
  EXPECT_TRUE(b.fits_u64());
  EXPECT_EQ(b.bit_length(), 64U);
}

// low 64

TEST(BigUintLow64, CombinesTheTwoLowestLimbs) {
  big4 const b = make_limbs<4>({0x89ABCDEFU, 0x01234567U, 0, 0});
  EXPECT_EQ(b.low64(), 0x0123456789ABCDEFULL);
}
TEST(BigUintLow64, IgnoresHigherLimbs) {
  big4 const b = make_limbs<4>({0x89ABCDEFU, 0x01234567U, 7, 9});
  EXPECT_EQ(b.low64(), 0x0123456789ABCDEFULL);
}
TEST(BigUintLow64, CoversTheFull64BitRange) {
  EXPECT_EQ((make_limbs<4>({max32, max32, 0, 0}).low64()), std::numeric_limits<std::uint64_t>::max());
  EXPECT_EQ((make_limbs<4>({0, 1, 0, 0}).low64()), 0x100000000ULL);
  EXPECT_EQ((make_limbs<4>({1, 0, 0, 0}).low64()), 1U);
}

// u64

TEST(BigUintFitsU64, TrueWhenOnlyTheTwoLowLimbsAreUsed) {
  EXPECT_TRUE((make_limbs<4>({max32, max32, 0, 0}).fits_u64()));
  EXPECT_TRUE((make_limbs<4>({0, 0, 0, 0}).fits_u64()));
  EXPECT_TRUE((make_limbs<4>({5, 0, 0, 0}).fits_u64()));
}
TEST(BigUintFitsU64, FalseWhenLimbTwoIsSet) {
  EXPECT_FALSE((make_limbs<4>({0, 0, 1, 0}).fits_u64()));
}
TEST(BigUintFitsU64, FalseWhenLimbThreeIsSet) {
  EXPECT_FALSE((make_limbs<4>({0, 0, 0, 1}).fits_u64()));
}
TEST(BigUintFitsU64, FalseWhenEveryHighBitIsSet) {
  EXPECT_FALSE((make_limbs<4>({max32, max32, max32, max32}).fits_u64()));
}
TEST(BigUintFitsU64, AlwaysTrueForTwoLimbs) {
  big2 b;
  b.set_limb(0, max32);
  b.set_limb(1, max32);
  EXPECT_TRUE(b.fits_u64());
}

// mul_add

TEST(BigUintMulAdd, MultiplyByZeroReplacesTheValueWithTheAddend) {
  big4 b = make_limbs<4>({max32, 5, 6, 7});
  EXPECT_EQ(b.mul_add(0, 42), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{42, 0, 0, 0}));
}
TEST(BigUintMulAdd, MultiplyByOneAndAddZeroIsIdentity) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  EXPECT_EQ(b.mul_add(1, 0), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 2, 3, 4}));
}
TEST(BigUintMulAdd, ZeroTimesAnythingPlusAddend) {
  big4 b;
  EXPECT_EQ(b.mul_add(max32, 7), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{7, 0, 0, 0}));
}
TEST(BigUintMulAdd, PlainSmallArithmetic) {
  big4 b = make_limbs<4>({10, 0, 0, 0});
  EXPECT_EQ(b.mul_add(10, 3), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{103, 0, 0, 0}));
}
TEST(BigUintMulAdd, IncrementIsMultiplyByOneAddOne) {
  big4 b = make_limbs<4>({5, 0, 0, 0});
  EXPECT_EQ(b.mul_add(1, 1), 0U);
  EXPECT_EQ(b.limb(0), 6U);
}
TEST(BigUintMulAdd, CarryMovesIntoTheNextLimb) {
  big4 b = make_limbs<4>({max32, 0, 0, 0});
  EXPECT_EQ(b.mul_add(1, 1), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 1, 0, 0}));
}
TEST(BigUintMulAdd, DoublingALimbCarriesOneBit) {
  big4 b = make_limbs<4>({0x80000000U, 0, 0, 0});
  EXPECT_EQ(b.mul_add(2, 0), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 1, 0, 0}));
}
TEST(BigUintMulAdd, CarryRipplesThroughEveryLimb) {
  big4 b = make_limbs<4>({max32, max32, max32, 0});
  EXPECT_EQ(b.mul_add(1, 1), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 1}));
}
TEST(BigUintMulAdd, CarryOutOfTheTopLimbIsReturned) {
  big4 b = make_limbs<4>({0, 0, 0, 0x80000000U});
  EXPECT_EQ(b.mul_add(2, 0), 1U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintMulAdd, AllOnesPlusOneWrapsToZeroWithCarryOne) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  EXPECT_EQ(b.mul_add(1, 1), 1U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintMulAdd, LargestOperandsDoNotOverflowTheIntermediate) {
  // (2^32-1) * (2^32-1) + (2^32-1) == 2^64 - 2^32
  big4 b = make_limbs<4>({max32, 0, 0, 0});
  EXPECT_EQ(b.mul_add(max32, max32), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, max32, 0, 0}));
}
TEST(BigUintMulAdd, LargestCarryOutStillFitsIn32Bits) {
  // (2^128-1) * (2^32-1) + (2^32-1) == (2^32-1) * 2^128
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  EXPECT_EQ(b.mul_add(max32, max32), max32);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintMulAdd, AddendOnlyReachesTheLowestLimbBeforeCarrying) {
  big4 b = make_limbs<4>({max32, 1, 0, 0});
  EXPECT_EQ(b.mul_add(1, max32), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{max32 - 1U, 2, 0, 0}));
}
TEST(BigUintMulAdd, BuildsADecimalNumberDigitByDigit) {
  big4 b;
  for (char const c : {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5',
                       '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',}) {
    EXPECT_EQ(b.mul_add(10, static_cast<std::uint32_t>(c - '0')), 0U);
  }
  // 123456789012345678901234567890
  EXPECT_EQ(limbs_of(b), (limbs4{0x4E3F0AD2U, 0xC373E0EEU, 0x8EE90FF6U, 0x00000001U}));
}
TEST(BigUintMulAdd, TwoLimbInstanceReturnsCarryFromLimbOne) {
  big2 b;
  b.set_limb(0, max32);
  b.set_limb(1, max32);
  EXPECT_EQ(b.mul_add(2, 0), 1U);
  EXPECT_EQ(b.limb(0), max32 - 1U);
  EXPECT_EQ(b.limb(1), max32);
}
TEST(BigUintMulAdd, PatternTimesTenPlusSeven) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.mul_add(10, 7), 4U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x60B60B5DU, 0x0B60B60BU, 0xF49F49F0U, 0x9F49F4A9U}));
}
TEST(BigUintMulAdd, PatternTimesTopBit) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.mul_add(0x80000000U, 0), 0x3B2A1908U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x80000000U, 0xC4D5E6F7U, 0x0091A2B3U, 0x7F6E5D4CU}));
}
TEST(BigUintMulAdd, PatternTimesLargestLimbPlusLargestLimb) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.mul_add(max32, max32), 0x76543210U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x76543210U, 0x88888888U, 0x02468ACFU, 0x88888887U}));
}

// div small

TEST(BigUintDivSmall, ZeroDividedByAnythingIsZero) {
  big4 b;
  EXPECT_EQ(b.div_small(7), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintDivSmall, DivideByOneIsIdentity) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  EXPECT_EQ(b.div_small(1), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 2, 3, 4}));
}
TEST(BigUintDivSmall, ExactDivisionHasNoRemainder) {
  big4 b = make_limbs<4>({100, 0, 0, 0});
  EXPECT_EQ(b.div_small(10), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{10, 0, 0, 0}));
}
TEST(BigUintDivSmall, InexactDivisionReportsTheRemainder) {
  big4 b = make_limbs<4>({103, 0, 0, 0});
  EXPECT_EQ(b.div_small(10), 3U);
  EXPECT_EQ(limbs_of(b), (limbs4{10, 0, 0, 0}));
}
TEST(BigUintDivSmall, DivisorLargerThanValueGivesZeroAndTheValue) {
  big4 b = make_limbs<4>({5, 0, 0, 0});
  EXPECT_EQ(b.div_small(7), 5U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintDivSmall, RemainderPropagatesDownThroughTheLimbs) {
  // 2^64 / 3 = 0x5555555555555555, remainder 1
  big4 b = make_limbs<4>({0, 0, 1, 0});
  EXPECT_EQ(b.div_small(3), 1U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x55555555U, 0x55555555U, 0, 0}));
}
TEST(BigUintDivSmall, ValueHeldOnlyInTheTopLimb) {
  big4 b = make_limbs<4>({0, 0, 0, 1});
  EXPECT_EQ(b.div_small(2), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0x80000000U, 0}));
}
TEST(BigUintDivSmall, LargestDivisor) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  EXPECT_EQ(b.div_small(max32), 0U);  // (2^128-1) is divisible by (2^32-1)
  EXPECT_EQ(limbs_of(b), (limbs4{1, 1, 1, 1}));
}
TEST(BigUintDivSmall, LargestDivisorWithRemainder) {
  big4 b = make_limbs<4>({max32 - 1U, 0, 0, 0});
  EXPECT_EQ(b.div_small(max32), max32 - 1U);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintDivSmall, UndoesMulAddByFiveToTheThirteenth) {
  constexpr std::uint32_t pow5_13 = 1'220'703'125U;
  big4 const original = make_limbs<4>({0xDEADBEEFU, 0x12345678U, 0, 0});
  big4 b = original;
  EXPECT_EQ(b.mul_add(pow5_13, 0), 0U);
  EXPECT_EQ(b.div_small(pow5_13), 0U);
  EXPECT_EQ(limbs_of(b), limbs_of(original));
}
TEST(BigUintDivSmall, PatternDividedByTen) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.div_small(10), 5U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x5A912E31U, 0x001D208AU, 0xE64945DCU, 0x0BD53834U}));
}
TEST(BigUintDivSmall, PatternDividedByTopBit) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.div_small(0x80000000U), 0x09ABCDEFU);
  EXPECT_EQ(limbs_of(b), (limbs4{0x02468ACFU, 0xFDB97530U, 0xECA86421U, 0}));
}
TEST(BigUintDivSmall, PatternDividedByLargestLimb) {
  big4 b = from_limbs(pattern4);
  EXPECT_EQ(b.div_small(max32), 0U);
  EXPECT_EQ(limbs_of(b), (limbs4{0x76543211U, 0x7530ECA9U, 0x76543211U, 0}));
}

// shift left

TEST(BigUintShiftLeft, ByZeroIsIdentity) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.shift_left(0);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 2, 3, 4}));
}
TEST(BigUintShiftLeft, OneBitWithinALimb) {
  big4 b = make_limbs<4>({1, 0, 0, 0});
  b.shift_left(1);
  EXPECT_EQ(limbs_of(b), (limbs4{2, 0, 0, 0}));
}
TEST(BigUintShiftLeft, BitsCrossIntoTheNextLimb) {
  big4 b = make_limbs<4>({0x80000001U, 0, 0, 0});
  b.shift_left(1);
  EXPECT_EQ(limbs_of(b), (limbs4{2, 1, 0, 0}));
}
TEST(BigUintShiftLeft, ExactlyOneLimb) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.shift_left(32);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 1, 2, 3}));
}
TEST(BigUintShiftLeft, OneLimbPlusOneBit) {
  big4 b = make_limbs<4>({0x80000000U, 1, 0, 0});
  b.shift_left(33);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 3, 0}));
}
TEST(BigUintShiftLeft, ThirtyOneBits) {
  big4 b = make_limbs<4>({3, 0, 0, 0});
  b.shift_left(31);
  EXPECT_EQ(limbs_of(b), (limbs4{0x80000000U, 1, 0, 0}));
}
TEST(BigUintShiftLeft, MultiLimbWithAPartialShift) {
  big4 b = make_limbs<4>({max32, max32, 0, 0});
  b.shift_left(36);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0xFFFFFFF0U, 0xFFFFFFFFU, 0xF}));
}
TEST(BigUintShiftLeft, BitsShiftedPastTheTopAreDiscarded) {
  big4 b = make_limbs<4>({0, 0, 0, 0x80000001U});
  b.shift_left(1);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 2}));
}
TEST(BigUintShiftLeft, WholeLimbsShiftedPastTheTopAreDiscarded) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.shift_left(64);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 1, 2}));
}
TEST(BigUintShiftLeft, LargestInRangeShiftKeepsOneBit) {
  big4 b = make_limbs<4>({1, 0, 0, 0});
  b.shift_left(127);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0x80000000U}));
}
TEST(BigUintShiftLeft, ShiftByTheFullWidthClearsTheValue) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  b.shift_left(128);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftLeft, ShiftBeyondTheFullWidthClearsTheValue) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  b.shift_left(129);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
  big4 c = make_limbs<4>({1, 1, 1, 1});
  c.shift_left(100000);
  EXPECT_EQ(limbs_of(c), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftLeft, ZeroStaysZero) {
  big4 b;
  b.shift_left(77);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftLeft, PatternByFourBits) {
  big4 b = from_limbs(pattern4);
  b.shift_left(4);
  EXPECT_EQ(limbs_of(b), (limbs4{0x9ABCDEF0U, 0x12345678U, 0xEDCBA980U, 0x6543210FU}));
}
TEST(BigUintShiftLeft, PatternByOneLimbPlusEightBits) {
  big4 b = from_limbs(pattern4);
  b.shift_left(40);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0xABCDEF00U, 0x23456789U, 0xDCBA9801U}));
}
TEST(BigUintShiftLeft, PatternByThreeLimbsPlusFourBits) {
  big4 b = from_limbs(pattern4);
  b.shift_left(100);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0x9ABCDEF0U}));
}

// shift right

TEST(BigUintShiftRight, ByZeroIsIdentity) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.shift_right(0);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 2, 3, 4}));
}
TEST(BigUintShiftRight, OneBitWithinALimb) {
  big4 b = make_limbs<4>({6, 0, 0, 0});
  b.shift_right(1);
  EXPECT_EQ(limbs_of(b), (limbs4{3, 0, 0, 0}));
}
TEST(BigUintShiftRight, BitsCrossIntoThePreviousLimb) {
  big4 b = make_limbs<4>({2, 1, 0, 0});
  b.shift_right(1);
  EXPECT_EQ(limbs_of(b), (limbs4{0x80000001U, 0, 0, 0}));
}
TEST(BigUintShiftRight, ExactlyOneLimb) {
  big4 b = make_limbs<4>({1, 2, 3, 4});
  b.shift_right(32);
  EXPECT_EQ(limbs_of(b), (limbs4{2, 3, 4, 0}));
}
TEST(BigUintShiftRight, OneLimbPlusOneBit) {
  big4 b = make_limbs<4>({0, 3, 0, 0});
  b.shift_right(33);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 0, 0, 0}));
}
TEST(BigUintShiftRight, ThirtyOneBits) {
  big4 b = make_limbs<4>({0x80000000U, 1, 0, 0});
  b.shift_right(31);
  EXPECT_EQ(limbs_of(b), (limbs4{3, 0, 0, 0}));
}
TEST(BigUintShiftRight, TopLimbShiftsDownWithoutANeighbourAbove) {
  big4 b = make_limbs<4>({0, 0, 0, 0x80000000U});
  b.shift_right(1);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0x40000000U}));
}
TEST(BigUintShiftRight, BitsShiftedOutAreDiscarded) {
  big4 b = make_limbs<4>({1, 0, 0, 0});
  b.shift_right(1);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftRight, MultiLimbWithAPartialShift) {
  big4 b = make_limbs<4>({0, 0xFFFFFFF0U, 0xFFFFFFFFU, 0xF});
  b.shift_right(36);
  EXPECT_EQ(limbs_of(b), (limbs4{max32, max32, 0, 0}));
}
TEST(BigUintShiftRight, LargestInRangeShiftKeepsTheTopBit) {
  big4 b = make_limbs<4>({0, 0, 0, 0x80000000U});
  b.shift_right(127);
  EXPECT_EQ(limbs_of(b), (limbs4{1, 0, 0, 0}));
}
TEST(BigUintShiftRight, ShiftByTheFullWidthClearsTheValue) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  b.shift_right(128);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftRight, ShiftBeyondTheFullWidthClearsTheValue) {
  big4 b = make_limbs<4>({max32, max32, max32, max32});
  b.shift_right(129);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
  big4 c = make_limbs<4>({1, 1, 1, 1});
  c.shift_right(100000);
  EXPECT_EQ(limbs_of(c), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftRight, ZeroStaysZero) {
  big4 b;
  b.shift_right(77);
  EXPECT_EQ(limbs_of(b), (limbs4{0, 0, 0, 0}));
}
TEST(BigUintShiftRight, PatternByFourBits) {
  big4 b = from_limbs(pattern4);
  b.shift_right(4);
  EXPECT_EQ(limbs_of(b), (limbs4{0x789ABCDEU, 0x80123456U, 0x0FEDCBA9U, 0x07654321U}));
}
TEST(BigUintShiftRight, PatternByOneLimbPlusEightBits) {
  big4 b = from_limbs(pattern4);
  b.shift_right(40);
  EXPECT_EQ(limbs_of(b), (limbs4{0x98012345U, 0x10FEDCBAU, 0x00765432U, 0}));
}
TEST(BigUintShiftRight, PatternByThreeLimbsPlusFourBits) {
  big4 b = from_limbs(pattern4);
  b.shift_right(100);
  EXPECT_EQ(limbs_of(b), (limbs4{0x07654321U, 0, 0, 0}));
}

// bit length

TEST(BigUintBitLength, ZeroHasLengthZero) {
  EXPECT_EQ(big4{}.bit_length(), 0U);
}
TEST(BigUintBitLength, OneHasLengthOne) {
  EXPECT_EQ((make_limbs<4>({1, 0, 0, 0}).bit_length()), 1U);
}
TEST(BigUintBitLength, FullLowLimbHasLengthThirtyTwo) {
  EXPECT_EQ((make_limbs<4>({max32, 0, 0, 0}).bit_length()), 32U);
}
TEST(BigUintBitLength, LowestBitOfTheSecondLimbIsBitThirtyThree) {
  EXPECT_EQ((make_limbs<4>({0, 1, 0, 0}).bit_length()), 33U);
}
TEST(BigUintBitLength, TopBitGivesTheFullWidth) {
  EXPECT_EQ((make_limbs<4>({0, 0, 0, 0x80000000U}).bit_length()), 128U);
  EXPECT_EQ((make_limbs<4>({max32, max32, max32, max32}).bit_length()), 128U);
}
TEST(BigUintBitLength, LowerLimbsDoNotAffectASetHigherLimb) {
  EXPECT_EQ((make_limbs<4>({max32, max32, 4, 0}).bit_length()), 67U);
}
#if defined(__SIZEOF_INT128__)
TEST(BigUintBitLength, EverySingleBitPosition) {
  for (std::size_t k = 0; k < 128; ++k) {
    EXPECT_EQ(make_big(u128{1} << k).bit_length(), k + 1) << k;
  }
}
#endif  // defined(__SIZEOF_INT128__)
TEST(BigUintBitLength, Pattern) {
  EXPECT_EQ(from_limbs(pattern4).bit_length(), 127U);
}

// bit

#if defined(__SIZEOF_INT128__)
TEST(BigUintBit, ReadsEverySingleBitPosition) {
  for (std::size_t k = 0; k < 128; ++k) {
    big4 const b = make_big(u128{1} << k);
    for (std::size_t j = 0; j < 128; ++j) {
      EXPECT_EQ(b.bit(j), j == k) << "k=" << k << " j=" << j;
    }
  }
}
#endif  // defined(__SIZEOF_INT128__)
TEST(BigUintBit, LimbBoundaries) {
  big4 const b = make_limbs<4>({0x80000000U, 1, 0, 0});
  EXPECT_TRUE(b.bit(31));
  EXPECT_TRUE(b.bit(32));
  EXPECT_FALSE(b.bit(30));
  EXPECT_FALSE(b.bit(33));
}
TEST(BigUintBit, IndicesPastTheCapacityReadAsZero) {
  big4 const b = make_limbs<4>({max32, max32, max32, max32});
  EXPECT_TRUE(b.bit(127));
  EXPECT_FALSE(b.bit(128));
  EXPECT_FALSE(b.bit(129));
  EXPECT_FALSE(b.bit(1000));
  EXPECT_FALSE(b.bit(npos));
}

// any below

TEST(BigUintAnyBelow, IndexZeroIsAlwaysFalse) {
  EXPECT_FALSE((make_limbs<4>({max32, max32, max32, max32}).any_below(0)));
}
TEST(BigUintAnyBelow, ZeroValueIsAlwaysFalse) {
  big4 const b;
  for (std::size_t shift = 0; shift < 200; ++shift) {
    EXPECT_FALSE(b.any_below(shift)) << shift;
  }
}
TEST(BigUintAnyBelow, PartialLimbMasksOffHigherBits) {
  big4 const b = make_limbs<4>({0x00000100U, 0, 0, 0});  // bit 8
  EXPECT_FALSE(b.any_below(8));
  EXPECT_TRUE(b.any_below(9));
  EXPECT_TRUE(b.any_below(31));
}
TEST(BigUintAnyBelow, WholeLimbBoundaryChecksTheLimbsBelow) {
  EXPECT_TRUE((make_limbs<4>({1, 0, 0, 0}).any_below(32)));
  EXPECT_FALSE((make_limbs<4>({0, 1, 0, 0}).any_below(32)));
  EXPECT_TRUE((make_limbs<4>({0, 1, 0, 0}).any_below(64)));
}
TEST(BigUintAnyBelow, WholeLimbsBelowAreEmptyButThePartialLimbIsNot) {
  big4 const b = make_limbs<4>({0, 0, 1, 0});  // bit 64
  EXPECT_FALSE(b.any_below(64));
  EXPECT_TRUE(b.any_below(65));
  EXPECT_FALSE(b.any_below(63));
}
#if defined(__SIZEOF_INT128__)
TEST(BigUintAnyBelow, BitAtTheIndexItselfIsExcluded) {
  for (std::size_t shift = 0; shift < 128; ++shift) {
    big4 const b = make_big(u128{1} << shift);
    EXPECT_FALSE(b.any_below(shift)) << shift;
    EXPECT_TRUE(b.any_below(shift + 1)) << shift;
  }
}
#endif  // defined(__SIZEOF_INT128__)
TEST(BigUintAnyBelow, IndexEqualToTheCapacityLooksAtEverything) {
  EXPECT_TRUE((make_limbs<4>({0, 0, 0, 0x80000000U}).any_below(128)));
  EXPECT_FALSE(big4{}.any_below(128));
}
TEST(BigUintAnyBelow, IndexBeyondTheCapacityLooksAtEverything) {
  EXPECT_TRUE((make_limbs<4>({0, 0, 0, 1}).any_below(133)));  // whole == 4, rest != 0
  EXPECT_TRUE((make_limbs<4>({0, 0, 0, 1}).any_below(160)));  // whole == 5, rest == 0
  EXPECT_TRUE((make_limbs<4>({1, 0, 0, 0}).any_below(npos)));
  EXPECT_FALSE(big4{}.any_below(133));
  EXPECT_FALSE(big4{}.any_below(npos));
}
TEST(BigUintAnyBelow, Pattern) {
  // The lowest set bit of the pattern is bit 0.
  big4 const b = from_limbs(pattern4);
  EXPECT_FALSE(b.any_below(0));
  EXPECT_TRUE(b.any_below(1));
  EXPECT_TRUE(b.any_below(100));
  EXPECT_TRUE(b.any_below(128));
}

// propterties for many sizes

template <typename T> class BigUintGeneric : public ::testing::Test {};

using generic_sizes = ::testing::Types<std::integral_constant<std::size_t, 2>, std::integral_constant<std::size_t, 3>,
                                       std::integral_constant<std::size_t, 4>, std::integral_constant<std::size_t, 5>,
                                       std::integral_constant<std::size_t, 8>, std::integral_constant<std::size_t, 32>>;
TYPED_TEST_SUITE(BigUintGeneric, generic_sizes);

// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, EverySingleBitBehavesConsistently) {
  constexpr auto n = TypeParam::value;
  constexpr auto bits = 32U * n;
  for (std::size_t shift = 0; shift < bits; ++shift) {
    big_uint<n> b;
    b.set_limb(shift / 32, std::uint32_t{1} << (shift % 32));
    EXPECT_EQ(b.bit_length(), shift + 1) << shift;
    EXPECT_TRUE(b.bit(shift)) << shift;
    EXPECT_FALSE(b.bit(shift + 1)) << shift;
    EXPECT_FALSE(b.any_below(shift)) << shift;
    EXPECT_TRUE(b.any_below(shift + 1)) << shift;
    EXPECT_EQ(b.fits_u64(), shift < 64) << shift;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, AnyBelowPastTheCapacityInspectsTheWholeValue) {
  constexpr auto n = TypeParam::value;
  constexpr auto bits = 32U * n;
  for (std::size_t const index : {bits, bits + 1, bits + 32, bits + 33, npos}) {
    big_uint<n> const zero;
    EXPECT_FALSE(zero.any_below(index)) << index;
    for (std::size_t const shift : {std::size_t{0}, bits / 2, bits - 1}) {
      big_uint<n> b;
      b.set_limb(shift / 32, std::uint32_t{1} << (shift % 32));
      EXPECT_TRUE(b.any_below(index)) << index << " " << shift;
    }
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, ShiftingOneLeftThenRightRoundTrips) {
  constexpr auto n = TypeParam::value;
  constexpr auto bits = 32U * n;
  for (std::size_t shift = 0; shift < bits; ++shift) {
    big_uint<n> b;
    b.set_limb(0, 1);
    b.shift_left(shift);
    EXPECT_EQ(b.bit_length(), shift + 1U) << shift;
    EXPECT_TRUE(b.bit(shift)) << shift;
    b.shift_right(shift);
    EXPECT_EQ(b.bit_length(), 1U) << shift;
    EXPECT_EQ(b.limb(0), 1U) << shift;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, ShiftingByTheWidthOrMoreClears) {
  constexpr auto n = TypeParam::value;
  for (constexpr auto bits = 32U * n; std::size_t const shift : {bits, bits + 1, bits + 31, bits + 32, bits + 1000}) {
    big_uint<n> left;
    big_uint<n> right;
    for (std::size_t index = 0; index < n; ++index) {
      left.set_limb(index, max32);
      right.set_limb(index, max32);
    }
    left.shift_left(shift);
    right.shift_right(shift);
    EXPECT_EQ(left.bit_length(), 0U) << shift;
    EXPECT_EQ(right.bit_length(), 0U) << shift;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, TopBitShiftedLeftByOneVanishes) {
  constexpr auto n = TypeParam::value;
  big_uint<n> b;
  b.set_limb(n - 1, 0x80000000U);
  b.shift_left(1);
  EXPECT_EQ(b.bit_length(), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, AllOnesPlusOneWrapsToZeroWithCarryOne) {
  constexpr auto n = TypeParam::value;
  big_uint<n> b;
  for (std::size_t index = 0; index < n; ++index) {
    b.set_limb(index, max32);
  }
  EXPECT_EQ(b.mul_add(1, 1), 1U);
  EXPECT_EQ(b.bit_length(), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, RepeatedMultiplicationByFiveIsUndoneByDivision) {
  constexpr auto n = TypeParam::value;
  constexpr std::uint32_t pow5_13 = 1'220'703'125U;  // < 2^31, adds under 31 bits per pass
  big_uint<n> original;
  original.set_limb(0, 0xCAFEBABEU);
  original.set_limb(1, 0x0BADF00DU);

  // The number of multiplications that fit above the 64 bits of the original value (none for two limbs).
  constexpr std::size_t passes = ((32 * n) - 64) / 31;
  big_uint<n> b = original;
  if constexpr (passes > 0) {
    for (std::size_t shift = 0; shift < passes; ++shift) {
      EXPECT_EQ(b.mul_add(pow5_13, 0), 0U) << shift;
    }
    EXPECT_GT(b.bit_length(), original.bit_length());
    for (std::size_t shift = 0; shift < passes; ++shift) {
      EXPECT_EQ(b.div_small(pow5_13), 0U) << shift;
    }
  }
  for (std::size_t shift = 0; shift < n; ++shift) {
    EXPECT_EQ(b.limb(shift), original.limb(shift)) << shift;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, FiveToTheHundredthHas233Bits) {
  using testing::DoubleEq;
  constexpr std::size_t n = TypeParam::value;
  if constexpr (n >= 8) {
    big_uint<n> b;
    b.set_limb(0, 1);
    for (auto ctr = 0; ctr < 100; ++ctr) {
      EXPECT_EQ(b.mul_add(5, 0), 0U);
    }
    auto const log2 = [] (auto x) { return std::log(x) / std::numbers::ln2; };
    EXPECT_THAT(b.bit_length(), static_cast<long>(std::floor(100.0 * log2(5.0))) + 1L);
    for (auto ctr = 0; ctr < 100; ++ctr) {
      EXPECT_EQ(b.div_small(5), 0U);
    }
    EXPECT_EQ(b.bit_length(), 1U);
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, QuotientTimesDivisorPlusRemainderRestoresTheValue) {
  constexpr std::size_t n = TypeParam::value;
  for (std::uint32_t const fill : {0xDEADBEEFU, max32}) {
    for (std::uint32_t const d : {10U, 0x80000000U, max32}) {
      big_uint<n> const original = filled<n>(fill);
      big_uint<n> q = original;
      std::uint32_t const r = q.div_small(d);
      EXPECT_LT(r, d);
      EXPECT_EQ(q.mul_add(d, r), 0U);
      EXPECT_EQ(limbs_of(q), limbs_of(original)) << std::hex << "fill=" << fill << " d=" << d;
    }
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintGeneric, ShiftLeftEqualsRepeatedDoubling) {
  constexpr std::size_t n = TypeParam::value;
  constexpr std::size_t bits = 32U * n;
  for (std::size_t const k : {std::size_t{1}, std::size_t{31}, std::size_t{32}, std::size_t{33}, bits - 1, bits, bits + 1}) {
    big_uint<n> shifted = filled<n>(0x9E3779B9U);
    big_uint<n> doubled = shifted;
    shifted.shift_left(k);
    for (std::size_t ctr = 0; ctr < k; ++ctr) {
      static_cast<void>(doubled.mul_add(2, 0));
    }
    EXPECT_EQ(limbs_of(shifted), limbs_of(doubled)) << "k=" << k;
  }
}

template <std::size_t N>
void QuotientTimesDivisorPlusRemainderRestoresTheValue(uint_fast64_t value1, std::uint32_t value2, std::uint32_t d) {
  if (d == 0) {
    d = 3;
  }
  for (auto round = 0; round < 500; ++round) {
    big_uint<N> original;
    for (std::size_t index = 0; index < N; ++index) {
      original.set_limb(index, value1 % 3 == 0 ? std::uint32_t{0} : value2);
    }
    big_uint<N> q = original;
    std::uint32_t const r = q.div_small(d);
    EXPECT_LT(r, d);
    EXPECT_EQ(q.mul_add(d, r), 0U);
    for (std::size_t index = 0; index < N; ++index) {
      EXPECT_EQ(q.limb(index), original.limb(index)) << index;
    }
  }
}

TEST(QTD, A) {
  QuotientTimesDivisorPlusRemainderRestoresTheValue<32>(0, 0, 0);
}
#if defined(PEEJAY_FUZZTEST) && PEEJAY_FUZZTEST
void QTD64(uint_fast64_t value1, std::uint32_t value2, std::uint32_t d) {
  QuotientTimesDivisorPlusRemainderRestoresTheValue<64>(value1, value2, d);
}
FUZZ_TEST(QTD, QTD64);
void QTD32(uint_fast64_t value1, std::uint32_t value2, std::uint32_t d) {
  QuotientTimesDivisorPlusRemainderRestoresTheValue<32>(value1, value2, d);
}
FUZZ_TEST(QTD, QTD32);
void QTD16(uint_fast64_t value1, std::uint32_t value2, std::uint32_t d) {
  QuotientTimesDivisorPlusRemainderRestoresTheValue<16>(value1, value2, d);
}
FUZZ_TEST(QTD, QTD16);
void QTD8(uint_fast64_t value1, std::uint32_t value2, std::uint32_t d) {
  QuotientTimesDivisorPlusRemainderRestoresTheValue<8>(value1, value2, d);
}
FUZZ_TEST(QTD, QTD8);
#endif // PEEJAY_FUZZTEST

// Limb types

using peejay::details::big_uint_limb;

template <std::size_t N, typename Limb>
concept big_uint_instantiable_with = requires { typename peejay::details::big_uint<N, Limb>; };

static_assert(big_uint_limb<std::uint8_t>);
static_assert(big_uint_limb<std::uint16_t>);
static_assert(big_uint_limb<std::uint32_t>);
static_assert(big_uint_limb<std::uint64_t>);
static_assert(big_uint_limb<unsigned char>);
static_assert(big_uint_limb<unsigned short>);
static_assert(big_uint_limb<unsigned int>);
static_assert(big_uint_limb<unsigned long>);
static_assert(big_uint_limb<unsigned long long>);
static_assert(big_uint_limb<std::uint_fast8_t>);
static_assert(big_uint_limb<std::uint_fast16_t>);
static_assert(big_uint_limb<std::uint_fast32_t>);
static_assert(big_uint_limb<std::uint_fast64_t>);
static_assert(big_uint_limb<std::uint_least8_t>);
static_assert(big_uint_limb<std::uint_least16_t>);
static_assert(big_uint_limb<std::uint_least32_t>);
static_assert(big_uint_limb<std::uint_least64_t>);

static_assert(!big_uint_limb<bool>);
static_assert(!big_uint_limb<char>);
static_assert(!big_uint_limb<char8_t>);
static_assert(!big_uint_limb<char16_t>);
static_assert(!big_uint_limb<char32_t>);
static_assert(!big_uint_limb<wchar_t>);
static_assert(!big_uint_limb<signed char>);
static_assert(!big_uint_limb<std::int8_t>);
static_assert(!big_uint_limb<std::int16_t>);
static_assert(!big_uint_limb<std::int32_t>);
static_assert(!big_uint_limb<std::int64_t>);
static_assert(!big_uint_limb<float>);
static_assert(!big_uint_limb<double>);
#if defined(__SIZEOF_INT128__)
static_assert(!big_uint_limb<u128>);
#endif  // defined(__SIZEOF_INT128__)
static_assert(!big_uint_limb<std::uint32_t&>);
static_assert(!big_uint_limb<std::uint32_t*>);

static_assert(!big_uint_instantiable_with<7, std::uint8_t>);
static_assert(big_uint_instantiable_with<8, std::uint8_t>);
static_assert(big_uint_instantiable_with<9, std::uint8_t>);
static_assert(!big_uint_instantiable_with<3, std::uint16_t>);
static_assert(big_uint_instantiable_with<4, std::uint16_t>);
static_assert(!big_uint_instantiable_with<1, std::uint32_t>);
static_assert(big_uint_instantiable_with<2, std::uint32_t>);
static_assert(!big_uint_instantiable_with<0, std::uint64_t>);
static_assert(big_uint_instantiable_with<1, std::uint64_t>);
static_assert(!big_uint_instantiable_with<4, std::int32_t>);
static_assert(!big_uint_instantiable_with<4, bool>);
static_assert(!big_uint_instantiable_with<64, char>);

static_assert(std::is_same_v<big4::limb_type, std::uint32_t>);
static_assert(std::is_same_v<big4, peejay::details::big_uint<4, std::uint32_t>>);

TEST(BigUintLimbConcept, AcceptsUnsignedIntegersOfTheSupportedWidths) {
  EXPECT_TRUE(big_uint_limb<std::uint8_t>);
  EXPECT_TRUE(big_uint_limb<std::uint64_t>);
}
TEST(BigUintLimbConcept, RejectsOtherTypes) {
  EXPECT_FALSE(big_uint_limb<bool>);
  EXPECT_FALSE(big_uint_limb<std::int32_t>);
}
TEST(BigUintLimbConcept, CapacityMustBeAtLeast64Bits) {
  EXPECT_FALSE((big_uint_instantiable_with<7, std::uint8_t>));
  EXPECT_TRUE((big_uint_instantiable_with<1, std::uint64_t>));
}
TEST(BigUintLimbConcept, DefaultLimbIsUint32) {
  EXPECT_EQ(big4::limb_bits, 32U);
  EXPECT_EQ(big4::capacity_bits, 128U);
}

// Prints a limb as a number (a std::uint8_t would otherwise be shown as a character).
template <typename Limb> std::uint64_t as_u64(Limb const v) {
  return static_cast<std::uint64_t>(v);
}

// Single limb helpers

template <typename T> class BigUintLimbHelpers : public ::testing::Test {
protected:
  static constexpr T max = std::numeric_limits<T>::max();
  /// A limb with only its most significant bit set.
  static constexpr auto top = static_cast<T>(T{1} << (std::numeric_limits<T>::digits - 1));
};

using limb_types = ::testing::Types<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, unsigned long,
                                    std::uint_fast16_t, std::uint_fast32_t>;
TYPED_TEST_SUITE(BigUintLimbHelpers, limb_types);

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbHelpers, MulAddLimbCommonCases) {
  using limb = TypeParam;
  constexpr auto max = TestFixture::max;
  constexpr auto top = TestFixture::top;
  using peejay::details::mul_add_limb;
  auto r = mul_add_limb<limb>(3, 5, 7);
  EXPECT_EQ(as_u64(r.low), 22U);
  EXPECT_EQ(as_u64(r.high), 0U);
  // (2^w-1) * 2 + 1 == 2^(w+1) - 1
  r = mul_add_limb<limb>(max, 2, 1);
  EXPECT_EQ(as_u64(r.low), as_u64(max));
  EXPECT_EQ(as_u64(r.high), 1U);
  // 2^(w-1) * 2 == 2^w
  r = mul_add_limb<limb>(top, 2, 0);
  EXPECT_EQ(as_u64(r.low), 0U);
  EXPECT_EQ(as_u64(r.high), 1U);
  // 2^(w-1) * 2^(w-1) == 2^(2w-2)
  r = mul_add_limb<limb>(top, top, 0);
  EXPECT_EQ(as_u64(r.low), 0U);
  EXPECT_EQ(as_u64(r.high), as_u64(top >> 1U));
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbHelpers, MulAddLimbEdgeCases) {
  using limb = TypeParam;
  constexpr auto max = TestFixture::max;
  using peejay::details::mul_add_limb;
  // (2^w-1) * (2^w-1) + (2^w-1) == 2^2w - 2^w: the largest possible result.
  auto r = mul_add_limb<limb>(max, max, max);
  EXPECT_EQ(as_u64(r.low), 0U);
  EXPECT_EQ(as_u64(r.high), as_u64(max));
  // (2^w-1) * (2^w-1) == 2^2w - 2^(w+1) + 1
  r = mul_add_limb<limb>(max, max, 0);
  EXPECT_EQ(as_u64(r.low), 1U);
  EXPECT_EQ(as_u64(r.high), as_u64(static_cast<limb>(max - 1U)));
  // Adding the carry wraps the low half into the high half.
  r = mul_add_limb<limb>(max, 1, 1);
  EXPECT_EQ(as_u64(r.low), 0U);
  EXPECT_EQ(as_u64(r.high), 1U);
  r = mul_add_limb<limb>(0, max, max);
  EXPECT_EQ(as_u64(r.low), as_u64(max));
  EXPECT_EQ(as_u64(r.high), 0U);
  r = mul_add_limb<limb>(0, 0, 0);
  EXPECT_EQ(as_u64(r.low), 0U);
  EXPECT_EQ(as_u64(r.high), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbHelpers, DivLimbCommonCases) {
  using limb = TypeParam;
  constexpr auto max = TestFixture::max;
  using peejay::details::div_limb;
  auto r = div_limb<limb>(0, 22, 5);
  EXPECT_EQ(as_u64(r.quotient), 4U);
  EXPECT_EQ(as_u64(r.remainder), 2U);
  // 2^w / 3: 2^w - 1 is divisible by 3 for every even w.
  r = div_limb<limb>(1, 0, 3);
  EXPECT_EQ(as_u64(r.quotient), as_u64(static_cast<limb>(max / 3U)));
  EXPECT_EQ(as_u64(r.remainder), 1U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbHelpers, DivLimbEdgeCases) {
  using limb = TypeParam;
  constexpr auto max = TestFixture::max;
  constexpr auto top = TestFixture::top;
  using peejay::details::div_limb;
  // The largest dividend that the precondition allows: (2^w-2):(2^w-1) / (2^w-1).
  auto r = div_limb<limb>(static_cast<limb>(max - 1U), max, max);
  EXPECT_EQ(as_u64(r.quotient), as_u64(max));
  EXPECT_EQ(as_u64(r.remainder), as_u64(static_cast<limb>(max - 1U)));
  // Dividing by one gives back the low limb.
  r = div_limb<limb>(0, max, 1);
  EXPECT_EQ(as_u64(r.quotient), as_u64(max));
  EXPECT_EQ(as_u64(r.remainder), 0U);
  // A divisor with only its top bit set (already normalised in the portable algorithm).
  r = div_limb<limb>(static_cast<limb>(top - 1U), max, top);
  EXPECT_EQ(as_u64(r.quotient), as_u64(max));
  EXPECT_EQ(as_u64(r.remainder), as_u64(static_cast<limb>(top - 1U)));
  // A small divisor that needs the largest normalising shift.
  r = div_limb<limb>(0, 7, 2);
  EXPECT_EQ(as_u64(r.quotient), 3U);
  EXPECT_EQ(as_u64(r.remainder), 1U);
  r = div_limb<limb>(1, 0, 2);  // 2^w / 2
  EXPECT_EQ(as_u64(r.quotient), as_u64(top));
  EXPECT_EQ(as_u64(r.remainder), 0U);
  r = div_limb<limb>(0, 0, max);
  EXPECT_EQ(as_u64(r.quotient), 0U);
  EXPECT_EQ(as_u64(r.remainder), 0U);
}

// Dividends that exercise each of the quotient digit corrections in the portable (no __int128)
// implementation of the 128 / 64 division. "q1" and "q0" are the high and low 32-bit quotient
// digits; an estimate is corrected because it is at least 2^32 ("too big") or because the product
// test fails ("product"), and the correction loop ends early when rhat reaches 2^32 ("rhat").
TEST(BigUintLimbHelpers64, DivLimbQuotientDigitCorrections) {
  struct test_case {
    std::uint64_t high;
    std::uint64_t low;
    std::uint64_t d;
    std::uint64_t quotient;
    std::uint64_t remainder;
  };
  // clang-format off
  std::array<test_case, 9> const cases{{
    // no corrections
    {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000000ULL, 0x0000000000000000ULL},
    // q1: product
    {0x800000007FFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0x8000000080000000ULL, 0x000000007FFFFFFFULL},
    // q0: product
    {0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0x7FFFFFFFFFFFFFFFULL, 0x0000000000000001ULL, 0x7FFFFFFF00000001ULL},
    // q1: product; q0: too big
    {0x00000000FFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x00000000FFFFFFFFULL, 0x00000000FFFFFFFFULL},
    // q1: product, rhat; q0: too big, rhat
    {0x00000000FFFFFFFFULL, 0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL, 0x00000001FFFFFFFFULL, 0x00000001FFFFFFFFULL},
    // q1: too big; q0: product, rhat
    {0x7FFFFFFF80000000ULL, 0x0000000080000001ULL, 0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000001ULL, 0x7FFFFFFF80000002ULL},
    // q1: too big, too big, rhat; q0: product
    {0x8000000080000000ULL, 0x0000000100000000ULL, 0x80000000FFFFFFFFULL, 0xFFFFFFFF00000003ULL, 0x7FFFFFFD00000003ULL},
    // q1: product; q0: product, product, rhat
    {0x0000000100000000ULL, 0x0000000000000000ULL, 0x80000000FFFFFFFFULL, 0x00000001FFFFFFFCULL, 0x00000005FFFFFFFCULL},
    // q1: too big, product, rhat; q0: too big, too big, rhat
    {0xFFFFFFFE00000000ULL, 0x0000000000000000ULL, 0xFFFFFFFEFFFFFFFFULL, 0xFFFFFFFEFFFFFFFFULL, 0xFFFFFFFDFFFFFFFFULL},
  }};
  // clang-format on
  for (auto const& [high, low, d, quotient, remainder] : cases) {
    auto const r = peejay::details::div_limb<std::uint64_t>(high, low, d);
    EXPECT_EQ(r.quotient, quotient) << std::hex << high << ':' << low << '/' << d;
    EXPECT_EQ(r.remainder, remainder) << std::hex << high << ':' << low << '/' << d;
  }
}

// --------------------------------------------------- whole-value behaviour ---

/// Describes one big_uint instantiation for the typed tests below.
template <std::size_t LimbCount, typename Limb> struct limb_config {
  using limb_type = Limb;
  using type = peejay::details::big_uint<LimbCount, Limb>;
  static constexpr std::size_t limb_count = LimbCount;
  static constexpr std::size_t limb_bits = std::numeric_limits<Limb>::digits;
  static constexpr std::size_t capacity = LimbCount * limb_bits;
};
/// A configuration of `Bits` bits whatever the width of Limb (for types such as std::uint_fast16_t).
template <std::size_t Bits, typename Limb>
using limb_config_bits = limb_config<Bits / std::numeric_limits<Limb>::digits, Limb>;

template <typename T> class BigUintLimbType : public ::testing::Test {
protected:
  using limb = typename T::limb_type;
  using big = typename T::type;
  static constexpr std::size_t n = T::limb_count;
  static constexpr std::size_t limb_bits = T::limb_bits;
  static constexpr std::size_t capacity = T::capacity;
  static constexpr limb max = std::numeric_limits<limb>::max();
  /// A limb with only its most significant bit set.
  static constexpr auto top = static_cast<limb>(limb{1} << (limb_bits - 1U));

  /// Returns a big_uint with the value \p v.
  static big make(std::uint64_t const v) {
    big b;
    for (std::size_t index = 0; index < n && index * limb_bits < 64U; ++index) {
      b.set_limb(index, static_cast<limb>(v >> (index * limb_bits)));
    }
    return b;
  }
  /// Returns a big_uint with only bit \p index set.
  static big single_bit(std::size_t const index) {
    big b;
    b.set_limb(index / limb_bits, static_cast<limb>(limb{1} << (index % limb_bits)));
    return b;
  }
  /// Returns a big_uint with every bit set.
  static big all_ones() {
    big b;
    for (std::size_t index = 0; index < n; ++index) {
      b.set_limb(index, max);
    }
    return b;
  }
};

// Each capacity is between 64 and 128 bits.
using limb_configs = ::testing::Types<
    limb_config<8, std::uint8_t>, limb_config<9, std::uint8_t>, limb_config<16, std::uint8_t>,
    limb_config<4, std::uint16_t>, limb_config<5, std::uint16_t>, limb_config<8, std::uint16_t>,
    limb_config<2, std::uint32_t>, limb_config<3, std::uint32_t>, limb_config<4, std::uint32_t>,
    limb_config<1, std::uint64_t>, limb_config<2, std::uint64_t>, limb_config<2, unsigned long>,
    limb_config_bits<64, std::uint_fast16_t>, limb_config_bits<128, std::uint_fast32_t>,
    limb_config_bits<64, std::uint_fast64_t>, limb_config_bits<128, std::uint_least8_t>>;
TYPED_TEST_SUITE(BigUintLimbType, limb_configs);

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, TypesAndConstantsReflectTheTemplateArguments) {
  using big = typename TestFixture::big;
  static_assert(std::is_same_v<typename big::limb_type, typename TypeParam::limb_type>);
  EXPECT_EQ(big::limb_bits, TypeParam::limb_bits);
  EXPECT_EQ(big::capacity_bits, TypeParam::capacity);
  EXPECT_EQ(big::capacity_bits, TypeParam::limb_count * big::limb_bits);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, StartsAtZero) {
  typename TestFixture::big const b;
  for (std::size_t index = 0; index < TestFixture::n; ++index) {
    EXPECT_EQ(as_u64(b.limb(index)), 0U) << index;
  }
  EXPECT_EQ(b.bit_length(), 0U);
  EXPECT_EQ(b.low64(), 0U);
  EXPECT_TRUE(b.fits_u64());
  EXPECT_FALSE(b.any_below(TestFixture::capacity));
  EXPECT_FALSE(b.any_below(TestFixture::capacity + TestFixture::limb_bits + 1U));
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, SetLimbOverwritesOnlyThatLimb) {
  using limb = typename TestFixture::limb;
  constexpr auto n = TestFixture::n;
  typename TestFixture::big b;
  for (std::size_t index = 0; index < n; ++index) {
    b.set_limb(index, static_cast<limb>(index + 1U));
  }
  b.set_limb(n - 1U, TestFixture::max);
  for (std::size_t index = 0; index < n; ++index) {
    EXPECT_EQ(as_u64(b.limb(index)), index == n - 1U ? as_u64(TestFixture::max) : index + 1U) << index;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, AllOnes) {
  auto const b = TestFixture::all_ones();
  EXPECT_EQ(b.bit_length(), TestFixture::capacity);
  EXPECT_EQ(b.low64(), std::numeric_limits<std::uint64_t>::max());
  EXPECT_EQ(b.fits_u64(), TestFixture::capacity == 64U);
  EXPECT_TRUE(b.bit(TestFixture::capacity - 1U));
  EXPECT_FALSE(b.bit(TestFixture::capacity));
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, EverySingleBitBehavesConsistently) {
  for (std::size_t shift = 0; shift < TestFixture::capacity; ++shift) {
    auto const b = TestFixture::single_bit(shift);
    EXPECT_EQ(b.bit_length(), shift + 1) << shift;
    EXPECT_TRUE(b.bit(shift)) << shift;
    EXPECT_FALSE(b.bit(shift + 1)) << shift;
    EXPECT_FALSE(b.any_below(shift)) << shift;
    EXPECT_TRUE(b.any_below(shift + 1)) << shift;
    EXPECT_EQ(b.fits_u64(), shift < 64) << shift;
  }
}

// mul_add

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, MulAddCarriesThroughEveryLimbOfTheLow64Bits) {
  auto b = TestFixture::make(0x0FFFFFFFFFFFFFFFULL);
  EXPECT_EQ(as_u64(b.mul_add(10, 7)), 0U);
  EXPECT_EQ(b.low64(), 0x9FFFFFFFFFFFFFFDULL);
  EXPECT_TRUE(b.fits_u64());
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, MulAddByTheLargestByte) {
  auto b = TestFixture::make(0x00FFFFFFFFFFFFFFULL);
  EXPECT_EQ(as_u64(b.mul_add(255, 255)), 0U);
  EXPECT_EQ(b.low64(), 0xFF00000000000000ULL);
  EXPECT_TRUE(b.fits_u64());
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, AllOnesPlusOneWrapsToZeroWithCarryOne) {
  auto b = TestFixture::all_ones();
  EXPECT_EQ(as_u64(b.mul_add(1, 1)), 1U);
  EXPECT_EQ(b.bit_length(), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, LargestCarryOutFitsInALimb) {
  // (2^cap-1) * (2^w-1) + (2^w-1) == (2^w-1) * 2^cap
  constexpr auto max = TestFixture::max;
  auto b = TestFixture::all_ones();
  EXPECT_EQ(as_u64(b.mul_add(max, max)), as_u64(max));
  EXPECT_EQ(b.bit_length(), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, BuildsADecimalNumberDigitByDigit) {
  using limb = typename TestFixture::limb;
  // The largest power of ten below 2^64, so that it fits in every capacity being tested.
  constexpr std::uint64_t expected = 10'000'000'000'000'000'000ULL;
  typename TestFixture::big b;
  EXPECT_EQ(as_u64(b.mul_add(1, 1)), 0U);
  for (int digit = 0; digit < 19; ++digit) {
    EXPECT_EQ(as_u64(b.mul_add(limb{10}, limb{0})), 0U) << digit;
  }
  EXPECT_TRUE(b.fits_u64());
  EXPECT_EQ(b.low64(), expected);
  for (int digit = 0; digit < 19; ++digit) {
    EXPECT_EQ(as_u64(b.div_small(limb{10})), 0U) << digit;
  }
  EXPECT_EQ(b.low64(), 1U);
}

// div_small

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, DivSmallUndoesMulAdd) {
  auto b = TestFixture::make(0x9FFFFFFFFFFFFFFDULL);
  EXPECT_EQ(as_u64(b.div_small(10)), 7U);
  EXPECT_EQ(b.low64(), 0x0FFFFFFFFFFFFFFFULL);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, DivSmallCarriesTheRemainderThroughEveryLimb) {
  auto b = TestFixture::make(0x8000000000000000ULL);
  EXPECT_EQ(as_u64(b.div_small(3)), 2U);
  EXPECT_EQ(b.low64(), 0x2AAAAAAAAAAAAAAAULL);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, DivSmallByTheLargestByte) {
  auto b = TestFixture::make(0xFF00000000000000ULL);
  EXPECT_EQ(as_u64(b.div_small(255)), 0U);
  EXPECT_EQ(b.low64(), 0x0100000000000000ULL);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, DivSmallByTheTopBitOfALimb) {
  auto b = TestFixture::make(0x8000000000000000ULL);
  EXPECT_EQ(as_u64(b.div_small(TestFixture::top)), 0U);
  EXPECT_EQ(b.low64(), std::uint64_t{1} << (64U - TestFixture::limb_bits));
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, LargestDivisorDividesAllOnes) {
  // 2^cap - 1 is divisible by 2^w - 1, giving a quotient whose limbs are all 1.
  auto b = TestFixture::all_ones();
  EXPECT_EQ(as_u64(b.div_small(TestFixture::max)), 0U);
  for (std::size_t index = 0; index < TestFixture::n; ++index) {
    EXPECT_EQ(as_u64(b.limb(index)), 1U) << index;
  }
}

// shifts

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, ShiftLeftWithinTheLow64Bits) {
  // Shifts that land in the middle of a limb and either side of each 8, 16 and 32 bit boundary.
  struct test_case {
    std::size_t bits;
    std::uint64_t expected;
  };
  // clang-format off
  std::array<test_case, 13> const cases{{
    {0, 0x000000000000008BULL}, {1, 0x0000000000000116ULL}, {7, 0x0000000000004580ULL},
    {8, 0x0000000000008B00ULL}, {9, 0x0000000000011600ULL}, {15, 0x0000000000458000ULL},
    {16, 0x00000000008B0000ULL}, {17, 0x0000000001160000ULL}, {31, 0x0000004580000000ULL},
    {32, 0x0000008B00000000ULL}, {33, 0x0000011600000000ULL}, {55, 0x4580000000000000ULL},
    {56, 0x8B00000000000000ULL},
  }};
  // clang-format on
  for (auto const& [bits, expected] : cases) {
    auto b = TestFixture::make(0x8BU);
    b.shift_left(bits);
    EXPECT_EQ(b.low64(), expected) << "bits=" << bits;
    EXPECT_TRUE(b.fits_u64()) << "bits=" << bits;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, ShiftRightWithinTheLow64Bits) {
  struct test_case {
    std::size_t bits;
    std::uint64_t expected;
  };
  // clang-format off
  std::array<test_case, 15> const cases{{
    {0, 0xFEDCBA9876543210ULL}, {1, 0x7F6E5D4C3B2A1908ULL}, {4, 0x0FEDCBA987654321ULL},
    {7, 0x01FDB97530ECA864ULL}, {8, 0x00FEDCBA98765432ULL}, {9, 0x007F6E5D4C3B2A19ULL},
    {15, 0x0001FDB97530ECA8ULL}, {16, 0x0000FEDCBA987654ULL}, {17, 0x00007F6E5D4C3B2AULL},
    {31, 0x00000001FDB97530ULL}, {32, 0x00000000FEDCBA98ULL}, {33, 0x000000007F6E5D4CULL},
    {60, 0x000000000000000FULL}, {63, 0x0000000000000001ULL}, {64, 0x0000000000000000ULL},
  }};
  // clang-format on
  for (auto const& [bits, expected] : cases) {
    auto b = TestFixture::make(0xFEDCBA9876543210ULL);
    b.shift_right(bits);
    EXPECT_EQ(b.low64(), expected) << "bits=" << bits;
  }
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, TopBitShiftedLeftByOneVanishes) {
  auto b = TestFixture::single_bit(TestFixture::capacity - 1U);
  b.shift_left(1);
  EXPECT_EQ(b.bit_length(), 0U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, TopBitShiftedRightToTheBottom) {
  auto b = TestFixture::single_bit(TestFixture::capacity - 1U);
  b.shift_right(TestFixture::capacity - 1U);
  EXPECT_EQ(b.bit_length(), 1U);
  EXPECT_EQ(b.low64(), 1U);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, ShiftingByTheWidthOrMoreClears) {
  constexpr auto capacity = TestFixture::capacity;
  constexpr auto limb_bits = TestFixture::limb_bits;
  for (std::size_t const shift : {capacity, capacity + 1, capacity + limb_bits - 1, capacity + limb_bits,
                                  capacity + 1000}) {
    auto left = TestFixture::all_ones();
    auto right = TestFixture::all_ones();
    left.shift_left(shift);
    right.shift_right(shift);
    EXPECT_EQ(left.bit_length(), 0U) << shift;
    EXPECT_EQ(right.bit_length(), 0U) << shift;
  }
}

// bit queries

// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, BitQueriesOnAValueWithThreeBitsSet) {
  auto const b = TestFixture::make(0x0000010000000201ULL);  // bits 0, 9 and 40
  EXPECT_EQ(b.bit_length(), 41U);
  EXPECT_TRUE(b.bit(0));
  EXPECT_FALSE(b.bit(1));
  EXPECT_TRUE(b.bit(9));
  EXPECT_FALSE(b.bit(10));
  EXPECT_TRUE(b.bit(40));
  EXPECT_FALSE(b.bit(41));
  EXPECT_FALSE(b.bit(TestFixture::capacity));
  EXPECT_TRUE(b.fits_u64());
  EXPECT_EQ(b.low64(), 0x0000010000000201ULL);
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, AnyBelowOnAValueWithTwoBitsSet) {
  auto const b = TestFixture::make(0x0000010000000200ULL);  // bits 9 and 40
  EXPECT_FALSE(b.any_below(0));
  EXPECT_FALSE(b.any_below(9));
  EXPECT_TRUE(b.any_below(10));
  EXPECT_TRUE(b.any_below(40));
  EXPECT_TRUE(b.any_below(TestFixture::capacity));
  EXPECT_TRUE(b.any_below(TestFixture::capacity + TestFixture::limb_bits + 1U));
}
// NOLINTNEXTLINE
TYPED_TEST(BigUintLimbType, AnyBelowOnAValueWithOneBitSet) {
  auto const b = TestFixture::make(0x0000010000000000ULL);  // bit 40
  EXPECT_FALSE(b.any_below(40));
  EXPECT_TRUE(b.any_below(41));
}

}  // anonymous namespace
