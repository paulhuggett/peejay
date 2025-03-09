//===- include/peejay/uri/find_last.hpp -------------------*- mode: C++ -*-===//
//*   __ _           _   _           _    *
//*  / _(_)_ __   __| | | | __ _ ___| |_  *
//* | |_| | '_ \ / _` | | |/ _` / __| __| *
//* |  _| | | | | (_| | | | (_| \__ \ |_  *
//* |_| |_|_| |_|\__,_| |_|\__,_|___/\__| *
//*                                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_URI_FIND_LAST_HPP
#define PEEJAY_URI_FIND_LAST_HPP

#include <functional>
#include <optional>
#include <ranges>
#include <version>

namespace uri {

namespace details {

template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel, typename T,
          typename Proj = std::identity>
  requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<Iterator, Proj>, T const *>
constexpr std::ranges::subrange<Iterator> find_last_forward(Iterator first, Sentinel last, T const &value,
                                                            Proj proj = {}) {
  // If Iterator is a forward_iterator, we can only go from begin to end.
  std::optional<Iterator> found;
  for (; first != last; ++first) {
    if (std::invoke(proj, *first) == value) {
      found = first;
    }
  }
  if (!found) {
    return {first, first};
  }
  return {*found, std::ranges::next(*found, last)};
}

template <std::bidirectional_iterator Iterator, std::sentinel_for<Iterator> Sentinel, typename T,
          typename Proj = std::identity>
  requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<Iterator, Proj>, T const *>
constexpr std::ranges::subrange<Iterator> find_last_bidi(Iterator const first, Sentinel const last, T const &value,
                                                         Proj const proj = {}) {
  if (first == last) {
    return {last, last};
  }
  auto const rend = std::make_reverse_iterator(first);
  auto const rb = std::ranges::find(std::make_reverse_iterator(last), rend, value, proj);
  if (rb == rend) {
    auto const end = std::ranges::next(rb.base(), last);
    return {end, end};
  }
  auto const result = std::ranges::subrange<Iterator>{std::prev(rb.base()), last};
#ifndef NDEBUG
  auto const forward_result = details::find_last_forward(first, last, value, proj);
  assert(forward_result.begin() == result.begin());
  assert(forward_result.end() == result.end());
#endif
  return result;
}

}  // end namespace details

#if defined(__cpp_lib_ranges_find_last) && __cpp_lib_ranges_find_last >= 202207L
inline constexpr auto find_last = std::ranges::find_last;
#else
namespace details {

struct find_last_fn {
  template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel, typename T,
            typename Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<Iterator, Proj>, T const *>
  constexpr std::ranges::subrange<Iterator> operator()(Iterator const first, Sentinel const last, T const &value,
                                                       Proj const proj = {}) const {
    if constexpr (std::bidirectional_iterator<Iterator> && std::bidirectional_iterator<Sentinel>) {
      return details::find_last_bidi(first, last, value, proj);
    } else {
      return details::find_last_forward(first, last, value, proj);
    }
  }

  template <std::ranges::forward_range Range, typename T, typename Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<Range>, Proj>,
                                            T const *>
  constexpr std::ranges::borrowed_subrange_t<Range> operator()(Range &&range, T const &value, Proj proj = {}) const {
    return this->operator()(std::ranges::begin(range), std::ranges::end(range), value, proj);
  }
};

}  // end namespace details

inline constexpr details::find_last_fn find_last;

#endif  // __cpp_lib_ranges_find_last

}  // end namespace uri

#endif  // PEEJAY_URI_FIND_LAST_HPP
