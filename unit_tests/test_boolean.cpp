//===- unit_tests/test_boolean.cpp ----------------------------------------===//
//*  _                 _                   *
//* | |__   ___   ___ | | ___  __ _ _ __   *
//* | '_ \ / _ \ / _ \| |/ _ \/ _` | '_ \  *
//* | |_) | (_) | (_) | |  __/ (_| | | | | *
//* |_.__/ \___/ \___/|_|\___|\__,_|_| |_| *
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
#include "peejay/json.hpp"

// Standard library
#include <string>
// 3rd party
#include <gtest/gtest.h>
// Local
#include "callbacks.hpp"

using testing::StrictMock;
using namespace std::string_view_literals;

namespace {

class JsonBoolean : public testing::Test {
protected:
  StrictMock<mock_json_callbacks<std::uint64_t, double, char8_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t, double, char8_t>> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F(JsonBoolean, True) {
  EXPECT_CALL(callbacks_, boolean_value(true)).Times(1);

  auto p = peejay::make_parser(proxy_);
  p.input(u8"true"sv).eof();
  EXPECT_FALSE(p.has_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonBoolean, False) {
  EXPECT_CALL(callbacks_, boolean_value(false)).Times(1);

  peejay::parser p = peejay::make_parser(proxy_);
  p.input(u8" false "sv).eof();
  EXPECT_FALSE(p.has_error());
}
