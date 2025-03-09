//===- include/peejay/uri/punycode.hpp --------------------*- mode: C++ -*-===//
//*                                        _       *
//*  _ __  _   _ _ __  _   _  ___ ___   __| | ___  *
//* | '_ \| | | | '_ \| | | |/ __/ _ \ / _` |/ _ \ *
//* | |_) | |_| | | | | |_| | (_| (_) | (_| |  __/ *
//* | .__/ \__,_|_| |_|\__, |\___\___/ \__,_|\___| *
//* |_|                |___/                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#ifndef URI_PUNYCODE_HPP
#define URI_PUNYCODE_HPP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string_view>
#include <system_error>
#include <tuple>
#include <variant>

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201811L
#include <ranges>
#endif

#include "peejay/uri/find_last.hpp"
#include "peejay/uri/starts_with.hpp"

namespace uri::punycode {

enum class decode_error_code : int {
  none,
  bad_input,
  overflow,
};

class error_category final : public std::error_category {
public:
  char const* name() const noexcept override;
  std::string message(int error) const override;
};
std::error_code make_error_code(decode_error_code e);

namespace details {

constexpr auto damp = 700U;
constexpr auto base = 36U;
constexpr auto tmin = 1U;
constexpr auto tmax = 26U;
constexpr auto skew = 38U;
constexpr auto initial_bias = std::size_t{72};
constexpr auto initial_n = std::size_t{0x80};
constexpr auto delimiter = '-';  // U+002D HYPHEN-MINUS

/// \param c  The code-point to be checked.
/// \returns true if the \p c represents a "basic" code-point. That is,
///   a code-point less than U+0080.
constexpr bool is_basic_code_point(char32_t const c) noexcept {
  return c < 0x80;
}

/// \param d  A value in the range [0,base) to be encoded as an ASCII character.
/// \returns The basic code point whose value (when used for representing
///   integers) is d, which needs to be in the range 0 to base-1. The lowercase
///   form is used.
constexpr char encode_digit(unsigned const d) noexcept {
  //  0..25 maps to ASCII a..z; 26..35 maps to ASCII 0..9
  assert(d < 36U);
  static_assert(base == 36U);
  if (d < 26U) {
    return static_cast<char>(d + static_cast<unsigned>('a'));
  }
  return static_cast<char>(d - 26U + static_cast<unsigned>('0'));
}

constexpr std::size_t clamp(std::size_t const k, std::size_t const bias) noexcept {
  if (k <= bias) {
    return tmin;
  }
  if (k >= bias + tmax) {
    return tmax;
  }
  return k - bias;
}

template <typename OutputIterator>
OutputIterator encode_vli(std::size_t q, std::size_t const bias, OutputIterator out) {
  for (auto k = base;; k += base) {
    auto const t = clamp(k, bias);
    if (q < t) {
      break;
    }
    assert(base >= t);
    *(out++) = encode_digit(static_cast<unsigned>(t + (q - t) % (base - t)));
    q = (q - t) / (base - t);
  }
  *(out++) = encode_digit(static_cast<unsigned>(q));
  return out;
}

constexpr std::size_t adapt(std::size_t delta, std::size_t const numpoints, bool const firsttime) {
  delta = firsttime ? delta / damp : delta >> 1U;
  delta += delta / numpoints;
  auto k = 0U;
  while (delta > ((base - tmin) * tmax) / 2U) {
    delta = delta / (base - tmin);
    k += base;
  }
  return k + (base - tmin + 1) * delta / (delta + skew);
}

template <typename Container> void sort_and_remove_duplicates(Container& container) {
  auto const first = std::begin(container);
  auto const last = std::end(container);
  std::sort(first, last);
  container.erase(std::unique(first, last), last);
}

}  // namespace details

template <std::ranges::forward_range Range, std::output_iterator<char> OutputIterator>
  requires std::is_same_v<std::remove_cv_t<std::ranges::range_value_t<Range>>, char32_t>
std::tuple<std::ranges::iterator_t<Range>, OutputIterator, bool> encode(Range&& range, bool allow_plain,
                                                                        OutputIterator output) {
  std::u32string non_basic;
  auto num_basics = std::size_t{0};
  // Handle the basic code points. Copy them to the output in order followed by
  // a delimiter if any were copied.
  auto in = std::ranges::for_each(range, [&](char32_t const cp) {
              if (details::is_basic_code_point(cp)) {
                *(output++) = static_cast<char>(cp);
                ++num_basics;
              } else {
                non_basic += cp;
              }
            }).in;
  if (allow_plain && non_basic.empty()) {
    return std::make_tuple(std::move(in), std::move(output), false);
  }
  details::sort_and_remove_duplicates(non_basic);
  auto i = num_basics;
  // Add a delimiter if we have at least one basic code point.
  if (num_basics > 0) {
    *(output++) = details::delimiter;
  }
  auto n = details::initial_n;
  auto delta = std::size_t{0};
  auto bias = details::initial_bias;
  for (char32_t const m : non_basic) {
    assert(m >= n);
    delta += (m - n) * (i + 1);
    n = m;
    // for each code point c in the input (in order)
    std::ranges::for_each(range, [&](char32_t const c) {
      if (c < n) {
        ++delta;  // increment delta (fail on overflow)
      } else if (c == n) {
        // Represent delta as a generalized variable-length integer.
        output = details::encode_vli(delta, bias, output);
        bias = details::adapt(delta, i + 1, i == num_basics);
        delta = 0U;
        ++i;
      }
    });
    ++delta;
    ++n;
  }
  return std::make_tuple(std::move(in), std::move(output), !non_basic.empty());
}

template <std::bidirectional_iterator BidirectionalIterator> struct decode_success_result {
  std::u32string str;
  BidirectionalIterator in;
};

template <std::bidirectional_iterator BidirectionalIterator>
using decode_result = std::variant<std::error_code, decode_success_result<BidirectionalIterator>>;

namespace details {

/// \returns The numeric value of a basic code point (for use in representing
///   integers) in the range 0 to base-1, or base if cp does not represent a
///   value.
constexpr unsigned decode_digit(std::uint_least8_t cp) noexcept {
  constexpr auto alphabet_size = 26U;
  if (cp >= '0' && cp <= '9') {
    // Digits 0..9 represent values 26..35
    return cp - ('0' - alphabet_size);
  }
  if (cp >= 'a') {
    cp -= 'a' - 'A';  // Convert to upper case.
  }
  return (cp >= 'A' && cp <= 'Z') ? cp - 'A' : uri::punycode::details::base;
}

constexpr std::size_t clampk(std::size_t const k, std::size_t const bias) noexcept {
  using uri::punycode::details::tmax;
  using uri::punycode::details::tmin;
  if (k <= bias) {
    return tmin;
  }
  if (k >= bias + tmax) {
    return tmax;
  }
  return k - bias;
}

// Decode a generalized variable-length integer.
template <typename Iterator, typename Sentinel>
std::variant<std::error_code, std::tuple<std::size_t, Iterator>> decode_vli(Iterator first, Sentinel last,
                                                                            std::size_t vli, std::size_t bias) {
  static constexpr auto max = std::numeric_limits<std::size_t>::max();
  using uri::punycode::decode_error_code;
  using uri::punycode::details::base;

  auto w = std::size_t{1};
  for (auto k = base;; k += base) {
    if (first == last) {
      return make_error_code(decode_error_code::bad_input);
    }
    auto const digit = decode_digit(static_cast<std::uint_least8_t>(*first));
    assert(digit <= base);
    ++first;

    if (digit >= base) {
      return make_error_code(decode_error_code::bad_input);
    }
    if (digit > (max - vli) / w) {
      return make_error_code(decode_error_code::overflow);
    }
    vli += digit * w;
    std::size_t const t = clampk(k, bias);
    if (digit < t) {
      break;
    }
    if (w > max / (base - t)) {
      return make_error_code(decode_error_code::overflow);
    }
    w *= (base - t);
  }
  return std::make_tuple(vli, first);
}

template <std::bidirectional_iterator Iterator, std::sentinel_for<Iterator> Sentinel, typename OutputIterator>
std::variant<std::error_code, Iterator> copy_plain_ascii_part(Iterator first, Sentinel last, OutputIterator out) {
  // Find the end of the literal portion (if there is one) by scanning for the
  // last delimiter.
  auto const encoded_range = find_last(first, last, delimiter);
  if (encoded_range.empty()) {
    return first;
  }
  // Copy the plain ASCII part of the string to the output (if any).
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  for (auto pos = first, end = encoded_range.begin(); pos != end; ++pos) {
    auto const code_point = static_cast<char32_t>(*pos);
    if (!details::is_basic_code_point(code_point)) {
      return make_error_code(decode_error_code::bad_input);
    }
    *(out++) = code_point;
  }

  return std::next(encoded_range.begin());
}

template <std::bidirectional_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
decode_result<Iterator> decode_loop(Iterator in, Sentinel last, std::u32string&& output) {
  // The main decoding loop.
  auto n = initial_n;
  auto index = std::size_t{0};
  auto bias = initial_bias;

  while (in != last) {
    // Decode a generalized variable-length integer into delta, which gets added
    // to 'index'. The overflow checking is easier if we increase index as we go, then
    // subtract off its starting value at the end to obtain delta.
    auto const decode_res = details::decode_vli(in, last, index, bias);
    static_assert(
        std::is_same_v<std::error_code, std::remove_const_t<std::variant_alternative_t<0, decltype(decode_res)>>>);
    if (auto const* const err = std::get_if<std::error_code>(&decode_res)) {
      return *err;
    }
    auto const old_vli = index;
    auto output_length = output.length() + 1;
    std::tie(index, in) = std::get<1>(decode_res);
    bias = adapt(index - old_vli, output_length, old_vli == 0);

    // index was supposed to wrap around from out+1 to 0, incrementing n each time,
    // so we'll fix that now.
    if (index / output_length > std::numeric_limits<std::uint_least32_t>::max() - n) {
      return make_error_code(decode_error_code::overflow);
    }
    n += index / output_length;
    index %= output_length;

    // Insert n into the output at position i.
    output.insert(index, std::size_t{1}, static_cast<char32_t>(n));
    ++index;
  }
  return decode_success_result<Iterator>{std::move(output), std::move(in)};
}

}  // end namespace details

/// A type that is always false. Used to improve the failure mesages from
/// static_assert().
template <typename... T> [[maybe_unused]] constexpr bool always_false = false;

template <std::bidirectional_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
decode_result<Iterator> decode(Iterator first, Sentinel last) {
  std::u32string output;
  return std::visit(
      [&last, &output]<typename Arg>(Arg&& arg) {
        using T = std::decay_t<Arg>;
        if constexpr (std::is_same_v<T, std::error_code>) {
          return decode_result<Iterator>(std::forward<Arg>(arg));
        } else if constexpr (std::is_same_v<T, Iterator>) {
          return details::decode_loop(arg, last, std::move(output));
        } else {
          static_assert(always_false<T>, "non-exhaustive visitor!");
        }
      },
      details::copy_plain_ascii_part(first, last, std::back_inserter(output)));
}

template <std::ranges::bidirectional_range Range>
  requires std::is_same_v<std::remove_cv_t<std::ranges::range_value_t<Range>>, char>
decode_result<std::ranges::iterator_t<Range>> decode(Range&& input) {
  return decode(std::ranges::begin(input), std::ranges::end(input));
}

}  // end namespace uri::punycode

#endif  // URI_PUNYCODE_HPP
