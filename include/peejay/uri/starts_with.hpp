//===- include/peejay/uri/starts_with.hpp -----------------*- mode: C++ -*-===//
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
#ifndef PEEJAY_URI_STARTS_WITH_HPP
#define PEEJAY_URI_STARTS_WITH_HPP

#include <cstdint>
#include <functional>
#include <ranges>
#include <version>

namespace uri {

#if defined(__cpp_lib_ranges_starts_ends_with) && __cpp_lib_ranges_starts_ends_with >= 202106L
inline constexpr auto starts_with = std::ranges::starts_with;
#else
struct starts_with_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            typename Pred = std::ranges::equal_to, typename Proj1 = std::identity, typename Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const {
    return std::ranges::mismatch(std::move(first1), last1, std::move(first2), last2, std::move(pred), std::move(proj1),
                                 std::move(proj2))
               .in2 == last2;
  }

  template <std::ranges::input_range R1, std::ranges::input_range R2, typename Pred = std::ranges::equal_to,
            typename Proj1 = std::identity, typename Proj2 = std::identity>
    requires std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

inline constexpr starts_with_fn starts_with{};
#endif  // __cpp_lib_ranges_starts_ends_with

}  // end namespace uri

#endif  // PEEJAY_URI_STARTS_WITH_HPP
