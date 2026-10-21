// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <ranges>
#include <string>

#include "peejay/null.hpp"

int main() {
  // Text which arrives as std::string (for example, from a network library).
  std::string const json = R"({"greeting": "hello"})";

  // parser<>::input() accepts any input range of char8_t. A transform view
  // converts each char without copying the string.
  auto const as_utf8 = json | std::views::transform([](char const c) { return static_cast<char8_t>(c); });

  peejay::parser<peejay::null<>> p;
  p.input(as_utf8).eof();
  std::cout << (p.has_error() ? p.last_error().message() : "OK") << '\n';
  return p.has_error() ? EXIT_FAILURE : EXIT_SUCCESS;
}
