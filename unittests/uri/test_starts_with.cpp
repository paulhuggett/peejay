//===- unittests/uri/test_starts_with.cpp ---------------------------------===//
//*      _             _                  _ _   _      *
//*  ___| |_ __ _ _ __| |_ ___  __      _(_) |_| |__   *
//* / __| __/ _` | '__| __/ __| \ \ /\ / / | __| '_ \  *
//* \__ \ || (_| | |  | |_\__ \  \ V  V /| | |_| | | | *
//* |___/\__\__,_|_|   \__|___/   \_/\_/ |_|\__|_| |_| *
//*                                                    *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include <array>

#include "uri/starts_with.hpp"

using namespace std::string_view_literals;

namespace {

constexpr auto ascii_upper (char8_t const c) {
  return u8'a' <= c && c <= u8'z' ? static_cast<char8_t>(c + u8'A' - u8'a') : c;
}

}  // end anonymous namespace

static_assert (uri::starts_with ("const_cast", "const"sv));
static_assert (uri::starts_with ("constexpr", "const"sv));
static_assert (!uri::starts_with ("volatile", "const"sv));

// NOLINTNEXTLINE
TEST (StartsWith, Projections) {
  EXPECT_TRUE (uri::starts_with (u8"Constantinopolis", u8"constant"sv, {}, ascii_upper, ascii_upper));
  EXPECT_FALSE (uri::starts_with (u8"Istanbul", u8"constant"sv, {}, ascii_upper, ascii_upper));
}
// NOLINTNEXTLINE
TEST (StartsWith, Predicate) {
  constexpr auto cmp_ignore_case = [] (char8_t const x, char8_t const y) {
    return ascii_upper (x) == ascii_upper (y);
  };
  EXPECT_TRUE (uri::starts_with (u8"Metropolis", u8"metro"sv, cmp_ignore_case));
  EXPECT_FALSE (uri::starts_with(u8"Acropolis", u8"metro"sv, cmp_ignore_case));
}
// NOLINTNEXTLINE
TEST (StartsWith, Pipeline) {
  constexpr auto v = std::array{ 1, 3, 5, 7, 9 };
  constexpr auto odd = [](int const x) { return x % 2; };
  EXPECT_TRUE (uri::starts_with(v, std::views::iota(1) | std::views::filter(odd) | std::views::take(3)));
}
