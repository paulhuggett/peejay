// This file was auto-generated. DO NOT EDIT!
#ifndef PEEJAY_CPRUN_HPP
#define PEEJAY_CPRUN_HPP

#include <array>
#include <cstdint>
#include <ostream>

namespace peejay {

enum class grammar_rule {
  whitespace = 0,
  identifier_start = 1,
  identifier_part = 2,
  none = 4
};

std::ostream& operator<< (std::ostream& os, grammar_rule rule);

struct cprun {
  uint_least32_t code_point: 21;
  uint_least32_t length: 9;
  uint_least32_t rule: 2;
};

extern std::array<cprun, 586> const code_point_runs;

} // end namespace peejay
#endif // PEEJAY_CPRUN_HPP
