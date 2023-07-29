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
using testing::Test;
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
  EXPECT_EQ (std::get<std::int64_t> (*root), 1U);
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
  EXPECT_THAT (
      *std::get<array> (*root),
      ElementsAre (element{std::int64_t{1}}, element{std::int64_t{2}}));
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
      *std::get<object> (*root),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{std::int64_t{1}}),
                            std::make_pair (u8"b"s, element{std::int64_t{2}})));
}

// NOLINTNEXTLINE
TEST (Dom, ObjectInsideArray1) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"([{"a":1,"b":2},3])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr->size (), 2U);
  EXPECT_THAT (
      *std::get<object> ((*arr)[0]),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{std::int64_t{1}}),
                            std::make_pair (u8"b"s, element{std::int64_t{2}})));
  EXPECT_THAT ((*arr)[1], element{std::int64_t{3}});
}

// NOLINTNEXTLINE
TEST (Dom, ObjectInsideArray2) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"([1,{"a":2,"b":3}])"sv).eof ();
  ASSERT_TRUE (root);
  auto const &arr = std::get<array> (*root);
  ASSERT_EQ (arr->size (), 2U);
  EXPECT_THAT ((*arr)[0], element{std::int64_t{1}});
  EXPECT_THAT (
      *std::get<object> ((*arr)[1]),
      UnorderedElementsAre (std::make_pair (u8"a"s, element{std::int64_t{2}}),
                            std::make_pair (u8"b"s, element{std::int64_t{3}})));
}

// NOLINTNEXTLINE
TEST (Dom, ArrayInsideObject) {
  std::optional<element> const root =
      make_parser (dom{}).input (u8R"({"a":[1,2],"b":3})"sv).eof ();
  ASSERT_TRUE (root);
  auto const &obj = std::get<object> (*root);
  ASSERT_EQ (obj->size (), 2U);
  EXPECT_THAT (
      *std::get<array> (obj->at (u8"a")),
      ElementsAre (element{std::int64_t{1}}, element{std::int64_t{2}}));
  EXPECT_EQ (obj->at (u8"b"), element{std::int64_t{3}});
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
  EXPECT_THAT (*std::get<object> (*root),
               UnorderedElementsAre (std::make_pair (u8"a"s, element{u8"c"s})));
}

// NOLINTNEXTLINE
TEST (Dom, ArrayStack) {
  dom<size_t{2}> d;
  EXPECT_FALSE (static_cast<bool> (d.begin_array ()));
  EXPECT_FALSE (static_cast<bool> (d.begin_array ()));

  auto const err = make_error_code (error::dom_nesting_too_deep);
  EXPECT_EQ (d.string_value (u8"string"sv), err);
  EXPECT_EQ (d.integer_value (std::int64_t{37}), err);
  EXPECT_EQ (d.double_value (37.9), err);
  EXPECT_EQ (d.boolean_value (true), err);
  EXPECT_EQ (d.null_value (), err);

  EXPECT_EQ (d.begin_array (), err);
  EXPECT_EQ (d.begin_object (), err);
  EXPECT_EQ (d.key (u8"key"sv), err);
}

TEST (Element, EqObject) {
  std::optional<element> const a =
      make_parser (dom{}).input (u8R"({"a":[1,2,3]})"sv).eof ();
  std::optional<element> const b =
      make_parser (dom{}).input (u8R"({"a":[1,2,3]})"sv).eof ();
  ASSERT_TRUE (a);
  ASSERT_TRUE (b);
  EXPECT_TRUE (*a == *b);
}
TEST (Element, EqObjectArraysOfDifferentLength) {
  std::optional<element> const a =
      make_parser (dom{}).input (u8R"({"a":[1,2,3]})"sv).eof ();
  std::optional<element> const b =
      make_parser (dom{}).input (u8R"({"a":[1,2,3,4]})"sv).eof ();
  ASSERT_TRUE (a);
  ASSERT_TRUE (b);
  EXPECT_FALSE (*a == *b);
}
TEST (Element, EqObjectDifferentProperties) {
  std::optional<element> const a =
      make_parser (dom{}).input (u8R"({"a":[1,2,3]})"sv).eof ();
  std::optional<element> const b =
      make_parser (dom{}).input (u8R"({"b":[1,2,3]})"sv).eof ();
  ASSERT_TRUE (a);
  ASSERT_TRUE (b);
  EXPECT_FALSE (*a == *b);
}
TEST (Element, EqArray) {
  std::optional<element> const a =
      make_parser (dom{}).input (u8R"([{"a":1},2])"sv).eof ();
  std::optional<element> const b =
      make_parser (dom{}).input (u8R"([{"a":1},2])"sv).eof ();
  ASSERT_TRUE (a);
  ASSERT_TRUE (b);
  EXPECT_TRUE (*a == *b);
}

// The tests from RFC6901 (April 2013) paragraph 5.
class PointerRFC6901 : public Test {
public:
  void SetUp () override {
    root_ = &(doc_.value ());
    ASSERT_TRUE (std::holds_alternative<object> (*root_));
    obj_ = std::get<object> (*root_);
  }

protected:
  std::optional<element> doc_ = make_parser (dom{})
                                    .input (
                                        u8R"(
 {
  "foo": ["bar", "baz"],
  "": 0,
  "a/b": 1,
  "c%d": 2,
  "e^f": 3,
  "g|h": 4,
  "i\\j": 5,
  "k\"l": 6,
  " ": 7,
  "m~n": 8
 }
)"sv)
                                    .eof ();
  element *root_ = nullptr;
  object obj_;  // The object contained by document root element.
};

TEST_F (PointerRFC6901, Empty) {
  EXPECT_EQ (root_->eval_pointer (u8""), root_);
}
TEST_F (PointerRFC6901, SlashFoo) {
  auto const &foo_value = (*obj_)[u8"foo"];
  EXPECT_EQ (root_->eval_pointer (u8"/foo"), &foo_value);
}
TEST_F (PointerRFC6901, SlashFooZero) {
  auto const &foo_value = (*obj_)[u8"foo"];
  ASSERT_TRUE (std::holds_alternative<array> (foo_value));
  auto const &foo_array = std::get<array> (foo_value);
  EXPECT_EQ (root_->eval_pointer (u8"/foo/0"), &(*foo_array)[0]);
}
TEST_F (PointerRFC6901, SlashEmpty) {
  auto const &empty_key_value = (*obj_)[u8""];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (empty_key_value));
  EXPECT_EQ (root_->eval_pointer (u8"/"), &empty_key_value);
}
TEST_F (PointerRFC6901, SlashASlashB) {
  auto const &value = (*obj_)[u8"a/b"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/a~1b"), &value);
}
TEST_F (PointerRFC6901, SlashCPercentD) {
  auto const &value = (*obj_)[u8"c%d"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/c%d"), &value);
}
TEST_F (PointerRFC6901, SlashECircumflexF) {
  auto const &value = (*obj_)[u8"e^f"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/e^f"), &value);
}
TEST_F (PointerRFC6901, SlashGBarH) {
  auto const &value = (*obj_)[u8"g|h"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/g|h"), &value);
}
TEST_F (PointerRFC6901, SlashIBackslashJ) {
  auto const &value = (*obj_)[u8R"(i\j)"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8R"(/i\j)"), &value);
}
TEST_F (PointerRFC6901, SlashKQuoteL) {
  auto const &value = (*obj_)[u8R"(k"l)"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8R"(/k"l)"), &value);
}
TEST_F (PointerRFC6901, SlashSpace) {
  auto const &value = (*obj_)[u8" "];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/ "), &value);
}
TEST_F (PointerRFC6901, MTildeN) {
  auto const &value = (*obj_)[u8"m~n"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/m~0n"), &value);
}
