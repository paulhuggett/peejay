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
#include <cstddef>

#include "klee/klee.h"

#include "peejay/null.hpp"

int main () {
  static constexpr std::size_t const size = 5;
  peejay::char8 input[size];

  klee_make_symbolic (input, sizeof input, "input");
  klee_assume(input[0] == '"');
  klee_assume(input[size - 1] == '\0');

  make_parser (peejay::null{}).input (peejay::u8string{input}).eof();
}
