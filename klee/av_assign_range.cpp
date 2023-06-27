//===- klee/av_assign_range.cpp -------------------------------------------===//
//*                              _                                           *
//*   __ ___   __   __ _ ___ ___(_) __ _ _ __    _ __ __ _ _ __   __ _  ___  *
//*  / _` \ \ / /  / _` / __/ __| |/ _` | '_ \  | '__/ _` | '_ \ / _` |/ _ \ *
//* | (_| |\ V /  | (_| \__ \__ \ | (_| | | | | | | | (_| | | | | (_| |  __/ *
//*  \__,_| \_/    \__,_|___/___/_|\__, |_| |_| |_|  \__,_|_| |_|\__, |\___| *
//*                                |___/                         |___/       *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/arrayvec.hpp"

template <typename Container>
void populate (Container& c) {
  c.emplace_back (1);
  c.emplace_back (3);
  c.emplace_back (5);
}

int main () {
  try {
    constexpr std::size_t av_size = 8;

    std::size_t first;
    std::size_t last;
    klee_make_symbolic (&first, sizeof first, "first");
    klee_make_symbolic (&last, sizeof last, "last");
    klee_assume (last < av_size);
    klee_assume (first <= last);
    klee_make_symbolic (&member::throw_number, sizeof (member::throw_number),
                        "throw_number");

    std::array<member, av_size> src{{member{11}, member{13}, member{17},
                                     member{19}, member{23}, member{29},
                                     member{31}, member{37}}};

    peejay::arrayvec<member, av_size> av;
    populate (av);

    // Call the function under test.
    av.assign (&src[first], &src[last]);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v);
    // A mirror call to std::vector<>::assign for comparison.
    v.assign (&src[first], &src[last]);

    if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }
#endif  // KLEE_RUN
  } catch (memberex const&) {
  }
#ifdef KLEE_RUN
  if (auto const inst = member::instances (); inst != 0) {
    std::cerr << "** Fail: instances = " << inst << '\n';
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
#endif  // KLEE_RUN
  return EXIT_SUCCESS;
}
