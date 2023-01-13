#include <gmock/gmock.h>

#include <algorithm>
#include <ostream>

#include "peejay/cprun.hpp"

using peejay::grammar_rule;

std::ostream& operator<< (std::ostream& os, grammar_rule rule);
std::ostream& operator<< (std::ostream& os, grammar_rule rule) {
  switch (rule) {
  case grammar_rule::whitespace: os << "whitespace"; break;
  case grammar_rule::identifier_start: os << "identifier_start"; break;
  case grammar_rule::identifier_part: os << "identifier_part"; break;
  case grammar_rule::none: os << "none"; break;
  }
  return os;
}

static grammar_rule find (char32_t const code_point) {
  auto const end = std::end (peejay::code_point_runs);
  auto const it = std::lower_bound (
      std::begin (peejay::code_point_runs), end, peejay::cprun{code_point, 0, 0},
      [] (peejay::cprun const& cpr, peejay::cprun const& value) {
        return cpr.code_point + cpr.length < value.code_point;
      });
  return (it != end && it->code_point <= code_point)
             ? static_cast<grammar_rule> (it->rule)
             : grammar_rule::none;
}

TEST (CodePointRun, a) {
  auto const cp = char32_t{'a'};
  auto c = find (cp);
  EXPECT_EQ (c, grammar_rule::identifier_start);
}

TEST (CodePointRun, Null) {
  auto const cp = char32_t{0};
  auto c = find (cp);
  EXPECT_EQ (c, grammar_rule::none);
}

TEST (CodePointRun, Space) {
  EXPECT_EQ (find (char32_t{0x0020}), grammar_rule::whitespace);
}

TEST (CodePointRun, MaxCodePoint) {
  EXPECT_EQ (find (char32_t{0x0010FFFF}), grammar_rule::none);
}
