//===- unit_tests/test_variant.cpp ----------------------------------------===//
//*                  _             _    *
//* __   ____ _ _ __(_) __ _ _ __ | |_  *
//* \ \ / / _` | '__| |/ _` | '_ \| __| *
//*  \ V / (_| | |  | | (_| | | | | |_  *
//*   \_/ \__,_|_|  |_|\__,_|_| |_|\__| *
//*                                     *
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
#include <gtest/gtest.h>

#include "peejay/details/type_list.hpp"
#include "peejay/details/variant.hpp"

namespace {

using peejay::details::variant;
using namespace peejay::type_list;

// NOLINTNEXTLINE
TEST(Variant, Empty) {
  variant<type_list<char, int>> v;
#ifndef NDEBUG
  EXPECT_EQ(v.holds(), peejay::type_list::npos);
#endif
}

// NOLINTNEXTLINE
TEST(Variant, EmplaceAndDestroy) {
  variant<type_list<char, int>> v;
  v.emplace<char>('a');
  EXPECT_EQ(v.get<char>(), 'a');
  v.destroy<char>();
  v.emplace<int>(42);
  EXPECT_EQ(v.get<int>(), 42);
  v.destroy<int>();
}

}  // end anonymous namespace
