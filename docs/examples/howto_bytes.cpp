// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <span>

#include "icubaby/icubaby.hpp"
#include "peejay/null.hpp"

namespace {

// Returns true if bytes hold valid JSON in any of the Unicode encodings.
bool check(std::span<std::byte const> const bytes) {
  peejay::parser<peejay::null<>> p;
  // The byte transcoder chooses the encoding from a byte order mark at the start
  // of the input. If there isn't one, it assumes UTF-8.
  icubaby::tx_8 transcoder;
  // true: stop at the first ill-formed sequence.
  p.input(transcoder, bytes, true).eof(transcoder, true);
  std::cout << (p.has_error() ? p.last_error().message() : "OK") << '\n';
  return !p.has_error();
}

// Makes an array of std::byte from a list of integers.
template <typename... Values> constexpr auto bytes(Values... values) {
  return std::array{static_cast<std::byte>(values)...};
}

}  // end anonymous namespace

int main() {
  // [1] encoded in five different ways.
  // UTF-8 without a byte order mark.
  bool ok = check(bytes('[', '1', ']'));
  // UTF-8 with a byte order mark.
  ok = check(bytes(0xEF, 0xBB, 0xBF, '[', '1', ']')) && ok;
  // UTF-16 BE.
  ok = check(bytes(0xFE, 0xFF, 0, '[', 0, '1', 0, ']')) && ok;
  // UTF-16 LE.
  ok = check(bytes(0xFF, 0xFE, '[', 0, '1', 0, ']', 0)) && ok;
  // UTF-32 BE.
  ok = check(bytes(0, 0, 0xFE, 0xFF, 0, 0, 0, '[', 0, 0, 0, '1', 0, 0, 0, ']')) && ok;

  // A UTF-16 LE string holding an unpaired high surrogate (D800).
  ok = !check(bytes(0xFF, 0xFE, '"', 0, 0x00, 0xD8, '"', 0)) && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
