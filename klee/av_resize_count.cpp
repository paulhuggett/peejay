//===- klee/av_resize_count.cpp -------------------------------------------===//
//*                              _                                 _    *
//*   __ ___   __  _ __ ___  ___(_)_______    ___ ___  _   _ _ __ | |_  *
//*  / _` \ \ / / | '__/ _ \/ __| |_  / _ \  / __/ _ \| | | | '_ \| __| *
//* | (_| |\ V /  | | |  __/\__ \ |/ /  __/ | (_| (_) | |_| | | | | |_  *
//*  \__,_| \_/   |_|  \___||___/_/___\___|  \___\___/ \__,_|_| |_|\__| *
//*                                                                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/json/arrayvec.hpp"

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
