//===- unit_tests/test_null.cpp -------------------------------------------===//
//*                          _  *
//*   ___ ___   ___  _ __ __| | *
//*  / __/ _ \ / _ \| '__/ _` | *
//* | (_| (_) | (_) | | | (_| | *
//*  \___\___/ \___/|_|  \__,_| *
//*                             *
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
#include <string>
// Google test/mock/fuzz
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#if defined(PEEJAY_FUZZTEST) && PEEJAY_FUZZTEST
#include <fuzztest/fuzztest.h>
#endif
// Local
#include "callbacks.hpp"

using namespace std::string_view_literals;

namespace {

class JsonNull : public testing::Test {
public:
  mockable_callbacks<peejay::default_policies> mock_;
};

// NOLINTNEXTLINE
TEST_F(JsonNull, False) {
  EXPECT_CALL(mock_.callbacks, null_value()).Times(1);

  peejay::parser p = peejay::make_parser(mock_.proxy);
  p.input(u8" null "sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(JsonNull, CallbackReturnsError) {
  using testing::Return;
  auto const err = make_error_code(std::errc::io_error);
  EXPECT_CALL(mock_.callbacks, null_value()).Times(1).WillOnce(Return(err));

  peejay::parser p = peejay::make_parser(mock_.proxy);
  p.input(u8"null"sv).eof();
  EXPECT_EQ(p.last_error(), err) << "Real error was: " << p.last_error().message();
}

void NullTokenNeverCrashes(std::u8string const& input) {
  using testing::AnyOf;
  using testing::Eq;
  using testing::Return;

  mockable_callbacks<peejay::default_policies> mock;
  if (input == u8"ull"sv) {
    EXPECT_CALL(mock.callbacks, null_value()).WillOnce(Return(std::error_code{}));
  }

  peejay::parser p = peejay::make_parser(mock.proxy);
  p.input(u8"n"sv).input(input).eof();
  EXPECT_THAT(p.last_error(), AnyOf(std::error_code{}, make_error_code(peejay::error::unrecognized_token),
                                    make_error_code(peejay::error::unexpected_extra_input)));
}

TEST(NullToken, Empty) {
  NullTokenNeverCrashes(u8"");
}
#if defined(PEEJAY_FUZZTEST) && PEEJAY_FUZZTEST
FUZZ_TEST(NullToken, NullTokenNeverCrashes);
#endif  // PEEJAY_FUZZTEST

}  // end anonymous namespace
