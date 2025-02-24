//===- klee/quote.cpp -----------------------------------------------------===//
//*                    _        *
//*   __ _ _   _  ___ | |_ ___  *
//*  / _` | | | |/ _ \| __/ _ \ *
//* | (_| | |_| | (_) | ||  __/ *
//*  \__, |\__,_|\___/ \__\___| *
//*     |_|                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <array>
#include <cstddef>

#include "klee/klee.h"
#include "peejay/null.hpp"

int main() {
  static constexpr std::size_t const size = 5;
  std::array<std::byte, size> input;

  klee_make_symbolic(input.data(), size, "input");
  klee_assume(input[0] == static_cast<std::byte>('"'));
  klee_assume(input[size - 1] == std::byte{0});

  make_parser(peejay::null{}).input(input.begin(), input.end()).eof();
}
