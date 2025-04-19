//===- unit_tests/test_pointer_based_iterator.cpp -------------------------===//
//*              _       _              _                        _  *
//*  _ __   ___ (_)_ __ | |_ ___ _ __  | |__   __ _ ___  ___  __| | *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__| | '_ \ / _` / __|/ _ \/ _` | *
//* | |_) | (_) | | | | | ||  __/ |    | |_) | (_| \__ \  __/ (_| | *
//* | .__/ \___/|_|_| |_|\__\___|_|    |_.__/ \__,_|___/\___|\__,_| *
//* |_|                                                             *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
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
#include "peejay/details/pointer_based_iterator.hpp"
// Standard library
#include <array>

// 3rd party
#include <gtest/gtest.h>

namespace {

class PointerBasedIterator : public testing::Test {
protected:
  using iterator = peejay::pointer_based_iterator<int>;
  using const_iterator = peejay::pointer_based_iterator<int const>;
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, PreIncrement) {
  std::array<int, 2> arr{{1, 3}};
  int y = 1;
  int* x = &y;
  iterator it{x};
  int* a = &arr[0];
  iterator i{a};
  EXPECT_EQ(*i, 1);
  EXPECT_EQ(++i, iterator{&arr[1]});
  EXPECT_EQ(*i, 3);
  EXPECT_EQ(++i, iterator{&arr[0] + 2});
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, PostIncrement) {
  std::array<int, 2> arr{{1, 3}};
  iterator i{&arr[0]};
  EXPECT_EQ(*i, 1);
  EXPECT_EQ(i++, iterator{&arr[0]});
  EXPECT_EQ(*i, 3);
  EXPECT_EQ(i++, iterator{&arr[1]});
  EXPECT_EQ(i, iterator{&arr[0] + 2});
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, PreDecrement) {
  std::array<int, 3> arr{{1, 3, 5}};
  iterator i{&arr[0] + 3};
  EXPECT_EQ(--i, iterator{&arr[2]});
  EXPECT_EQ(*i, 5);
  EXPECT_EQ(--i, iterator{&arr[1]});
  EXPECT_EQ(*i, 3);
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, PostDecrement) {
  std::array<int, 3> arr{{1, 3, 5}};
  iterator i{&arr[0] + 3};
  EXPECT_EQ(i--, iterator{&arr[0] + 3});
  EXPECT_EQ(*i, 5);
  EXPECT_EQ(i--, iterator{&arr[2]});
  EXPECT_EQ(*i, 3);
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, IPlusEqualN) {
  std::array<int, 2> arr{{1, 3}};

  iterator i1{&arr[0]};
  EXPECT_EQ(i1 += 2, iterator{&arr[0] + 2});

  iterator i2{&arr[0] + 2};
  EXPECT_EQ(i2 += -2, iterator{&arr[0]});
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, IPlusN) {
  std::array<int, 2> arr{{1, 3}};
  iterator i{&arr[0]};
  EXPECT_EQ(i + 2, iterator{&arr[0] + 2});
  // Check i+n and n+i
  EXPECT_EQ(i + 2, 2 + i);
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, IMinusEqualN) {
  std::array<int, 2> arr{{1, 3}};

  iterator i1{&arr[0] + 2};
  EXPECT_EQ(i1 -= 2, iterator{&arr[0]});

  iterator i2{&arr[0]};
  EXPECT_EQ(i2 -= -2, iterator{&arr[0] + 2});
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, IMinusN) {
  std::array<int, 2> arr{{1, 3}};
  iterator it{&arr[0] + 2};
  EXPECT_EQ(it - 2, iterator{&arr[0]});
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, BMinusA) {
  std::array<int, 2> arr{{1, 3}};
  iterator b{&arr[0] + 2};
  iterator a{&arr[0]};
  EXPECT_EQ(b - a, 2);
  EXPECT_EQ(b, a + (b - a));
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, TotalOrder) {
  std::array<int, 2> arr{{1, 3}};
  iterator b{&arr[0] + 2};
  iterator a{&arr[0]};
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(b >= a);
  EXPECT_FALSE(b < a);
  EXPECT_FALSE(b <= a);
  EXPECT_FALSE(b == a);
  EXPECT_TRUE(b != a);

  // Comparing const- and non-const iterators.
  const_iterator c{&arr[0]};
  EXPECT_TRUE(b > c);
  EXPECT_TRUE(b >= c);
  EXPECT_FALSE(b < c);
  EXPECT_FALSE(b <= c);
  EXPECT_FALSE(b == c);
  EXPECT_TRUE(b != c);
}

// NOLINTNEXTLINE
TEST_F(PointerBasedIterator, Assign) {
  std::array<int, 2> arr{{3, 5}};
  iterator b{&arr[0] + 2};
  iterator a{&arr[0]};
  b = a;
  EXPECT_EQ(b, a);

  std::array<int const, 2> const carr{{7, 11}};
  const_iterator c{&carr[0]};
  c = a;
  EXPECT_EQ(c, a);
}
