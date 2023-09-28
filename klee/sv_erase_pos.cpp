//===- klee/sv_erase_pos.cpp ----------------------------------------------===//
//*                                                         *
//*  _____   __   ___ _ __ __ _ ___  ___   _ __   ___  ___  *
//* / __\ \ / /  / _ \ '__/ _` / __|/ _ \ | '_ \ / _ \/ __| *
//* \__ \\ V /  |  __/ | | (_| \__ \  __/ | |_) | (_) \__ \ *
//* |___/ \_/    \___|_|  \__,_|___/\___| | .__/ \___/|___/ *
//*                                       |_|               *
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
#include "peejay/small_vector.hpp"
#include "vcommon.hpp"

int main () {
  try {
    constexpr std::size_t body_elements = 5;
    constexpr std::size_t max_elements = 13;
    using small_vector_type = peejay::small_vector<member, body_elements>;

    MAKE_SYMBOLIC (member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC (size);
    klee_assume (size <= max_elements);

    small_vector_type::difference_type pos;
    MAKE_SYMBOLIC (pos);
    // (The end iterator is not valid for erase().)
    klee_assume (pos >= 0);
    klee_assume (static_cast<std::make_unsigned_t<decltype (pos)>> (pos) <
                 size);

    small_vector_type sv;
    populate (sv, size);

    // Call the function under test.
    sv.erase (sv.begin () + pos);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);

    // A mirror call to std::vector<>::insert for comparison.
    v.erase (v.begin () + pos);

    if (!std::equal (sv.begin (), sv.end (), v.begin (), v.end ())) {
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
