//===- unittests/uri/test_pctencode.cpp -----------------------------------===//
//*             _                           _       *
//*  _ __   ___| |_ ___ _ __   ___ ___   __| | ___  *
//* | '_ \ / __| __/ _ \ '_ \ / __/ _ \ / _` |/ _ \ *
//* | |_) | (__| ||  __/ | | | (_| (_) | (_| |  __/ *
//* | .__/ \___|\__\___|_| |_|\___\___/ \__,_|\___| *
//* |_|                                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <iterator>
#include <limits>
#include <numeric>
#include <string_view>

#include "gmock/gmock.h"
#include "uri/pctdecode.hpp"
#include "uri/pctencode.hpp"

#if URI_FUZZTEST
#include "fuzztest/fuzztest.h"
#endif

using namespace std::string_view_literals;

// NOLINTNEXTLINE
TEST(PctEncode, Hello) {
  auto input = "Hello"sv;
  std::string out;
  uri::pctencode(std::begin(input), std::end(input), std::back_inserter(out), uri::pctencode_set::component);
  EXPECT_EQ(out, "Hello");
}

// NOLINTNEXTLINE
TEST(PctEncode, Percent) {
  auto input = "H%llo"sv;
  std::string out;
  uri::pctencode(std::begin(input), std::end(input), std::back_inserter(out), uri::pctencode_set::component);
  EXPECT_EQ(out, "H%25llo");
}

// NOLINTNEXTLINE
TEST(PctEncode, CodePointNeedsEncodeExhaustive) {
  using std::uint_least8_t;

  // Quoting from <https://url.spec.whatwg.org/#percent-encoded-bytes>:
  //
  // The C0 control percent-encode set are the C0 controls and all code points
  // greater than U+007E (~).
  auto const is_c0 = [](uint_least8_t c) {
    if (c == '%') {
      return true;
    }
    return c < 0x20 || c > 0x7E;
  };
  // The fragment percent-encode set is the C0 control percent-encode set and
  // U+0020 SPACE, U+0022 ("), U+003C (<), U+003E (>), and U+0060 (`).
  auto const is_fragment = [=](uint_least8_t c) {
    return is_c0(c) || c == 0x20 || c == 0x22 || c == 0x3C || c == 0x3E;
  };
  // The query percent-encode set is the C0 control percent-encode set and
  // U+0020 SPACE, U+0022 ("), U+0023 (#), U+003C (<), and U+003E (>).
  auto const is_query = [=](uint_least8_t c) {
    return is_c0(c) || c == 0x20 || c == 0x22 || c == 0x23 || c == 0x3C || c == 0x3E;
  };
  // The special-query percent-encode set is the query percent-encode set and
  // U+0027 (').
  auto const is_special_query = [=](uint_least8_t c) { return is_query(c) || c == 0x27; };
  // The path percent-encode set is the query percent-encode set and U+003F (?),
  // U+0060 (`), U+007B ({), and U+007D (}).
  auto const is_path = [=](uint_least8_t c) { return is_query(c) || c == 0x3F || c == 0x60 || c == 0x7B || c == 0x7D; };
  // The userinfo percent-encode set is the path percent-encode set and U+002F
  // (/), U+003A (:), U+003B (;), U+003D (=), U+0040 (@), U+005B ([) to U+005E
  // (^), inclusive, and U+007C (|).
  auto const is_userinfo = [=](uint_least8_t c) {
    return is_path(c) || c == 0x2F || c == 0x3A || c == 0x3B || c == 0x3D || c == 0x40 || c == 0x5B || c == 0x5C ||
           c == 0x5D || c == 0x5E || c == 0x7C;
  };
  // The component percent-encode set is the userinfo percent-encode set and
  // U+0024 ($) to U+0026 (&), inclusive, U+002B (+), and U+002C (,).
  auto const is_component = [=](uint_least8_t c) {
    return is_userinfo(c) || c == 0x24 || c == 0x25 || c == 0x26 || c == 0x2B || c == 0x2C;
  };
  // The application/x-www-form-urlencoded percent-encode set is the component
  // percent-encode set and U+0021 (!), U+0027 (') to U+0029 RIGHT PARENTHESIS,
  // inclusive, and U+007E (~).
  auto const is_form_urlencoded = [=](uint_least8_t c) noexcept {
    return is_component(c) || c == 0x21 || c == 0x27 || c == 0x28 || c == 0x29 || c == 0x7E;
  };

  // Exhaustively test every UTF-8 code unit.
  std::vector<uint_least8_t> chars;
  chars.resize(0xFFU);
  std::iota(std::begin(chars), std::end(chars), std::numeric_limits<uint_least8_t>::min());
  for (auto const c : chars) {
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::fragment), is_fragment(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::query), is_query(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::special_query), is_special_query(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::path), is_path(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::userinfo), is_userinfo(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::component), is_component(c));
    EXPECT_EQ(uri::needs_pctencode(c, uri::pctencode_set::form_urlencoded), is_form_urlencoded(c));
  }
}

#if URI_FUZZTEST
static void EncodeNeverCrashes(std::string const& s, uri::pctencode_set encodeset) {
  std::string encoded;
  uri::pctencode(std::begin(s), std::end(s), std::back_inserter(encoded), encodeset);
}
static auto AnyEncodeSet() {
  return fuzztest::ElementOf<uri::pctencode_set>({uri::pctencode_set::fragment, uri::pctencode_set::query,
                                                  uri::pctencode_set::special_query, uri::pctencode_set::path,
                                                  uri::pctencode_set::userinfo, uri::pctencode_set::component,
                                                  uri::pctencode_set::form_urlencoded});
}
// NOLINTNEXTLINE
FUZZ_TEST(PctEncodeFuzz, EncodeNeverCrashes).WithDomains(fuzztest::String(), AnyEncodeSet());
#endif  // URI_FUZZTEST

#if URI_FUZZTEST
static void RoundTrip(std::string const& s, uri::pctencode_set encodeset) {
  std::string encoded;
  uri::pctencode(std::begin(s), std::end(s), std::back_inserter(encoded), encodeset);

  std::string out;
  std::copy(uri::pctdecode_begin(encoded), uri::pctdecode_end(encoded), std::back_inserter(out));

  EXPECT_THAT(out, testing::StrEq(s));
}
// NOLINTNEXTLINE
FUZZ_TEST(PctEncodeFuzz, RoundTrip).WithDomains(fuzztest::String(), AnyEncodeSet());
#endif  // URI_FUZZTEST
