//===- unittests/test_emit.cpp --------------------------------------------===//
//*                 _ _    *
//*   ___ _ __ ___ (_) |_  *
//*  / _ \ '_ ` _ \| | __| *
//* |  __/ | | | | | | |_  *
//*  \___|_| |_| |_|_|\__| *
//*                        *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "json/emit.hpp"

// standard library
#include <sstream>
#include <string>

// 3rd party
#include <gmock/gmock.h>

using namespace peejay;
using namespace std::string_literals;

TEST (Emit, Null) {
  std::stringstream os;
  emit (os, dom::element{dom::null{}});
  EXPECT_EQ (os.str (), "null\n");
}

TEST (Emit, True) {
  std::stringstream os;
  emit (os, dom::element{true});
  EXPECT_EQ (os.str (), "true\n");
}

TEST (Emit, False) {
  std::stringstream os;
  emit (os, dom::element{false});
  EXPECT_EQ (os.str (), "false\n");
}

TEST (Emit, Zero) {
  std::stringstream os;
  emit (os, dom::element{uint64_t{0}});
  EXPECT_EQ (os.str (), "0\n");
}

TEST (Emit, One) {
  std::stringstream os;
  emit (os, dom::element{uint64_t{1}});
  EXPECT_EQ (os.str (), "1\n");
}

TEST (Emit, MinusOne) {
  std::stringstream os;
  emit (os, dom::element{int64_t{-1}});
  EXPECT_EQ (os.str (), "-1\n");
}

TEST (Emit, Float) {
  std::stringstream os;
  emit (os, dom::element{2.2});
  EXPECT_EQ (os.str (), "2.2\n");
}

TEST (Emit, StringNoEscapes) {
  std::stringstream os;
  emit (os, dom::element{"string"});
  EXPECT_EQ (os.str (), "\"string\"\n");
}

TEST (Emit, StringBackslashT) {
  std::stringstream os;
  emit (os, dom::element{"abc\tdef"});
  EXPECT_EQ (os.str (), "\"abc\\tdef\"\n");
}

TEST (Emit, EmptyArray) {
  std::stringstream os;
  emit (os, dom::element{dom::array{}});
  EXPECT_EQ (os.str (), "[]\n");
}

TEST (Emit, ArrayOneMember) {
  std::stringstream os;
  emit (os, dom::element{dom::array{dom::element{uint64_t{1}}}});
  EXPECT_EQ (os.str (), R"([
  1
]
)");
}

TEST (Emit, ArrayTwoMembers) {
  std::stringstream os;
  emit (os, dom::element{dom::array{dom::element{uint64_t{1}},
                                    dom::element{uint64_t{2}}}});
  EXPECT_EQ (os.str (), R"([
  1,
  2
]
)");
}

TEST (Emit, EmptyObject) {
  std::stringstream os;
  emit (os, dom::element{dom::object{}});
  EXPECT_EQ (os.str (), "{}\n");
}

TEST (Emit, ObjectOneMember) {
  std::stringstream os;
  dom::object obj;
  obj["key"] = dom::element{"value"s};
  emit (os, dom::element{std::move (obj)});
  EXPECT_EQ (os.str (), R"({
  "key": "value"
}
)");
}

TEST (Emit, ObjectArrayMember) {
  std::stringstream os;
  dom::object obj;
  obj["key1"] = dom::element{
      dom::array{dom::element{uint64_t{1}}, dom::element{uint64_t{2}}}};
  emit (os, dom::element{std::move (obj)});
  EXPECT_EQ (os.str (), R"({
  "key1": [
    1,
    2
  ]
}
)");
}
