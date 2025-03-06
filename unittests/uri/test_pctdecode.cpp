//===- unittests/uri/test_pctdecode.cpp -----------------------------------===//
//*             _      _                    _       *
//*  _ __   ___| |_ __| | ___  ___ ___   __| | ___  *
//* | '_ \ / __| __/ _` |/ _ \/ __/ _ \ / _` |/ _ \ *
//* | |_) | (__| || (_| |  __/ (_| (_) | (_| |  __/ *
//* | .__/ \___|\__\__,_|\___|\___\___/ \__,_|\___| *
//* |_|                                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "uri/pctdecode.hpp"

// google test/fuzz.
#include "gmock/gmock.h"
#if URI_FUZZTEST
#include "fuzztest/fuzztest.h"
#endif

#include <tuple>

using namespace std::string_view_literals;

class UriPctDecode : public testing::TestWithParam<
                       std::tuple<std::string_view, std::string_view>> {};

// NOLINTNEXTLINE
TEST_P (UriPctDecode, RawIterator) {
  auto const& [input, expected] = GetParam ();
  std::string out;
  std::copy (uri::pctdecode_begin (input), uri::pctdecode_end (input),
             std::back_inserter (out));
  EXPECT_EQ (out, expected);
}
// NOLINTNEXTLINE
TEST_P (UriPctDecode, RangeBasedForLoop) {
  auto const& [input, expected] = GetParam ();
  std::string out;
  for (auto const& c : uri::pctdecoder{input}) {
    out += c;
  }
  EXPECT_EQ (out, expected);
}

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201811L
// NOLINTNEXTLINE
TEST_P (UriPctDecode, RangesCopy) {
  auto const& [input, expected] = GetParam ();
  std::string out;
  auto r = input | uri::views::pctdecode;
  std::ranges::copy (r, std::back_inserter (out));
  EXPECT_EQ (out, expected);
}
// NOLINTNEXTLINE
TEST_P (UriPctDecode, RangesForEach) {
  auto const& [input, expected] = GetParam ();
  std::string out;
  auto r = input | uri::views::pctdecode;
  std::ranges::for_each (r, [&out] (auto c) { out += c; });
  EXPECT_EQ (out, expected);
}
#endif  // __cpp_lib_ranges

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P (
  UriPctDecode, UriPctDecode,
  testing::Values (
    std::make_tuple (""sv, ""sv),                  // empty
    std::make_tuple ("abcdef"sv, "abcdef"sv),      // no escapes
    std::make_tuple ("a%62%63def"sv, "abcdef"sv),  // two encoded characters
    std::make_tuple ("a%7ad"sv, "azd"sv),          // lower hex
    std::make_tuple ("a%7Ad"sv, "azd"sv),          // upper hex
    std::make_tuple ("ab%"sv, "ab%"sv),            // lonely percent at end
    std::make_tuple ("ab%a"sv, "ab%a"sv),    // percent then one hex at end
    std::make_tuple ("ab%qq"sv, "ab%qq"sv),  // percent then no hex
    std::make_tuple ("ab%1q"sv, "ab%1q"sv)   // percent then one hex
    ));

#if URI_FUZZTEST
static void PctDecodeNeverCrashes (std::string const& input) {
  std::string out;
  std::copy (uri::pctdecode_begin (input), uri::pctdecode_end (input),
             std::back_inserter (out));
}
// NOLINTNEXTLINE
FUZZ_TEST (PctDecodeFuzz, PctDecodeNeverCrashes);

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201811L
static void PctDecodeViewNeverCrashes (std::string const& input) {
  std::string out;
  auto r = input | uri::views::pctdecode;
  std::ranges::copy (r, std::back_inserter (out));
}
// NOLINTNEXTLINE
FUZZ_TEST (PctDecodeFuzz, PctDecodeViewNeverCrashes);

#endif  // __cpp_lib_ranges

#endif  // URI_FUZZTEST
