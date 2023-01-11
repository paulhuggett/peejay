//===- unittests/test_coord.cpp -------------------------------------------===//
//*                          _  *
//*   ___ ___   ___  _ __ __| | *
//*  / __/ _ \ / _ \| '__/ _` | *
//* | (_| (_) | (_) | | | (_| | *
//*  \___\___/ \___/|_|  \__,_| *
//*                             *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "peejay/json.hpp"

// 3rd party includes
#include <gtest/gtest.h>

using peejay::column;
using peejay::coord;
using peejay::line;

// NOLINTNEXTLINE
TEST (Coord, DefaultCtor) {
  coord c;
  EXPECT_EQ (c.line, 1U);
  EXPECT_EQ (c.column, 1U);
}

// NOLINTNEXTLINE
TEST (Coord, RowThenColumnInit) {
  coord c{line{2U}, column{3U}};
  EXPECT_EQ (c.line, 2U);
  EXPECT_EQ (c.column, 3U);
}

// NOLINTNEXTLINE
TEST (Coord, ColumnThenRowInit) {
  coord c{column{2U}, line{3U}};
  EXPECT_EQ (c.line, 3U);
  EXPECT_EQ (c.column, 2U);
}

// NOLINTNEXTLINE
TEST (Coord, Eq) {
  coord lhs{column{2U}, line{3U}};
  coord rhs{line{3U}, column{2U}};
  EXPECT_TRUE (lhs == rhs);
  EXPECT_FALSE (lhs != rhs);
}

// NOLINTNEXTLINE
TEST (Coord, Neq) {
  coord lhs{column{2U}, line{3U}};
  coord rhs{line{5U}, column{7U}};
  EXPECT_TRUE (lhs != rhs);
  EXPECT_FALSE (lhs == rhs);
}

// NOLINTNEXTLINE
TEST (Coord, Lt) {
  EXPECT_TRUE ((coord{line{3U}, column{1U}} < coord{line{4U}, column{1U}}));
  EXPECT_TRUE ((coord{line{3U}, column{2U}} < coord{line{4U}, column{1U}}));
  EXPECT_TRUE ((coord{line{3U}, column{1U}} < coord{line{3U}, column{2U}}));
  EXPECT_FALSE ((coord{line{4U}, column{1U}} < coord{line{3U}, column{2U}}));
  EXPECT_FALSE (coord{} < coord{});
}

// NOLINTNEXTLINE
TEST (Coord, LtEq) {
  EXPECT_TRUE ((coord{line{3U}, column{1U}} <= coord{line{4U}, column{1U}}));
  EXPECT_TRUE ((coord{line{3U}, column{2U}} <= coord{line{4U}, column{1U}}));
  EXPECT_TRUE ((coord{line{3U}, column{1U}} <= coord{line{3U}, column{2U}}));
  EXPECT_FALSE ((coord{line{4U}, column{1U}} <= coord{line{3U}, column{2U}}));
  EXPECT_TRUE (coord{} <= coord{});
}
