// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "icubaby/icubaby.hpp"
#include "peejay/null.hpp"

namespace {

// Returns true if json is both well-formed UTF-8 and valid JSON.
template <typename Range>
bool check(Range const &json) {
  peejay::parser<peejay::null<>> p;
  // A UTF-8 to UTF-8 transcoder checks that its input is well formed.
  icubaby::t8_8 utf8;
  // true: stop at the first ill-formed sequence.
  p.input(utf8, json, true).eof(utf8, true);
  if (p.has_error()) {
    std::cout << p.last_error().message() << '\n';
    return false;
  }
  std::cout << "OK\n";
  return true;
}

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;
  bool ok = check(u8"[\"caf\u00E9\"]"sv);
  // The byte 0xFF never appears in UTF-8.
  ok = !check(std::array{u8'"', u8'c', u8'a', u8'f', static_cast<char8_t>(0xFF), u8'"'}) && ok;
  // The input ends part-way through a two-byte sequence.
  ok = !check(std::array{u8'1', static_cast<char8_t>(0xC3)}) && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
