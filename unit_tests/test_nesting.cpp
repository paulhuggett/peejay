//===- unit_tests/test_nesting.cpp ----------------------------------------===//
//*                                     *
//*                  _   _              *
//*  _ __   ___  ___| |_(_)_ __   __ _  *
//* | '_ \ / _ \/ __| __| | '_ \ / _` | *
//* | | | |  __/\__ \ |_| | | | | (_| | *
//* |_| |_|\___||___/\__|_|_| |_|\__, | *
//*                              |___/  *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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

// DUT
#include "peejay/json.hpp"
#include "peejay/null.hpp"

// Standard library
#include <cstddef>
#include <string_view>
#include <system_error>
#include <tuple>
// Google test
#include <gtest/gtest.h>

using namespace std::string_view_literals;

namespace {

template <std::size_t Depth> struct depth_policies : peejay::default_policies {
  static constexpr std::size_t max_stack_depth = Depth;
};

// A successful parse leaves an empty error code rather than one holding peejay::error::none.
std::error_code expected(peejay::error const err) {
  return err == peejay::error::none ? std::error_code{} : make_error_code(err);
}

// Each value counts towards max_stack_depth (the top-level value is at depth 1) but whitespace never does,
// wherever it appears.

class NestingDepth1 : public testing::TestWithParam<std::tuple<std::u8string_view, peejay::error>> {};

// NOLINTNEXTLINE
TEST_P(NestingDepth1, Parse) {
  auto const& [src, err] = GetParam();
  peejay::parser<peejay::null<depth_policies<1>>> p;
  p.input(src).eof();
  EXPECT_EQ(p.last_error(), expected(err)) << "Real error was: " << p.last_error().message();
}

INSTANTIATE_TEST_SUITE_P(NestingDepth1, NestingDepth1,
                         testing::Values(std::make_tuple(u8"1"sv, peejay::error::none),
                                         std::make_tuple(u8" 1 "sv, peejay::error::none),
                                         std::make_tuple(u8"[]"sv, peejay::error::none),
                                         std::make_tuple(u8"[ ]"sv, peejay::error::none),
                                         std::make_tuple(u8" { } "sv, peejay::error::none),
                                         std::make_tuple(u8"\r\n[\r\n]\r\n"sv, peejay::error::none),
                                         std::make_tuple(u8"[1]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"[ 1 ]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"[[]]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"{\"a\":1}"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"{ \"a\" : 1 }"sv, peejay::error::nesting_too_deep)));

class NestingDepth2 : public testing::TestWithParam<std::tuple<std::u8string_view, peejay::error>> {};

// NOLINTNEXTLINE
TEST_P(NestingDepth2, Parse) {
  auto const& [src, err] = GetParam();
  peejay::parser<peejay::null<depth_policies<2>>> p;
  p.input(src).eof();
  EXPECT_EQ(p.last_error(), expected(err)) << "Real error was: " << p.last_error().message();
}

INSTANTIATE_TEST_SUITE_P(NestingDepth2, NestingDepth2,
                         testing::Values(std::make_tuple(u8"[1, 2]"sv, peejay::error::none),
                                         std::make_tuple(u8"[ 1 , 2 ]"sv, peejay::error::none),
                                         std::make_tuple(u8"[\n1,\r\n2\r]"sv, peejay::error::none),
                                         std::make_tuple(u8"[[ ]]"sv, peejay::error::none),
                                         std::make_tuple(u8"[ { } ]"sv, peejay::error::none),
                                         std::make_tuple(u8"{\"a\": 1}"sv, peejay::error::none),
                                         std::make_tuple(u8"{ \"a\" : 1 , \"b\" : 2 }"sv, peejay::error::none),
                                         std::make_tuple(u8"[[1]]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"[ [ 1 ] ]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"[1, [2]]"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"{\"a\": [1]}"sv, peejay::error::nesting_too_deep),
                                         std::make_tuple(u8"{ \"a\" : { \"b\" : 1 } }"sv,
                                                         peejay::error::nesting_too_deep)));

}  // end anonymous namespace
