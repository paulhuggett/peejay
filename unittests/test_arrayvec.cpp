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

using peejay::arrayvec;
using testing::ElementsAre;

// NOLINTNEXTLINE
TEST (ArrayVec, DefaultCtor) {
  arrayvec<int, 8> b;
  EXPECT_EQ (0U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_TRUE (b.empty ());
}

// NOLINTNEXTLINE
TEST (ArrayVec, CtorInitializerList) {
  arrayvec<int, 8> const b{1, 2, 3};
  EXPECT_EQ (3U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_THAT (b, ElementsAre (1, 2, 3));
}

// NOLINTNEXTLINE
TEST (ArrayVec, CtorCopy) {
  arrayvec<int, 3> const b{3, 5};
  // Disable clang-tidy warning since that's the point of the test.
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  arrayvec<int, 3> c = b;
  EXPECT_EQ (2U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5));
}

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class no_copy {
public:
  constexpr explicit no_copy (int v) noexcept : v_{v} {}
  no_copy (no_copy const &) = delete;
  no_copy (no_copy &&other) noexcept : v_{other.v_} { other.v_ = 0; }

  no_copy &operator= (no_copy const &) = delete;
  no_copy &operator= (no_copy &&other) noexcept {
    v_ = other.v_;
    other.v_ = 0;
    return *this;
  }

#if PEEJAY_CXX20
  bool operator== (no_copy const &rhs) const noexcept = default;
#else
  bool operator== (no_copy const &rhs) const noexcept {
    return v_ == rhs.v_;
  }
#endif

  [[nodiscard]] int get () const noexcept { return v_; }

private:
  int v_ = 0;
};

std::ostream &operator<< (std::ostream &os, no_copy const &x) {
  return os << x.get ();
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class no_move {
public:
  constexpr explicit no_move (int v) noexcept : v_{v} {}
  no_move (no_move const &) = default;
  no_move (no_move &&other) noexcept = delete;

  no_move &operator= (no_move const &) = default;
  no_move &operator= (no_move &&other) noexcept = delete;

#if PEEJAY_CXX20
  bool operator== (no_move const &rhs) const noexcept = default;
#else
  bool operator== (no_move const &rhs) const noexcept { return v_ == rhs.v_; }
#endif

  [[nodiscard]] int get () const noexcept { return v_; }

private:
  int v_ = 0;
};

std::ostream &operator<< (std::ostream &os, no_move const &x) {
  return os << x.get ();
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (ArrayVec, MoveCtor) {
  arrayvec<no_copy, 4> a;
  a.emplace_back (2);
  a.emplace_back (3);
  a.emplace_back (5);
  arrayvec<no_copy, 4> const b (std::move (a));
  EXPECT_EQ (b.size (), size_t{3});
  EXPECT_EQ (b[0], no_copy{2});
  EXPECT_EQ (b[1], no_copy{3});
  EXPECT_EQ (b[2], no_copy{5});
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveAssign) {
  arrayvec<no_copy, 4> a;
  a.emplace_back (2);
  a.emplace_back (3);
  a.emplace_back (5);
  arrayvec<no_copy, 4> b;
  b.emplace_back (7);
  b = std::move (a);
  EXPECT_EQ (b.size (), size_t{3});
  EXPECT_EQ (b[0], no_copy{2});
  EXPECT_EQ (b[1], no_copy{3});
  EXPECT_EQ (b[2], no_copy{5});
}

// NOLINTNEXTLINE
TEST (ArrayVec, MoveAssign2) {
  arrayvec<no_copy, 2> a;
  a.emplace_back (2);
  arrayvec<no_copy, 2> b;
  b.emplace_back (3);
  b.emplace_back (5);
  b = std::move (a);
  EXPECT_EQ (b.size (), size_t{1});
  EXPECT_EQ (b[0], no_copy{2});
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignCount) {
  arrayvec<int, 3> b{1};
  b.assign (size_t{3}, 7);
  EXPECT_THAT (b, ElementsAre (7, 7, 7));
}
// NOLINTNEXTLINE
TEST (ArrayVec, AssignInitializerList) {
  arrayvec<int, 3> b{1, 2, 3};
  b.assign ({4, 5, 6});
  EXPECT_THAT (b, ElementsAre (4, 5, 6));
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignCopyLargeToSmall) {
  arrayvec<no_move, 3> const b{no_move{5}, no_move{7}};
  arrayvec<no_move, 3> c{no_move{11}};
  c = b;
  EXPECT_THAT (c, ElementsAre (no_move{5}, no_move{7}));
}

// NOLINTNEXTLINE
TEST (ArrayVec, AssignCopySmallToLarge) {
  arrayvec<no_move, 3> const b{no_move{5}};
  arrayvec<no_move, 3> c{no_move{7}, no_move{9}};
  c = b;
  EXPECT_THAT (c, ElementsAre (no_move{5}));
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
  // const iterator to do it this time. Don't use a range-based for loop so we
  // get to declare a const iterator.
  std::vector<int> actual;
  // NOLINTNEXTLINE(modernize-loop-convert)
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
  // I want to state this loop explicitly for the purposes of the test.
  // NOLINTNEXTLINE(modernize-loop-convert)
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
  EXPECT_THAT (b, ElementsAre (3, 5, 7));
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

  std::array<int, 4> extra{};
  std::iota (std::begin (extra), std::end (extra), 100);

  a.append (std::begin (extra), std::end (extra));

  EXPECT_THAT (a, ElementsAre (0, 1, 2, 3, 100, 101, 102, 103));
}

namespace {

class no_default_ctor {
public:
  explicit constexpr no_default_ctor (int v) noexcept : v_{v} {}
#if PEEJAY_CXX20
  constexpr bool operator== (no_default_ctor const &rhs) const noexcept =
      default;
#else
  constexpr bool operator== (no_default_ctor const &rhs) const noexcept {
    return v_ == rhs.v_;
  }
#endif

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

// NOLINTNEXTLINE
TEST (ArrayVec, Eq) {
  EXPECT_TRUE ((arrayvec<int, 2>{1, 2} == arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1, 3} == arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1} == arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} ==
                arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_FALSE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} ==
                 arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, Neq) {
  EXPECT_FALSE ((arrayvec<int, 2>{1, 2} != arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1, 3} != arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1} != arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} !=
                 arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_TRUE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} !=
                arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, Ge) {
  EXPECT_TRUE ((arrayvec<int, 2>{1, 2} >= arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1, 3} >= arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1} >= arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} >=
                arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_TRUE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} >=
                arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, Gt) {
  EXPECT_FALSE ((arrayvec<int, 2>{1, 2} > arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1, 3} > arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1} > arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} >
                 arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_TRUE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} >
                arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, Le) {
  EXPECT_TRUE ((arrayvec<int, 2>{1, 2} <= arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1, 3} <= arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1} <= arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} <=
                arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_FALSE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} <=
                 arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, Lt) {
  EXPECT_FALSE ((arrayvec<int, 2>{1, 2} < arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<int, 2>{1, 3} < arrayvec<int, 2>{1, 2}));
  EXPECT_TRUE ((arrayvec<int, 2>{1} < arrayvec<int, 2>{1, 2}));
  EXPECT_FALSE ((arrayvec<char, 4>{'a', 'b', 'c', 'd'} <
                 arrayvec<char, 4>{'a', 'b', 'c', 'd'}));
  EXPECT_FALSE ((arrayvec<char, 4>{'d', 'a', 'b', 'c'} <
                 arrayvec<char, 4>{'c', 'b', 'd', 'a'}));
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseSinglePos) {
  peejay::arrayvec<int, 3> v{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const e1 = v.erase (v.cbegin ());
  EXPECT_EQ (e1, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (2, 3));
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const e2 = v.erase (v.cbegin ());
  EXPECT_EQ (e2, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (3));
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const e3 = v.erase (v.cbegin ());
  EXPECT_EQ (e3, v.begin ());
  EXPECT_TRUE (v.empty ());
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseSingleSecondElement) {
  peejay::arrayvec<int, 3> v{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = v.erase (v.begin () + 1);
  EXPECT_EQ (last, v.begin () + 1);
  EXPECT_THAT (v, testing::ElementsAre (1, 3));
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseSingleFinalElement) {
  peejay::arrayvec<int, 3> v{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = v.erase (v.begin () + 2);
  EXPECT_EQ (last, v.begin () + 2);
  EXPECT_THAT (v, testing::ElementsAre (1, 2));
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseRangeAll) {
  peejay::arrayvec<int, 3> a{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = a.erase (a.begin (), a.end ());
  EXPECT_EQ (last, a.end ());
  EXPECT_TRUE (a.empty ());
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseRangeFirstTwo) {
  peejay::arrayvec<int, 3> b{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = b.begin ();
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = b.erase (first, first + 2);
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (3));
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseRangeFirstOnly) {
  peejay::arrayvec<int, 3> b{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = b.begin ();
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = b.erase (first, first + 1);
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (2, 3));
}
// NOLINTNEXTLINE
TEST (ArrayVec, EraseRangeSecondToEnd) {
  peejay::arrayvec<int, 3> b{1, 2, 3};
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = b.begin () + 1;
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = b.erase (first, b.end ());
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (1));
}

enum class action { added, deleted, moved, copied };

static std::ostream &operator<< (std::ostream &os, action a) {
  char const * str = "";
  switch (a) {
  case action::added: str = "added"; break;
  case action::deleted: str = "deleted"; break;
  case action::moved: str = "moved"; break;
  case action::copied: str = "copied"; break;
  }
  return os << str;
}

struct tracker {
  std::vector<std::tuple<int, int, action>> actions;
};

class trackee {
public:
  trackee (tracker *const t, int v) : t_{t}, v_{v} {
    t_->actions.emplace_back (v_, 0, action::added);
  }
  trackee (trackee const &rhs) : t_{rhs.t_}, v_{rhs.v_} {
    t_->actions.emplace_back (v_, rhs.v_, action::copied);
  }
  trackee (trackee &&rhs) noexcept : t_{rhs.t_}, v_{rhs.v_} {
    t_->actions.emplace_back (v_, rhs.v_, action::moved);
    if (rhs.v_ > 0) {
      rhs.v_ = -rhs.v_;
    }
  }

  ~trackee () noexcept {
    try {
      t_->actions.emplace_back (v_, 0, action::deleted);
    } catch (...) {
      std::abort ();
    }
  }

  trackee &operator= (trackee const &rhs) {
    if (this != &rhs) {
      t_->actions.emplace_back (v_, rhs.v_, action::copied);
      t_ = rhs.t_;
      v_ = rhs.v_;
    }
    return *this;
  }
  trackee &operator= (trackee &&rhs) noexcept {
    t_->actions.emplace_back (v_, rhs.v_, action::moved);
    t_ = rhs.t_;
    v_ = rhs.v_;
    if (rhs.v_ > 0) {
      rhs.v_ = -rhs.v_;
    }
    return *this;
  }

  constexpr int get () const noexcept { return v_; }
  constexpr tracker const *owner () const noexcept { return t_; }

private:
  tracker *t_;
  int v_;
};

constexpr bool operator== (trackee const &lhs, trackee const &rhs) {
  return lhs.owner () == rhs.owner () && lhs.get () == rhs.get ();
}
constexpr bool operator== (trackee const &lhs, int rhs) {
  return lhs.get () == rhs;
}
constexpr bool operator== (int lhs, trackee const &rhs) {
  return lhs == rhs.get ();
}
constexpr bool operator!= (trackee const &lhs, trackee const &rhs) {
  return !operator== (lhs, rhs);
}
constexpr bool operator!= (trackee const &lhs, int rhs) {
  return !operator== (lhs, rhs);
}
constexpr bool operator!= (int lhs, trackee const &rhs) {
  return !operator== (lhs, rhs);
}

// NOLINTNEXTLINE
TEST (ArrayVec, TrackedCopyInsert) {
  tracker t;
  peejay::arrayvec<trackee, 3> v{trackee{&t, 1}, trackee{&t, 2},
                                 trackee{&t, 3}};
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 0, action::added),
                                     std::make_tuple (2, 0, action::added),
                                     std::make_tuple (3, 0, action::added),
                                     std::make_tuple (1, 1, action::copied),
                                     std::make_tuple (2, 2, action::copied),
                                     std::make_tuple (3, 3, action::copied),
                                     std::make_tuple (3, 0, action::deleted),
                                     std::make_tuple (2, 0, action::deleted),
                                     std::make_tuple (1, 0, action::deleted)));
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedMoveInsert) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 0, action::added),
                                     std::make_tuple (2, 0, action::added),
                                     std::make_tuple (3, 0, action::added)));
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedEraseSinglePos) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 0, action::added),
                                     std::make_tuple (2, 0, action::added),
                                     std::make_tuple (3, 0, action::added)));
  t.actions.clear ();

  // Remove the first element.
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto last1 = v.erase (v.cbegin ());
  EXPECT_EQ (last1, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (2, 3));
  EXPECT_THAT (
      t.actions,
      testing::ElementsAre (
          std::make_tuple (1, 2, action::moved),    // 2 moved to replace 1
          std::make_tuple (-2, 3, action::moved),   // 3 moved to replace 2
          std::make_tuple (-3, 0, action::deleted)  // original 3 deleted
          ));
  t.actions.clear ();

  // Remove the first element.
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto last2 = v.erase (v.cbegin ());
  EXPECT_EQ (last2, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (3));
  EXPECT_THAT (
      t.actions,
      testing::ElementsAre (
          std::make_tuple (2, 3, action::moved),    // 3 moved to replace 2
          std::make_tuple (-3, 0, action::deleted)  // original 3 deleted
          ));
  t.actions.clear ();

  // Remove the single remaining element.
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto last3 = v.erase (v.cbegin ());
  EXPECT_EQ (last3, v.begin ());
  EXPECT_TRUE (v.empty ());
  EXPECT_THAT (t.actions, testing::ElementsAre (std::make_tuple (
                              3, 0, action::deleted)  // 3 deleted
                                                ));
  t.actions.clear ();
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedEraseRangeAll) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  t.actions.clear ();

  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto last = v.erase (v.begin (), v.end ());
  EXPECT_EQ (last, v.end ());
  EXPECT_TRUE (v.empty ());

  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 0, action::deleted),
                                     std::make_tuple (2, 0, action::deleted),
                                     std::make_tuple (3, 0, action::deleted)));
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedEraseRangeFirstTwo) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  t.actions.clear ();

  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = v.begin ();
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = v.erase (first, first + 2);
  EXPECT_EQ (last, first);
  EXPECT_THAT (v, testing::ElementsAre (3));
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 3, action::moved),
                                     std::make_tuple (2, 0, action::deleted),
                                     std::make_tuple (-3, 0, action::deleted)));
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedEraseRangeFirstOnly) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  t.actions.clear ();

  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = v.begin ();
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = v.erase (first, first + 1);
  EXPECT_EQ (last, first);
  EXPECT_THAT (v, testing::ElementsAre (2, 3));
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (1, 2, action::moved),
                                     std::make_tuple (-2, 3, action::moved),
                                     std::make_tuple (-3, 0, action::deleted)));
}
// NOLINTNEXTLINE
TEST (ArrayVec, TrackedEraseRangeSecondToEnd) {
  tracker t;
  peejay::arrayvec<trackee, 3> v;
  v.emplace_back (&t, 1);
  v.emplace_back (&t, 2);
  v.emplace_back (&t, 3);
  t.actions.clear ();

  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const first = v.begin () + 1;
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = v.erase (first, v.end ());
  EXPECT_EQ (last, first);
  EXPECT_THAT (v, testing::ElementsAre (1));
  EXPECT_THAT (t.actions,
               testing::ElementsAre (std::make_tuple (2, 0, action::deleted),
                                     std::make_tuple (3, 0, action::deleted)));
}
