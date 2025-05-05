//===- klee/keyword.cpp ---------------------------------------------------===//
//*  _                                    _  *
//* | | _____ _   ___      _____  _ __ __| | *
//* | |/ / _ \ | | \ \ /\ / / _ \| '__/ _` | *
//* |   <  __/ |_| |\ V  V / (_) | | | (_| | *
//* |_|\_\___|\__, | \_/\_/ \___/|_|  \__,_| *
//*           |___/                          *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#include <array>
#include <cctype>
#include <cstddef>

#include "klee/klee.h"
#include "peejay/null.hpp"

struct policies : peejay::default_policies {
  using char_type = char;
};
int main() {
  static constexpr std::size_t const size = 9;
  std::array<char, size> input;

  klee_make_symbolic(input.data(), size, "input");
  klee_assume(std::isalpha(static_cast<char>(input[0])) != 0);
  klee_assume(input[size - 1] == '\0');

  make_parser(peejay::null<policies>{}).input(input).eof();
}
