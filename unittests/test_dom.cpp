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

#include "json/dom.hpp"
#include "json/json.hpp"

using namespace peejay;
using namespace std::string_literals;
using namespace std::string_view_literals;

using testing::ElementsAre;
using testing::UnorderedElementsAre;

TEST (Dom, MarkObjectsAllEqual) {
  EXPECT_TRUE (mark{} == mark{});
  EXPECT_FALSE (mark{} != mark{});
}

TEST (Dom, NullObjectsAllEqual) {
  EXPECT_TRUE (null{} == null{});
  EXPECT_FALSE (null{} != null{});
}

TEST (Dom, Null) {
  std::optional<element> const root =
      make_parser (dom{}).input ("null"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<null> (*root), null{});
}

TEST (Dom, One) {
  std::optional<element> const root = make_parser (dom{}).input ("1"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<uint64_t> (*root), 1U);
}

TEST (Dom, NegativeOne) {
  std::optional<element> const root = make_parser (dom{}).input ("-1"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_EQ (std::get<int64_t> (*root), -1);
}

TEST (Dom, String) {
  std::optional<element> const root =
      make_parser (dom{}).input (R"("string")"sv).eof ();
  EXPECT_EQ (std::get<std::string> (*root), "string");
}

TEST (Dom, Double) {
  std::optional<element> const root =
      make_parser (dom{}).input ("3.14"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_DOUBLE_EQ (std::get<double> (*root), 3.14);
}

TEST (Dom, BooleanTrue) {
  std::optional<element> const root =
      make_parser (dom{}).input ("true"sv).eof ();
  EXPECT_TRUE (std::get<bool> (*root));
}

TEST (Dom, BooleanFalse) {
  std::optional<element> const root =
      make_parser (dom{}).input ("false"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_FALSE (std::get<bool> (*root));
}

TEST (Dom, Array) {
  std::optional<element> const root =
      make_parser (dom{}).input ("[1,2]"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_THAT (std::get<array> (*root),
               ElementsAre (element{uint64_t{1}}, element{uint64_t{2}}));
}

TEST (Dom, Array2) {
  auto const src = R"(["\uFFFF"])"sv;
  auto p = make_parser (dom{});
  std::optional<element> const root = p.input (src).eof ();
  EXPECT_FALSE (p.has_error ());
  ASSERT_TRUE (root);
}

TEST (Dom, Object) {
  std::optional<element> const root =
      make_parser (dom{}).input (R"({"a":1,"b":2})"sv).eof ();
  ASSERT_TRUE (root);
  EXPECT_THAT (
      std::get<object> (*root),
      UnorderedElementsAre (std::make_pair ("a"s, element{uint64_t{1}}),
                            std::make_pair ("b"s, element{uint64_t{2}})));
}

TEST (Dom, ObjectInsideArray1) {
  std::optional<element> const root =
      make_parser (dom{}).input (R"([{"a":1,"b":2},3])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (
      std::get<object> (arr[0]),
      UnorderedElementsAre (std::make_pair ("a"s, element{uint64_t{1}}),
                            std::make_pair ("b"s, element{uint64_t{2}})));
  EXPECT_THAT (arr[1], element{uint64_t{3}});
}

TEST (Dom, ObjectInsideArray2) {
  std::optional<element> const root =
      make_parser (dom{}).input (R"([1,{"a":2,"b":3}])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (arr[0], element{uint64_t{1}});
  EXPECT_THAT (
      std::get<object> (arr[1]),
      UnorderedElementsAre (std::make_pair ("a"s, element{uint64_t{2}}),
                            std::make_pair ("b"s, element{uint64_t{3}})));
}

TEST (Dom, ArrayInsideObject) {
  std::optional<element> const root =
      make_parser (dom{}).input (R"({"a":[1,2],"b":3})"sv).eof ();
  ASSERT_TRUE (root);
  auto const &obj = std::get<object> (*root);
  ASSERT_EQ (obj.size (), 2U);
  EXPECT_THAT (std::get<array> (obj.at ("a")),
               ElementsAre (element{uint64_t{1}}, element{uint64_t{2}}));
  EXPECT_EQ (obj.at ("b"), element{uint64_t{3}});
}

TEST (Dom, DuplicateKeys) {
  auto p = make_parser (dom{});
  p.input (R"({"a":"b","a":"c"})"sv);
  EXPECT_FALSE (p.has_error ());
  std::optional<element> const root = p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_THAT (std::get<object> (*root),
               UnorderedElementsAre (std::make_pair ("a"s, element{"c"s})));
}

TEST (Dom, ArrayStack) {
  dom<size_t{2}> d;
  EXPECT_EQ (d.begin_array (), std::error_code{});
  EXPECT_EQ (d.begin_array (), std::error_code{});

  auto const err = make_error_code (dom_error::nesting_too_deep);
  EXPECT_EQ (d.string_value ("string"sv), err);
  EXPECT_EQ (d.int64_value (int64_t{37}), err);
  EXPECT_EQ (d.uint64_value (uint64_t{37}), err);
  EXPECT_EQ (d.double_value (37.9), err);
  EXPECT_EQ (d.boolean_value (true), err);
  EXPECT_EQ (d.null_value (), err);

  EXPECT_EQ (d.begin_array (), err);
  EXPECT_EQ (d.begin_object (), err);
  EXPECT_EQ (d.key ("key"sv), err);
}
