//===- klee/av_emplace_pos_args.cpp ---------------------------------------===//
//*                                     _                                   *
//*   __ ___   __   ___ _ __ ___  _ __ | | __ _  ___ ___   _ __   ___  ___  *
//*  / _` \ \ / /  / _ \ '_ ` _ \| '_ \| |/ _` |/ __/ _ \ | '_ \ / _ \/ __| *
//* | (_| |\ V /  |  __/ | | | | | |_) | | (_| | (_|  __/ | |_) | (_) \__ \ *
//*  \__,_| \_/    \___|_| |_| |_| .__/|_|\__,_|\___\___| | .__/ \___/|___/ *
//*                              |_|                      |_|               *
//*                       *
//*   __ _ _ __ __ _ ___  *
//*  / _` | '__/ _` / __| *
//* | (_| | | | (_| \__ \ *
//*  \__,_|_|  \__, |___/ *
//*            |___/      *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/arrayvec.hpp"
#include "vcommon.hpp"

int main () {
  try {
    constexpr std::size_t max_elements = 7;
    static_assert (max_elements <= av_size);
    using arrayvec_type = peejay::arrayvec<member, max_elements>;

    MAKE_SYMBOLIC (member::throw_number);

    // The size of the initial array.
    std::size_t size;
    MAKE_SYMBOLIC (size);
    // less-than max_elements to ensure room for a new element.
    klee_assume (size < max_elements);

    // Insert position.
    arrayvec_type::size_type pos;
    MAKE_SYMBOLIC (pos);
    klee_assume (pos <= size);

    arrayvec_type av;
    populate (av, size);

    // Call the function under test.
    arrayvec_type::iterator const avit = av.emplace (av.begin () + pos, 43);
    (void)avit;  // Don't warn unused.
    assert (avit >= av.begin () && avit <= av.end ());
#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);
    // A mirror call to std::vector<>::insert for comparison.
    std::vector<member>::iterator const vit = v.emplace (v.begin () + pos, 43);

    // Compare the return of the two functions.
    if (vit - v.begin () != avit - av.begin ()) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }

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
