//===- klee/av_ctor_count.cpp ---------------------------------------------===//
//*                     _                                     _    *
//*   __ ___   __   ___| |_ ___  _ __    ___ ___  _   _ _ __ | |_  *
//*  / _` \ \ / /  / __| __/ _ \| '__|  / __/ _ \| | | | '_ \| __| *
//* | (_| |\ V /  | (__| || (_) | |    | (_| (_) | |_| | | | | |_  *
//*  \__,_| \_/    \___|\__\___/|_|     \___\___/ \__,_|_| |_|\__| *
//*                                                                *
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

#define MAKE_SYMBOLIC(x) klee_make_symbolic(&(x), sizeof(x), #x)

namespace {

constexpr std::size_t av_size = 8;

}  // namespace

int main() {
  try {
    MAKE_SYMBOLIC(member::throw_number);

    peejay::arrayvec<member, av_size>::size_type count;
    MAKE_SYMBOLIC(count);
    klee_assume(count <= av_size);

    peejay::arrayvec<member, av_size> av{count};

#ifdef KLEE_RUN
    std::vector<member> v{count};

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
