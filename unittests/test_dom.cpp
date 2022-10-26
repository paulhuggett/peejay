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

TEST (Dom, MarkObjectsAllEqual) {
  EXPECT_TRUE (dom::mark{} == dom::mark{});
  EXPECT_FALSE (dom::mark{} != dom::mark{});
}

TEST (Dom, NullObjectsAllEqual) {
  EXPECT_TRUE (dom::null{} == dom::null{});
  EXPECT_FALSE (dom::null{} != dom::null{});
}

TEST (Dom, Null) {
  dom::element const root = make_parser (dom{}).input ("null"sv).eof ();
  EXPECT_EQ (std::get<dom::null> (root), dom::null{});
}

TEST (Dom, One) {
  dom::element const root = make_parser (dom{}).input ("1"sv).eof ();
  EXPECT_EQ (std::get<uint64_t> (root), 1U);
}

TEST (Dom, NegativeOne) {
  dom::element const root = make_parser (dom{}).input ("-1"sv).eof ();
  EXPECT_EQ (std::get<int64_t> (root), -1);
}

TEST (Dom, String) {
  dom::element const root = make_parser (dom{}).input (R"("string")"sv).eof ();
  EXPECT_EQ (std::get<std::string> (root), "string");
}

TEST (Dom, Double) {
  dom::element const root = make_parser (dom{}).input ("3.14"sv).eof ();
  EXPECT_DOUBLE_EQ (std::get<double> (root), 3.14);
}

TEST (Dom, BooleanTrue) {
  dom::element const root = make_parser (dom{}).input ("true"sv).eof ();
  EXPECT_TRUE (std::get<bool> (root));
}

TEST (Dom, BooleanFalse) {
  dom::element const root = make_parser (dom{}).input ("false"sv).eof ();
  EXPECT_FALSE (std::get<bool> (root));
}

TEST (Dom, Array) {
  using testing::ElementsAre;
  dom::element const root = make_parser (dom{}).input ("[1,2]"sv).eof ();
  EXPECT_THAT (
      std::get<dom::array> (root),
      ElementsAre (dom::element{uint64_t{1}}, dom::element{uint64_t{2}}));
}

TEST (Dom, Object) {
  using testing::UnorderedElementsAre;
  dom::element const root =
      make_parser (dom{}).input (R"({"a":1,"b":2})"sv).eof ();
  EXPECT_THAT (
      std::get<dom::object> (root),
      UnorderedElementsAre (std::make_pair ("a"s, dom::element{uint64_t{1}}),
                            std::make_pair ("b"s, dom::element{uint64_t{2}})));
}

TEST (Dom, ObjectInsideArray1) {
  using testing::UnorderedElementsAre;
  auto const arr = std::get<dom::array> (
      make_parser (dom{}).input (R"([{"a":1,"b":2},3])"sv).eof ());
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (
      std::get<dom::object> (arr[0]),
      UnorderedElementsAre (std::make_pair ("a"s, dom::element{uint64_t{1}}),
                            std::make_pair ("b"s, dom::element{uint64_t{2}})));
  EXPECT_THAT (arr[1], dom::element{uint64_t{3}});
}

TEST (Dom, ObjectInsideArray2) {
  using testing::UnorderedElementsAre;
  auto const arr = std::get<dom::array> (
      make_parser (dom{}).input (R"([1,{"a":2,"b":3}])"sv).eof ());
  ASSERT_EQ (arr.size (), 2U);
  EXPECT_THAT (arr[0], dom::element{uint64_t{1}});
  EXPECT_THAT (
      std::get<dom::object> (arr[1]),
      UnorderedElementsAre (std::make_pair ("a"s, dom::element{uint64_t{2}}),
                            std::make_pair ("b"s, dom::element{uint64_t{3}})));
}

TEST (Dom, ArrayInsideObject) {
  using testing::ElementsAre;
  auto const obj = std::get<dom::object> (
      make_parser (dom{}).input (R"({"a":[1,2],"b":3})"sv).eof ());
  ASSERT_EQ (obj.size (), 2U);
  EXPECT_THAT (
      std::get<dom::array> (obj.at ("a")),
      ElementsAre (dom::element{uint64_t{1}}, dom::element{uint64_t{2}}));
  EXPECT_EQ (obj.at ("b"), dom::element{uint64_t{3}});
}
