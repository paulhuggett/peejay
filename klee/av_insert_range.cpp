//===- klee/av_insert_range.cpp -------------------------------------------===//
//*                _                     _                                 *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __ __ _ _ __   __ _  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '__/ _` | '_ \ / _` |/ _ \ *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | | | (_| | | | | (_| |  __/ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| |_|  \__,_|_| |_|\__, |\___| *
//*                                                            |___/       *
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
#include "vcommon.hpp"

int main() {
  try {
    constexpr auto max_elements = std::size_t{8};
    MAKE_SYMBOLIC(member::throw_number);

    peejay::arrayvec<member, max_elements> av;

    std::size_t size;
    MAKE_SYMBOLIC(size);
    klee_assume(size <= max_elements);

    decltype(av)::difference_type pos;
    MAKE_SYMBOLIC(pos);
    klee_assume(pos >= 0);
    klee_assume(pos <= av.size());

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);
    // range check 'first' and 'last'.
    klee_assume(last <= av_size);
    klee_assume(first <= last);
    // Don't overflow the container.
    klee_assume(av.size() + (last - first) <= max_elements);

    auto b = std::begin(primes);
    // Call the function under test.
    av.insert(av.begin() + pos, b + first, b + last);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);
    // A mirror call to std::vector<>::insert() for comparison.
    v.insert(v.begin() + pos, b + first, b + last);
    if (!std::equal(av.begin(), av.end(), v.begin(), v.end())) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }
#endif
  } catch (memberex const&) {
    // catch and ignore.
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
