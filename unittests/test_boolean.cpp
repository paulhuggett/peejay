//===- unittests/test_boolean.cpp -----------------------------------------===//
//*  _                 _                   *
//* | |__   ___   ___ | | ___  __ _ _ __   *
//* | '_ \ / _ \ / _ \| |/ _ \/ _` | '_ \  *
//* | |_) | (_) | (_) | |  __/ (_| | | | | *
//* |_.__/ \___/ \___/|_|\___|\__,_|_| |_| *
//*                                        *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
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
  StrictMock<mock_json_callbacks<std::uint64_t>> callbacks_;
  callbacks_proxy<mock_json_callbacks<std::uint64_t>> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (JsonBoolean, True) {
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);

  auto p = peejay::make_parser (proxy_);
  p.input (u8"true"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (JsonBoolean, False) {
  EXPECT_CALL (callbacks_, boolean_value (false)).Times (1);

  peejay::parser<decltype (proxy_)> p = peejay::make_parser (proxy_);
  p.input (u8" false "sv).eof ();
  EXPECT_FALSE (p.has_error ());
}
