//===- unittests/uri/test_rule.cpp ----------------------------------------===//
//*             _       *
//*  _ __ _   _| | ___  *
//* | '__| | | | |/ _ \ *
//* | |  | |_| | |  __/ *
//* |_|   \__,_|_|\___| *
//*                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include <string>
#include <vector>

#include "uri/rule.hpp"

using namespace std::string_literals;

using testing::ElementsAre;
using uri::char_range;
using uri::rule;
using uri::single_char;

struct Rule : public testing::Test {
  std::vector<std::string> output;

  auto remember() {
    return [this](std::string_view str) { output.emplace_back(str); };
  }
};
// NOLINTNEXTLINE
TEST_F(Rule, Concat) {
  bool const ok = rule("ab")
                      .concat([](rule const& r) { return r.single_char('a'); }, remember())
                      .concat([](rule const& r) { return r.single_char('b'); }, remember())
                      .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "b"));
}
// NOLINTNEXTLINE
TEST_F(Rule, ConcatAcceptorOrder) {
  bool const ok = rule("ab")
                      .concat(
                          [this](rule const& r) {
                            return r.concat([](rule const& r1) { return r1.single_char('a'); }, remember())
                                .concat([](rule const& r2) { return r2.single_char('b'); }, remember())
                                .matched("ab", r);
                          },
                          [this](std::string_view str) { output.push_back("post "s + std::string{str}); })
                      .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "b", "post ab"));
}
// NOLINTNEXTLINE
TEST_F(Rule, FirstAlternative) {
  bool const ok =
      rule("ab")
          .concat(single_char('a'), remember())
          .alternative([this](rule const& r) { return r.concat(single_char('b'), remember()).matched("b", r); },
                       [this](rule const& r) { return r.concat(single_char('c'), remember()).matched("c", r); })
          .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "b"));
}
// NOLINTNEXTLINE
TEST_F(Rule, SecondAlternative) {
  bool const ok =
      rule("ac")
          .concat(single_char('a'), remember())
          .alternative([this](rule const& r) { return r.concat(single_char('b'), remember()).matched("b", r); },
                       [this](rule const& r) { return r.concat(single_char('c'), remember()).matched("c", r); })
          .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "c"));
}
// NOLINTNEXTLINE
TEST_F(Rule, AlternativeFail) {
  bool const ok =
      rule("ad")
          .concat(single_char('a'), remember())
          .alternative([this](rule const& r) { return r.concat(single_char('b'), remember()).matched("b", r); },
                       [this](rule const& r) { return r.concat(single_char('c'), remember()).matched("c", r); })
          .done();
  EXPECT_FALSE(ok);
  EXPECT_TRUE(output.empty());
}
// NOLINTNEXTLINE
TEST_F(Rule, Star) {
  bool const ok =
      rule("aaa").star([this](rule const& r) { return r.concat(single_char('a'), remember()).matched("a", r); }).done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "a", "a"));
}
// NOLINTNEXTLINE
TEST_F(Rule, StarConcat) {
  bool const ok = rule("aaab")
                      .star([this](rule const& r) { return r.concat(single_char('a'), remember()).matched("a", r); })
                      .concat(single_char('b'), remember())
                      .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "a", "a", "b"));
}
// NOLINTNEXTLINE
TEST_F(Rule, Star2) {
  bool const ok =
      rule("/")
          .star([this](rule const& r1) {
            return r1.concat(single_char('/'), remember())
                .concat([](rule const& r2) { return r2.star(char_range('a', 'z')).matched("a-z", r2); }, remember())
                .matched("*(a-z)", r1);
          })
          .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("/", ""));
}
// NOLINTNEXTLINE
TEST_F(Rule, OptionalPresent) {
  bool const ok = rule("abc")
                      .concat(single_char('a'), remember())
                      .optional(single_char('b'), remember())
                      .concat(single_char('c'), remember())
                      .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "b", "c"));
}
// NOLINTNEXTLINE
TEST_F(Rule, OptionalNotPresent) {
  bool const ok = rule("ac")
                      .concat(single_char('a'), remember())
                      .optional(single_char('b'), remember())
                      .concat(single_char('c'), remember())
                      .done();
  EXPECT_TRUE(ok);
  EXPECT_THAT(output, ElementsAre("a", "c"));
}
