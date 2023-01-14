#include <gmock/gmock.h>

#include <algorithm>
#include <ostream>

#include "peejay/json.hpp"

using peejay::grammar_rule;
using peejay::details::code_point_grammar_rule;

TEST (CodePointRun, LatinSmallLetterA) {
  constexpr auto a = char32_t{0x0061};  // LATIN SMALL LETTER A

  EXPECT_EQ (code_point_grammar_rule (char32_t{a - 1}), grammar_rule::none);
  EXPECT_EQ (code_point_grammar_rule (a), grammar_rule::identifier_start);
  EXPECT_EQ (code_point_grammar_rule (char32_t{a + 1}), grammar_rule::identifier_start);
  EXPECT_EQ (code_point_grammar_rule (char32_t{a + 25}), grammar_rule::identifier_start);
  EXPECT_EQ (code_point_grammar_rule (char32_t{a + 26}), grammar_rule::none);
}

TEST (CodePointRun, Null) {
  EXPECT_EQ (code_point_grammar_rule (char32_t{0}), grammar_rule::none);
}

TEST (CodePointRun, Space) {
  EXPECT_EQ (code_point_grammar_rule (char32_t{0x001F}), grammar_rule::none);
  EXPECT_EQ (code_point_grammar_rule (char32_t{0x0020}), grammar_rule::whitespace);
  EXPECT_EQ (code_point_grammar_rule (char32_t{0x0021}), grammar_rule::none);
}

TEST (CodePointRun, MaxCodePoint) {
  EXPECT_EQ (code_point_grammar_rule (char32_t{0x0010FFFF}), grammar_rule::none);
}

TEST (CodePointRun, VariationSelector17) {
  static constexpr auto vs17 = char32_t{0xe0100};  // VARIATION SELECTOR-17

  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17 - 1}), grammar_rule::none);
  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17}), grammar_rule::identifier_part);
  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17 + 239}), grammar_rule::identifier_part);
  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17 + 240}), grammar_rule::none);
}
