//===- klee/av_resize_count.cpp -------------------------------------------===//
//*                              _                                 _    *
//*   __ ___   __  _ __ ___  ___(_)_______    ___ ___  _   _ _ __ | |_  *
//*  / _` \ \ / / | '__/ _ \/ __| |_  / _ \  / __/ _ \| | | | '_ \| __| *
//* | (_| |\ V /  | | |  __/\__ \ |/ /  __/ | (_| (_) | |_| | | | | |_  *
//*  \__,_| \_/   |_|  \___||___/_/___\___|  \___\___/ \__,_|_| |_|\__| *
//*                                                                     *
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

template <typename Container> void populate(Container& c) {
  c.emplace_back(1);
  c.emplace_back(3);
  c.emplace_back(5);
}

int main() {
  try {
    constexpr std::size_t av_size = 8;

    peejay::arrayvec<member, av_size>::size_type count;
    klee_make_symbolic(&count, sizeof(count), "count");
    klee_assume(count <= av_size);
    klee_make_symbolic(&member::throw_number, sizeof(member::throw_number), "throw_number");

    peejay::arrayvec<member, av_size> av;
    populate(av);

    // Call the function under test.
    av.resize(count);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v);
    // A mirror call to std::vector<>::assign for comparison.
    v.resize(count);

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
