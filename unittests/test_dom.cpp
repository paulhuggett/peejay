//===- unittests/test_dom.cpp ---------------------------------------------===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include "peejay/dom.hpp"
#include "peejay/json.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using peejay::array;
using peejay::dom;
using peejay::element;
using peejay::error;
using peejay::mark;
using peejay::null;
using peejay::object;
using peejay::u8string;

using testing::ElementsAre;
using testing::UnorderedElementsAre;

// NOLINTNEXTLINE
TEST (Dom, MarkObjectsAllEqual) {
  EXPECT_TRUE (mark{} == mark{});
  EXPECT_FALSE (mark{} != mark{});
}

// NOLINTNEXTLINE
TEST (Dom, NullObjectsAllEqual) {
  EXPECT_TRUE (null{} == null{});
  EXPECT_FALSE (null{} != null{});
}

// NOLINTNEXTLINE
TEST (Dom, Null) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"null"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<null> (*root), null{});
}

// NOLINTNEXTLINE
TEST (Dom, One) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"1"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<uint64_t> (*root), 1U);
}

// NOLINTNEXTLINE
TEST (Dom, NegativeOne) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"-1"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<int64_t> (*root), -1);
}

// NOLINTNEXTLINE
TEST (Dom, String) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"("string")"sv).eof ();
  EXPECT_EQ (std::get<u8string> (*root), u8"string");
}

// NOLINTNEXTLINE
TEST (Dom, Double) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"3.14"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_DOUBLE_EQ (std::get<double> (*root), 3.14);
}

// NOLINTNEXTLINE
TEST (Dom, BooleanTrue) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"true"sv).eof ();
  EXPECT_TRUE (std::get<bool> (*root));
}

// NOLINTNEXTLINE
TEST (Dom, BooleanFalse) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"false"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_FALSE (std::get<bool> (*root));
}

// NOLINTNEXTLINE
TEST (Dom, Array) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8"[1,2]"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_THAT (std::get<array> (*root),
               ElementsAre (element{uint64_t{1}}, element{uint64_t{2}}));
}

// NOLINTNEXTLINE
TEST (Dom, Array2) {
  auto const src = u8R"(["\uFFFF"])"sv;
  auto p = make_parser (dom{});
  std::optional<element> const root = p.input (src).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  ASSERT_TRUE (root);
}

// NOLINTNEXTLINE
TEST (Dom, Object) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"({"a":1,"b":2})"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_THAT (
      std::get<object> (*root),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{uint64_t{1}}),
                            std::make_pair (u8"b"s, element{uint64_t{2}})));
}

// NOLINTNEXTLINE
TEST (Dom, ObjectInsideArray1) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"([{"a":1,"b":2},3])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (
      std::get<object> (arr[0]),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{uint64_t{1}}),
                            std::make_pair (u8"b"s, element{uint64_t{2}})));
  EXPECT_THAT (arr[1], element{uint64_t{3}});
}

// NOLINTNEXTLINE
TEST (Dom, ObjectInsideArray2) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"([1,{"a":2,"b":3}])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (arr[0], element{uint64_t{1}});
  EXPECT_THAT (
      std::get<object> (arr[1]),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{uint64_t{2}}),
                            std::make_pair (u8"b"s, element{uint64_t{3}})));
}

// NOLINTNEXTLINE
TEST (Dom, ArrayInsideObject) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"({"a":[1,2],"b":3})"sv).eof ();
  ASSERT_TRUE (root);
  auto const &obj = std::get<object> (*root);
  ASSERT_EQ (obj.size (), 2U);
  EXPECT_THAT (std::get<array> (obj.at (u8"a")),
               ElementsAre (element{uint64_t{1}}, element{uint64_t{2}}));
  EXPECT_EQ (obj.at (u8"b"), element{uint64_t{3}});
}

// NOLINTNEXTLINE
TEST (Dom, DuplicateKeys) {
  auto p = make_parser (dom{});
  p.input (u8R"({"a":"b","a":"c"})"sv);
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  std::optional<element> const root = p.eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_THAT (std::get<object> (*root),
               UnorderedElementsAre (std::make_pair (u8"a"s, element{u8"c"s})));
}

// NOLINTNEXTLINE
TEST (Dom, ArrayStack) {
  dom<size_t{2}> d;
  EXPECT_FALSE (static_cast<bool> (d.begin_array ()));
  EXPECT_FALSE (static_cast<bool> (d.begin_array ()));

  auto const err = make_error_code (error::dom_nesting_too_deep);
  EXPECT_EQ (d.string_value (u8"string"sv), err);
  EXPECT_EQ (d.int64_value (int64_t{37}), err);
  EXPECT_EQ (d.uint64_value (uint64_t{37}), err);
  EXPECT_EQ (d.double_value (37.9), err);
  EXPECT_EQ (d.boolean_value (true), err);
  EXPECT_EQ (d.null_value (), err);

  EXPECT_EQ (d.begin_array (), err);
  EXPECT_EQ (d.begin_object (), err);
  EXPECT_EQ (d.key (u8"key"sv), err);
}
