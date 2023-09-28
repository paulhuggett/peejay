//===- klee/av_erase_pos.cpp ----------------------------------------------===//
//*                                                           *
//*   __ ___   __   ___ _ __ __ _ ___  ___   _ __   ___  ___  *
//*  / _` \ \ / /  / _ \ '__/ _` / __|/ _ \ | '_ \ / _ \/ __| *
//* | (_| |\ V /  |  __/ | | (_| \__ \  __/ | |_) | (_) \__ \ *
//*  \__,_| \_/    \___|_|  \__,_|___/\___| | .__/ \___/|___/ *
//*                                         |_|               *
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
#include "peejay/arrayvec.hpp"
#include "vcommon.hpp"

int main () {
  try {
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    MAKE_SYMBOLIC (member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC (size);
    klee_assume (size > 0);
    klee_assume (size <= av_size);

    arrayvec_type::difference_type pos;
    MAKE_SYMBOLIC (pos);
    // (The end iterator is not valid for erase().)
    klee_assume (pos >= 0);
    klee_assume (static_cast<std::make_unsigned_t<decltype (pos)>> (pos) <
                 size);

    arrayvec_type av;
    populate (av, size);

    // Call the function under test.
    av.erase (av.begin () + pos);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);

    // A mirror call to std::vector<>::insert for comparison.
    v.erase (v.begin () + pos);

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
