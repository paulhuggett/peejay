//===- klee/av_insert_pos_rvalue.cpp --------------------------------------===//
//*                _                     _                      *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __   ___  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '_ \ / _ \/ __| *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | |_) | (_) \__ \ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| | .__/ \___/|___/ *
//*                                           |_|               *
//*                  _             *
//*  _ ____   ____ _| |_   _  ___  *
//* | '__\ \ / / _` | | | | |/ _ \ *
//* | |   \ V / (_| | | |_| |  __/ *
//* |_|    \_/ \__,_|_|\__,_|\___| *
//*                                *
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
#include "peejay/json/arrayvec.hpp"

#define MAKE_SYMBOLIC(x) klee_make_symbolic(&(x), sizeof(x), #x)

namespace {

constexpr std::size_t av_size = 8;
inline std::array<int, av_size> const primes{{2, 3, 5, 7, 11, 13, 17, 19}};

template <typename Container> void populate(Container& c, std::size_t n) {
  assert(n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back(primes[ctr]);
  }
}

}  // namespace

int main() {
  try {
    MAKE_SYMBOLIC(member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC(size);
    // Ensure room for 1 new instance.
    klee_assume(size < av_size);

    using arrayvec_type = peejay::arrayvec<member, av_size>;

    arrayvec_type::size_type pos;
    MAKE_SYMBOLIC(pos);
    klee_assume(pos <= size);

    arrayvec_type av;
    populate(av, size);

    // Call the function under test.
    av.insert(av.begin() + pos, member{23});

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);
    // A mirror call to std::vector<>::insert for comparison.
    v.insert(v.begin() + pos, member{23});

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
