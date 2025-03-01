//===- klee/sv_assign_range.cpp -------------------------------------------===//
//*                            _                                           *
//*  _____   __   __ _ ___ ___(_) __ _ _ __    _ __ __ _ _ __   __ _  ___  *
//* / __\ \ / /  / _` / __/ __| |/ _` | '_ \  | '__/ _` | '_ \ / _` |/ _ \ *
//* \__ \\ V /  | (_| \__ \__ \ | (_| | | | | | | | (_| | | | | (_| |  __/ *
//* |___/ \_/    \__,_|___/___/_|\__, |_| |_| |_|  \__,_|_| |_|\__, |\___| *
//*                              |___/                         |___/       *
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
    constexpr std::size_t body_elements = 7;
    constexpr std::size_t max_elements = 13;
    static_assert(max_elements <= av_size);
    using small_vector_type = peejay::small_vector<member, body_elements>;

    MAKE_SYMBOLIC(member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC(size);
    klee_assume(size <= max_elements);

    std::array<member, max_elements> const src{{member{457}, member{461}, member{463}, member{467}, member{479},
                                                member{487}, member{491}, member{499}, member{503}, member{509},
                                                member{521}, member{523}, member{541}}};
    small_vector_type::size_type first;
    small_vector_type::size_type last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);
    klee_assume(first <= last);
    klee_assume(last <= max_elements);

    small_vector_type sv;
    populate(sv, size);
    // Call the function under test.
    sv.assign(src.begin() + first, src.begin() + last);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);
    // A mirror call to std::vector<>::assign for comparison.
    v.assign(src.begin() + first, src.begin() + last);

    if (!std::equal(sv.begin(), sv.end(), v.begin(), v.end())) {
      std::cerr << "** Fail (not equal)!\n";
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
