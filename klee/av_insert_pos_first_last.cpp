//===- klee/av_insert_pos_first_last.cpp ----------------------------------===//
//*                _                     _                      *
//*   __ ___   __ (_)_ __  ___  ___ _ __| |_   _ __   ___  ___  *
//*  / _` \ \ / / | | '_ \/ __|/ _ \ '__| __| | '_ \ / _ \/ __| *
//* | (_| |\ V /  | | | | \__ \  __/ |  | |_  | |_) | (_) \__ \ *
//*  \__,_| \_/   |_|_| |_|___/\___|_|   \__| | .__/ \___/|___/ *
//*                                           |_|               *
//*   __ _          _     _           _    *
//*  / _(_)_ __ ___| |_  | | __ _ ___| |_  *
//* | |_| | '__/ __| __| | |/ _` / __| __| *
//* |  _| | |  \__ \ |_  | | (_| \__ \ |_  *
//* |_| |_|_|  |___/\__| |_|\__,_|___/\__| *
//*                                        *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/details/arrayvec.hpp"
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
    // less-than max_elements to ensure room for a new element.
    klee_assume(size < max_elements);

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC(first);
    MAKE_SYMBOLIC(last);

    // Insert position.
    arrayvec_type::size_type pos;
    MAKE_SYMBOLIC(pos);
    klee_assume(pos <= size);

    arrayvec_type av;
    populate(av, size);

    std::array<member, max_elements> src{
        {member{419}, member{421}, member{431}, member{433}, member{439}, member{443}, member{449}}};
    klee_assume(last <= max_elements);
    klee_assume(first <= last);
    klee_assume(last - first <= max_elements - size);

    // Call the function under test.
    arrayvec_type::iterator const avit = av.insert(av.begin() + pos, src.begin() + first, src.begin() + last);
    (void)avit;  // Don't warn unused.
    assert(avit >= av.begin() && avit <= av.end());
#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);
    // A mirror call to std::vector<>::insert for comparison.
    std::vector<member>::iterator const vit = v.insert(v.begin() + pos, src.begin() + first, src.begin() + last);

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
