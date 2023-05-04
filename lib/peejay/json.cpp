//===- lib/peejay/json.cpp ------------------------------------------------===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "peejay/json.hpp"

namespace peejay::details {

grammar_rule code_point_grammar_rule (char32_t const code_point) noexcept {
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const end = std::end (code_point_runs);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const it = std::lower_bound (
      std::begin (code_point_runs), end,
      cprun{code_point, 0, 0},
      [] (cprun const& cpr, cprun const& value) {
        return cpr.code_point + cpr.length < value.code_point;
      });
  return (it != end && code_point >= it->code_point &&
          code_point < static_cast<char32_t> (it->code_point + it->length))
             ? static_cast<grammar_rule> (it->rule)
             : grammar_rule::none;
}

} // end namespace peejay::details
