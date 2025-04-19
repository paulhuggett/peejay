//===- unit_tests/test_dom.cpp --------------------------------------------===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
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
#include <gmock/gmock.h>

#include "callbacks.hpp"
#include "peejay/dom.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using peejay::error;
using peejay::dom::dom;
using peejay::dom::element;

using testing::_;
using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::Field;
using testing::Optional;
using testing::Test;
using testing::UnorderedElementsAre;

template <typename T> class ElementMatcher {
public:
  explicit ElementMatcher(::testing::Matcher<T const&> matcher) : matcher_{std::move(matcher)} {}

  template <typename Element>
  bool MatchAndExplain(Element const& value, ::testing::MatchResultListener* const listener) const {
    using std::get;
    if (!listener->IsInterested()) {
      if (T const* const t = value.template get_if<T>(); t != nullptr) {
        return matcher_.Matches(*t);
      }
      return false;
    }

    if (!value.template holds<T>()) {
      *listener << "whose value is not of type '" << GetTypeName() << "'";
      return false;
    }

    T const* const elem = value.template get_if<T>();
    testing::StringMatchResultListener elem_listener;
    bool const match = matcher_.MatchAndExplain(*elem, &elem_listener);
    *listener << "whose value " << testing::PrintToString(*elem) << (match ? " matches" : " doesn't match");

    if (std::string const explanation = elem_listener.str(); !explanation.empty()) {
      *listener << ", " << explanation;
    }

    return match;
  }

  void DescribeTo(std::ostream* const os) const {
    *os << "is a element with value of type '" << GetTypeName() << "' and the value ";
    matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* const os) const {
    *os << "is an element with value of type other than '" << GetTypeName() << "' or the value ";
    matcher_.DescribeNegationTo(os);
  }

private:
  static std::string GetTypeName() {
    if constexpr (GTEST_HAS_RTTI) {
      return testing::internal::GetTypeName<T>();
    } else {
      return "the element type";
    }
  }

  const ::testing::Matcher<T const&> matcher_;
};

template <typename T>
testing::PolymorphicMatcher<ElementMatcher<T>> ElementWith(testing::Matcher<T const&> const& matcher) {
  return testing::MakePolymorphicMatcher(ElementMatcher<T>(matcher));
}

namespace {

template <peejay::policy Policies = peejay::default_policies>
std::optional<element<Policies>> parse(std::basic_string_view<typename Policies::char_type> s) {
  auto p = make_parser(dom<Policies>{});
  return input(p, s).eof();
}

}  // end anonymous namespace

class Dom : public Test {
protected:
  using policies = peejay::default_policies;
  using el = element<policies>;
  using object = peejay::dom::element<policies>::object;
  using array = peejay::dom::element<policies>::array;
  using string = std::basic_string<typename policies::char_type>;
  using null = peejay::dom::null;

  //  el *el::*object_parent_field = &object::element_type::value_type::second_type::parent;
  //  el *el::*parent_field = &el::parent;
};

// NOLINTNEXTLINE
TEST_F(Dom, NullObjectsAllEqual) {
  EXPECT_TRUE(null{} == null{});
  EXPECT_FALSE(null{} != null{});
}
// NOLINTNEXTLINE
TEST_F(Dom, Null) {
  auto const root = parse(u8"null"sv);
  ASSERT_THAT(root, Optional(ElementWith<null>(_)));
}
// NOLINTNEXTLINE
TEST_F(Dom, One) {
  auto const root = parse(u8"1"sv);
  ASSERT_THAT(root, Optional(ElementWith<std::int64_t>(1)));
}
// NOLINTNEXTLINE
TEST_F(Dom, NegativeOne) {
  auto const root = parse(u8"-1"sv);
  ASSERT_THAT(root, Optional(ElementWith<std::int64_t>(-1)));
}
// NOLINTNEXTLINE
TEST_F(Dom, String) {
  auto const root = parse(u8R"("string")"sv);
  ASSERT_THAT(root, Optional(ElementWith<string>(u8"string"s)));
}
// NOLINTNEXTLINE
TEST_F(Dom, Double) {
  auto const root = parse(u8"3.14"sv);
  ASSERT_THAT(root, Optional(ElementWith<double>(3.14)));
}
// NOLINTNEXTLINE
TEST_F(Dom, BooleanTrue) {
  auto const root = parse(u8"true"sv);
  ASSERT_THAT(root, Optional(ElementWith<bool>(true)));
}
// NOLINTNEXTLINE
TEST_F(Dom, BooleanFalse) {
  auto const root = parse(u8"false"sv);
  ASSERT_THAT(root, Optional(ElementWith<bool>(false)));
}
// NOLINTNEXTLINE
TEST_F(Dom, Array) {
  auto const root = parse(u8"[1,2]"sv);
  // Check the array contents.
  ASSERT_THAT(root, Optional(ElementWith<array>(_)));
  auto const* const arr = root->get_if<array>();
  ASSERT_NE(arr, nullptr);
  ASSERT_THAT(*arr, ElementsAre(ElementWith<std::int64_t>(1), ElementWith<std::int64_t>(2)));
}
// NOLINTNEXTLINE
TEST_F(Dom, Array2) {
  auto const root = parse(u8R"(["\uFFFD"])"sv);
  ASSERT_THAT(root, Optional(ElementWith<array>(_)));
  auto const* const arr = root->get_if<array>();
  ASSERT_NE(arr, nullptr);
  // Check the array contents.
  std::array<std::byte, 4> const expected_bytes = {{
      std::byte{0xEF}, std::byte{0xBF}, std::byte{0xBD},  // REPLACEMENT CHARACTER
      std::byte{0x00}                                     // NULL
  }};
  ASSERT_THAT(*arr, ElementsAre(ElementWith<string>(string{reinterpret_cast<char8_t const*>(expected_bytes.data())})));
}
// NOLINTNEXTLINE
TEST_F(Dom, Object) {
  auto const root = parse(u8R"({"a":1,"b":2})"sv);
  ASSERT_THAT(root, Optional(ElementWith<object>(_)));
  auto const* const root_element = root->get_if<object>();
  ASSERT_NE(root_element, nullptr);
  EXPECT_THAT(*root_element, UnorderedElementsAre(Pair(u8"a"s, ElementWith<std::int64_t>(1)),
                                                  Pair(u8"b"s, ElementWith<std::int64_t>(2))));
}
// NOLINTNEXTLINE
TEST_F(Dom, ObjectInsideArray1) {
  auto const root = parse(u8R"([{"a":1,"b":2},3])"sv);
  ASSERT_THAT(root, Optional(ElementWith<array>(_)));
  element<policies>::array const* const arr = root->get_if<array>();
  ASSERT_NE(arr, nullptr);
  ASSERT_THAT(*arr, ElementsAre(ElementWith<object>(_), ElementWith<std::int64_t>(3)));
  element<policies>::object const* const obj = arr->at(0).get_if<object>();
  ASSERT_NE(obj, nullptr);
  EXPECT_THAT(*obj, UnorderedElementsAre(Pair(u8"a"s, ElementWith<std::int64_t>(1)),
                                         Pair(u8"b"s, ElementWith<std::int64_t>(2))));
}
// NOLINTNEXTLINE
TEST_F(Dom, ObjectInsideArray2) {
  auto const root = parse(u8R"([1,{"a":2,"b":3}])"sv);
  ASSERT_THAT(root, Optional(ElementWith<array>(_)));
  element<policies>::array const* const arr = root->get_if<array>();
  ASSERT_NE(arr, nullptr);
  ASSERT_THAT(*arr, ElementsAre(ElementWith<std::int64_t>(1), ElementWith<object>(_)));
  element<policies>::object const* const ind1 = arr->at(1).get_if<object>();
  EXPECT_THAT(*ind1, UnorderedElementsAre(Pair(u8"a"s, ElementWith<std::int64_t>(2)),
                                          Pair(u8"b"s, ElementWith<std::int64_t>(3))));
}
// NOLINTNEXTLINE
TEST_F(Dom, ArrayInsideObject) {
  auto const root = parse(u8R"({"a":[1,2],"b":3})"sv);
  ASSERT_THAT(root, Optional(ElementWith<object>(_)));
  auto const* const obj = root->get_if<object>();
  ASSERT_NE(obj, nullptr);
  ASSERT_THAT(*obj,
              UnorderedElementsAre(Pair(u8"a"s, ElementWith<array>(_)), Pair(u8"b"s, ElementWith<std::int64_t>(3))));
  auto const pos = obj->find(u8"a");
  ASSERT_NE(pos, obj->end());
  auto const* const arr = pos->second.get_if<array>();
  ASSERT_NE(arr, nullptr);
  ASSERT_THAT(*arr, ElementsAre(ElementWith<std::int64_t>(1), ElementWith<std::int64_t>(2)));
}
// NOLINTNEXTLINE
TEST_F(Dom, DuplicateKeys) {
  auto const root = parse(u8R"({"a":"b","a":"c"})"sv);
  ASSERT_THAT(root, Optional(ElementWith<object>(_)));
  auto const* const obj = root->get_if<object>();
  ASSERT_NE(obj, nullptr);
  EXPECT_THAT(*obj, UnorderedElementsAre(Pair(u8"a"s, ElementWith<string>(u8"c"s))));
}
#if 0
// NOLINTNEXTLINE
TEST_F(Dom, ArrayStack) {
  dom<size_t{2}, peejay::default_policies> d;
  EXPECT_FALSE(static_cast<bool>(d.begin_array()));
  EXPECT_FALSE(static_cast<bool>(d.begin_array()));

  auto const err = make_error_code(error::nesting_too_deep); // dom_nesting_too_deep
  EXPECT_EQ(d.string_value(u8"string"sv), err);
  EXPECT_EQ(d.integer_value(std::int64_t{37}), err);
  EXPECT_EQ(d.float_value(37.9), err);
  EXPECT_EQ(d.boolean_value(true), err);
  EXPECT_EQ(d.null_value(), err);

  EXPECT_EQ(d.begin_array(), err);
  EXPECT_EQ(d.begin_object(), err);
  EXPECT_EQ(d.key(u8"key"sv), err);
}
#endif

// NOLINTNEXTLINE
TEST(Element, EqObject) {
  auto const a = parse(u8R"({"a":[1,2,3]})"sv);
  auto const b = parse(u8R"({"a":[1,2,3]})"sv);
  ASSERT_THAT(a, Optional(_));
  ASSERT_THAT(b, Optional(_));
  EXPECT_TRUE(*a == *b);
}

// NOLINTNEXTLINE
TEST(Element, EqObjectArraysOfDifferentLength) {
  auto const a = parse(u8R"({"a":[1,2,3]})"sv);
  auto const b = parse(u8R"({"a":[1,2,3,4]})"sv);
  ASSERT_THAT(a, Optional(_));
  ASSERT_THAT(b, Optional(_));
  EXPECT_FALSE(*a == *b);
}
// NOLINTNEXTLINE
TEST(Element, EqObjectDifferentProperties) {
  auto const a = parse(u8R"({"a":[1,2,3]})"sv);
  auto const b = parse(u8R"({"b":[1,2,3]})"sv);
  ASSERT_THAT(a, Optional(_));
  ASSERT_THAT(b, Optional(_));
  EXPECT_FALSE(*a == *b);
}
// NOLINTNEXTLINE
TEST(Element, EqArray) {
  auto const a = parse(u8R"([{"a":1},2])"sv);
  auto const b = parse(u8R"([{"a":1},2])"sv);
  ASSERT_THAT(a, Optional(_));
  ASSERT_THAT(b, Optional(_));
  EXPECT_TRUE(*a == *b);
}
