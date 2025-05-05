//===- klee/av_ctor_first_last.cpp ----------------------------------------===//
//*                     _                __ _          _     _           _    *
//*   __ ___   __   ___| |_ ___  _ __   / _(_)_ __ ___| |_  | | __ _ ___| |_  *
//*  / _` \ \ / /  / __| __/ _ \| '__| | |_| | '__/ __| __| | |/ _` / __| __| *
//* | (_| |\ V /  | (__| || (_) | |    |  _| | |  \__ \ |_  | | (_| \__ \ |_  *
//*  \__,_| \_/    \___|\__\___/|_|    |_| |_|_|  |___/\__| |_|\__,_|___/\__| *
//*                                                                           *
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
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/details/arrayvec.hpp"

#define MAKE_SYMBOLIC(x) klee_make_symbolic(&(x), sizeof(x), #x)

namespace {

constexpr std::size_t av_size = 8;
inline std::array<int, av_size> const primes{{2, 3, 5, 7, 11, 13, 17, 19}};

}  // namespace

int main() {
  try {
    MAKE_SYMBOLIC(member::throw_number);

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);
    klee_assume(last <= av_size);
    klee_assume(first <= last);

    auto const b = primes.begin();
    peejay::arrayvec<member, av_size> av{b + first, b + last};

#ifdef KLEE_RUN
    std::vector<member> v{b + first, b + last};

    if (!std::equal(av.begin(), av.end(), v.begin(), v.end())) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }
#endif  // KLEE_RUN
  } catch (memberex const&) {
  }
#ifdef KLEE_RUN
  if (auto const inst = member::instances(); inst != 0) {
    std::cerr << "** Fail: instances = " << inst << '\n';
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
#endif  // KLEE_RUN
  return EXIT_SUCCESS;
}
