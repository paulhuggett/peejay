//===- klee/sv_ctor_first_last.cpp ----------------------------------------===//
//*                   _                __ _          _     _           _    *
//*  _____   __   ___| |_ ___  _ __   / _(_)_ __ ___| |_  | | __ _ ___| |_  *
//* / __\ \ / /  / __| __/ _ \| '__| | |_| | '__/ __| __| | |/ _` / __| __| *
//* \__ \\ V /  | (__| || (_) | |    |  _| | |  \__ \ |_  | | (_| \__ \ |_  *
//* |___/ \_/    \___|\__\___/|_|    |_| |_|_|  |___/\__| |_|\__,_|___/\__| *
//*                                                                         *
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
#include "peejay/json/small_vector.hpp"
#include "vcommon.hpp"

int main() {
  try {
    constexpr std::size_t body_elements = 5;
    constexpr std::size_t max_elements = 13;

    MAKE_SYMBOLIC(member::throw_number);

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);
    klee_assume(last <= av_size);
    klee_assume(last <= max_elements);
    klee_assume(first <= last);

    auto const b = primes.begin();
    peejay::small_vector<member, body_elements> av{b + first, b + last};

#ifdef KLEE_RUN
    std::vector<member> v{b + first, b + last};

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
