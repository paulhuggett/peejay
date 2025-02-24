//===- klee/keyword.cpp ---------------------------------------------------===//
//*  _                                    _  *
//* | | _____ _   ___      _____  _ __ __| | *
//* | |/ / _ \ | | \ \ /\ / / _ \| '__/ _` | *
//* |   <  __/ |_| |\ V  V / (_) | | | (_| | *
//* |_|\_\___|\__, | \_/\_/ \___/|_|  \__,_| *
//*           |___/                          *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <array>
#include <cctype>
#include <cstddef>

#include "klee/klee.h"
#include "peejay/json/null.hpp"

int main() {
  static constexpr std::size_t const size = 9;
  std::array<std::byte, size> input;

  klee_make_symbolic(input.data(), size, "input");
  klee_assume(std::isalpha(static_cast<char>(input[0])) != 0);
  klee_assume(input[size - 1] == std::byte{0});

  make_parser(peejay::null{}).input(input.begin(), input.end()).eof();
}
