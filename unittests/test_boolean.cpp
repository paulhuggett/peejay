//===- unittests/test_boolean.cpp -----------------------------------------===//
//*  _                 _                   *
//* | |__   ___   ___ | | ___  __ _ _ __   *
//* | '_ \ / _ \ / _ \| |/ _ \/ _` | '_ \  *
//* | |_) | (_) | (_) | |  __/ (_| | | | | *
//* |_.__/ \___/ \___/|_|\___|\__,_|_| |_| *
//*                                        *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/json/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "callbacks.hpp"
#include "json/json.hpp"

using testing::StrictMock;

namespace {

class JsonBoolean : public testing::Test {
protected:
  StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

TEST_F (JsonBoolean, True) {
  EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);

  json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
  p.input ("true").eof ();
  EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonBoolean, False) {
  EXPECT_CALL (callbacks_, boolean_value (false)).Times (1);

  json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
  p.input (" false ").eof ();
  EXPECT_FALSE (p.has_error ());
}
