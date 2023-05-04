//===- unittests/test_stack.cpp -------------------------------------------===//
//*      _             _     *
//*  ___| |_ __ _  ___| | __ *
//* / __| __/ _` |/ __| |/ / *
//* \__ \ || (_| | (__|   <  *
//* |___/\__\__,_|\___|_|\_\ *
//*                          *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "peejay/stack.hpp"

// standard library
#include <array>
#include <limits>
#include <string>
#include <vector>

// 3rd party
#include <gtest/gtest.h>

using namespace std::string_literals;

TEST (Stack, DefaultCtor) {
  peejay::stack<int> stack;
  EXPECT_TRUE (stack.empty ());
}

TEST (Stack, CopyCtor) {
  peejay::stack<int> s1;
  s1.push (1);
  peejay::stack<int> s2 = s1;
  ASSERT_EQ (s2.size (), 1);
  EXPECT_EQ (s2.top (), 1);
  s2.pop ();
  EXPECT_TRUE (s2.empty ());

  ASSERT_EQ (s1.size (), 1);
  EXPECT_EQ (s1.top (), 1);
  s1.pop ();
  EXPECT_TRUE (s1.empty ());
}

#if PEEJAY_CXX20
TEST (Stack, IteratorCtor) {
  std::array<int, 4> const a = {{4, 3, 2, 1}};
  peejay::stack stack (std::begin (a), std::end (a));
  ASSERT_EQ (stack.size (), 4);
  EXPECT_EQ (stack.top (), 1);
  stack.pop ();
  EXPECT_EQ (stack.top (), 2);
  stack.pop ();
  EXPECT_EQ (stack.top (), 3);
  stack.pop ();
  EXPECT_EQ (stack.top (), 4);
  stack.pop ();
  EXPECT_TRUE (stack.empty ());
}
#endif

TEST (Stack, ContainerCtor) {
  std::deque<int> const d{4, 3, 2, 1};
  peejay::stack stack (d);
  EXPECT_EQ (d.size (), 4);
  ASSERT_EQ (stack.size (), 4);
  EXPECT_EQ (stack.top (), 1);
  stack.pop ();
  EXPECT_EQ (stack.top (), 2);
  stack.pop ();
  EXPECT_EQ (stack.top (), 3);
  stack.pop ();
  EXPECT_EQ (stack.top (), 4);
  stack.pop ();
  EXPECT_TRUE (stack.empty ());
}

TEST (Stack, ContainerRValueRefCtor) {
  peejay::stack stack (std::deque<int>{4, 3, 2, 1});
  ASSERT_EQ (stack.size (), 4);
  EXPECT_EQ (stack.top (), 1);
  stack.pop ();
  EXPECT_EQ (stack.top (), 2);
  stack.pop ();
  EXPECT_EQ (stack.top (), 3);
  stack.pop ();
  EXPECT_EQ (stack.top (), 4);
  stack.pop ();
  EXPECT_TRUE (stack.empty ());
}

TEST (Stack, Eq) {
  std::vector<int> const a = {{4, 3, 2, 1}};
  peejay::stack s1 (a);
  peejay::stack s2 (a);
  EXPECT_TRUE (s1 == s2);
  EXPECT_FALSE (s1 != s2);
  EXPECT_FALSE (s1 < s2);
  EXPECT_TRUE (s1 <= s2);
  EXPECT_FALSE (s1 > s2);
  EXPECT_TRUE (s1 >= s2);
}

TEST (Stack, Push1Value) {
  peejay::stack<int> stack;
  stack.push (17);
  ASSERT_EQ (1U, stack.size ());
  EXPECT_FALSE (stack.empty ());
  EXPECT_EQ (17, stack.top ());
}

TEST (ArrayStack, PushMoveValue) {
  peejay::stack<std::string> stack;
  auto value = "str"s;
  stack.push (std::move (value));

  std::string const& top = stack.top ();
  EXPECT_EQ ("str", top);
}

TEST (ArrayStack, Emplace) {
  peejay::stack<std::string> stack;
  stack.emplace ("str");
  EXPECT_EQ ("str", stack.top ());
}

TEST (ArrayStack, PushAndPop) {
  peejay::stack<int> stack;
  stack.push (31);
  ASSERT_EQ (1U, stack.size ());
  stack.pop ();
  EXPECT_EQ (0U, stack.size ());
  EXPECT_TRUE (stack.empty ());
}
