//===- unittests/test_utf.cpp ---------------------------------------------===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gmock/gmock.h>

#include "callbacks.hpp"
#include "peejay/json/json.hpp"
#include "peejay/json/small_vector.hpp"

class Utf : public testing::TestWithParam<std::vector<std::byte>> {};

// NOLINTNEXTLINE
TEST_P (Utf, NullCallbackIsInvoked) {
  using mocks = mock_json_callbacks<peejay::default_policies::integer_type>;
  testing::StrictMock<mocks> callbacks;
  testing::InSequence const _;
  EXPECT_CALL (callbacks, null_value ()).Times (1);

  peejay::parser p{callbacks_proxy<mocks>{callbacks}};
  auto const& src = GetParam ();
  p.input (std::begin (src), std::end (src)).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
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
        std::vector<std::byte>{
            std::byte{0xFF}, std::byte{0xFE},
            std::byte{'n'}, std::byte{0x00},
            std::byte{'u'}, std::byte{0x00},
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
            std::byte{0xFF}, std::byte{0xFE}, std::byte{0x00}, std::byte{0x00},
            std::byte{'n'}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{'u'}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{'l'}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{'l'}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}}
    )
);
