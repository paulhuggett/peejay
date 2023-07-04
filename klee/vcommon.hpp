//===- klee/vcommon.hpp -----------------------------------*- mode: C++ -*-===//
//*                                                  *
//* __   _____ ___  _ __ ___  _ __ ___   ___  _ __   *
//* \ \ / / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//*  \ V / (_| (_) | | | | | | | | | | | (_) | | | | *
//*   \_/ \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                                  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef VCOMMON_HPP
#define VCOMMON_HPP

#include <array>
#include <cassert>

#define MAKE_SYMBOLIC(x) klee_make_symbolic (&(x), sizeof (x), #x)

constexpr std::size_t av_size = 20;
inline std::array<int, av_size> const primes{{2,  3,  5,  7,  11, 13, 17,
                                              19, 23, 29, 31, 37, 41, 43,
                                              47, 53, 59, 61, 67, 71}};

template <typename Container>
Container& populate (Container& c, std::size_t n) {
  assert (n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back (primes[ctr]);
  }
  return c;
}

#endif  // VCOMMON_HPP
