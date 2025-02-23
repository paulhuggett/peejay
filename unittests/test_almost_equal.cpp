//===- unittests/test_almost_equal.cpp ------------------------------------===//
//*        _                     _                            _  *
//*   __ _| |_ __ ___   ___  ___| |_    ___  __ _ _   _  __ _| | *
//*  / _` | | '_ ` _ \ / _ \/ __| __|  / _ \/ _` | | | |/ _` | | *
//* | (_| | | | | | | | (_) \__ \ |_  |  __/ (_| | |_| | (_| | | *
//*  \__,_|_|_| |_| |_|\___/|___/\__|  \___|\__, |\__,_|\__,_|_| *
//*                                            |_|               *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "peejay/schema/almost_equal.hpp"

// NOLINTNEXTLINE
TEST (AlmostEqual, TwoIdentical) {
  EXPECT_TRUE (peejay::almost_equal (3.14, 3.14));
}

// NOLINTNEXTLINE
TEST (AlmostEqual, Infinities) {
  if constexpr (std::numeric_limits<float>::has_infinity) {
    EXPECT_TRUE (
        peejay::almost_equal (std::numeric_limits<float>::infinity (),
                              std::numeric_limits<float>::infinity ()));
    EXPECT_FALSE (peejay::almost_equal (std::numeric_limits<float>::infinity (),
                                        std::numeric_limits<float>::max ()));
    EXPECT_FALSE (peejay::almost_equal (std::numeric_limits<float>::infinity (),
                                        std::numeric_limits<float>::min ()));

    EXPECT_TRUE (
        peejay::almost_equal (-std::numeric_limits<float>::infinity (),
                              -std::numeric_limits<float>::infinity ()));
    EXPECT_FALSE (
        peejay::almost_equal (-std::numeric_limits<float>::infinity (),
                              std::numeric_limits<float>::max ()));
    EXPECT_FALSE (
        peejay::almost_equal (-std::numeric_limits<float>::infinity (),
                              std::numeric_limits<float>::min ()));
  }
  if constexpr (std::numeric_limits<double>::has_infinity) {
    EXPECT_TRUE (
        peejay::almost_equal (std::numeric_limits<double>::infinity (),
                              std::numeric_limits<double>::infinity ()));
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<double>::infinity (),
                              std::numeric_limits<double>::max ()));
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<double>::infinity (),
                              std::numeric_limits<double>::min ()));

    EXPECT_TRUE (
        peejay::almost_equal (-std::numeric_limits<double>::infinity (),
                              -std::numeric_limits<double>::infinity ()));
    EXPECT_FALSE (
        peejay::almost_equal (-std::numeric_limits<double>::infinity (),
                              std::numeric_limits<double>::max ()));
    EXPECT_FALSE (
        peejay::almost_equal (-std::numeric_limits<double>::infinity (),
                              std::numeric_limits<double>::min ()));
  }
}

// NOLINTNEXTLINE
TEST (AlmostEqual, NaNs) {
  if constexpr (std::numeric_limits<float>::has_quiet_NaN) {
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<float>::quiet_NaN (),
                              std::numeric_limits<float>::quiet_NaN ()));
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<float>::quiet_NaN (), 0.0F));
    EXPECT_FALSE (
        peejay::almost_equal (0.0F, std::numeric_limits<float>::quiet_NaN ()));
  }
  if constexpr (std::numeric_limits<double>::has_quiet_NaN) {
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<double>::quiet_NaN (),
                              std::numeric_limits<double>::quiet_NaN ()));
    EXPECT_FALSE (
        peejay::almost_equal (std::numeric_limits<double>::quiet_NaN (), 0.0));
    EXPECT_FALSE (
        peejay::almost_equal (0.0, std::numeric_limits<double>::quiet_NaN ()));
  }
}

// NOLINTNEXTLINE
TEST (AlmostEqual, TwoNearZero) {
  EXPECT_FALSE (peejay::almost_equal (
      1.0F / static_cast<float> (std::numeric_limits<std::uint64_t>::max ()),
      0.0F));
}
