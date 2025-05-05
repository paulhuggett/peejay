//===- klee/vcommon.hpp -----------------------------------*- mode: C++ -*-===//
//*                                                  *
//* __   _____ ___  _ __ ___  _ __ ___   ___  _ __   *
//* \ \ / / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//*  \ V / (_| (_) | | | | | | | | | | | (_) | | | | *
//*   \_/ \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                                  *
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
#ifndef VCOMMON_HPP
#define VCOMMON_HPP

#include <array>
#include <cassert>

#ifdef KLEE_RUN
#include <iostream>
#endif
#define MAKE_SYMBOLIC(x) klee_make_symbolic(&(x), sizeof(x), #x)

constexpr std::size_t av_size = 20;
inline std::array<int, av_size> const primes{
    {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71}};

template <typename Container> Container& populate(Container& c, std::size_t n) {
  assert(n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back(primes[ctr]);
  }
  return c;
}

#ifdef KLEE_RUN
template <typename Container1, typename Container2> void check_equal(Container1 const& c1, Container2 const& c2) {
  if (!std::equal(c1.begin(), c1.end(), c2.begin(), c2.end())) {
    std::cerr << "** Fail!\n";
    std::abort();
  }
}

inline void check_instances() {
  if (auto const inst = member::instances(); inst != 0) {
    std::cerr << "** Fail: instances = " << inst << '\n';
    std::abort();
  }
}
#else
template <typename Container1, typename Container2>
constexpr void check_equal(Container1 const& /*c1*/, Container2 const& /*c2*/) {
  // Do nothing when running KLEE.
}
constexpr void check_instances() {
  // Do nothing when running KLEE.
}
#endif  // KLEE_RUN

#endif  // VCOMMON_HPP
