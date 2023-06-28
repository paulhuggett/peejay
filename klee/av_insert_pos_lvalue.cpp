//===- klee/av_insert_pos_lvalue.cpp --------------------------------------===//
//*                _                     _                      *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __   ___  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '_ \ / _ \/ __| *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | |_) | (_) \__ \ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| | .__/ \___/|___/ *
//*                                           |_|               *
//*  _            _             *
//* | |_   ____ _| |_   _  ___  *
//* | \ \ / / _` | | | | |/ _ \ *
//* | |\ V / (_| | | |_| |  __/ *
//* |_| \_/ \__,_|_|\__,_|\___| *
//*                             *
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
    constexpr std::size_t size = 3;
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    arrayvec_type::size_type pos;
    klee_make_symbolic (&pos, sizeof pos, "pos");
    klee_assume (pos <= size);

    klee_make_symbolic (&member::throw_number, sizeof (member::throw_number),
                        "throw_number");

    arrayvec_type av;
    populate (av);
    assert (av.size () == size);

    member value{11};

    // Call the function under test.
    av.insert (av.begin () + pos, value);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v);
    // A mirror call to std::vector<>::insert for comparison.
    v.insert (v.begin () + pos, value);

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