//===- main_klee.cpp ------------------------------------------------------===//
//*                  _         _    _            *
//*  _ __ ___   __ _(_)_ __   | | _| | ___  ___  *
//* | '_ ` _ \ / _` | | '_ \  | |/ / |/ _ \/ _ \ *
//* | | | | | | (_| | | | | | |   <| |  __/  __/ *
//* |_| |_| |_|\__,_|_|_| |_| |_|\_\_|\___|\___| *
//*                                              *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <cstddef>
#include <iostream>

#ifdef KLEE
#include "klee/klee.h"
#endif

#include "peejay/json.hpp"
int main () {
#ifdef KLEE
  static constexpr std::size_t const size = 10;
  char input[size];

  // Make the input symbolic.
  klee_make_symbolic (input, sizeof input, "input");
  input[size - 1] = '\0';

  peejay::parser p;
  p.match_number (input);
#endif
}
