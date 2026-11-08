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
#include "peejay/rules/helpers.hpp"
#include "peejay/rules/terminals.hpp"

// Standard library
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <vector>

// Google test
#include <gtest/gtest.h>

using namespace std::string_view_literals;

namespace {

static_assert(peejay::rules::emplace_back_container<std::vector<int>>);
static_assert(peejay::rules::emplace_back_container<std::deque<int>>);
static_assert(peejay::rules::emplace_back_container<std::list<int>>);
static_assert(!peejay::rules::emplace_back_container<std::vector<bool>>, "emplace_back() returns a proxy");
static_assert(!peejay::rules::emplace_back_container<std::array<int, 2>>, "no emplace_back()");
static_assert(!peejay::rules::emplace_back_container<std::set<int>>, "no emplace_back()");

struct address {
  std::string name;
  unsigned port = 0;
};

template <peejay::policy Policies> class address_rule final : public peejay::rules::object_helper<Policies, 3> {
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

template <peejay::policy Policies>
using multiport_rule = peejay::rules::homogeneous_array_helper<Policies, std::vector<unsigned>>;

template <peejay::policy Policies>
class server_rule final
    : public peejay::rules::object_helper<Policies, 3, address_rule<Policies>, multiport_rule<Policies>> {
  using inherited = peejay::rules::object_helper<Policies, 3, address_rule<Policies>, multiport_rule<Policies>>;

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

template <peejay::policy Policies>
class multiport_address_rule final : public peejay::rules::object_helper<Policies, 2, multiport_rule<Policies>> {
  using inherited = peejay::rules::object_helper<Policies, 2, multiport_rule<Policies>>;

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
TEST(ObjectHelperUserRules, MultiplePorts) {
  multiport_address addr;
  using policies = peejay::default_policies;
  using my_rule = multiport_address_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&addr};
  auto parser = context.make_parser();
  parser.input(u8R"({"name":"http","ports":[80,8080]})"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(addr.name, "http");
  EXPECT_EQ(addr.ports, (std::vector<unsigned>{80, 8080}));
}

// NOLINTNEXTLINE
TEST(ObjectHelperUserRules, TwoUserRules) {
  server s;
  using policies = peejay::default_policies;
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
TEST(ObjectHelperUserRules, UserRuleError) {
  server s;
  using policies = peejay::default_policies;
  using my_rule = server_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&s};
  auto parser = context.make_parser();
  parser.input(u8R"({"host":"example","address":{"name":"web","colour":"red"},"ports":[]})"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(peejay::rules::error::unrecognized_property));
}

// NOLINTNEXTLINE
TEST(ObjectHelperUserRules, EmptyArray) {
  multiport_address addr;
  using policies = peejay::default_policies;
  using my_rule = multiport_address_rule<policies>;
  peejay::rules::context<policies, my_rule> context{&addr};
  auto parser = context.make_parser();
  parser.input(u8R"({"name":"http","ports":[ ]})"sv).eof();
  EXPECT_FALSE(parser.has_error()) << "Real error was: " << parser.last_error().message();
  EXPECT_EQ(addr.name, "http");
  EXPECT_TRUE(addr.ports.empty());
}

//-MARK: context

using test_policies = peejay::default_policies;
using test_context_base = peejay::rules::context_base<test_policies>;

// A frame's size is the sizeof the rule because the base class's alignment is sufficient.
using bool_context = peejay::rules::context<test_policies, peejay::rules::bool_rule<test_policies>>;
static_assert(bool_context::stack_bytes == sizeof(peejay::rules::bool_rule<test_policies>));

// A container and its element.
using multiport_context = peejay::rules::context<test_policies, multiport_rule<test_policies>>;
static_assert(multiport_context::stack_bytes ==
              sizeof(multiport_rule<test_policies>) + sizeof(peejay::rules::uinteger_rule<test_policies>));

// The longest path through server_rule is through address_rule to one of the terminals.
using server_context = peejay::rules::context<test_policies, server_rule<test_policies>>;
static_assert(server_context::stack_bytes == sizeof(server_rule<test_policies>) + sizeof(address_rule<test_policies>) +
                                                 sizeof(peejay::rules::string_rule<test_policies>));

// An array whose elements are arrays.
struct tree {
  std::vector<tree> children;
};
class tree_rule final : public peejay::rules::rule<test_policies> {
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
using tree_context = peejay::rules::context<test_policies, tree_rule>;
static_assert(tree_context::stack_bytes == test_policies::max_stack_depth * sizeof(tree_rule));

// Two rules which push each other. Rule 'small' has fewer members than rule 'large'. Each counts the arrays seen.
class large_rule;
class small_rule final : public peejay::rules::rule<test_policies> {
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
class large_rule final : public peejay::rules::rule<test_policies> {
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
static_assert(peejay::rules::context<test_policies, small_rule>::stack_bytes ==
              test_policies::max_stack_depth * sizeof(large_rule));

// A rule which never pops and pushes a new instance of itself for each array element. Each instance uses stack space
// without any change to the depth of the input so the parser's own nesting limit does not apply.
class runaway_rule final : public peejay::rules::rule<test_policies> {
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
  std::error_code integer_value(test_context_base* const, test_policies::integer_type const) override { return {}; }

private:
  int* count_;
};

// A rule which requires stricter alignment than the rule base class.
struct aligned_value {
  test_policies::integer_type value = 0;
  bool aligned = false;
};
class alignas(64) aligned_rule final : public peejay::rules::rule<test_policies> {
public:
  using value_type = aligned_value;
  explicit aligned_rule(aligned_value* const v) noexcept : value_{v} {}
  [[nodiscard]] char const* name() const noexcept override { return "aligned"; }
  std::error_code integer_value(test_context_base* const context, test_policies::integer_type const value) override {
    value_->value = value;
    value_->aligned = reinterpret_cast<std::uintptr_t>(this) % alignof(aligned_rule) == 0U;
    return this->pop(context);
  }

private:
  aligned_value* value_;
};
using aligned_array_rule =
    peejay::rules::homogeneous_array_helper<test_policies, std::vector<aligned_value>, aligned_rule>;
using aligned_context = peejay::rules::context<test_policies, aligned_array_rule>;
static_assert(aligned_context::stack_bytes == sizeof(aligned_array_rule) + sizeof(aligned_rule) +
                                                  alignof(aligned_rule) - alignof(peejay::rules::rule<test_policies>));

// A rule whose constructor accepts anything, so a context holding it is "constructible" from a context. The context's
// forwarding constructor must not be used to copy or move a context.
class greedy_rule final : public peejay::rules::rule<test_policies> {
public:
  using value_type = int;
  template <typename T> explicit greedy_rule(T&&) noexcept {}
  [[nodiscard]] char const* name() const noexcept override { return "greedy"; }
};
using greedy_context = peejay::rules::context<test_policies, greedy_rule>;
static_assert(std::is_constructible_v<greedy_context, int*>);
static_assert(!std::is_constructible_v<greedy_context, greedy_context&>);
static_assert(!std::is_copy_constructible_v<greedy_context>);
static_assert(!std::is_move_constructible_v<greedy_context>);
static_assert(!std::is_copy_constructible_v<bool_context>);
static_assert(!std::is_move_constructible_v<bool_context>);

// A rule which uses the default implementation of each event except for those needed to reach the others.
class defaults_rule final : public peejay::rules::rule<test_policies> {
public:
  using value_type = int;
  explicit defaults_rule(int*) noexcept {}
  [[nodiscard]] char const* name() const noexcept override { return "defaults"; }
  std::error_code begin_array(test_context_base* const) override { return {}; }
  std::error_code begin_object(test_context_base* const) override { return {}; }
};

template <typename Rule, typename T> std::error_code parse_with(std::u8string_view const src, T* const out) {
  peejay::rules::context<test_policies, Rule> context{out};
  auto parser = context.make_parser();
  parser.input(src).eof();
  return parser.last_error();
}

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRule) {
  tree t;
  EXPECT_FALSE(parse_with<tree_rule>(u8"[[],[[]]]"sv, &t));
  ASSERT_EQ(t.children.size(), 2U);
  EXPECT_TRUE(t.children[0].children.empty());
  ASSERT_EQ(t.children[1].children.size(), 1U);
  EXPECT_TRUE(t.children[1].children[0].children.empty());
}

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRuleAtMaximumDepth) {
  static_assert(test_policies::max_stack_depth == 8);
  tree t;
  EXPECT_FALSE(parse_with<tree_rule>(u8"[[[[[[[[]]]]]]]]"sv, &t));
}

// NOLINTNEXTLINE
TEST(RuleContext, RecursiveRuleTooDeep) {
  tree t;
  EXPECT_EQ(parse_with<tree_rule>(u8"[[[[[[[[[]]]]]]]]]"sv, &t), make_error_code(peejay::error::nesting_too_deep));
}

// NOLINTNEXTLINE
TEST(RuleContext, MutuallyRecursiveRules) {
  int arrays = 0;
  EXPECT_FALSE(parse_with<small_rule>(u8"[[[[]]]]"sv, &arrays));
  EXPECT_EQ(arrays, 4);
}

// NOLINTNEXTLINE
TEST(RuleContext, StackExhausted) {
  int pushes = 0;
  EXPECT_EQ(parse_with<runaway_rule>(u8"[1,2,3,4,5,6,7,8,9]"sv, &pushes),
            make_error_code(peejay::error::nesting_too_deep));
  // The root rule occupies one of the entries.
  EXPECT_EQ(pushes, 7);
}

// NOLINTNEXTLINE
TEST(RuleContext, StrictlyAlignedRule) {
  std::vector<aligned_value> v;
  EXPECT_FALSE(parse_with<aligned_array_rule>(u8"[1,2]"sv, &v));
  ASSERT_EQ(v.size(), 2U);
  EXPECT_EQ(v[0].value, 1);
  EXPECT_TRUE(v[0].aligned);
  EXPECT_EQ(v[1].value, 2);
  EXPECT_TRUE(v[1].aligned);
}

// NOLINTNEXTLINE
TEST(RuleDefaults, Events) {
  int unused = 0;
  using peejay::rules::error;
  EXPECT_EQ(parse_with<defaults_rule>(u8"null"sv, &unused), make_error_code(error::unexpected_null));
  EXPECT_EQ(parse_with<defaults_rule>(u8"true"sv, &unused), make_error_code(error::unexpected_boolean));
  EXPECT_EQ(parse_with<defaults_rule>(u8"1"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse_with<defaults_rule>(u8"1.5"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse_with<defaults_rule>(u8R"("s")"sv, &unused), make_error_code(error::unexpected_string));
  EXPECT_EQ(parse_with<defaults_rule>(u8"[]"sv, &unused), make_error_code(error::unexpected_end_array));
  // The default element() succeeds, so the error comes from the value which follows it.
  EXPECT_EQ(parse_with<defaults_rule>(u8"[1]"sv, &unused), make_error_code(error::unexpected_number));
  EXPECT_EQ(parse_with<defaults_rule>(u8"{}"sv, &unused), make_error_code(error::unexpected_end_object));
  EXPECT_EQ(parse_with<defaults_rule>(u8R"({"k":1})"sv, &unused), make_error_code(error::unexpected_object_key));
}

// NOLINTNEXTLINE
TEST(RuleContext, GreedyRootConstructor) {
  int unused = 0;
  greedy_context context{&unused};
  auto parser = context.make_parser();
  parser.input(u8"1"sv).eof();
  EXPECT_EQ(parser.last_error(), make_error_code(peejay::rules::error::unexpected_number));
}

// NOLINTNEXTLINE
TEST(RuleDefaults, BeginObject) {
  bool b = false;
  EXPECT_EQ(parse_with<peejay::rules::bool_rule<test_policies>>(u8"{}"sv, &b),
            make_error_code(peejay::rules::error::unexpected_object));
}

// NOLINTNEXTLINE
TEST(RuleError, Messages) {
  using peejay::rules::error;
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
  EXPECT_EQ(make_error_code(static_cast<error>(-1)).message(), "unknown error");
}

// NOLINTNEXTLINE
TEST(RuleTerminals, Names) {
  using namespace peejay::rules;
  bool b = false;
  test_policies::integer_type i = 0;
  std::make_unsigned_t<test_policies::integer_type> u = 0;
  test_policies::float_type f = 0.0;
  std::u8string s;
  EXPECT_STREQ(bool_rule<test_policies>{&b}.name(), "bool");
  EXPECT_STREQ(integer_rule<test_policies>{&i}.name(), "integer");
  EXPECT_STREQ(uinteger_rule<test_policies>{&u}.name(), "unsigned-integer");
  short sh = 0;
  unsigned short ush = 0;
  EXPECT_STREQ((integer_rule<test_policies, short>{&sh}.name()), "integer");
  EXPECT_STREQ((integer_rule<test_policies, unsigned short>{&ush}.name()), "unsigned-integer");
  EXPECT_STREQ(float_rule<test_policies>{&f}.name(), "float");
  EXPECT_STREQ(string_rule<test_policies>{&s}.name(), "string");
}

}  // end anonymous namespace
