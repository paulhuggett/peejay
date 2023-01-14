#include <gmock/gmock.h>

#include <algorithm>
#include <ostream>

#include "peejay/cprun.hpp"

using peejay::grammar_rule;

static grammar_rule find (char32_t const code_point) {
  auto const end = std::end (peejay::code_point_runs);
  auto const it = std::lower_bound (
      std::begin (peejay::code_point_runs), end,
      peejay::cprun{code_point, 0, 0},
      [] (peejay::cprun const& cpr, peejay::cprun const& value) {
        return cpr.code_point + cpr.length < value.code_point;
      });
  return (it != end && code_point >= it->code_point &&
          code_point < static_cast<char32_t> (it->code_point + it->length))
             ? static_cast<grammar_rule> (it->rule)
             : grammar_rule::none;
}

TEST (CodePointRun, LatinSmallLetterA) {
  constexpr auto a = char32_t{0x0061};  // LATIN SMALL LETTER A

  EXPECT_EQ (find (char32_t{a - 1}), grammar_rule::none);
  EXPECT_EQ (find (a), grammar_rule::identifier_start);
  EXPECT_EQ (find (char32_t{a + 1}), grammar_rule::identifier_start);
  EXPECT_EQ (find (char32_t{a + 25}), grammar_rule::identifier_start);
  EXPECT_EQ (find (char32_t{a + 26}), grammar_rule::none);
}

TEST (CodePointRun, Null) {
  EXPECT_EQ (find (char32_t{0}), grammar_rule::none);
}

TEST (CodePointRun, Space) {
  EXPECT_EQ (find (char32_t{0x001F}), grammar_rule::none);
  EXPECT_EQ (find (char32_t{0x0020}), grammar_rule::whitespace);
  EXPECT_EQ (find (char32_t{0x0021}), grammar_rule::none);
}

TEST (CodePointRun, MaxCodePoint) {
  EXPECT_EQ (find (char32_t{0x0010FFFF}), grammar_rule::none);
}

TEST (CodePointRun, VariationSelector17) {
  static constexpr auto vs17 = char32_t{0xe0100};  // VARIATION SELECTOR-17

  EXPECT_EQ (find (char32_t{vs17 - 1}), grammar_rule::none);
  EXPECT_EQ (find (char32_t{vs17}), grammar_rule::identifier_part);
  EXPECT_EQ (find (char32_t{vs17 + 239}), grammar_rule::identifier_part);
  EXPECT_EQ (find (char32_t{vs17 + 240}), grammar_rule::none);
}
