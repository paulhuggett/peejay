// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <span>

#include "peejay/null.hpp"

int main() {
  peejay::parser<peejay::null<>> p;

  // Read standard input a block at a time and pass each block to the parser.
  // The parser keeps its state between calls, so a token may be split across
  // two blocks.
  std::array<char8_t, 256> buffer{};
  while (!p.has_error()) {
    // char may alias any object type, so reading into the char8_t array through a char* is fine.
    std::cin.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    auto const available = static_cast<std::size_t>(std::cin.gcount());
    if (available == 0) {
      break;  // End of input.
    }
    p.input(std::span{buffer.data(), available});
  }
  p.eof();

  if (std::error_code const err = p.last_error()) {
    std::cerr << "line " << p.pos().line << ": error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "valid JSON\n";
  return EXIT_SUCCESS;
}
