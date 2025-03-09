//===- unittests/uri/test_find_last.cpp -----------------------------------===//
//*   __ _           _   _           _    *
//*  / _(_)_ __   __| | | | __ _ ___| |_  *
//* | |_| | '_ \ / _` | | |/ _` / __| __| *
//* |  _| | | | | (_| | | | (_| \__ \ |_  *
//* |_| |_|_| |_|\__,_| |_|\__,_|___/\__| *
//*                                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include <algorithm>
#include <forward_list>
#include <vector>

#include "peejay/uri/find_last.hpp"

using namespace std::string_view_literals;

// NOLINTNEXTLINE
TEST(FindLast, A) {
  auto const v = {1, 2, 3, 1, 2, 3, 1, 2};
  auto const i1 = uri::find_last(v.begin(), v.end(), 3);
  EXPECT_EQ(std::ranges::distance(v.begin(), i1.begin()), 5);
}

// NOLINTNEXTLINE
TEST(FindLast, B) {
  auto const v = {1, 2, 3, 1, 2, 3, 1, 2};
  auto const i2 = uri::find_last(v, 3);
  EXPECT_EQ(std::ranges::distance(v.begin(), i2.begin()), 5);
}

// NOLINTNEXTLINE
TEST(FindLast, Empty) {
  constexpr auto empty = ""sv;
  auto const result = uri::find_last(empty, 'a');
  auto const last = std::end(empty);
  EXPECT_EQ(std::begin(result), last);
  EXPECT_EQ(std::end(result), last);
}

// NOLINTNEXTLINE
TEST(FindLast, FoundAtFirst) {
  auto const ab = {'a', 'b'};
  auto const result = uri::find_last(ab, 'a');
  EXPECT_EQ(std::begin(result), std::begin(ab));
  EXPECT_EQ(std::end(result), std::end(ab));
}

// NOLINTNEXTLINE
TEST(FindLast, FoundInMiddle) {
  auto const aba = {'a', 'b', 'a'};
  auto const result = uri::find_last(aba, 'b');
  EXPECT_EQ(std::ranges::distance(aba.begin(), result.begin()), 1);
  EXPECT_EQ(std::end(result), aba.end());
}

// NOLINTNEXTLINE
TEST(FindLast, FoundAtLast) {
  auto const aba = {'a', 'b', 'a'};
  auto const result = uri::find_last(aba, 'a');
  EXPECT_EQ(std::ranges::distance(aba.begin(), result.begin()), 2);
  EXPECT_EQ(std::end(result), aba.end());
}

// NOLINTNEXTLINE
TEST(FindLast, Filtered) {
  auto src = std::vector{1, 3, 5, 7, 7, 11};
  auto view = src | std::views::filter([](int v) { return v >= 5; });
  auto const result = uri::find_last(view, 7);
  EXPECT_EQ(std::ranges::distance(view.begin(), result.begin()), 2);
  EXPECT_EQ(view.end(), result.end());
}

namespace {
template <typename Container> class FindLastInt : public testing::Test {
protected:
  Container const values_ = {1, 2, 1, 2, 1, 2, 1, 2};

  auto pos(int const index) const {
    auto it = std::begin(values_);
    std::advance(it, index);
    return it;
  }
};

static_assert(std::bidirectional_iterator<std::vector<int>::iterator>);
static_assert(!std::bidirectional_iterator<std::forward_list<int>::iterator>);
using FindLastIntContainers = testing::Types<std::vector<int>, std::forward_list<int>>;

int add_three(int const v) {
  return v + 3;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TYPED_TEST_SUITE(FindLastInt, FindLastIntContainers);

// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, NotFound) {
  auto const result = uri::find_last(this->values_, 0);
  EXPECT_TRUE(std::ranges::empty(result));
  EXPECT_EQ(result.begin(), std::ranges::end(this->values_));
  EXPECT_EQ(result.end(), std::ranges::end(this->values_));
}
// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, One) {
  auto const result = uri::find_last(this->values_, 1);
  std::vector<int> const actual{std::begin(result), std::end(result)};
  EXPECT_THAT(actual, testing::ElementsAre(1, 2));
  EXPECT_EQ(result.begin(), this->pos(6));
}
// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, Two) {
  auto const result = uri::find_last(this->values_, 2);
  std::vector<int> const actual{std::begin(result), std::end(result)};
  EXPECT_THAT(actual, testing::ElementsAre(2));
  EXPECT_EQ(result.begin(), this->pos(7));
}
// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, Three) {
  auto const result = uri::find_last(this->values_, 3, add_three);
  EXPECT_TRUE(std::ranges::empty(result));
  EXPECT_EQ(result.begin(), std::ranges::end(this->values_));
}
// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, Four) {
  auto const result = uri::find_last(this->values_, 4, add_three);
  std::vector<int> const actual{std::begin(result), std::end(result)};
  EXPECT_THAT(actual, testing::ElementsAre(1, 2));
  EXPECT_EQ(result.begin(), this->pos(6));
}
// NOLINTNEXTLINE
TYPED_TEST(FindLastInt, Five) {
  auto const result = uri::find_last(this->values_, 5, add_three);
  std::vector<int> const actual{std::begin(result), std::end(result)};
  EXPECT_THAT(actual, testing::ElementsAre(2));
  EXPECT_EQ(result.begin(), this->pos(7));
}
