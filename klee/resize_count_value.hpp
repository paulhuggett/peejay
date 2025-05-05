//===- klee/resize_count_value.hpp ------------------------*- mode: C++ -*-===//
//*                _                                 _                _             *
//*  _ __ ___  ___(_)_______    ___ ___  _   _ _ __ | |_  __   ____ _| |_   _  ___  *
//* | '__/ _ \/ __| |_  / _ \  / __/ _ \| | | | '_ \| __| \ \ / / _` | | | | |/ _ \ *
//* | | |  __/\__ \ |/ /  __/ | (_| (_) | |_| | | | | |_   \ V / (_| | | |_| |  __/ *
//* |_|  \___||___/_/___\___|  \___\___/ \__,_|_| |_|\__|   \_/ \__,_|_|\__,_|\___| *
//*                                                                                 *
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
#ifndef RESIZE_COUNT_VALUE_HPP
#define RESIZE_COUNT_VALUE_HPP

#ifdef KLEE_RUN
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "vcommon.hpp"

template <typename Container> void resize_test(Container &c, std::size_t size, typename Container::size_type count) {
  populate(c, size);
  member value{99};
  // Call the function under test.
  c.resize(count, value);
}

constexpr std::size_t max_elements = 13;

template <typename TestVector> void resize_count_value() {
  try {
    MAKE_SYMBOLIC(member::throw_number);

    std::size_t initial_size;
    MAKE_SYMBOLIC(initial_size);
    klee_assume(initial_size <= max_elements);

    typename TestVector::size_type new_size;
    MAKE_SYMBOLIC(new_size);
    klee_assume(new_size <= max_elements);

    TestVector sv;
    resize_test(sv, initial_size, new_size);
#ifdef KLEE_RUN
    std::vector<member> v;
    resize_test(v, initial_size, new_size);
    check_equal(sv, v);
#endif  // KLEE_RUN
  } catch (memberex const &) {
    // catch and ignore.
  }
  check_instances();
#ifdef KLEE_RUN
  std::cerr << "Pass!\n";
#endif  // KLEE_RUN
}

#endif  // RESIZE_COUNT_VALUE_HPP
