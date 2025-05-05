//===- klee/quote.cpp -----------------------------------------------------===//
//*                    _        *
//*   __ _ _   _  ___ | |_ ___  *
//*  / _` | | | |/ _ \| __/ _ \ *
//* | (_| | |_| | (_) | ||  __/ *
//*  \__, |\__,_|\___/ \__\___| *
//*     |_|                     *
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
#include <cstddef>

#include "klee/klee.h"
#include "peejay/null.hpp"

struct policies : peejay::default_policies {
  using char_type = char;
};

int main() {
  static constexpr std::size_t const size = 5;
  std::array<char, size> input;

  klee_make_symbolic(input.data(), size, "input");
  klee_assume(input[0] == '"');
  klee_assume(input[size - 1] == '\0');

  make_parser(peejay::null<policies>{}).input(input).eof();
}
