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
using peejay::u8string_view;

using testing::_;
using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::Field;
using testing::Optional;
using testing::Test;
using testing::UnorderedElementsAre;
using testing::VariantWith;

namespace {

std::optional<element> parse (u8string_view s) {
  return make_parser (dom{}).input (s).eof ();
}

}  // end anonymous namespace

class Dom : public Test {
protected:
  element *element::*object_parent_field =
      &object::element_type::value_type::second_type::parent;
  element *element::*parent_field = &element::parent;
};

// NOLINTNEXTLINE
TEST_F (Dom, MarkObjectsAllEqual) {
  EXPECT_TRUE (mark{} == mark{});
  EXPECT_FALSE (mark{} != mark{});
}
// NOLINTNEXTLINE
TEST_F (Dom, NullObjectsAllEqual) {
  EXPECT_TRUE (null{} == null{});
  EXPECT_FALSE (null{} != null{});
}
// NOLINTNEXTLINE
TEST_F (Dom, Null) {
  std::optional<element> const root = parse (u8"null"sv);
  ASSERT_THAT (root, Optional (VariantWith<null> (_)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, One) {
  std::optional<element> const root = parse (u8"1"sv);
  ASSERT_THAT (root, Optional (VariantWith<std::int64_t> (1)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, NegativeOne) {
  std::optional<element> const root = parse (u8"-1"sv);
  ASSERT_THAT (root, Optional (VariantWith<std::int64_t> (-1)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, String) {
  std::optional<element> const root = parse (u8R"("string")"sv);
  ASSERT_THAT (root, Optional (VariantWith<u8string> (u8"string"s)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, Double) {
  std::optional<element> const root = parse (u8"3.14"sv);
  ASSERT_THAT (root, Optional (VariantWith<double> (3.14)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, BooleanTrue) {
  std::optional<element> const root = parse (u8"true"sv);
  ASSERT_THAT (root, Optional (VariantWith<bool> (true)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, BooleanFalse) {
  std::optional<element> const root = parse (u8"false"sv);
  ASSERT_THAT (root, Optional (VariantWith<bool> (false)));
  EXPECT_EQ (root->parent, nullptr);
}
// NOLINTNEXTLINE
TEST_F (Dom, Array) {
  std::optional<element> const root = parse (u8"[1,2]"sv);
  // Check the array contents.
  ASSERT_THAT (root, Optional (VariantWith<array> (_)));
  auto const &arr = *std::get<array> (*root);
  ASSERT_THAT (arr, ElementsAre (VariantWith<std::int64_t> (1),
                                 VariantWith<std::int64_t> (2)));
  // Check the parent pointers.
  EXPECT_EQ (root->parent, nullptr);
  EXPECT_THAT (arr, Each (Field ("parent", parent_field, Eq (&root.value ()))));
}
// NOLINTNEXTLINE
TEST_F (Dom, Array2) {
  std::optional<element> const root = parse (u8R"(["\uFFFD"])"sv);
  ASSERT_THAT (root, Optional (VariantWith<array> (_)));
  auto const &arr = *std::get<array> (*root);
  // Check the array contents.
  ASSERT_THAT (arr, ElementsAre (VariantWith<u8string> (u8"\xEF\xBF\xBD"s)));
  // Check the parent pointers.
  EXPECT_EQ (root->parent, nullptr);
  EXPECT_THAT (arr, Each (Field ("parent", parent_field, Eq (&root.value ()))));
}
// NOLINTNEXTLINE
TEST_F (Dom, Object) {
  std::optional<element> const root = parse (u8R"({"a":1,"b":2})"sv);
  ASSERT_THAT (root, Optional (VariantWith<object> (_)));
  auto const &root_element = *std::get<object> (*root);
  EXPECT_THAT (root_element, UnorderedElementsAre (
                                 Pair (u8"a"s, VariantWith<std::int64_t> (1)),
                                 Pair (u8"b"s, VariantWith<std::int64_t> (2))));
  // Check the parent pointers.
  EXPECT_EQ (root->parent, nullptr);
  // Object members have the root object as parent.
  EXPECT_THAT (root_element,
               Each (Pair (_, Field ("parent", object_parent_field,
                                     Eq (&root.value ())))));
}
// NOLINTNEXTLINE
TEST_F (Dom, ObjectInsideArray1) {
  std::optional<element> const root = parse (u8R"([{"a":1,"b":2},3])"sv);
  ASSERT_THAT (root, Optional (VariantWith<array> (_)));
  auto const &arr = *std::get<array> (*root);
  ASSERT_THAT (arr, ElementsAre (VariantWith<object> (_),
                                 VariantWith<std::int64_t> (3)));
  EXPECT_THAT (
      *std::get<object> (arr.at (0)),
      UnorderedElementsAre (Pair (u8"a"s, VariantWith<std::int64_t> (1)),
                            Pair (u8"b"s, VariantWith<std::int64_t> (2))));

  // The array elements have the root object as parent.
  EXPECT_THAT (arr, Each (Field ("parent", parent_field, Eq (&root.value ()))));
  // arr[0] members have the arr[0] object as parent.
  EXPECT_THAT (
      *std::get<object> (arr.at (0)),
      Each (Pair (_, Field ("parent", object_parent_field, Eq (&arr.at (0))))));
}
// NOLINTNEXTLINE
TEST_F (Dom, ObjectInsideArray2) {
  std::optional<element> const root = parse (u8R"([1,{"a":2,"b":3}])"sv);
  ASSERT_THAT (root, Optional (VariantWith<array> (_)));
  auto const &arr = *std::get<array> (*root);
  ASSERT_THAT (arr, ElementsAre (VariantWith<std::int64_t> (1),
                                 VariantWith<object> (_)));
  auto const &ind1 = *std::get<object> (arr.at (1));
  EXPECT_THAT (ind1, UnorderedElementsAre (
                         Pair (u8"a"s, VariantWith<std::int64_t> (2)),
                         Pair (u8"b"s, VariantWith<std::int64_t> (3))));

  // The array elements have the root object as parent.
  EXPECT_THAT (arr, Each (Field ("parent", parent_field, Eq (&root.value ()))));
  // arr[1] members have the arr[0] object as parent.
  EXPECT_THAT (
      ind1,
      Each (Pair (_, Field ("parent", object_parent_field, Eq (&arr.at (1))))));
}

// NOLINTNEXTLINE
TEST_F (Dom, ArrayInsideObject) {
  std::optional<element> const root = parse (u8R"({"a":[1,2],"b":3})"sv);
  ASSERT_THAT (root, Optional (VariantWith<object> (_)));
  auto const &obj = *std::get<object> (*root);
  ASSERT_THAT (
      obj, UnorderedElementsAre (Pair (u8"a"s, VariantWith<array> (_)),
                                 Pair (u8"b"s, VariantWith<std::int64_t> (3))));
  auto const &arr = *std::get<array> (obj.find (u8"a")->second);
  ASSERT_THAT (arr, ElementsAre (VariantWith<std::int64_t> (1),
                                 VariantWith<std::int64_t> (2)));
}
// NOLINTNEXTLINE
TEST_F (Dom, DuplicateKeys) {
  std::optional<element> const root = parse (u8R"({"a":"b","a":"c"})"sv);
  ASSERT_THAT (root, Optional (VariantWith<object> (_)));
  auto const &obj = *std::get<object> (*root);
  EXPECT_THAT (obj, UnorderedElementsAre (
                        Pair (u8"a"s, VariantWith<u8string> (u8"c"s))));
}
// NOLINTNEXTLINE
TEST_F (Dom, ArrayStack) {
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

// NOLINTNEXTLINE
TEST (Element, EqObject) {
  std::optional<element> const a = parse (u8R"({"a":[1,2,3]})"sv);
  std::optional<element> const b = parse (u8R"({"a":[1,2,3]})"sv);
  ASSERT_THAT (a, Optional (_));
  ASSERT_THAT (b, Optional (_));
  EXPECT_TRUE (*a == *b);
}

// NOLINTNEXTLINE
TEST (Element, EqObjectArraysOfDifferentLength) {
  std::optional<element> const a = parse (u8R"({"a":[1,2,3]})"sv);
  std::optional<element> const b = parse (u8R"({"a":[1,2,3,4]})"sv);
  ASSERT_THAT (a, Optional (_));
  ASSERT_THAT (b, Optional (_));
  EXPECT_FALSE (*a == *b);
}
// NOLINTNEXTLINE
TEST (Element, EqObjectDifferentProperties) {
  std::optional<element> const a = parse (u8R"({"a":[1,2,3]})"sv);
  std::optional<element> const b = parse (u8R"({"b":[1,2,3]})"sv);
  ASSERT_THAT (a, Optional (_));
  ASSERT_THAT (b, Optional (_));
  EXPECT_FALSE (*a == *b);
}
// NOLINTNEXTLINE
TEST (Element, EqArray) {
  std::optional<element> const a = parse (u8R"([{"a":1},2])"sv);
  std::optional<element> const b = parse (u8R"([{"a":1},2])"sv);
  ASSERT_THAT (a, Optional (_));
  ASSERT_THAT (b, Optional (_));
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

// NOLINTNEXTLINE
TEST_F (PointerRFC6901, Empty) {
  EXPECT_EQ (root_->eval_pointer (u8""), root_);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashFoo) {
  auto const &foo_value = (*obj_)[u8"foo"];
  EXPECT_EQ (root_->eval_pointer (u8"/foo"), &foo_value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashFooZero) {
  auto const &foo_value = (*obj_)[u8"foo"];
  ASSERT_TRUE (std::holds_alternative<array> (foo_value));
  auto const &foo_array = std::get<array> (foo_value);
  EXPECT_EQ (root_->eval_pointer (u8"/foo/0"), &(*foo_array)[0]);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashEmpty) {
  auto const &empty_key_value = (*obj_)[u8""];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (empty_key_value));
  EXPECT_EQ (root_->eval_pointer (u8"/"), &empty_key_value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashASlashB) {
  auto const &value = (*obj_)[u8"a/b"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/a~1b"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashCPercentD) {
  auto const &value = (*obj_)[u8"c%d"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/c%d"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashECircumflexF) {
  auto const &value = (*obj_)[u8"e^f"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/e^f"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashGBarH) {
  auto const &value = (*obj_)[u8"g|h"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/g|h"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashIBackslashJ) {
  auto const &value = (*obj_)[u8R"(i\j)"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8R"(/i\j)"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashKQuoteL) {
  auto const &value = (*obj_)[u8R"(k"l)"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8R"(/k"l)"), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, SlashSpace) {
  auto const &value = (*obj_)[u8" "];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/ "), &value);
}
// NOLINTNEXTLINE
TEST_F (PointerRFC6901, MTildeN) {
  auto const &value = (*obj_)[u8"m~n"];
  ASSERT_TRUE (std::holds_alternative<std::int64_t> (value));
  EXPECT_EQ (root_->eval_pointer (u8"/m~0n"), &value);
}
