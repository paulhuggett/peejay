//===- unit_tests/test_rule_non_terminals.cpp -----------------------------===//
//*             _                           *
//*  _ __ _   _| | ___   _ __   ___  _ __   *
//* | '__| | | | |/ _ \ | '_ \ / _ \| '_ \  *
//* | |  | |_| | |  __/ | | | | (_) | | | | *
//* |_|   \__,_|_|\___| |_| |_|\___/|_| |_| *
//*                                         *
//*  _                      _             _      *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*                                              *
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
#include "peejay/rules/non_terminal_rule.hpp"
#include "peejay/rules/terminals.hpp"

// Standard library
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Google test
#include <gtest/gtest.h>

using namespace std::string_view_literals;

namespace {

using policies = peejay::default_policies;
using peejay::rules::error;
using test_context_base = peejay::rules::context_base<policies>;

/// Parses \p src using a stack holding a single rule of type \p Rule which writes to \p out.
template <typename Rule, typename T> std::error_code parse(std::u8string_view const src, T* const out) {
  peejay::rules::context<policies, Rule> context{out};
  auto parser = context.make_parser();
  parser.input(src).eof();
  return parser.last_error();
}

static_assert(peejay::rules::emplace_back_container<std::vector<int>>);
static_assert(peejay::rules::emplace_back_container<std::deque<int>>);
static_assert(peejay::rules::emplace_back_container<std::list<int>>);
static_assert(peejay::rules::emplace_back_container<peejay::arrayvec<int, 2>>);
static_assert(!peejay::rules::emplace_back_container<std::vector<bool>>, "emplace_back() returns a proxy");
static_assert(!peejay::rules::emplace_back_container<std::array<int, 2>>, "no emplace_back()");
static_assert(!peejay::rules::emplace_back_container<std::set<int>>, "no emplace_back()");

struct point {
  policies::integer_type x = 0;
  policies::integer_type y = 0;
};

// An object with two properties. Elements gives the capacity of the object_rule so that we can test both full and
// partially used property tables. Note that this rule does not override name().
template <std::size_t Elements> class point_rule final : public peejay::rules::object_rule<policies, Elements> {
public:
  using value_type = point;

  explicit point_rule(point* const pt) { this->set_properties({{u8"x"sv, {&pt->x}}, {u8"y"sv, {&pt->y}}}); }
};

// An object with no properties at all.
class empty_rule final : public peejay::rules::object_rule<policies, 2> {
  using inherited = peejay::rules::object_rule<policies, 2>;

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

class builtins_rule final : public peejay::rules::object_rule<policies, 6> {
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

class integers_rule final : public peejay::rules::object_rule<policies, 10> {
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
using uint_array_rule = peejay::rules::homogeneous_array_rule<policies, Container, uint_rule, MaxElements>;

using uint_vector_rule = uint_array_rule<std::vector<uint_type>>;
using uint_deque_rule = uint_array_rule<std::deque<uint_type>>;
using uint_list_rule = uint_array_rule<std::list<uint_type>>;
using max2_uint_vector_rule = uint_array_rule<std::vector<uint_type>, 2>;
using uint_arrayvec = peejay::arrayvec<uint_type, 2>;
using uint_arrayvec_rule = uint_array_rule<uint_arrayvec>;
// MaxElements is larger than the arrayvec's capacity.
using max3_uint_arrayvec_rule = uint_array_rule<uint_arrayvec, 3>;
static_assert(peejay::rules::max_size_container<uint_arrayvec>);
static_assert(peejay::rules::max_size_container<std::vector<uint_type>>);
static_assert(!peejay::rules::max_size_container<std::vector<uint_type> const*>);

// The element rules are chosen by type_to_terminal.
using short_vector_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<short>>;
using double_vector_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<double>>;
using string_vector_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<std::u8string>>;
using std_string_vector_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<std::string>>;
static_assert(std::is_same_v<std_string_vector_rule::children,
                             peejay::type_list::type_list<peejay::rules::string_rule<policies, std::string>>>);
static_assert(std::is_same_v<short_vector_rule::children,
                             peejay::type_list::type_list<peejay::rules::integer_rule<policies, short>>>);
static_assert(
    std::is_same_v<double_vector_rule::children, peejay::type_list::type_list<peejay::rules::float_rule<policies>>>);
static_assert(
    std::is_same_v<string_vector_rule::children, peejay::type_list::type_list<peejay::rules::string_rule<policies>>>);

enum class colour { red, green, blue };

static_assert(peejay::rules::enum_target<colour>);
static_assert(peejay::rules::enum_target<int>);
static_assert(!peejay::rules::enum_target<bool>);
static_assert(!peejay::rules::enum_target<colour const>);

// Elements gives the capacity of the enum_rule so that we can test both full and partially used tables.
template <std::size_t Elements> class colour_rule final : public peejay::rules::enum_rule<policies, colour, Elements> {
public:
  explicit colour_rule(colour* const c) : peejay::rules::enum_rule<policies, colour, Elements>{c} {
    this->set_enumerators({{u8"red"sv, colour::red}, {u8"green"sv, colour::green}, {u8"blue"sv, colour::blue}});
  }
};

// Integer values, a duplicate string, and an empty string. Uses the range overload of set_enumerators().
class level_rule final : public peejay::rules::enum_rule<policies, int, 3> {
public:
  explicit level_rule(int* const v) : enum_rule{v} {
    this->set_enumerators(std::array{enumerator{u8"low"sv, 1}, enumerator{u8"low"sv, 2}, enumerator{u8""sv, 10}});
  }
};

struct paint {
  std::string name;
  colour shade = colour::red;
};

class paint_rule final : public peejay::rules::object_rule<policies, 2, colour_rule<3>> {
public:
  using value_type = paint;
  explicit paint_rule(paint* const p) { this->set_properties({{u8"name"sv, {&p->name}}, {u8"shade"sv, {&p->shade}}}); }
};

using colours_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<colour>, colour_rule<3>>;

using points_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<point>, point_rule<2>>;

// Strings with room for four UTF-8 code units.
using short_string = peejay::arrayvec<char8_t, 4>;
using short_string_rule = peejay::rules::string_rule<policies, short_string>;

/// \returns A copy of the characters held by \p s.
std::u8string to_u8string(short_string const& s) {
  return {s.data(), s.size()};
}

// The element rule is chosen by type_to_terminal.
using short_strings_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<short_string>>;
static_assert(std::is_same_v<short_strings_rule::children, peejay::type_list::type_list<short_string_rule>>);

struct label {
  short_string code;
  int id = 0;
};

// An arrayvec string property needs its string_rule to be one of the UserRules.
class label_rule final : public peejay::rules::object_rule<policies, 2, short_string_rule> {
public:
  using value_type = label;
  explicit label_rule(label* const l) { this->set_properties({{u8"code"sv, {&l->code}}, {u8"id"sv, {&l->id}}}); }
};

//-MARK: object_rule

// NOLINTNEXTLINE
TEST(ObjectRule, Name) {
  point pt;
  point_rule<2> const rule{&pt};
  EXPECT_STREQ(rule.name(), "object");
}

// NOLINTNEXTLINE
TEST(ObjectRule, AllPropertiesFullTable) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"x":1,"y":2})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectRule, AllPropertiesPartTable) {
  point pt;
  EXPECT_FALSE(parse<point_rule<4>>(u8R"({"x":1,"y":2})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectRule, PropertiesInReverseOrder) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"y":2,"x":1})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectRule, DuplicateProperty) {
  point pt;
  EXPECT_FALSE(parse<point_rule<2>>(u8R"({"x":1,"y":2,"x":3})"sv, &pt));
  EXPECT_EQ(pt.x, 3);
  EXPECT_EQ(pt.y, 2);
}

// NOLINTNEXTLINE
TEST(ObjectRule, MissingProperty) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":1})"sv, &pt), make_error_code(error::incomplete_object));
}

// NOLINTNEXTLINE
TEST(ObjectRule, MissingAllProperties) {
  point pt;
  EXPECT_EQ(parse<point_rule<4>>(u8"{}"sv, &pt), make_error_code(error::incomplete_object));
}

// NOLINTNEXTLINE
TEST(ObjectRule, UnrecognizedProperty) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":1,"z":3})"sv, &pt), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectRule, EmptyKeyDoesNotMatchUnusedEntry) {
  point pt;
  EXPECT_EQ(parse<point_rule<4>>(u8R"({"":3})"sv, &pt), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectRule, WrongValueType) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8R"({"x":"one"})"sv, &pt), make_error_code(error::unexpected_string));
}

// NOLINTNEXTLINE
TEST(ObjectRule, NotAnObject) {
  point pt;
  EXPECT_EQ(parse<point_rule<2>>(u8"[]"sv, &pt), make_error_code(error::unexpected_array));
}

// NOLINTNEXTLINE
TEST(ObjectRule, NoProperties) {
  int unused = 0;
  EXPECT_FALSE(parse<empty_rule>(u8"{}"sv, &unused));
}

// NOLINTNEXTLINE
TEST(ObjectRule, NoPropertiesUnrecognized) {
  int unused = 0;
  EXPECT_EQ(parse<empty_rule>(u8R"({"x":1})"sv, &unused), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectRule, BuiltinTypes) {
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
TEST(ObjectRule, IntegerTypes) {
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
TEST(ObjectRule, IntegerOutOfRange) {
  integers v;
  EXPECT_EQ(parse<integers_rule>(u8R"({"uc":256})"sv, &v), make_error_code(peejay::error::number_out_of_range));
}

//-MARK: homogeneous_array_rule

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Name) {
  std::vector<uint_type> v;
  uint_vector_rule const rule{&v};
  EXPECT_STREQ(rule.name(), "homogeneous-array");
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Empty) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[]"sv, &v));
  EXPECT_TRUE(v.empty());
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, OneElement) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[ 1 ]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ThreeElements) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[1, 2, 3]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1, 2, 3}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Deque) {
  std::deque<uint_type> v;
  EXPECT_FALSE(parse<uint_deque_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::deque<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, List) {
  std::list<uint_type> v;
  EXPECT_FALSE(parse<uint_list_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::list<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ArrayOfObjects) {
  std::vector<point> v;
  EXPECT_FALSE(parse<points_rule>(u8R"([{"x":1,"y":2},{"y":4,"x":3}])"sv, &v));
  ASSERT_EQ(v.size(), 2U);
  EXPECT_EQ(v[0].x, 1);
  EXPECT_EQ(v[0].y, 2);
  EXPECT_EQ(v[1].x, 3);
  EXPECT_EQ(v[1].y, 4);
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Shorts) {
  std::vector<short> v;
  EXPECT_FALSE(parse<short_vector_rule>(u8"[-32768,0,32767]"sv, &v));
  EXPECT_EQ(v, (std::vector<short>{-32768, 0, 32767}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Doubles) {
  std::vector<double> v;
  EXPECT_FALSE(parse<double_vector_rule>(u8"[1.5,-2]"sv, &v));
  EXPECT_EQ(v, (std::vector<double>{1.5, -2.0}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, Strings) {
  std::vector<std::u8string> v;
  EXPECT_FALSE(parse<string_vector_rule>(u8R"(["a","bc"])"sv, &v));
  EXPECT_EQ(v, (std::vector<std::u8string>{u8"a", u8"bc"}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ShortOutOfRange) {
  std::vector<short> v;
  EXPECT_EQ(parse<short_vector_rule>(u8"[1,32768]"sv, &v), make_error_code(peejay::error::number_out_of_range));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, StdStrings) {
  std::vector<std::string> v;
  EXPECT_FALSE(parse<std_string_vector_rule>(u8R"(["a","bc"])"sv, &v));
  EXPECT_EQ(v, (std::vector<std::string>{"a", "bc"}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ElementError) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<uint_vector_rule>(u8"[1,-1]"sv, &v), make_error_code(peejay::error::number_out_of_range));
  EXPECT_EQ(v, (std::vector<uint_type>{1})) << "the failed element is removed";
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ReplacesContents) {
  std::vector<uint_type> v{9};
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, EmptyReplacesContents) {
  std::vector<uint_type> v{9};
  EXPECT_FALSE(parse<uint_vector_rule>(u8"[]"sv, &v));
  EXPECT_TRUE(v.empty());
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, MaxElementsIgnoresExistingContents) {
  std::vector<uint_type> v{7, 7};
  EXPECT_FALSE(parse<max2_uint_vector_rule>(u8"[1,2]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{1, 2}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, IncompleteObjectElementRemoved) {
  std::vector<point> v;
  EXPECT_EQ(parse<points_rule>(u8R"([{"x":1,"y":2},{"x":3}])"sv, &v), make_error_code(error::incomplete_object));
  ASSERT_EQ(v.size(), 1U);
  EXPECT_EQ(v[0].x, 1);
  EXPECT_EQ(v[0].y, 2);
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, TruncatedInputElementRemoved) {
  std::vector<point> v;
  EXPECT_TRUE(parse<points_rule>(u8R"([{"x":1,"y":2},{"x":)"sv, &v));
  ASSERT_EQ(v.size(), 1U);
  EXPECT_EQ(v[0].x, 1);
  EXPECT_EQ(v[0].y, 2);
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, NotAnArray) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<uint_vector_rule>(u8"1"sv, &v), make_error_code(error::unexpected_number));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, MaxElementsEmpty) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<max2_uint_vector_rule>(u8"[]"sv, &v));
  EXPECT_TRUE(v.empty());
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, MaxElementsFull) {
  std::vector<uint_type> v;
  EXPECT_FALSE(parse<max2_uint_vector_rule>(u8"[80,8080]"sv, &v));
  EXPECT_EQ(v, (std::vector<uint_type>{80, 8080}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, TooManyElements) {
  std::vector<uint_type> v;
  EXPECT_EQ(parse<max2_uint_vector_rule>(u8"[80,8080,443]"sv, &v), make_error_code(error::too_many_array_elements));
  EXPECT_EQ(v, (std::vector<uint_type>{80, 8080}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ArrayvecFull) {
  uint_arrayvec v;
  EXPECT_FALSE(parse<uint_arrayvec_rule>(u8"[80,8080]"sv, &v));
  EXPECT_EQ(v, (uint_arrayvec{80, 8080}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ArrayvecTooManyElements) {
  uint_arrayvec v;
  EXPECT_EQ(parse<uint_arrayvec_rule>(u8"[80,8080,443]"sv, &v), make_error_code(error::too_many_array_elements));
  EXPECT_EQ(v, (uint_arrayvec{80, 8080}));
}

// NOLINTNEXTLINE
TEST(HomogeneousArrayRule, ArrayvecSmallerThanMaxElements) {
  uint_arrayvec v;
  EXPECT_EQ(parse<max3_uint_arrayvec_rule>(u8"[80,8080,443]"sv, &v), make_error_code(error::too_many_array_elements));
  EXPECT_EQ(v, (uint_arrayvec{80, 8080}));
}

//-MARK: enum_rule

// NOLINTNEXTLINE
TEST(EnumRule, Name) {
  colour c = colour::red;
  colour_rule<3> const rule{&c};
  EXPECT_STREQ(rule.name(), "enum");
}

// NOLINTNEXTLINE
TEST(EnumRule, FirstEnumerator) {
  colour c = colour::blue;
  EXPECT_FALSE(parse<colour_rule<3>>(u8R"("red")"sv, &c));
  EXPECT_EQ(c, colour::red);
}

// NOLINTNEXTLINE
TEST(EnumRule, LastEnumerator) {
  colour c = colour::red;
  EXPECT_FALSE(parse<colour_rule<3>>(u8R"("blue")"sv, &c));
  EXPECT_EQ(c, colour::blue);
}

// NOLINTNEXTLINE
TEST(EnumRule, PartTable) {
  colour c = colour::red;
  EXPECT_FALSE(parse<colour_rule<5>>(u8R"("green")"sv, &c));
  EXPECT_EQ(c, colour::green);
}

// NOLINTNEXTLINE
TEST(EnumRule, Unrecognized) {
  colour c = colour::green;
  EXPECT_EQ(parse<colour_rule<3>>(u8R"("purple")"sv, &c), make_error_code(error::unrecognized_enumerator));
  EXPECT_EQ(c, colour::green);
}

// NOLINTNEXTLINE
TEST(EnumRule, EmptyStringDoesNotMatchUnusedEntry) {
  colour c = colour::red;
  EXPECT_EQ(parse<colour_rule<5>>(u8R"("")"sv, &c), make_error_code(error::unrecognized_enumerator));
}

// NOLINTNEXTLINE
TEST(EnumRule, CaseSensitive) {
  colour c = colour::red;
  EXPECT_EQ(parse<colour_rule<3>>(u8R"("Red")"sv, &c), make_error_code(error::unrecognized_enumerator));
}

// NOLINTNEXTLINE
TEST(EnumRule, NotAString) {
  colour c = colour::red;
  EXPECT_EQ(parse<colour_rule<3>>(u8"1"sv, &c), make_error_code(error::unexpected_number));
}

// NOLINTNEXTLINE
TEST(EnumRule, IntegerValue) {
  int v = 0;
  EXPECT_FALSE(parse<level_rule>(u8R"("low")"sv, &v));
  EXPECT_EQ(v, 1) << "the first of two enumerators with the same string is used";
}

// NOLINTNEXTLINE
TEST(EnumRule, EmptyStringEnumerator) {
  int v = 0;
  EXPECT_FALSE(parse<level_rule>(u8R"("")"sv, &v));
  EXPECT_EQ(v, 10);
}

// NOLINTNEXTLINE
TEST(EnumRule, ObjectProperty) {
  paint p;
  EXPECT_FALSE(parse<paint_rule>(u8R"({"name":"sky","shade":"blue"})"sv, &p));
  EXPECT_EQ(p.name, "sky");
  EXPECT_EQ(p.shade, colour::blue);
}

// NOLINTNEXTLINE
TEST(EnumRule, ObjectPropertyUnrecognized) {
  paint p;
  EXPECT_EQ(parse<paint_rule>(u8R"({"name":"sky","shade":"cyan"})"sv, &p),
            make_error_code(error::unrecognized_enumerator));
}

// NOLINTNEXTLINE
TEST(EnumRule, ArrayElements) {
  std::vector<colour> v;
  EXPECT_FALSE(parse<colours_rule>(u8R"(["green","red","green"])"sv, &v));
  EXPECT_EQ(v, (std::vector<colour>{colour::green, colour::red, colour::green}));
}

//-MARK: arrayvec strings

// NOLINTNEXTLINE
TEST(ArrayvecString, ArrayElements) {
  std::vector<short_string> v;
  EXPECT_FALSE(parse<short_strings_rule>(u8R"(["a","","abcd"])"sv, &v));
  ASSERT_EQ(v.size(), 3U);
  EXPECT_EQ(to_u8string(v[0]), u8"a");
  EXPECT_EQ(to_u8string(v[1]), u8"");
  EXPECT_EQ(to_u8string(v[2]), u8"abcd");
}

// NOLINTNEXTLINE
TEST(ArrayvecString, ArrayElementTooLong) {
  std::vector<short_string> v;
  EXPECT_EQ(parse<short_strings_rule>(u8R"(["a","abcde"])"sv, &v), make_error_code(peejay::error::string_too_long));
}

// NOLINTNEXTLINE
TEST(ArrayvecString, ObjectProperty) {
  label l;
  EXPECT_FALSE(parse<label_rule>(u8R"({"code":"en","id":7})"sv, &l));
  EXPECT_EQ(to_u8string(l.code), u8"en");
  EXPECT_EQ(l.id, 7);
}

// NOLINTNEXTLINE
TEST(ArrayvecString, ObjectPropertyTooLong) {
  label l;
  EXPECT_EQ(parse<label_rule>(u8R"({"code":"en-GB","id":7})"sv, &l), make_error_code(peejay::error::string_too_long));
  EXPECT_EQ(to_u8string(l.code), u8"");
}

//-MARK: object_rule with user rules

struct address {
  std::string name;
  unsigned port = 0;
};

template <peejay::policies Policies> class address_rule final : public peejay::rules::object_rule<Policies, 3> {
public:
  using policies = Policies;
  using value_type = address;
  explicit address_rule(address* const addr) {
    this->set_properties({{u8"name"sv, {&addr->name}}, {u8"port"sv, {&addr->port}}});
  }
  [[nodiscard]] constexpr char const* name() const noexcept override { return "address"; }
};

struct server {
  std::string host;
  address addr;
  std::vector<unsigned> ports;
};

template <peejay::policies Policies>
using multiport_rule = peejay::rules::homogeneous_array_rule<Policies, std::vector<unsigned>>;

template <peejay::policies Policies>
class server_rule final
    : public peejay::rules::object_rule<Policies, 3, address_rule<Policies>, multiport_rule<Policies>> {
  using inherited = peejay::rules::object_rule<Policies, 3, address_rule<Policies>, multiport_rule<Policies>>;

public:
  using policies = Policies;
  using value_type = server;

  explicit server_rule(server* const s) {
    this->set_properties({
        {u8"host"sv, &s->host},
        {u8"address"sv, &s->addr},
        {u8"ports"sv, &s->ports},
    });
  }
  [[nodiscard]] constexpr char const* name() const noexcept override { return "server"; }
};

struct multiport_address {
  std::string name;
  std::vector<unsigned> ports;
};

template <peejay::policies Policies>
class multiport_address_rule final : public peejay::rules::object_rule<Policies, 2, multiport_rule<Policies>> {
  using inherited = peejay::rules::object_rule<Policies, 2, multiport_rule<Policies>>;

public:
  using policies = Policies;
  using value_type = multiport_address;

  explicit multiport_address_rule(multiport_address* const addr) {
    using property = inherited::property;
    this->set_properties(std::array{
        property{u8"name"sv, {&addr->name}},
        property{u8"ports"sv, {&addr->ports}},
    });
  }
  [[nodiscard]] constexpr char const* name() const noexcept override { return "multiport-address"; }
};

// NOLINTNEXTLINE
TEST(ObjectRuleUserRules, MultiplePorts) {
  multiport_address addr;
  using my_rule = multiport_address_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&addr};
  auto parser = context.make_parser();
  parser.input(u8R"({"name":"http","ports":[80,8080]})"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(addr.name, "http");
  EXPECT_EQ(addr.ports, (std::vector<unsigned>{80, 8080}));
}

// NOLINTNEXTLINE
TEST(ObjectRuleUserRules, TwoUserRules) {
  server s;
  using my_rule = server_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&s};
  auto parser = context.make_parser();
  parser.input(u8R"({"ports":[22],"host":"example","address":{"name":"web","port":8080}})"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(s.host, "example");
  EXPECT_EQ(s.addr.name, "web");
  EXPECT_EQ(s.addr.port, 8080U);
  EXPECT_EQ(s.ports, (std::vector<unsigned>{22}));
}

// NOLINTNEXTLINE
TEST(ObjectRuleUserRules, UserRuleError) {
  server s;
  using my_rule = server_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&s};
  auto parser = context.make_parser();
  parser.input(u8R"({"host":"example","address":{"name":"web","colour":"red"},"ports":[]})"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectRuleUserRules, EmptyArray) {
  multiport_address addr;
  using my_rule = multiport_address_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&addr};
  auto parser = context.make_parser();
  parser.input(u8R"({"name":"http","ports":[ ]})"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(addr.name, "http");
  EXPECT_TRUE(addr.ports.empty());
}

//-MARK: context

// A frame's size is the sizeof the rule because the base class's alignment is sufficient.
using bool_context = peejay::rules::context<policies, peejay::rules::bool_rule<policies>>;
static_assert(bool_context::stack_bytes == sizeof(peejay::rules::bool_rule<policies>));

// A container and its element.
using multiport_context = peejay::rules::context<policies, multiport_rule<policies>>;
static_assert(multiport_context::stack_bytes ==
              sizeof(multiport_rule<policies>) + sizeof(peejay::rules::uinteger_rule<policies>));

// The longest path through server_rule is through address_rule to one of the terminals.
using server_context = peejay::rules::context<policies, server_rule<policies>>;
static_assert(server_context::stack_bytes == sizeof(server_rule<policies>) + sizeof(address_rule<policies>) +
                                                 sizeof(peejay::rules::string_rule<policies>));

// An array whose elements are arrays.
struct tree {
  std::vector<tree> children;
};
class tree_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = tree;
  using children = peejay::type_list::type_list<tree_rule>;

  explicit tree_rule(tree* const t) noexcept : tree_{t} {}
  [[nodiscard]] char const* name() const noexcept override { return "tree"; }
  std::error_code begin_array(test_context_base* const) override { return {}; }
  std::error_code element(test_context_base* const context) override {
    return push<tree_rule>(this, context, &tree_->children.emplace_back());
  }
  std::error_code end_array(test_context_base* const context) override { return this->pop(context); }

private:
  tree* tree_;
};
using tree_context = peejay::rules::context<policies, tree_rule>;
static_assert(tree_context::stack_bytes == policies::max_stack_depth * sizeof(tree_rule));

// Two rules which push each other. Rule 'small' has fewer members than rule 'large'. Each counts the arrays seen.
class large_rule;
class small_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = int;
  using children = peejay::type_list::type_list<large_rule>;
  explicit small_rule(int* const count) noexcept : count_{count} {}
  [[nodiscard]] char const* name() const noexcept override { return "small"; }
  std::error_code begin_array(test_context_base*) override {
    ++*count_;
    return {};
  }
  std::error_code element(test_context_base* context) override;
  std::error_code end_array(test_context_base* const context) override { return this->pop(context); }

private:
  int* count_;
};
class large_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = int;
  using children = peejay::type_list::type_list<small_rule>;
  explicit large_rule(int* const count) noexcept : count_{count} {}
  [[nodiscard]] char const* name() const noexcept override { return "large"; }
  std::error_code begin_array(test_context_base*) override {
    ++*count_;
    return {};
  }
  std::error_code element(test_context_base* const context) override { return push<small_rule>(this, context, count_); }
  std::error_code end_array(test_context_base* const context) override { return this->pop(context); }

private:
  int* count_;
  [[maybe_unused]] std::array<void*, 4> padding_{};
};
std::error_code small_rule::element(test_context_base* const context) {
  return push<large_rule>(this, context, count_);
}
static_assert(peejay::rules::context<policies, small_rule>::stack_bytes ==
              policies::max_stack_depth * sizeof(large_rule));

// A rule which never pops and pushes a new instance of itself for each array element. Each instance uses stack space
// without any change to the depth of the input so the parser's own nesting limit does not apply.
class runaway_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = int;
  using children = peejay::type_list::type_list<runaway_rule>;
  explicit runaway_rule(int* const count) noexcept : count_{count} {}
  [[nodiscard]] char const* name() const noexcept override { return "runaway"; }
  std::error_code begin_array(test_context_base* const) override { return {}; }
  std::error_code element(test_context_base* const context) override {
    auto const erc = push<runaway_rule>(this, context, count_);
    if (!erc) {
      ++*count_;
    }
    return erc;
  }
  std::error_code integer_value(test_context_base* const, policies::integer_type const) override { return {}; }

private:
  int* count_;
};

// A rule which requires stricter alignment than the rule base class.
struct aligned_value {
  policies::integer_type value = 0;
  bool aligned = false;
};
class alignas(64) aligned_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = aligned_value;
  explicit aligned_rule(aligned_value* const v) noexcept : value_{v} {}
  [[nodiscard]] char const* name() const noexcept override { return "aligned"; }
  std::error_code integer_value(test_context_base* const context, policies::integer_type const value) override {
    value_->value = value;
    value_->aligned = reinterpret_cast<std::uintptr_t>(this) % alignof(aligned_rule) == 0U;
    return this->pop(context);
  }

private:
  aligned_value* value_;
};
using aligned_array_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<aligned_value>, aligned_rule>;
using aligned_context = peejay::rules::context<policies, aligned_array_rule>;
static_assert(aligned_context::stack_bytes == sizeof(aligned_array_rule) + sizeof(aligned_rule) +
                                                  alignof(aligned_rule) - alignof(peejay::rules::rule<policies>));

// A rule whose constructor accepts anything, so a context holding it is "constructible" from a context. The context's
// forwarding constructor must not be used to copy or move a context.
class greedy_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = int;
  template <typename T> explicit greedy_rule(T&&) noexcept {}
  [[nodiscard]] char const* name() const noexcept override { return "greedy"; }
};
using greedy_context = peejay::rules::context<policies, greedy_rule>;
static_assert(std::is_constructible_v<greedy_context, int*>);
static_assert(!std::is_constructible_v<greedy_context, greedy_context&>);
static_assert(!std::is_copy_constructible_v<greedy_context>);
static_assert(!std::is_move_constructible_v<greedy_context>);
static_assert(!std::is_copy_constructible_v<bool_context>);
static_assert(!std::is_move_constructible_v<bool_context>);

// A rule which uses the default implementation of each event except for those needed to reach the others.
class defaults_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = int;
  explicit defaults_rule(int*) noexcept {}
  [[nodiscard]] char const* name() const noexcept override { return "defaults"; }
  std::error_code begin_array(test_context_base* const) override { return {}; }
  std::error_code begin_object(test_context_base* const) override { return {}; }
};

// A subclass of object_rule whose own (empty) children hides those of object_rule. The terminals pushed by
// object_rule must still be included in the size of the stack.
class hiding_rule final : public peejay::rules::object_rule<policies, 1> {
public:
  using value_type = point;
  using children = peejay::type_list::type_list<>;
  explicit hiding_rule(point* const pt) { this->set_properties({{u8"x"sv, {&pt->x}}}); }
};
static_assert(sizeof(peejay::rules::integer_rule<policies>) == sizeof(peejay::rules::string_rule<policies>),
              "the terminal rules all have the same size");
static_assert(peejay::rules::context<policies, hiding_rule>::stack_bytes ==
              sizeof(hiding_rule) + sizeof(peejay::rules::integer_rule<policies>));

// A rule which never pops. Each of the first six array elements gets a new instance of this rule, so the stack fills
// without deep nesting. The seventh element is parsed by an array rule which then has no room for its element rule.
struct filler_state {
  int pushes = 0;
  std::vector<uint_type> ints;
};
class filler_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = filler_state;
  using children = peejay::type_list::type_list<filler_rule, uint_vector_rule>;
  explicit filler_rule(filler_state* const state) noexcept : state_{state} {}
  [[nodiscard]] char const* name() const noexcept override { return "filler"; }
  std::error_code begin_array(test_context_base* const) override { return {}; }
  std::error_code element(test_context_base* const context) override {
    if (state_->pushes == 6) {
      return push<uint_vector_rule>(this, context, &state_->ints);
    }
    ++state_->pushes;
    return push<filler_rule>(this, context, state_);
  }
  std::error_code integer_value(test_context_base* const, policies::integer_type const) override { return {}; }

private:
  filler_state* state_;
};
static_assert(sizeof(filler_rule) == sizeof(uint_vector_rule) && sizeof(filler_rule) == sizeof(uint_rule),
              "the stack holds exactly max_stack_depth (8) of these rules");
static_assert(peejay::rules::context<policies, filler_rule>::stack_bytes ==
              policies::max_stack_depth * sizeof(filler_rule));

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRule) {
  tree t;
  EXPECT_FALSE(parse<tree_rule>(u8"[[],[[]]]"sv, &t));
  ASSERT_EQ(t.children.size(), 2U);
  EXPECT_TRUE(t.children[0].children.empty());
  ASSERT_EQ(t.children[1].children.size(), 1U);
  EXPECT_TRUE(t.children[1].children[0].children.empty());
}

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRuleAtMaximumDepth) {
  static_assert(policies::max_stack_depth == 8);
  tree t;
  EXPECT_FALSE(parse<tree_rule>(u8"[[[[[[[[]]]]]]]]"sv, &t));
}

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRuleTooDeep) {
  tree t;
  EXPECT_EQ(parse<tree_rule>(u8"[[[[[[[[[]]]]]]]]]"sv, &t), make_error_code(peejay::error::nesting_too_deep));
}

// NOLINTNEXTLINE
TEST(RuleContext, MutuallyRecursiveRules) {
  int arrays = 0;
  EXPECT_FALSE(parse<small_rule>(u8"[[[[]]]]"sv, &arrays));
  EXPECT_EQ(arrays, 4);
}

// NOLINTNEXTLINE
TEST(RuleContext, StackExhausted) {
  int pushes = 0;
  EXPECT_EQ(parse<runaway_rule>(u8"[1,2,3,4,5,6,7,8,9]"sv, &pushes), make_error_code(peejay::error::nesting_too_deep));
  // The root rule occupies one of the entries.
  EXPECT_EQ(pushes, 7);
}

// NOLINTNEXTLINE
TEST(RuleContext, StrictlyAlignedRule) {
  std::vector<aligned_value> v;
  EXPECT_FALSE(parse<aligned_array_rule>(u8"[1,2]"sv, &v));
  ASSERT_EQ(v.size(), 2U);
  EXPECT_EQ(v[0].value, 1);
  EXPECT_TRUE(v[0].aligned);
  EXPECT_EQ(v[1].value, 2);
  EXPECT_TRUE(v[1].aligned);
}

// NOLINTNEXTLINE
TEST(RuleContext, HiddenBaseChildren) {
  point pt;
  EXPECT_FALSE(parse<hiding_rule>(u8R"({"x":1})"sv, &pt));
  EXPECT_EQ(pt.x, 1);
}

// NOLINTNEXTLINE
TEST(RuleContext, ArrayElementPushFails) {
  filler_state state;
  EXPECT_EQ(parse<filler_rule>(u8"[0,0,0,0,0,0,[1]]"sv, &state), make_error_code(peejay::error::nesting_too_deep));
  EXPECT_EQ(state.pushes, 6);
  EXPECT_TRUE(state.ints.empty()) << "the element whose rule could not be pushed is removed";
}

// NOLINTNEXTLINE
TEST(RuleContext, SecondDocument) {
  bool b = false;
  bool_context context{&b};
  auto parser = context.make_parser();
  parser.input(u8"true"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  parser.input(u8"false"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::no_active_rule));
  EXPECT_TRUE(b);
}

// NOLINTNEXTLINE
TEST(RuleContext, SecondParserAfterError) {
  bool b = false;
  bool_context context{&b};
  auto parser1 = context.make_parser();
  parser1.input(u8"1"sv).eof();
  EXPECT_EQ(parser1.last_error(), make_error_code(error::unexpected_number));
  // The failed parse emptied the stack, so the bool_rule is not reused.
  auto parser2 = context.make_parser();
  parser2.input(u8"true"sv).eof();
  EXPECT_EQ(parser2.last_error(), make_error_code(error::no_active_rule));
  EXPECT_FALSE(b);
}

// NOLINTNEXTLINE
TEST(RuleContext, ResetWithSameContext) {
  bool b = false;
  bool_context context{&b};
  auto parser = context.make_parser();
  parser.input(u8"true"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  // Resetting the parser does not reset the context.
  parser.reset(&context).input(u8"false"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::no_active_rule));
  EXPECT_TRUE(b);
}

// NOLINTNEXTLINE
TEST(RuleContext, ResetWithNewContext) {
  bool b1 = false;
  bool_context context1{&b1};
  auto parser = context1.make_parser();
  parser.input(u8"1"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::unexpected_number));
  bool b2 = false;
  bool_context context2{&b2};
  parser.reset(&context2).input(u8"true"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_FALSE(b1);
  EXPECT_TRUE(b2);
}

// A root rule whose constructor throws if asked to.
class throwing_root_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = bool;
  explicit throwing_root_rule(bool const fail) {
    if (fail) {
      throw std::runtime_error{"rule construction failed"};
    }
  }
  [[nodiscard]] char const* name() const noexcept override { return "throwing"; }
};

using uint_vector_context = peejay::rules::context<policies, uint_vector_rule>;

// NOLINTNEXTLINE
TEST(RuleContextReset, AfterDocument) {
  bool b1 = false;
  bool_context context{&b1};
  auto parser = context.make_parser();
  parser.input(u8"true"sv).eof();
  ASSERT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  bool b2 = true;
  context.reset(&b2);
  parser.reset(&context).input(u8"false"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_TRUE(b1);
  EXPECT_FALSE(b2);
}

// NOLINTNEXTLINE
TEST(RuleContextReset, AfterError) {
  bool b = false;
  bool_context context{&b};
  auto parser = context.make_parser();
  parser.input(u8"1"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::unexpected_number));
  context.reset(&b);
  parser.reset(&context).input(u8"true"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_TRUE(b);
}

// NOLINTNEXTLINE
TEST(RuleContextReset, MidDocumentAbandonsRules) {
  std::vector<uint_type> v1;
  uint_vector_context context{&v1};
  auto parser = context.make_parser();
  // The rule for the second element is active: the parser has not yet seen the end of the number.
  parser.input(u8"[1,2"sv);
  ASSERT_EQ(v1, (std::vector<uint_type>{1, 0}));
  std::vector<uint_type> v2;
  context.reset(&v2);
  // The partial element was removed when the array rule was abandoned.
  EXPECT_EQ(v1, (std::vector<uint_type>{1}));
  parser.reset(&context).input(u8"[3]"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(v2, (std::vector<uint_type>{3}));
}

// NOLINTNEXTLINE
TEST(RuleContextReset, NewParser) {
  bool b = false;
  bool_context context{&b};
  context.make_parser().input(u8"false"sv).eof();
  auto parser = context.reset(&b).make_parser();
  parser.input(u8"true"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_TRUE(b);
}

// NOLINTNEXTLINE
TEST(RuleContextReset, RootConstructorThrows) {
  peejay::rules::context<policies, throwing_root_rule> context{false};
  EXPECT_THROW(context.reset(true), std::runtime_error);
  // The stack is empty, as it is after a parse has ended.
  auto parser = context.make_parser();
  parser.input(u8"true"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::no_active_rule));
}

// NOLINTNEXTLINE
TEST(RuleContext, ParserConstructedFromContext) {
  bool b = false;
  bool_context context{&b};
  // The parser's constructor forwards the context pointer to the backend's constructor.
  peejay::parser<peejay::rules::backend<policies>> parser{&context};
  parser.input(u8"true"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_TRUE(b);
}

// NOLINTNEXTLINE
TEST(RuleDefaults, Events) {
  int unused = 0;
  EXPECT_EQ(parse<defaults_rule>(u8"null"sv, &unused), make_error_code(error::unexpected_null));
  EXPECT_EQ(parse<defaults_rule>(u8"true"sv, &unused), make_error_code(error::unexpected_boolean));
  EXPECT_EQ(parse<defaults_rule>(u8"1"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse<defaults_rule>(u8"1.5"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse<defaults_rule>(u8R"("s")"sv, &unused), make_error_code(error::unexpected_string));
  EXPECT_EQ(parse<defaults_rule>(u8"[]"sv, &unused), make_error_code(error::unexpected_end_array));
  // The default element() succeeds, so the error comes from the value which follows it.
  EXPECT_EQ(parse<defaults_rule>(u8"[1]"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse<defaults_rule>(u8"{}"sv, &unused), make_error_code(error::unexpected_end_object));
  EXPECT_EQ(parse<defaults_rule>(u8R"({"k":1})"sv, &unused), make_error_code(error::unexpected_object_key));
}

// NOLINTNEXTLINE
TEST(RuleContext, GreedyRootConstructor) {
  int unused = 0;
  greedy_context context{&unused};
  auto parser = context.make_parser();
  parser.input(u8"1"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(error::unexpected_number));
}

// NOLINTNEXTLINE
TEST(RuleDefaults, BeginObject) {
  bool b = false;
  EXPECT_EQ(parse<peejay::rules::bool_rule<policies>>(u8"{}"sv, &b), make_error_code(error::unexpected_object));
}

// NOLINTNEXTLINE
TEST(RuleError, Messages) {
  EXPECT_STREQ(make_error_code(error::none).category().name(), "peejay rule");
  EXPECT_EQ(make_error_code(error::none).message(), "none");
  EXPECT_EQ(make_error_code(error::unexpected_null).message(), "unexpected null");
  EXPECT_EQ(make_error_code(error::unexpected_boolean).message(), "unexpected boolean");
  EXPECT_EQ(make_error_code(error::unexpected_number).message(), "unexpected number");
  EXPECT_EQ(make_error_code(error::unexpected_string).message(), "unexpected string");
  EXPECT_EQ(make_error_code(error::unexpected_array).message(), "unexpected array");
  EXPECT_EQ(make_error_code(error::unexpected_end_array).message(), "unexpected end of array");
  EXPECT_EQ(make_error_code(error::unexpected_object).message(), "unexpected object");
  EXPECT_EQ(make_error_code(error::unexpected_object_key).message(), "unexpected object key");
  EXPECT_EQ(make_error_code(error::unexpected_end_object).message(), "unexpected end object");
  EXPECT_EQ(make_error_code(error::unrecognized_property).message(), "unrecognized property");
  EXPECT_EQ(make_error_code(error::incomplete_object).message(), "incomplete object");
  EXPECT_EQ(make_error_code(error::too_many_array_elements).message(), "too many array elements");
  EXPECT_EQ(make_error_code(error::unrecognized_enumerator).message(), "unrecognized enumerator");
  EXPECT_EQ(make_error_code(error::no_active_rule).message(), "no active rule");
  EXPECT_EQ(make_error_code(static_cast<error>(-1)).message(), "unknown error");
}

// NOLINTNEXTLINE
TEST(RuleTerminals, Names) {
  using namespace peejay::rules;
  bool b = false;
  policies::integer_type i = 0;
  std::make_unsigned_t<policies::integer_type> u = 0;
  policies::float_type f = 0.0;
  std::u8string s;
  EXPECT_STREQ(bool_rule<policies>{&b}.name(), "bool");
  EXPECT_STREQ(integer_rule<policies>{&i}.name(), "integer");
  EXPECT_STREQ(uinteger_rule<policies>{&u}.name(), "unsigned-integer");
  short sh = 0;
  unsigned short ush = 0;
  EXPECT_STREQ((integer_rule<policies, short>{&sh}.name()), "integer");
  EXPECT_STREQ((integer_rule<policies, unsigned short>{&ush}.name()), "unsigned-integer");
  EXPECT_STREQ(float_rule<policies>{&f}.name(), "float");
  EXPECT_STREQ(string_rule<policies>{&s}.name(), "string");
  short_string av;
  EXPECT_STREQ((string_rule<policies, short_string>{&av}.name()), "string");
}

}  // end anonymous namespace
