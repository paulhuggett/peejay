//===- unittests/test_cprun.cpp -------------------------------------------===//
//*                               *
//*   ___ _ __  _ __ _   _ _ __   *
//*  / __| '_ \| '__| | | | '_ \  *
//* | (__| |_) | |  | |_| | | | | *
//*  \___| .__/|_|   \__,_|_| |_| *
//*      |_|                      *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include <algorithm>
#include <ostream>

#include "peejay/json.hpp"

using peejay::grammar_rule;
using peejay::details::code_point_grammar_rule;

// NOLINTNEXTLINE
TEST (CodePointRun, LatinSmallLetterA) {
  constexpr auto a = char32_t{0x0061};  // LATIN SMALL LETTER A

  EXPECT_FALSE (code_point_grammar_rule (char32_t{a - 1}));
  EXPECT_EQ (code_point_grammar_rule (a),
             std::optional{grammar_rule::identifier_start});
  EXPECT_EQ (code_point_grammar_rule (char32_t{a + 1}),
             std::optional{grammar_rule::identifier_start});
  EXPECT_EQ (code_point_grammar_rule (char32_t{a + 25}),
             std::optional{grammar_rule::identifier_start});
  EXPECT_FALSE (code_point_grammar_rule (char32_t{a + 26}));
}

// NOLINTNEXTLINE
TEST (CodePointRun, Null) {
  EXPECT_FALSE (code_point_grammar_rule (char32_t{0}));
}

// NOLINTNEXTLINE
TEST (CodePointRun, Space) {
  EXPECT_FALSE (code_point_grammar_rule (char32_t{0x001F}));
  EXPECT_EQ (code_point_grammar_rule (char32_t{0x0020}),
             std::optional{grammar_rule::whitespace});
  EXPECT_FALSE (code_point_grammar_rule (char32_t{0x0021}));
}

// NOLINTNEXTLINE
TEST (CodePointRun, MaxCodePoint) {
  EXPECT_FALSE (code_point_grammar_rule (char32_t{0x0010FFFF}));
}

// NOLINTNEXTLINE
TEST (CodePointRun, VariationSelector17) {
  static constexpr auto vs17 = char32_t{0xe0100};  // VARIATION SELECTOR-17

  EXPECT_FALSE (code_point_grammar_rule (char32_t{vs17 - 1}));
  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17}),
             std::optional{grammar_rule::identifier_part});
  EXPECT_EQ (code_point_grammar_rule (char32_t{vs17 + 239}),
             std::optional{grammar_rule::identifier_part});
  EXPECT_FALSE (code_point_grammar_rule (char32_t{vs17 + 240}));
}
