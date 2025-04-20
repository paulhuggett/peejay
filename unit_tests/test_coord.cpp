//===- unit_tests/test_coord.cpp ------------------------------------------===//
//*                          _  *
//*   ___ ___   ___  _ __ __| | *
//*  / __/ _ \ / _ \| '__/ _` | *
//* | (_| (_) | (_) | | | (_| | *
//*  \___\___/ \___/|_|  \__,_| *
//*                             *
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
#include "peejay/json.hpp"

// 3rd party includes
#include <gtest/gtest.h>

using coord = peejay::coord<true>;

// NOLINTNEXTLINE
TEST(Coord, DefaultCtor) {
  coord c;
  EXPECT_EQ(c.line, 1U);
  EXPECT_EQ(c.column, 1U);
}

// NOLINTNEXTLINE
TEST(Coord, RowThenColumnInit) {
  coord c{.line = 2U, .column = 3U};
  EXPECT_EQ(c.line, 2U);
  EXPECT_EQ(c.column, 3U);
}

// NOLINTNEXTLINE
TEST(Coord, ColumnThenRowInit) {
  coord c{.line = 3U, .column = 2U};
  EXPECT_EQ(c.line, 3U);
  EXPECT_EQ(c.column, 2U);
}

// NOLINTNEXTLINE
TEST(Coord, Eq) {
  coord lhs{.line = 3U, .column = 2U};
  coord rhs{.line = 3U, .column = 2U};
  EXPECT_TRUE(lhs == rhs);
  EXPECT_FALSE(lhs != rhs);
}

// NOLINTNEXTLINE
TEST(Coord, Neq) {
  coord lhs{.line = 3U, .column = 2U};
  coord rhs{.line = 5U, .column = 7U};
  EXPECT_TRUE(lhs != rhs);
  EXPECT_FALSE(lhs == rhs);
}

// NOLINTNEXTLINE
TEST(Coord, Lt) {
  EXPECT_TRUE((coord{.line = 3U, .column = 1U} < coord{.line = 4U, .column = 1U}));
  EXPECT_TRUE((coord{.line = 3U, .column = 2U} < coord{.line = 4U, .column = 1U}));
  EXPECT_TRUE((coord{.line = 3U, .column = 1U} < coord{.line = 3U, .column = 2U}));
  EXPECT_FALSE((coord{.line = 4U, .column = 1U} < coord{.line = 3U, .column = 2U}));
  EXPECT_FALSE(coord{} < coord{});
}

// NOLINTNEXTLINE
TEST(Coord, LtEq) {
  EXPECT_TRUE((coord{.line = 3U, .column = 1U} <= coord{.line = 4U, .column = 1U}));
  EXPECT_TRUE((coord{.line = 3U, .column = 2U} <= coord{.line = 4U, .column = 1U}));
  EXPECT_TRUE((coord{.line = 3U, .column = 1U} <= coord{.line = 3U, .column = 2U}));
  EXPECT_FALSE((coord{.line = 4U, .column = 1U} <= coord{.line = 3U, .column = 2U}));
  EXPECT_TRUE(coord{} <= coord{});
}
