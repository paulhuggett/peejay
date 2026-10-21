// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <system_error>

#include "peejay/null.hpp"

namespace {

void parse(std::string_view const name, std::u8string_view const json) {
  peejay::parser<peejay::null<>> p;
  p.input(json).eof();

  std::error_code const err = p.last_error();
  if (!err) {
    std::cout << name << ": OK\n";
    return;
  }
  // pos() is the position of the most recent token: usually the one that caused the error.
  std::cout << name << ':' << p.pos().line << ": error: " << err.message() << '\n';

  // Errors raised by the parser belong to peejay's error category and can be
  // compared directly with the values of the peejay::error enumeration.
  if (err == peejay::error::nesting_too_deep || err == peejay::error::string_too_long) {
    std::cout << "  (the input exceeded one of the parser's policy limits)\n";
  }
}

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;
  parse("good.json", u8"{\n  \"a\": [1, 2]\n}\n"sv);
  parse("comma.json", u8"{\n  \"a\": [1, 2,]\n}\n"sv);
  parse("deep.json", u8"[[[[[[[[[[]]]]]]]]]]"sv);
  return EXIT_SUCCESS;
}
