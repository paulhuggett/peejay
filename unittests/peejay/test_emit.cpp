//===- unittests/peejay/test_emit.cpp -------------------------------------===//
//*                 _ _    *
//*   ___ _ __ ___ (_) |_  *
//*  / _ \ '_ ` _ \| | __| *
//* |  __/ | | | | | | |_  *
//*  \___|_| |_| |_|_|\__| *
//*                        *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "peejay/json/emit.hpp"

// standard library
#include <sstream>
#include <string>

// 3rd party
#include <gmock/gmock.h>

using namespace std::string_literals;

using peejay::array;
using peejay::element;
using peejay::emit;
using peejay::null;
using peejay::object;

// NOLINTNEXTLINE
TEST(Emit, Nothing) {
  std::stringstream os;
  emit(os, std::nullopt);
  EXPECT_EQ(os.str(), "\n");
}

// NOLINTNEXTLINE
TEST(Emit, Null) {
  std::stringstream os;
  emit(os, element{null{}});
  EXPECT_EQ(os.str(), "null\n");
}

// NOLINTNEXTLINE
TEST(Emit, True) {
  std::stringstream os;
  emit(os, element{true});
  EXPECT_EQ(os.str(), "true\n");
}

// NOLINTNEXTLINE
TEST(Emit, False) {
  std::stringstream os;
  emit(os, element{false});
  EXPECT_EQ(os.str(), "false\n");
}

// NOLINTNEXTLINE
TEST(Emit, Zero) {
  std::stringstream os;
  emit(os, element{std::int64_t{0}});
  EXPECT_EQ(os.str(), "0\n");
}

// NOLINTNEXTLINE
TEST(Emit, One) {
  std::stringstream os;
  emit(os, element{std::int64_t{1}});
  EXPECT_EQ(os.str(), "1\n");
}

// NOLINTNEXTLINE
TEST(Emit, MinusOne) {
  std::stringstream os;
  emit(os, element{std::int64_t{-1}});
  EXPECT_EQ(os.str(), "-1\n");
}

// NOLINTNEXTLINE
TEST(Emit, Float) {
  std::stringstream os;
  emit(os, element{2.2});
  EXPECT_EQ(os.str(), "2.2\n");
}

// NOLINTNEXTLINE
TEST(Emit, StringNoEscapes) {
  std::stringstream os;
  emit(os, element{peejay::u8string{u8"string"}});
  EXPECT_EQ(os.str(), "\"string\"\n");
}

// NOLINTNEXTLINE
TEST(Emit, StringBackslashT) {
  std::stringstream os;
  emit(os, element{peejay::u8string{u8"abc\tdef"}});
  EXPECT_EQ(os.str(), "\"abc\\tdef\"\n");
}

// NOLINTNEXTLINE
TEST(Emit, EmptyArray) {
  std::stringstream os;
  emit(os, std::make_shared<array::element_type>());
  EXPECT_EQ(os.str(), "[]\n");
}

// NOLINTNEXTLINE
TEST(Emit, ArrayOneMember) {
  std::stringstream os;
  auto arr = std::make_shared<array::element_type>();
  arr->push_back(element{std::int64_t{1}});
  emit(os, arr);
  EXPECT_EQ(os.str(), R"([
  1
]
)");
}

// NOLINTNEXTLINE
TEST(Emit, ArrayTwoMembers) {
  std::stringstream os;
  auto arr = std::make_shared<array::element_type>();
  arr->push_back(std::int64_t{1});
  arr->push_back(std::int64_t{2});
  emit(os, arr);
  EXPECT_EQ(os.str(), R"([
  1,
  2
]
)");
}

// NOLINTNEXTLINE
TEST(Emit, EmptyObject) {
  std::stringstream os;
  emit(os, std::make_shared<object::element_type>());
  EXPECT_EQ(os.str(), "{}\n");
}

// NOLINTNEXTLINE
TEST(Emit, ObjectOneMember) {
  std::stringstream os;
  auto obj = std::make_shared<object::element_type>();
  (*obj)[u8"key"] = element{u8"value"s};
  emit(os, obj);
  EXPECT_EQ(os.str(), R"({
  "key": "value"
}
)");
}

// NOLINTNEXTLINE
TEST(Emit, ObjectArrayMember) {
  std::stringstream os;
  auto arr = std::make_shared<array::element_type>();
  arr->push_back(std::int64_t{1});
  arr->push_back(std::int64_t{2});
  auto obj = std::make_shared<object::element_type>();
  (*obj)[u8"key1"] = element{std::move(arr)};
  emit(os, element{std::move(obj)});
  EXPECT_EQ(os.str(), R"({
  "key1": [
    1,
    2
  ]
}
)");
}
