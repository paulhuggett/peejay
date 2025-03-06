//===- unittests/peejay/test_coord.cpp ------------------------------------===//
//*                          _  *
//*   ___ ___   ___  _ __ __| | *
//*  / __/ _ \ / _ \| '__/ _` | *
//* | (_| (_) | (_) | | | (_| | *
//*  \___\___/ \___/|_|  \__,_| *
//*                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "peejay/json/json.hpp"

// 3rd party includes
#include <gtest/gtest.h>

using peejay::coord;

// NOLINTNEXTLINE
TEST(Coord, DefaultCtor) {
  coord c;
  EXPECT_EQ(c.get_line(), 1U);
  EXPECT_EQ(c.get_column(), 1U);
}

// NOLINTNEXTLINE
TEST(Coord, RowThenColumnInit) {
  coord c{coord::line{2U}, coord::column{3U}};
  EXPECT_EQ(c.get_line(), 2U);
  EXPECT_EQ(c.get_column(), 3U);
}

// NOLINTNEXTLINE
TEST(Coord, ColumnThenRowInit) {
  coord c{coord::column{2U}, coord::line{3U}};
  EXPECT_EQ(c.get_line(), 3U);
  EXPECT_EQ(c.get_column(), 2U);
}

// NOLINTNEXTLINE
TEST(Coord, Eq) {
  coord lhs{coord::column{2U}, coord::line{3U}};
  coord rhs{coord::line{3U}, coord::column{2U}};
  EXPECT_TRUE(lhs == rhs);
  EXPECT_FALSE(lhs != rhs);
}

// NOLINTNEXTLINE
TEST(Coord, Neq) {
  coord lhs{coord::column{2U}, coord::line{3U}};
  coord rhs{coord::line{5U}, coord::column{7U}};
  EXPECT_TRUE(lhs != rhs);
  EXPECT_FALSE(lhs == rhs);
}

// NOLINTNEXTLINE
TEST(Coord, Lt) {
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{1U}} < coord{coord::line{4U}, coord::column{1U}}));
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{2U}} < coord{coord::line{4U}, coord::column{1U}}));
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{1U}} < coord{coord::line{3U}, coord::column{2U}}));
  EXPECT_FALSE((coord{coord::line{4U}, coord::column{1U}} < coord{coord::line{3U}, coord::column{2U}}));
  EXPECT_FALSE(coord{} < coord{});
}

// NOLINTNEXTLINE
TEST(Coord, LtEq) {
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{1U}} <= coord{coord::line{4U}, coord::column{1U}}));
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{2U}} <= coord{coord::line{4U}, coord::column{1U}}));
  EXPECT_TRUE((coord{coord::line{3U}, coord::column{1U}} <= coord{coord::line{3U}, coord::column{2U}}));
  EXPECT_FALSE((coord{coord::line{4U}, coord::column{1U}} <= coord{coord::line{3U}, coord::column{2U}}));
  EXPECT_TRUE(coord{} <= coord{});
}
