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
#include "peejay/emit.hpp"

// standard library
#include <sstream>
#include <string>

// 3rd party
#include <gmock/gmock.h>

using namespace peejay;
using namespace std::string_literals;

// NOLINTNEXTLINE
TEST (Emit, Nothing) {
  std::stringstream os;
  emit (os, std::nullopt);
  EXPECT_EQ (os.str (), "\n");
}

// NOLINTNEXTLINE
TEST (Emit, Null) {
  std::stringstream os;
  emit (os, element{null{}});
  EXPECT_EQ (os.str (), "null\n");
}

// NOLINTNEXTLINE
TEST (Emit, True) {
  std::stringstream os;
  emit (os, element{true});
  EXPECT_EQ (os.str (), "true\n");
}

// NOLINTNEXTLINE
TEST (Emit, False) {
  std::stringstream os;
  emit (os, element{false});
  EXPECT_EQ (os.str (), "false\n");
}

// NOLINTNEXTLINE
TEST (Emit, Zero) {
  std::stringstream os;
  emit (os, element{uint64_t{0}});
  EXPECT_EQ (os.str (), "0\n");
}

// NOLINTNEXTLINE
TEST (Emit, One) {
  std::stringstream os;
  emit (os, element{uint64_t{1}});
  EXPECT_EQ (os.str (), "1\n");
}

// NOLINTNEXTLINE
TEST (Emit, MinusOne) {
  std::stringstream os;
  emit (os, element{int64_t{-1}});
  EXPECT_EQ (os.str (), "-1\n");
}

// NOLINTNEXTLINE
TEST (Emit, Float) {
  std::stringstream os;
  emit (os, element{2.2});
  EXPECT_EQ (os.str (), "2.2\n");
}

// NOLINTNEXTLINE
TEST (Emit, StringNoEscapes) {
  std::stringstream os;
  emit (os, element{u8"string"});
  EXPECT_EQ (os.str (), "\"string\"\n");
}

// NOLINTNEXTLINE
TEST (Emit, StringBackslashT) {
  std::stringstream os;
  emit (os, element{u8"abc\tdef"});
  EXPECT_EQ (os.str (), "\"abc\\tdef\"\n");
}

// NOLINTNEXTLINE
TEST (Emit, EmptyArray) {
  std::stringstream os;
  emit (os, element{array{}});
  EXPECT_EQ (os.str (), "[]\n");
}

// NOLINTNEXTLINE
TEST (Emit, ArrayOneMember) {
  std::stringstream os;
  emit (os, element{array{element{uint64_t{1}}}});
  EXPECT_EQ (os.str (), R"([
  1
]
)");
}

// NOLINTNEXTLINE
TEST (Emit, ArrayTwoMembers) {
  std::stringstream os;
  emit (os, element{array{element{uint64_t{1}}, element{uint64_t{2}}}});
  EXPECT_EQ (os.str (), R"([
  1,
  2
]
)");
}

// NOLINTNEXTLINE
TEST (Emit, EmptyObject) {
  std::stringstream os;
  emit (os, element{object{}});
  EXPECT_EQ (os.str (), "{}\n");
}

// NOLINTNEXTLINE
TEST (Emit, ObjectOneMember) {
  std::stringstream os;
  object obj;
  obj[u8"key"] = element{u8"value"s};
  emit (os, element{std::move (obj)});
  EXPECT_EQ (os.str (), R"({
  "key": "value"
}
)");
}

// NOLINTNEXTLINE
TEST (Emit, ObjectArrayMember) {
  std::stringstream os;
  object obj;
  obj[u8"key1"] = element{array{element{uint64_t{1}}, element{uint64_t{2}}}};
  emit (os, element{std::move (obj)});
  EXPECT_EQ (os.str (), R"({
  "key1": [
    1,
    2
  ]
}
)");
}
