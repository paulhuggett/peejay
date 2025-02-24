//===- klee/av_assign_range.cpp -------------------------------------------===//
//*                              _                                           *
//*   __ ___   __   __ _ ___ ___(_) __ _ _ __    _ __ __ _ _ __   __ _  ___  *
//*  / _` \ \ / /  / _` / __/ __| |/ _` | '_ \  | '__/ _` | '_ \ / _` |/ _ \ *
//* | (_| |\ V /  | (_| \__ \__ \ | (_| | | | | | | | (_| | | | | (_| |  __/ *
//*  \__,_| \_/    \__,_|___/___/_|\__, |_| |_| |_|  \__,_|_| |_|\__, |\___| *
//*                                |___/                         |___/       *
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

template <typename Container>
void assign_first_last(Container &c, std::size_t size, std::size_t first, std::size_t last) {
  populate(c, size);

  auto b = std::begin(primes);
  // Call the function under test.
  c.assign(b + first, b + last);
}

int main() {
  try {
    constexpr auto max_elements = std::size_t{8};
    MAKE_SYMBOLIC(member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC(size);
    klee_assume(size <= max_elements);

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);
    // range check 'first' and 'last'.
    klee_assume(last < av_size);
    klee_assume(first <= last);
    klee_assume(last - first <= max_elements);

    peejay::arrayvec<member, max_elements> av;
    assign_first_last(av, size, first, last);

#ifdef KLEE_RUN
    std::vector<member> v;
    assign_first_last(v, size, first, last);
    check_equal(av, v);
#endif  // KLEE_RUN
  } catch (memberex const &) {
    // catch and ignore.
  }
  check_instances();
  return EXIT_SUCCESS;
}
