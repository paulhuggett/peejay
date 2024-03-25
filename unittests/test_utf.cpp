//===- unittests/test_utf.cpp ---------------------------------------------===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/small_vector.hpp"

using peejay::parser;
using peejay::u8string;

class Utf : public testing::TestWithParam<std::vector<std::byte>> {};

TEST_P (Utf, Utf) {
  parser p{json_out_callbacks{}};
  auto const src = this->GetParam ();
  u8string const res = p.input (std::begin (src), std::end (src)).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, u8"null");
}

INSTANTIATE_TEST_SUITE_P (
    NullKeyword, Utf,
    testing::Values (
        // UTF-8
        std::vector<std::byte>{std::byte{0xEF}, std::byte{0xBB},
                               std::byte{0xBF}, std::byte{'n'}, std::byte{'u'},
                               std::byte{'l'}, std::byte{'l'}},
        // UTF-16 BE
        std::vector<std::byte>{std::byte{0xFE}, std::byte{0xFF},
                               std::byte{0x00}, std::byte{'n'}, std::byte{0x00},
                               std::byte{'u'}, std::byte{0x00}, std::byte{'l'},
                               std::byte{0x00}, std::byte{'l'}},
        // UTF-16 LE
        std::vector<std::byte>{std::byte{0xFF}, std::byte{0xFE}, std::byte{'n'},
                               std::byte{0x00}, std::byte{'u'}, std::byte{0x00},
                               std::byte{'l'}, std::byte{0x00}, std::byte{'l'},
                               std::byte{0x00}},
        // UTF-32 BE
        std::vector<std::byte>{
            std::byte{0x00}, std::byte{0x00}, std::byte{0xFE}, std::byte{0xFF},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{'n'},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{'u'},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{'l'},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{'l'}},
        // UTF-32 LE
        std::vector<std::byte>{
            std::byte{0xFF}, std::byte{0xFE}, std::byte{0x00},
            std::byte{0x00}, std::byte{'n'},  std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}, std::byte{'u'},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{'l'},  std::byte{0x00}, std::byte{0x00},
            std::byte{0x00}, std::byte{'l'},  std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}}));
