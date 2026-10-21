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
bool check(std::u8string_view const json) {
  peejay::parser<peejay::null<>> p;
  // A UTF-8 to UTF-8 transcoder checks that its input is well formed. We don't need its output.
  icubaby::t8_8 utf8;
  std::array<char8_t, icubaby::longest_sequence_v<char8_t>> discard{};

  for (char8_t const cu : json) {
    utf8(cu, discard.begin());
    if (!utf8.well_formed()) {
      std::cout << "ill-formed UTF-8\n";
      return false;
    }
    p.input(std::u8string_view{&cu, 1});
  }
  if (utf8.partial()) {
    std::cout << "input ends part-way through a UTF-8 sequence\n";
    return false;
  }
  p.eof();
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
  bool ok = check(u8"[\"café\"]"sv);  // OK
  ok = !check(u8"[\"caf\xFF\"]"sv) && ok;  // ill-formed UTF-8
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
