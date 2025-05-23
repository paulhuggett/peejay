//===- klee/av_erase_pos.cpp ----------------------------------------------===//
//*                                                           *
//*   __ ___   __   ___ _ __ __ _ ___  ___   _ __   ___  ___  *
//*  / _` \ \ / /  / _ \ '__/ _` / __|/ _ \ | '_ \ / _ \/ __| *
//* | (_| |\ V /  |  __/ | | (_| \__ \  __/ | |_) | (_) \__ \ *
//*  \__,_| \_/    \___|_|  \__,_|___/\___| | .__/ \___/|___/ *
//*                                         |_|               *
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
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    MAKE_SYMBOLIC(member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC(size);
    klee_assume(size > 0);
    klee_assume(size <= av_size);

    arrayvec_type::difference_type pos;
    MAKE_SYMBOLIC(pos);
    // (The end iterator is not valid for erase().)
    klee_assume(pos >= 0);
    klee_assume(static_cast<std::make_unsigned_t<decltype(pos)>>(pos) < size);

    arrayvec_type av;
    populate(av, size);

    // Call the function under test.
    av.erase(av.begin() + pos);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate(v, size);

    // A mirror call to std::vector<>::insert for comparison.
    v.erase(v.begin() + pos);

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
