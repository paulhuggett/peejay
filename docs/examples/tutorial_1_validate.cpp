// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "peejay/null.hpp"

int main() {
  using namespace std::string_view_literals;

  // A parser whose backend discards everything: it only checks the syntax.
  peejay::parser<peejay::null<>> p;

  p.input(u8R"({ "name": "web", "port": 8080 })"sv);
  p.eof();

  if (std::error_code const err = p.last_error()) {
    std::cerr << "invalid JSON: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "valid JSON\n";
  return EXIT_SUCCESS;
}
