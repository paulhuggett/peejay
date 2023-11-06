//===- unittests/test_small_vector.cpp ------------------------------------===//
//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#include <gmock/gmock.h>

#include <numeric>
#include <sstream>

#include "peejay/small_vector.hpp"
#if __cpp_lib_ranges
#include <ranges>
#endif

using peejay::small_vector;
using testing::ElementsAre;

struct copy_ctor_ex : public std::domain_error {
  copy_ctor_ex () : std::domain_error{"copy ctor"} {}
};
struct copy_ctor_throws {
  copy_ctor_throws () noexcept = default;
  explicit copy_ctor_throws (int v_) noexcept : v{v_} {}
  copy_ctor_throws (copy_ctor_throws const& /*rhs*/) {
    if (throws) {
      throw copy_ctor_ex{};
    }
  }
  copy_ctor_throws (copy_ctor_throws&&) noexcept = default;

  ~copy_ctor_throws () noexcept = default;

  copy_ctor_throws& operator= (copy_ctor_throws const& rhs) {
    if (&rhs != this) {
      if (throws) {
        throw copy_ctor_ex{};
      }
    }
    return *this;
  }
  copy_ctor_throws& operator= (copy_ctor_throws&&) noexcept = default;

  bool operator== (copy_ctor_throws const& rhs) const noexcept {
    return throws == rhs.throws && v == rhs.v;
  }
  bool operator!= (copy_ctor_throws const& rhs) const noexcept {
    return !operator== (rhs);
  }

  bool throws = true;
  int v = 0;
};

struct move_ctor_ex : public std::domain_error {
  move_ctor_ex () : std::domain_error{"move ctor"} {}
};
struct move_ctor_throws {
  move_ctor_throws () = default;
  move_ctor_throws (move_ctor_throws const&) = default;
  move_ctor_throws (move_ctor_throws&&) {
    if (throws) {
      throw move_ctor_ex{};
    }
  }
  ~move_ctor_throws () noexcept = default;

  move_ctor_throws& operator= (move_ctor_throws const&) = default;
  move_ctor_throws& operator= (move_ctor_throws&&) noexcept = default;
  bool throws = true;
};

// NOLINTNEXTLINE
TEST (SmallVector, DefaultCtor) {
  peejay::small_vector<int, 8> b;
  EXPECT_EQ (0U, b.size ())
      << "expected the initial size to be number number of stack elements";
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_TRUE (b.empty ());
}

// NOLINTNEXTLINE
TEST (SmallVector, ExplicitCtorLessThanStackBuffer) {
  peejay::small_vector<int, 8> const b (std::size_t{5});
  EXPECT_EQ (5U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_EQ (5U * sizeof (int), b.size_bytes ());
}

// NOLINTNEXTLINE
TEST (SmallVector, ExplicitCtor0) {
  peejay::small_vector<int, 8> const b (std::size_t{0});
  EXPECT_EQ (0U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_EQ (0U * sizeof (int), b.size_bytes ());
  EXPECT_TRUE (b.empty ());
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorCountFirstLastFowardIteratorInBody) {
  std::array<int, 4> src{{2, 3, 5, 7}};
  peejay::small_vector<int, 4> const v (std::begin (src), std::end (src));
  EXPECT_THAT (v, ElementsAre (2, 3, 5, 7));
}
// NOLINTNEXTLINE
TEST (SmallVector, CtorCountFirstLastInputIteratorInBody) {
  std::istringstream src ("2 3 5 7");
  peejay::small_vector<int, 4> const v{std::istream_iterator<int>{src},
                                       std::istream_iterator<int>{}};
  EXPECT_THAT (v, ElementsAre (2, 3, 5, 7));
}
// NOLINTNEXTLINE
TEST (SmallVector, CtorCountFirstLastFowardIteratorLarge) {
  std::array<int, 4> src{{2, 3, 5, 7}};
  peejay::small_vector<int, 2> const v (std::begin (src), std::end (src));
  EXPECT_THAT (v, ElementsAre (2, 3, 5, 7));
}
// NOLINTNEXTLINE
TEST (SmallVector, CtorCountFirstLastInputIteratorLarge) {
  std::istringstream src ("2 3 5 7");
  peejay::small_vector<int, 2> const v{std::istream_iterator<int>{src},
                                       std::istream_iterator<int>{}};
  EXPECT_THAT (v, ElementsAre (2, 3, 5, 7));
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorCountValueInBody) {
  peejay::small_vector<int, 4> const v (std::size_t{4}, 23);
  EXPECT_THAT (v, ElementsAre (23, 23, 23, 23));
}
// NOLINTNEXTLINE
TEST (SmallVector, CtorCountValueLarge) {
  peejay::small_vector<int, 4> const v (std::size_t{5}, 23);
  EXPECT_THAT (v, ElementsAre (23, 23, 23, 23, 23));
}

// NOLINTNEXTLINE
TEST (SmallVector, ExplicitCtorGreaterThanStackBuffer) {
  peejay::small_vector<int, 8> const b (std::size_t{10});
  EXPECT_EQ (10U, b.size ());
  EXPECT_EQ (10U, b.capacity ());
  EXPECT_EQ (10 * sizeof (int), b.size_bytes ());
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorInitializerList) {
  peejay::small_vector<int, 8> const b{1, 2, 3};
  EXPECT_EQ (3U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_THAT (b, ::testing::ElementsAre (1, 2, 3));
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorInitializerList2) {
  peejay::small_vector<int, 2> b{1, 2, 3, 4};
  EXPECT_THAT (b, ::testing::ElementsAre (1, 2, 3, 4));
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorCopy) {
  peejay::small_vector<int, 3> const b{3, 5};
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  peejay::small_vector<int, 2> c = b;
  EXPECT_EQ (2U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5));
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorCopy2) {
  peejay::small_vector<int, 3> const b{3, 5, 7, 11, 13};
  peejay::small_vector<int, 3> c = b;
  EXPECT_EQ (5U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5, 7, 11, 13));
}

// NOLINTNEXTLINE
TEST (SmallVector, CtorCopyLargeToSmall) {
  // The first of these vectors has too few in-body elements to accommodate its
  // members and therefore uses a large buffer. It is assigned to a vector
  // which _can_ hold those elements in-body.
  peejay::small_vector<int, 3> const b{3, 5, 7, 11, 13};
  peejay::small_vector<int, 5> c = b;
  EXPECT_EQ (5U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5, 7, 11, 13));
}

// NOLINTNEXTLINE
TEST (SmallVector, AssignLargeToLarge) {
  // The first of these vectors has too few in-body elements to accommodate its
  // members and therefore uses a large buffer. It is assigned to a vector
  // which _can_ hold those elements in-body.
  peejay::small_vector<int, 3> const b{3, 5, 7, 11, 13};
  peejay::small_vector<int, 2> c{17, 19, 23};
  c = b;
  EXPECT_EQ (5U, c.size ());
  EXPECT_THAT (c, ElementsAre (3, 5, 7, 11, 13));
}

// NOLINTNEXTLINE
TEST (SmallVector, CopyAssignThrowsSmallToSmall) {
  peejay::small_vector<copy_ctor_throws, 1> b{1};
  peejay::small_vector<copy_ctor_throws, 2> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (b), copy_ctor_ex);
  EXPECT_EQ (b.size (), 1);
  EXPECT_EQ (c.size (), 0);
}
// NOLINTNEXTLINE
TEST (SmallVector, MoveAssignThrowsSmallToSmall) {
  peejay::small_vector<move_ctor_throws, 1> b{1};
  peejay::small_vector<move_ctor_throws, 2> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (std::move (b)), move_ctor_ex);
  EXPECT_EQ (c.size (), 0);
}

// NOLINTNEXTLINE
TEST (SmallVector, CopyAssignThrowsLargeToSmall) {
  peejay::small_vector<copy_ctor_throws, 1> b{2};
  peejay::small_vector<copy_ctor_throws, 2> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (b), copy_ctor_ex);
  EXPECT_EQ (b.size (), 2);
  EXPECT_EQ (c.size (), 0);
}
// NOLINTNEXTLINE
TEST (SmallVector, MoveAssignThrowsLargeToSmall) {
  peejay::small_vector<move_ctor_throws, 1> b{2};
  peejay::small_vector<move_ctor_throws, 2> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (std::move (b)), move_ctor_ex);
  EXPECT_EQ (c.size (), 0);
}

// NOLINTNEXTLINE
TEST (SmallVector, CopyAssignThrowsSmallToLarge) {
  peejay::small_vector<copy_ctor_throws, 2> b{2};
  peejay::small_vector<copy_ctor_throws, 1> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (b), copy_ctor_ex);
  EXPECT_EQ (b.size (), 2);
  EXPECT_EQ (c.size (), 0);
}
// NOLINTNEXTLINE
TEST (SmallVector, MoveAssignThrowsSmallToLarge) {
  peejay::small_vector<move_ctor_throws, 2> b{2};
  peejay::small_vector<move_ctor_throws, 1> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (std::move (b)), move_ctor_ex);
  EXPECT_EQ (c.size (), 0);
}

// NOLINTNEXTLINE
TEST (SmallVector, CopyAssignThrowsLargeToLarge) {
  peejay::small_vector<copy_ctor_throws, 2> b{3};
  peejay::small_vector<copy_ctor_throws, 1> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (b), copy_ctor_ex);
  EXPECT_EQ (b.size (), 3);
  EXPECT_EQ (c.size (), 0);
}
// NOLINTNEXTLINE
TEST (SmallVector, MoveAssignThrowsLargeToLarge) {
  peejay::small_vector<move_ctor_throws, 2> b{3};
  peejay::small_vector<move_ctor_throws, 1> c;
  // NOLINTNEXTLINE
  EXPECT_THROW (c.operator= (std::move (b)), move_ctor_ex);
  EXPECT_EQ (c.size (), 0);
}

// NOLINTNEXTLINE
TEST (SmallVector, MoveCtor) {
  peejay::small_vector<int, 4> a (std::size_t{4});
  std::iota (a.begin (), a.end (), 0);  // fill with increasing values
  peejay::small_vector<int, 3> const b (std::move (a));

  EXPECT_THAT (b, ElementsAre (0, 1, 2, 3));
}

// NOLINTNEXTLINE
TEST (SmallVector, MoveCtorLargeToLarge) {
  peejay::small_vector<int, 3> a (std::size_t{4});
  std::iota (a.begin (), a.end (), 0);  // fill with increasing values
  peejay::small_vector<int, 2> const b (std::move (a));

  EXPECT_THAT (b, ElementsAre (0, 1, 2, 3));
}

// NOLINTNEXTLINE
TEST (SmallVector, AssignInitializerList) {
  peejay::small_vector<int, 3> b{1, 2, 3};
  b.assign ({4, 5, 6, 7});
  EXPECT_THAT (b, ElementsAre (4, 5, 6, 7));
}

// NOLINTNEXTLINE
TEST (SmallVector, OperatorEqCopy) {
  peejay::small_vector<int, 3> const b{5, 7};
  peejay::small_vector<int, 3> c;
  c = b;
  EXPECT_THAT (c, ElementsAre (5, 7));
}

// NOLINTNEXTLINE
TEST (SmallVector, AssignCountLarger) {
  small_vector<int, 3> b{1};
  int const v = 7;
  b.assign (std::size_t{5}, v);
  EXPECT_THAT (b, ElementsAre (7, 7, 7, 7, 7));
}
// NOLINTNEXTLINE
TEST (SmallVector, AssignCountLargerThrows) {
  small_vector<copy_ctor_throws, 3> b{1};
  copy_ctor_throws const v;
  // NOLINTNEXTLINE
  EXPECT_THROW (b.assign (std::size_t{5}, v), copy_ctor_ex);
}
// NOLINTNEXTLINE
TEST (SmallVector, AssignCountSmallerThrows) {
  small_vector<copy_ctor_throws, 3> b{3};
  copy_ctor_throws const v{7};
  // NOLINTNEXTLINE
  EXPECT_THROW (b.assign (std::size_t{2}, v), copy_ctor_ex);
}
// NOLINTNEXTLINE
TEST (SmallVector, AssignCountSmaller) {
  small_vector<int, 3> b{1, 3};
  int const v = 7;
  b.assign (std::size_t{1}, v);
  EXPECT_THAT (b, ElementsAre (7));
}
// NOLINTNEXTLINE
TEST (SmallVector, AssignCountUnchanged) {
  small_vector<int, 3> b{1, 3};
  int const v = 5;
  b.assign (std::size_t{2}, v);
  EXPECT_THAT (b, ElementsAre (5, 5));
}
// NOLINTNEXTLINE
TEST (SmallVector, AssignCountZero) {
  small_vector<int, 3> b{1, 3};
  using size_type = decltype (b)::size_type;
  int const v = 7;
  b.assign (size_type{0}, v);
  EXPECT_THAT (b, ElementsAre ());
}

// NOLINTNEXTLINE
TEST (SmallVector, SizeAfterResizeLarger) {
  peejay::small_vector<int, 4> b (std::size_t{4});
  std::size_t const size{10};
  b.resize (size);
  EXPECT_EQ (size, b.size ());
  EXPECT_GE (size, b.capacity ())
      << "expected capacity to be at least " << size << " (the container size)";
}

// NOLINTNEXTLINE
TEST (SmallVector, ContentsAfterResizeLarger) {
  constexpr auto orig_size = std::size_t{8};
  constexpr auto new_size = std::size_t{10};

  peejay::small_vector<int, orig_size> b (orig_size);
  std::iota (std::begin (b), std::end (b), 37);
  b.resize (new_size);
  ASSERT_EQ (b.size (), new_size);

  std::vector<int> actual;
  std::copy_n (std::begin (b), orig_size, std::back_inserter (actual));
  EXPECT_THAT (actual, ::testing::ElementsAre (37, 38, 39, 40, 41, 42, 43, 44));
}

// NOLINTNEXTLINE
TEST (SmallVector, SizeAfterResizeSmaller) {
  peejay::small_vector<int, 8> b (std::size_t{8});
  b.resize (5);
  EXPECT_EQ (5U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_FALSE (b.empty ());
}

// NOLINTNEXTLINE
TEST (SmallVector, SizeAfterResize0) {
  peejay::small_vector<int, 8> b (std::size_t{8});
  b.resize (0);
  EXPECT_EQ (0U, b.size ());
  EXPECT_EQ (8U, b.capacity ());
  EXPECT_TRUE (b.empty ());
}

// NOLINTNEXTLINE
TEST (SmallVector, DataAndConstDataMatch) {
  peejay::small_vector<int, 8> b (std::size_t{8});
  auto const* const bconst = &b;
  EXPECT_EQ (bconst->data (), b.data ());
}

// NOLINTNEXTLINE
TEST (SmallVector, At) {
  peejay::small_vector<int, 1> a{3};
  EXPECT_EQ (a.at (0), 3);
  EXPECT_THROW (a.at (1), std::out_of_range);
  a.push_back (4);
  EXPECT_EQ (a.at (0), 3);
  EXPECT_EQ (a.at (1), 4);
  EXPECT_THROW (a.at (2), std::out_of_range);
}

// NOLINTNEXTLINE
TEST (SmallVector, IteratorNonConst) {
  peejay::small_vector<int, 4> buffer (std::size_t{4});

  // I populate the buffer manually here to ensure coverage of basic iterator
  // operations, but use std::iota() elsewhere to keep the tests simple.
  int value = 42;
  for (decltype (buffer)::iterator it = buffer.begin (), end = buffer.end ();
       it != end; ++it) {
    *it = value++;
  }

  {
    // Manually copy the contents of the buffer to a new vector.
    std::vector<int> actual;
    for (decltype (buffer)::iterator it = buffer.begin (), end = buffer.end ();
         it != end; ++it) {
      actual.push_back (*it);
    }
    EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
  }
}

// NOLINTNEXTLINE
TEST (SmallVector, IteratorConstFromNonConstContainer) {
  peejay::small_vector<int, 4> buffer (std::size_t{4});
  std::iota (buffer.begin (), buffer.end (), 42);

  {
    // Manually copy the contents of the buffer to a new vector but use a
    /// const iterator to do it this time.
    std::vector<int> actual;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (decltype (buffer)::const_iterator it = buffer.cbegin (),
                                           end = buffer.cend ();
         it != end; ++it) {
      actual.push_back (*it);
    }
    EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
  }
}

// NOLINTNEXTLINE
TEST (SmallVector, IteratorConstIteratorFromConstContainer) {
  peejay::small_vector<int, 4> buffer (std::size_t{4});
  std::iota (buffer.begin (), buffer.end (), 42);

  auto const& cbuffer = buffer;
  std::vector<int> const actual (cbuffer.begin (), cbuffer.end ());
  EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
}

// NOLINTNEXTLINE
TEST (SmallVector, IteratorNonConstReverse) {
  peejay::small_vector<int, 4> buffer (std::size_t{4});
  std::iota (buffer.begin (), buffer.end (), 42);

  {
    std::vector<int> const actual (buffer.rbegin (), buffer.rend ());
    EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
  }
  {
    std::vector<int> const actual (buffer.rcbegin (), buffer.rcend ());
    EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
  }
}

// NOLINTNEXTLINE
TEST (SmallVector, IteratorConstReverse) {
  // Wrap the buffer construction code in a lambda to hide the non-const
  // small_vector instance.
  auto const& cbuffer = [] () {
    peejay::small_vector<int, 4> buffer (std::size_t{4});
    std::iota (std::begin (buffer), std::end (buffer),
               42);  // fill with increasing values
    return buffer;
  }();

  std::vector<int> actual (cbuffer.rbegin (), cbuffer.rend ());
  EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
}

// NOLINTNEXTLINE
TEST (SmallVector, ElementAccess) {
  peejay::small_vector<int, 4> buffer (std::size_t{4});
  int count = 42;
  // NOLINTNEXTLINE(modernize-loop-convert)
  for (std::size_t index = 0, end = buffer.size (); index != end; ++index) {
    buffer[index] = count++;
  }

  std::array<int, 4> const expected{{42, 43, 44, 45}};
  EXPECT_TRUE (std::equal (std::begin (buffer), std::end (buffer),
                           std::begin (expected)));
}

// NOLINTNEXTLINE
TEST (SmallVector, MoveSmall) {
  peejay::small_vector<int, 4> a (std::size_t{3});
  peejay::small_vector<int, 4> b (std::size_t{4});
  std::fill (std::begin (a), std::end (a), 0);
  std::fill (std::begin (b), std::end (b), 73);

  a = std::move (b);
  EXPECT_THAT (a, ElementsAre (73, 73, 73, 73));
}

// NOLINTNEXTLINE
TEST (SmallVector, MoveLarge) {
  // The two containers start out with different sizes; one uses the small
  // buffer, the other, large.
  peejay::small_vector<int, 3> a (std::size_t{0});
  peejay::small_vector<int, 3> b (std::size_t{4});
  std::fill (std::begin (a), std::end (a), 0);
  std::fill (std::begin (b), std::end (b), 73);

  a = std::move (b);
  EXPECT_THAT (a, ElementsAre (73, 73, 73, 73));
}

// NOLINTNEXTLINE
TEST (SmallVector, Clear) {
  // The two containers start out with different sizes; one uses the small
  // buffer, the other, large.
  peejay::small_vector<int> a (std::size_t{4});
  EXPECT_EQ (4U, a.size ());
  a.clear ();
  EXPECT_EQ (0U, a.size ());
}

// NOLINTNEXTLINE
TEST (SmallVector, PushBack) {
  using ::testing::ElementsAre;
  peejay::small_vector<int, 2> a;
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
TEST (SmallVector, EmplaceBack) {
  using ::testing::ElementsAre;
  peejay::small_vector<int, 2> a;
  a.emplace_back (1);
  EXPECT_THAT (a, ElementsAre (1));
  a.emplace_back (2);
  EXPECT_THAT (a, ElementsAre (1, 2));
  a.emplace_back (3);
  EXPECT_THAT (a, ElementsAre (1, 2, 3));
  a.emplace_back (4);
  EXPECT_THAT (a, ElementsAre (1, 2, 3, 4));
}

// NOLINTNEXTLINE
TEST (SmallVector, Front) {
  peejay::small_vector<int, 1> a;
  a.push_back (1);
  EXPECT_EQ (a.front (), 1);
  a.push_back (2);
  EXPECT_EQ (a.front (), 1);
  a.insert (a.begin (), 1, 3);
  EXPECT_EQ (a.front (), 3);
}
// NOLINTNEXTLINE
TEST (SmallVector, Back) {
  peejay::small_vector<int, 1> a;
  a.push_back (1);
  EXPECT_EQ (a.back (), 1);
  a.push_back (2);
  EXPECT_EQ (a.back (), 2);
}

// NOLINTNEXTLINE
TEST (SmallVector, AppendIteratorRange) {
  peejay::small_vector<int, 4> a (std::size_t{4});
  std::iota (std::begin (a), std::end (a), 0);

  std::array<int, 4> extra;
  std::iota (std::begin (extra), std::end (extra), 100);

  a.append (std::begin (extra), std::end (extra));

  EXPECT_THAT (a, ::testing::ElementsAre (0, 1, 2, 3, 100, 101, 102, 103));
}

// NOLINTNEXTLINE
TEST (SmallVector, CapacityReserve) {
  peejay::small_vector<int, 4> a;
  EXPECT_EQ (a.capacity (), 4U);
  a.reserve (1U);
  EXPECT_EQ (a.capacity (), 4U);
  a.reserve (10U);
  EXPECT_EQ (a.capacity (), 10U);
  a.reserve (1U);
  EXPECT_EQ (a.capacity (), 10U);
}

// NOLINTNEXTLINE
TEST (SmallVector, PopBack) {
  peejay::small_vector<int, 2> a{1, 2};
  a.pop_back ();
  EXPECT_THAT (a, testing::ElementsAre (1));
  a.pop_back ();
  EXPECT_TRUE (a.empty ());

  peejay::small_vector<int, 2> b{1, 2, 3};
  b.pop_back ();
  EXPECT_THAT (b, testing::ElementsAre (1, 2));
  b.pop_back ();
  EXPECT_THAT (b, testing::ElementsAre (1));
  b.pop_back ();
  EXPECT_TRUE (b.empty ());
}

#if __cpp_lib_ranges
// Verify that we can use small_vector<> with ranges algorithms.
// NOLINTNEXTLINE
TEST (SmallVector, RangeReverse) {
  peejay::small_vector<int, 3> sv{1, 2, 3};
  std::ranges::reverse (sv);
  EXPECT_THAT (sv, testing::ElementsAre (3, 2, 1));
}
#endif

template <typename TypeParam>
class SmallVectorErase : public testing::Test {};

using Sizes = testing::Types<std::integral_constant<std::size_t, 2>,
                             std::integral_constant<std::size_t, 3>,
                             std::integral_constant<std::size_t, 4>>;
TYPED_TEST_SUITE (SmallVectorErase, Sizes, );

// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, SinglePos) {
  peejay::small_vector<int, TypeParam::value> v{1, 2, 3};
  auto const l1 = v.erase (v.cbegin ());
  EXPECT_EQ (l1, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (2, 3));
  auto const l2 = v.erase (v.cbegin ());
  EXPECT_EQ (l2, v.begin ());
  EXPECT_THAT (v, testing::ElementsAre (3));
  auto const l3 = v.erase (v.cbegin ());
  EXPECT_EQ (l3, v.begin ());
  EXPECT_TRUE (v.empty ());
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, SingleSecondElement) {
  peejay::small_vector<int, TypeParam::value> v{1, 2, 3};
  auto const last = v.erase (v.begin () + 1);
  EXPECT_EQ (last, v.begin () + 1);
  EXPECT_THAT (v, testing::ElementsAre (1, 3));
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, SingleFinalElement) {
  peejay::small_vector<int, TypeParam::value> v{1, 2, 3};
  auto const last = v.erase (v.begin () + 2);
  EXPECT_EQ (last, v.begin () + 2);
  EXPECT_THAT (v, testing::ElementsAre (1, 2));
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, RangeAll) {
  peejay::small_vector<int, TypeParam::value> a{1, 2, 3};
  auto const last = a.erase (a.begin (), a.end ());
  EXPECT_EQ (last, a.end ());
  EXPECT_TRUE (a.empty ());
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, RangeFirstTwo) {
  peejay::small_vector<int, TypeParam::value> b{1, 2, 3};
  auto const first = b.begin ();
  auto const last = b.erase (first, first + 2);
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (3));
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, RangeFirstOnly) {
  peejay::small_vector<int, TypeParam::value> b{1, 2, 3};
  auto const first = b.begin ();
  auto const last = b.erase (first, first + 1);
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (2, 3));
}
// NOLINTNEXTLINE
TYPED_TEST (SmallVectorErase, RangeSecondToEnd) {
  peejay::small_vector<int, TypeParam::value> b{1, 2, 3};
  auto const first = b.begin () + 1;
  auto const last = b.erase (first, b.end ());
  EXPECT_EQ (last, first);
  EXPECT_THAT (b, testing::ElementsAre (1));
}

struct throws_on_cast_to_int {
  class ex : public std::runtime_error {
  public:
    ex () : std::runtime_error{"test exception"} {}
  };
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator int () const { throw ex{}; }
};

// NOLINTNEXTLINE
TEST (SmallVector, VariantIsValuelessByException) {
  peejay::small_vector<int, 4> sv{1, 2, 3, 4};
  EXPECT_EQ (sv.size (), sv.body_elements ())
      << "The 'small' container is full";
  // During the switch from 'small' to 'large', we will throw an exception
  // from the object being inserted. Ensure that the container state remains
  // valid in this case.
  // NOLINTNEXTLINE
  EXPECT_THROW (sv.emplace_back (throws_on_cast_to_int{}),
                throws_on_cast_to_int::ex);
  EXPECT_THAT (sv, testing::ElementsAre (1, 2, 3, 4));
}

// NOLINTNEXTLINE
TEST (SmallVector, Insert1AtSecondIndex) {
  peejay::small_vector<int, 8> v{1, 2, 3};
  auto const x = 4;
  v.insert (v.begin () + 1, 1, x);
  EXPECT_THAT (v, testing::ElementsAre (1, 4, 2, 3));
}
// NOLINTNEXTLINE
TEST (SmallVector, InsertN) {
  peejay::small_vector<int, 8> v{1, 2};
  auto const x = 3;
  v.insert (v.begin () + 1, 3, x);  // insert 3 copies of 'x' at [1].
  EXPECT_THAT (v, testing::ElementsAre (1, 3, 3, 3, 2));
}
// NOLINTNEXTLINE
TEST (SmallVector, InsertNAtEnd) {
  peejay::small_vector<int, 8> v{1, 2};

  auto const x = 3;
  v.insert (v.end (), 3, x);  // append 3 copies of 'x'.
  EXPECT_THAT (v, testing::ElementsAre (1, 2, 3, 3, 3));
}
