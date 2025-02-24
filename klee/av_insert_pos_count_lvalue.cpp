//===- klee/av_insert_pos_count_lvalue.cpp --------------------------------===//
//*                _                     _                      *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __   ___  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '_ \ / _ \/ __| *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | |_) | (_) \__ \ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| | .__/ \___/|___/ *
//*                                           |_|               *
//*                        _     _            _             *
//*   ___ ___  _   _ _ __ | |_  | |_   ____ _| |_   _  ___  *
//*  / __/ _ \| | | | '_ \| __| | \ \ / / _` | | | | |/ _ \ *
//* | (_| (_) | |_| | | | | |_  | |\ V / (_| | | |_| |  __/ *
//*  \___\___/ \__,_|_| |_|\__| |_| \_/ \__,_|_|\__,_|\___| *
//*                                                         *
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
#include "vcommon.hpp"

int main() {
  try {
    constexpr std::size_t max_elements = 7;
    static_assert(max_elements <= av_size);
    using arrayvec_type = peejay::arrayvec<member, max_elements>;

    MAKE_SYMBOLIC(member::throw_number);

    // The size of the initial array.
    std::size_t size;
    MAKE_SYMBOLIC(size);
    klee_assume(size <= max_elements);
    // Where to insert.
    arrayvec_type::size_type pos;
    MAKE_SYMBOLIC(pos);
    klee_assume(pos <= size);
    // The number of instances to insert.
    arrayvec_type::size_type count;
    MAKE_SYMBOLIC(count);
    klee_assume(count <= max_elements - size);

    arrayvec_type av;
    populate(av, size);
    assert(av.size() == size);

    member value{43};

    // Call the function under test.
    arrayvec_type::iterator const avit = av.insert(av.begin() + pos, count, value);
    (void)avit;

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);
    // A mirror call to std::vector<>::insert for comparison.
    std::vector<member>::iterator const vit = v.insert(v.begin() + pos, count, value);

    // Compare the return of the two functions.
    if (vit - v.begin() != avit - av.begin()) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }

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
