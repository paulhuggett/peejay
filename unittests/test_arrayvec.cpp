//===- unittests/test_arrayvec.cpp ----------------------------------------===//
//*                                             *
//*   __ _ _ __ _ __ __ _ _   ___   _____  ___  *
//*  / _` | '__| '__/ _` | | | \ \ / / _ \/ __| *
//* | (_| | |  | | | (_| | |_| |\ V /  __/ (__  *
//*  \__,_|_|  |_|  \__,_|\__, | \_/ \___|\___| *
//*                       |___/                 *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "peejay/arrayvec.hpp"

// standard library
#include <numeric>

// 3rd party
#include <gmock/gmock.h>

using namespace peejay;
using testing::ElementsAre;

TEST (ArrayVec, DefaultCtor) {
  arrayvec<int, 8> b;
  EXPECT_EQ (0U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_TRUE (b.empty ());
}

TEST (ArrayVec, CtorInitializerList) {
  arrayvec<int, 8> const b{1, 2, 3};
  EXPECT_EQ (3U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_THAT (b, ElementsAre (1, 2, 3));
}

TEST (ArrayVec, CtorCopy) {
  arrayvec<int, 3> const b{3, 5};
  arrayvec<int, 3> c = b;
  EXPECT_EQ (2U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5));
}

namespace {

class no_copy_or_assign {
  friend std::ostream &operator<< (std::ostream &, no_copy_or_assign const &);

public:
  constexpr explicit no_copy_or_assign (int v) noexcept : v_{v} {}
  no_copy_or_assign (no_copy_or_assign const &) = delete;
  no_copy_or_assign (no_copy_or_assign &&other) noexcept : v_{other.v_} {
    other.v_ = 0;
  }

  no_copy_or_assign &operator= (no_copy_or_assign const &) = delete;
  no_copy_or_assign &operator= (no_copy_or_assign &&other) noexcept {
    v_ = other.v_;
    other.v_ = 0;
    return *this;
  }

  bool operator== (no_copy_or_assign const &rhs) const noexcept {
    return v_ == rhs.v_;
  }

private:
  int v_ = 0;
};

std::ostream &operator<< (std::ostream &os, no_copy_or_assign const &x) {
  return os << x.v_;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (ArrayVec, MoveCtor) {
  arrayvec<no_copy_or_assign, 4> a;
  a.emplace_back (2);
  a.emplace_back (3);
  a.emplace_back (5);
  arrayvec<no_copy_or_assign, 4> const b (std::move (a));
  EXPECT_EQ (b.size (), size_t{3});
  EXPECT_EQ (b[0], no_copy_or_assign{2});
  EXPECT_EQ (b[1], no_copy_or_assign{3});
  EXPECT_EQ (b[2], no_copy_or_assign{5});
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveAssign) {
  arrayvec<no_copy_or_assign, 4> a;
  a.emplace_back (2);
  a.emplace_back (3);
  a.emplace_back (5);
  arrayvec<no_copy_or_assign, 4> b;
  b.emplace_back (7);
  b = std::move (a);
  EXPECT_EQ (b.size (), size_t{3});
  EXPECT_EQ (b[0], no_copy_or_assign{2});
  EXPECT_EQ (b[1], no_copy_or_assign{3});
  EXPECT_EQ (b[2], no_copy_or_assign{5});
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveAssign2) {
  arrayvec<no_copy_or_assign, 2> a;
  a.emplace_back (2);
  arrayvec<no_copy_or_assign, 2> b;
  b.emplace_back (3);
  b.emplace_back (5);
  b = std::move (a);
  EXPECT_EQ (b.size (), size_t{1});
  EXPECT_EQ (b[0], no_copy_or_assign{2});
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignInitializerList) {
  arrayvec<int, 3> b{1, 2, 3};
  b.assign ({4, 5, 6});
  EXPECT_THAT (b, ElementsAre (4, 5, 6));
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignCopy) {
  arrayvec<int, 3> const b{5, 7};
  arrayvec<int, 3> c;
  c = b;
  EXPECT_THAT (c, ElementsAre (5, 7));
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignCopy2) {
  arrayvec<int, 3> const b{5};
  arrayvec<int, 3> c{7, 9};
  c = b;
  EXPECT_THAT (c, ElementsAre (5));
}

// NOLINTNEXTLINE
TEST (ArrayVec, SizeAfterResizeSmaller) {
  arrayvec<int, 8> b (std::size_t{8}, int{});
  b.resize (5);
  EXPECT_EQ (5U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_EQ (std::distance (std::begin (b), std::end (b)), 5);
  EXPECT_FALSE (b.empty ());
}

// NOLINTNEXTLINE
TEST (ArrayVec, SizeAfterResizeLarger) {
  arrayvec<int, 8> b (std::size_t{2}, int{});
  b.resize (5);
  EXPECT_EQ (5U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_EQ (std::distance (std::begin (b), std::end (b)), 5);
  EXPECT_FALSE (b.empty ());
}

// NOLINTNEXTLINE
TEST (ArrayVec, SizeAfterResize0) {
  arrayvec<int, 8> b (std::size_t{8}, int{});
  b.resize (0);
  EXPECT_EQ (0U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_TRUE (b.empty ());
}

// NOLINTNEXTLINE
TEST (ArrayVec, IteratorNonConst) {
  arrayvec<int, 4> avec (size_t{4}, int{});

  // I populate the arrayvec manually here to ensure coverage of basic iterator
  // operations, but use std::iota() elsewhere to keep the tests simple.
  int value = 42;
  decltype (avec)::const_iterator end = avec.end ();
  for (decltype (avec)::iterator it = avec.begin (); it != end; ++it) {
    *it = value++;
  }

  // Manually copy the contents of the arrayvec to a new vector.
  std::vector<int> actual;
  for (int x : avec) {
    actual.push_back (x);
  }
  EXPECT_THAT (actual, ElementsAre (42, 43, 44, 45));
}

// NOLINTNEXTLINE
TEST (ArrayVec, IteratorConstFromNonConstContainer) {
  arrayvec<int, 4> avec (std::size_t{4}, int{});
  std::iota (avec.begin (), avec.end (), 42);

  // Manually copy the contents of the arrayvec to a new vector but use a
  // const iterator to do it this time.
  std::vector<int> actual;
  for (decltype (avec)::const_iterator it = avec.cbegin (), end = avec.cend ();
       it != end; ++it) {
    actual.push_back (*it);
  }
  EXPECT_THAT (actual, ElementsAre (42, 43, 44, 45));
}

// NOLINTNEXTLINE
TEST (ArrayVec, IteratorConstIteratorFromConstContainer) {
  arrayvec<int, 4> avec (std::size_t{4}, int{});
  std::iota (avec.begin (), avec.end (), 42);

  auto const &cbuffer = avec;
  EXPECT_THAT (std::vector<int> (cbuffer.begin (), cbuffer.end ()),
               ElementsAre (42, 43, 44, 45));
}

// NOLINTNEXTLINE
TEST (ArrayVec, IteratorNonConstReverse) {
  arrayvec<int, 4> avec (std::size_t{4}, int{});
  std::iota (avec.begin (), avec.end (), 42);
  EXPECT_THAT (std::vector<int> (avec.rbegin (), avec.rend ()),
               ElementsAre (45, 44, 43, 42));
  EXPECT_THAT (std::vector<int> (avec.rcbegin (), avec.rcend ()),
               ElementsAre (45, 44, 43, 42));
}

// NOLINTNEXTLINE
TEST (ArrayVec, IteratorConstReverse) {
  arrayvec<int, 4> vec (std::size_t{4}, int{});
  std::iota (std::begin (vec), std::end (vec), 42);
  auto const &cvec = vec;
  EXPECT_THAT (std::vector<int> (cvec.rbegin (), cvec.rend ()),
               ElementsAre (45, 44, 43, 42));
}

// NOLINTNEXTLINE
TEST (ArrayVec, ElementAccess) {
  arrayvec<int, 4> avec (std::size_t{4}, int{});
  int count = 42;
  for (std::size_t index = 0, end = avec.size (); index != end; ++index) {
    avec[index] = count++;
  }

  std::array<int, 4> const expected{{42, 43, 44, 45}};
  EXPECT_TRUE (
      std::equal (std::begin (avec), std::end (avec), std::begin (expected)));
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveSmallToLarge) {
  arrayvec<int, 4> a (std::size_t{1}, int{42});
  arrayvec<int, 4> b{73, 74, 75, 76};
  a = std::move (b);
  EXPECT_THAT (a, ElementsAre (73, 74, 75, 76));
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveLargeToSmall) {
  arrayvec<int, 3> a{3, 5, 7};
  arrayvec<int, 3> b{11};
  b = std::move (a);
  EXPECT_THAT (a, ElementsAre (3, 5, 7));
}

// NOLINTNEXTLINE
TEST (ArrayVec, Clear) {
  // The two containers start out with different sizes; one uses the small
  // buffer, the other, large.
  arrayvec<int> a (std::size_t{4}, int{});
  EXPECT_EQ (4U, a.size ());
  a.clear ();
  EXPECT_EQ (0U, a.size ());
}

// NOLINTNEXTLINE
TEST (ArrayVec, PushBack) {
  arrayvec<int, 4> a;
  a.push_back (1);
  EXPECT_THAT (a, ElementsAre (1));
  a.push_back (2);
  EXPECT_THAT (a, ElementsAre (1, 2));
  a.push_back (3);
  EXPECT_THAT (a, ElementsAre (1, 2, 3));
  a.push_back (4);
  EXPECT_THAT (a, ElementsAre (1, 2, 3, 4));
}

// NOLINTNEXTLINE
TEST (ArrayVec, AppendIteratorRange) {
  arrayvec<int, 8> a (std::size_t{4}, int{});
  std::iota (std::begin (a), std::end (a), 0);

  std::array<int, 4> extra;
  std::iota (std::begin (extra), std::end (extra), 100);

  a.append (std::begin (extra), std::end (extra));

  EXPECT_THAT (a, ElementsAre (0, 1, 2, 3, 100, 101, 102, 103));
}

namespace {

class no_default_ctor {
public:
  explicit no_default_ctor (int v) noexcept : v_{v} {}
  bool operator== (no_default_ctor const &rhs) const noexcept {
    return v_ == rhs.v_;
  }

private:
  int v_;
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (ArrayVec, NoDefaultPushBack) {
  arrayvec<no_default_ctor, 2> sv;
  sv.push_back (no_default_ctor{7});
  EXPECT_THAT (sv, ElementsAre (no_default_ctor{7}));
}

// NOLINTNEXTLINE
TEST (ArrayVec, NoDefaultEmplace) {
  arrayvec<no_default_ctor, 2> sv;
  sv.emplace_back (7);
  EXPECT_THAT (sv, ElementsAre (no_default_ctor{7}));
}
