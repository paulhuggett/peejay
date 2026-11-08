//===- unit_tests/test_rule_helpers.cpp -----------------------------------===//
//*             _        _          _                      *
//*  _ __ _   _| | ___  | |__   ___| |_ __   ___ _ __ ___  *
//* | '__| | | | |/ _ \ | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* | |  | |_| | |  __/ | | | |  __/ | |_) |  __/ |  \__ \ *
//* |_|   \__,_|_|\___| |_| |_|\___|_| .__/ \___|_|  |___/ *
//*                                  |_|                   *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
// DUT
#include "peejay/rules/helpers.hpp"

// Standard library
#include <deque>
#include <list>
#include <string>
#include <vector>

// Google test
#include <gtest/gtest.h>

using namespace std::string_view_literals;

using policies = peejay::default_policies;
using peejay::rules::error;

namespace {

/// Parses \p src using a stack holding a single rule of type \p Rule which writes to \p out.
template <typename Rule, typename T> std::error_code parse(std::u8string_view const src, T* const out) {
  peejay::rules::context<policies, Rule> context{out};
  auto parser = context.make_parser();
  parser.input(src).eof();
  return parser.last_error();
}

struct point {
  policies::integer_type x = 0;
  policies::integer_type y = 0;
};

// An object with two properties. Elements gives the capacity of the object_helper so that we can test both full and
// partially used property tables. Note that this rule does not override name().
template <std::size_t Elements> class point_rule final : public peejay::rules::object_helper<policies, Elements> {
public:
  using value_type = point;

  explicit point_rule(point* const pt) { this->set_properties({{u8"x"sv, {&pt->x}}, {u8"y"sv, {&pt->y}}}); }
};

// An object with no properties at all.
class empty_rule final : public peejay::rules::object_helper<policies, 2> {
  using inherited = peejay::rules::object_helper<policies, 2>;

public:
  using value_type = int;

  explicit empty_rule(int*) { this->set_properties(std::array<inherited::property, 0>{}); }
};

// An object with a property for each of the built-in types.
struct builtins {
  bool b = false;
  policies::integer_type i = 0;
  std::make_unsigned_t<policies::integer_type> u = 0;
  policies::float_type f = 0.0;
  std::u8string u8s;
  std::string s;
};

class builtins_rule final : public peejay::rules::object_helper<policies, 6> {
public:
  using value_type = builtins;

  explicit builtins_rule(builtins* const v) {
    this->set_properties({
        {u8"b"sv, {&v->b}},
        {u8"i"sv, {&v->i}},
        {u8"u"sv, {&v->u}},
        {u8"f"sv, {&v->f}},
        {u8"u8s"sv, {&v->u8s}},
        {u8"s"sv, {&v->s}},
    });
  }
};

// An object with a property for each of the standard integer types.
struct integers {
  signed char sc = 0;
  short s = 0;
  int i = 0;
  long l = 0;
  long long ll = 0;
  unsigned char uc = 0;
  unsigned short us = 0;
  unsigned u = 0;
  unsigned long ul = 0;
  unsigned long long ull = 0;
};

class integers_rule final : public peejay::rules::object_helper<policies, 10> {
public:
  using value_type = integers;

  explicit integers_rule(integers* const v) {
    this->set_properties({
        {u8"sc"sv, {&v->sc}},
        {u8"s"sv, {&v->s}},
        {u8"i"sv, {&v->i}},
        {u8"l"sv, {&v->l}},
        {u8"ll"sv, {&v->ll}},
        {u8"uc"sv, {&v->uc}},
        {u8"us"sv, {&v->us}},
        {u8"u"sv, {&v->u}},
        {u8"ul"sv, {&v->ul}},
        {u8"ull"sv, {&v->ull}},
    });
  }
};

using uint_type = std::make_unsigned_t<policies::integer_type>;
using uint_rule = peejay::rules::uinteger_rule<policies>;

template <typename Container, std::size_t MaxElements = 0>
using uint_array_rule = peejay::rules::homogeneous_array_helper<policies, Container, uint_rule, MaxElements>;

using uint_vector_rule = uint_array_rule<std::vector<uint_type>>;
using uint_deque_rule = uint_array_rule<std::deque<uint_type>>;
using uint_list_rule = uint_array_rule<std::list<uint_type>>;
using max2_uint_vector_rule = uint_array_rule<std::vector<uint_type>, 2>;

// The element rules are chosen by type_to_terminal.
using short_vector_rule = peejay::rules::homogeneous_array_helper<policies, std::vector<short>>;
using double_vector_rule = peejay::rules::homogeneous_array_helper<policies, std::vector<double>>;
using string_vector_rule = peejay::rules::homogeneous_array_helper<policies, std::vector<std::u8string>>;
static_assert(std::is_same_v<short_vector_rule::children,
                             peejay::type_list::type_list<peejay::rules::integer_rule<policies, short>>>);
static_assert(
    std::is_same_v<double_vector_rule::children, peejay::type_list::type_list<peejay::rules::float_rule<policies>>>);
static_assert(
    std::is_same_v<string_vector_rule::children, peejay::type_list::type_list<peejay::rules::string_rule<policies>>>);

using points_rule = peejay::rules::homogeneous_array_helper<policies, std::vector<point>, point_rule<2>>;

}  // end anonymous namespace

//-MARK: object_helper

// NOLINTNEXTLINE
TEST(ObjectHelper, Name) {
  point pt;
  point_rule<2> const rule{&pt};
  EXPECT_STREQ(rule.name(), "object");
}

// NOLINTNEXTLINE
TEST(ObjectHelper, AllPropertiesFullTable) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"x":1,"y":2})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectHelper, AllPropertiesPartTable) {
  point pt;
  EXPECT_FALSE(parse<point_rule<4>>(u8R"({"x":1,"y":2})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectHelper, PropertiesInReverseOrder) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"y":2,"x":1})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectHelper, DuplicateProperty) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"x":1,"y":2,"x":3})"sv, &pt));
  EXPECT_EQ(pt.x, 3);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectHelper, MissingProperty) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":1})"sv, &pt), make_error_code(error::incomplete_object));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, MissingAllProperties) {
  point pt;
  EXPECT_EQ(parse<point_rule<4>>(u8"{}"sv, &pt), make_error_code(error::incomplete_object));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, UnrecognizedProperty) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":1,"z":3})"sv, &pt), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, EmptyKeyDoesNotMatchUnusedEntry) {
  point pt;
  EXPECT_EQ(parse<point_rule<4>>(u8R"({"":3})"sv, &pt), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, WrongValueType) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":"one"})"sv, &pt), make_error_code(error::unexpected_string));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, NotAnObject) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8"[]"sv, &pt), make_error_code(error::unexpected_array));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, NoProperties) {
  int unused = 0;
  EXPECT_FALSE(parse<empty_rule>(u8"{}"sv, &unused));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, NoPropertiesUnrecognized) {
  int unused = 0;
  EXPECT_EQ(parse<empty_rule>(u8R"({"x":1})"sv, &unused), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectHelper, BuiltinTypes) {
  builtins v;
  EXPECT_FALSE(parse<builtins_rule>(u8R"({"b":true,"i":-1,"u":2,"f":1.5,"u8s":"abc","s":"def"})"sv, &v));
  EXPECT_TRUE(v.b);
  EXPECT_EQ(v.i, -1);
  EXPECT_EQ(v.u, 2U);
  EXPECT_EQ(v.f, 1.5);
  EXPECT_EQ(v.u8s, u8"abc");
  EXPECT_EQ(v.s, "def");
}

// NOLINTNEXTLINE
TEST(ObjectHelper, IntegerTypes) {
  integers v;
  EXPECT_FALSE(parse<integers_rule>(
      u8R"({"sc":-128,"s":-32768,"i":-3,"l":-4,"ll":-5,"uc":255,"us":65535,"u":6,"ul":7,"ull":8})"sv, &v));
  EXPECT_EQ(v.sc, -128);
  EXPECT_EQ(v.s, -32768);
  EXPECT_EQ(v.i, -3);
  EXPECT_EQ(v.l, -4L);
  EXPECT_EQ(v.ll, -5LL);
  EXPECT_EQ(v.uc, 255U);
  EXPECT_EQ(v.us, 65535U);
  EXPECT_EQ(v.u, 6U);
  EXPECT_EQ(v.ul, 7UL);
  EXPECT_EQ(v.ull, 8ULL);
}

// NOLINTNEXTLINE
TEST(ObjectHelper, IntegerOutOfRange) {
  integers v;
  EXPECT_EQ(parse<integers_rule>(u8R"({"uc":256})"sv, &v), make_error_code(peejay::error::number_out_of_range));
}

//-MARK: homogeneous_array_helper

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Name) {
  std::vector<uint_type> v;
  uint_vector_rule const rule{&v};
  EXPECT_STREQ(rule.name(), "homogeneous-array");
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Empty) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[]"sv, &v));
  EXPECT_TRUE(v.empty());
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, OneElement) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[ 1 ]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, ThreeElements) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[1, 2, 3]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1, 2, 3}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Deque) {
  std::deque<uint_type> v;
  EXPECT_FALSE(parse<uint_deque_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::deque<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, List) {
  std::list<uint_type> v;
  EXPECT_FALSE(parse<uint_list_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::list<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, ArrayOfObjects) {
  std::vector<point> v;
  EXPECT_FALSE(parse<points_rule>(u8R"([{"x":1,"y":2},{"y":4,"x":3}])"sv, &v));
  ASSERT_EQ(v.size(), 2U);
  EXPECT_EQ(v[0].x, 1);
  EXPECT_EQ(v[0].y, 2);
  EXPECT_EQ(v[1].x, 3);
  EXPECT_EQ(v[1].y, 4);
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Shorts) {
  std::vector<short> v;
  EXPECT_FALSE(parse<short_vector_rule>(u8"[-32768,0,32767]"sv, &v));
  EXPECT_EQ(v, (std::vector<short>{-32768, 0, 32767}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Doubles) {
  std::vector<double> v;
  EXPECT_FALSE(parse<double_vector_rule>(u8"[1.5,-2]"sv, &v));
  EXPECT_EQ(v, (std::vector<double>{1.5, -2.0}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, Strings) {
  std::vector<std::u8string> v;
  EXPECT_FALSE(parse<string_vector_rule>(u8R"(["a","bc"])"sv, &v));
  EXPECT_EQ(v, (std::vector<std::u8string>{u8"a", u8"bc"}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, ShortOutOfRange) {
  std::vector<short> v;
  EXPECT_EQ(parse<short_vector_rule>(u8"[1,32768]"sv, &v), make_error_code(peejay::error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, ElementError) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<uint_vector_rule>(u8"[1,-1]"sv, &v), make_error_code(peejay::error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, NotAnArray) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<uint_vector_rule>(u8"1"sv, &v), make_error_code(error::unexpected_number));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, MaxElementsEmpty) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<max2_uint_vector_rule>(u8"[]"sv, &v));
  EXPECT_TRUE(v.empty());
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, MaxElementsFull) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<max2_uint_vector_rule>(u8"[80,8080]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{80, 8080}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayHelper, TooManyElements) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<max2_uint_vector_rule>(u8"[80,8080,443]"sv, &v), make_error_code(error::too_many_array_elements));
  EXPECT_EQ(v, (std::vector<uint_type>{80, 8080}));
}
