// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "peejay/null.hpp"

namespace {

// Start from the defaults and override only what you need.
struct big_policies : peejay::default_policies {
  static constexpr std::size_t max_length = 1024;     // Strings of up to 1 KiB.
  static constexpr std::size_t max_stack_depth = 64;  // Deeply nested documents.
  using integer_type = std::int64_t;                  // 64-bit integers.
};

// A policy for input which must contain only integers.
struct integers_only : peejay::default_policies {
  using float_type = peejay::no_float_type;  // Reject numbers which aren't integers.
  using integer_type = std::int64_t;
  static constexpr bool pos_tracking = false;  // Don't track line numbers.
};

template <typename Policies> void check(std::u8string_view const json) {
  peejay::parser<peejay::null<Policies>> p;
  p.input(json).eof();
  std::cout << (p.has_error() ? p.last_error().message() : "OK") << '\n';
}

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;
  check<big_policies>(u8"[9007199254740993]"sv);   // OK
  check<integers_only>(u8"[1, 2, 3]"sv);           // OK
  check<integers_only>(u8"[1, 2.5]"sv);            // number out of range
  return EXIT_SUCCESS;
}
