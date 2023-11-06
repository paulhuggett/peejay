//===- klee/sv_ctor_count_value.cpp ---------------------------------------===//
//*                   _                                     _    *
//*  _____   __   ___| |_ ___  _ __    ___ ___  _   _ _ __ | |_  *
//* / __\ \ / /  / __| __/ _ \| '__|  / __/ _ \| | | | '_ \| __| *
//* \__ \\ V /  | (__| || (_) | |    | (_| (_) | |_| | | | | |_  *
//* |___/ \_/    \___|\__\___/|_|     \___\___/ \__,_|_| |_|\__| *
//*                                                              *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
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

    MAKE_SYMBOLIC (member::throw_number);

    peejay::small_vector<member, body_elements>::size_type count;
    MAKE_SYMBOLIC (count);
    klee_assume (count <= max_elements);

    member value{23};
    peejay::small_vector<member, body_elements> av (count, value);

#ifdef KLEE_RUN
    std::vector<member> v (count, value);

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
