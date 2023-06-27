//===- klee/av_insert_range.cpp -------------------------------------------===//
//*                _                     _                                 *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __ __ _ _ __   __ _  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '__/ _` | '_ \ / _` |/ _ \ *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | | | (_| | | | | (_| |  __/ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| |_|  \__,_|_| |_|\__, |\___| *
//*                                                            |___/       *
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

#include "peejay/arrayvec.hpp"

int main () {
  static constexpr std::size_t av_size = 8;
  std::array<int, av_size> const src{{11, 13, 17, 19, 23, 29, 31, 37}};
  peejay::arrayvec<int, av_size> av{3, 5, 7};

  decltype (av)::difference_type pos;
  std::size_t first;
  std::size_t last;

  klee_make_symbolic (&pos, sizeof pos, "pos");
  klee_make_symbolic (&first, sizeof first, "first");
  klee_make_symbolic (&last, sizeof last, "last");

  // range check 'first' and 'last'.
  klee_assume (first <= av_size);
  klee_assume (last <= av_size);
  klee_assume (first <= last);

  // Don't try to overflow the container.
  klee_assume (av.size () + (last - first) <= av_size);

  // Range check 'pos'.
  klee_assume (pos >= 0);
  klee_assume (pos <= av.size ());

  auto b = std::begin (src);
  av.insert (av.begin () + pos, b + first, b + last);

#ifdef KLEE_RUN
  std::vector<int> v{3, 5, 7};
  v.insert (v.begin () + pos, b + first, b + last);
  if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
    std::cerr << "** Fail!\n";
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
  return EXIT_SUCCESS;
#endif
}
