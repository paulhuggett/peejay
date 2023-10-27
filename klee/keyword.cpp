//===- klee/keyword.cpp ---------------------------------------------------===//
//*  _                                    _  *
//* | | _____ _   ___      _____  _ __ __| | *
//* | |/ / _ \ | | \ \ /\ / / _ \| '__/ _` | *
//* |   <  __/ |_| |\ V  V / (_) | | | (_| | *
//* |_|\_\___|\__, | \_/\_/ \___/|_|  \__,_| *
//*           |___/                          *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <cctype>
#include <cstddef>

#include "klee/klee.h"
#include "peejay/null.hpp"

int main () {
  static constexpr std::size_t const size = 9;
  peejay::char8 input[size];

  klee_make_symbolic (input, sizeof input, "input");
  klee_assume (std::isalpha (static_cast<char> (input[0])) != 0);
  klee_assume (input[size - 1] == '\0');

  make_parser (peejay::null{}).input (peejay::u8string{input}).eof ();
}
