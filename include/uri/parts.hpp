//===- include/uri/parts.hpp ------------------------------*- mode: C++ -*-===//
//*                   _        *
//*  _ __   __ _ _ __| |_ ___  *
//* | '_ \ / _` | '__| __/ __| *
//* | |_) | (_| | |  | |_\__ \ *
//* | .__/ \__,_|_|   \__|___/ *
//* |_|                        *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#ifndef URI_PARTS_HPP
#define URI_PARTS_HPP

#include <bitset>

#include "uri/icubaby.hpp"
#include "uri/pctdecode.hpp"
#include "uri/pctencode.hpp"
#include "uri/punycode.hpp"
#include "uri/uri.hpp"

namespace uri {

enum class parts_field : std::uint_least8_t {
  scheme = 0,
  userinfo = 1,
  host = 2,
  port = 3,
  query = 4,
  fragment = 5,
  path = 6,
  last = 32
};

constexpr pctencode_set pctencode_set_from_parts_field (
  parts_field const field) noexcept {
  switch (field) {
  case parts_field::userinfo: return pctencode_set::userinfo;
  case parts_field::path: return pctencode_set::path;
  case parts_field::query: return pctencode_set::query;
  case parts_field::fragment: return pctencode_set::fragment;
  case parts_field::host:
  case parts_field::port:
  case parts_field::scheme:
  case parts_field::last:
  default: return pctencode_set::none;
  }
}

inline constexpr auto idna_prefix = std::string_view{"xn--"};

namespace details {

template <typename T>
class ro_sink_container {
public:
  using value_type = T;

  // NOLINTNEXTLINE(hicpp-named-parameter,readability-named-parameter)
  constexpr void push_back (T const&) noexcept { ++size_; }
  // NOLINTNEXTLINE(hicpp-named-parameter,readability-named-parameter)
  constexpr void push_back (T&&) noexcept { ++size_; }
  [[nodiscard]] constexpr std::size_t size () const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty () const noexcept { return size_ == 0; }

private:
  std::size_t size_ = 0;
};

std::size_t pct_encoded_size (std::string_view str, pctencode_set encodeset);
std::size_t pct_decoded_size (std::string_view str);

template <typename Function>
  requires std::is_invocable_r_v<std::string_view, Function, std::string_view, unsigned, parts_field>
void parts_strings (parts& parts, Function const function) {
  if (parts.scheme.has_value () && parts.scheme->data () != nullptr) {
    parts.scheme = function (*parts.scheme, 0U, parts_field::scheme);
  }
  auto count = 0U;
  for (auto& segment : parts.path.segments) {
    if (segment.data () != nullptr) {
      segment = function (segment, count, parts_field::path);
      ++count;
    }
  }
  if (parts.authority.has_value ()) {
    auto& auth = *parts.authority;
    if (auth.userinfo.has_value () && auth.userinfo->data () != nullptr) {
      auth.userinfo = function (*auth.userinfo, 0U, parts_field::userinfo);
    }
    if (auth.host.data () != nullptr) {
      auth.host = function (auth.host, 0U, parts_field::host);
    }
    if (auth.port.has_value () && auth.port->data () != nullptr) {
      auth.port = function (*auth.port, 0U, parts_field::port);
    }
  }
  if (parts.query.has_value () && parts.query->data () != nullptr) {
    parts.query = function (*parts.query, 0U, parts_field::query);
  }
  if (parts.fragment.has_value () && parts.fragment->data () != nullptr) {
    parts.fragment = function (*parts.fragment, 0U, parts_field::fragment);
  }
}

template <std::output_iterator<char8_t> OutputIterator>
struct puny_encoded_result {
  OutputIterator out;
  bool any_non_ascii = false;
};

constexpr inline bool is_not_dot (char32_t const code_point) {
  return code_point != '.';
}

template <std::ranges::input_range Range,
          std::output_iterator<char> OutputIterator>
  requires std::is_same_v<std::remove_cv_t<std::ranges::range_value_t<Range>>,
                          char32_t>
puny_encoded_result<OutputIterator> puny_encoded (Range&& range,
                                                  OutputIterator out) {
  using std::ranges::subrange;
  bool any_needed = false;
  auto const end = std::ranges::end (range);

  auto const find_dot = std::views::take_while (is_not_dot);
  auto view = subrange{std::ranges::begin (range), end} | find_dot;

  for (;;) {
    std::string segment;
    auto const& [in, _, any_non_ascii] =
      punycode::encode (view, true, std::back_inserter (segment));
    if (any_non_ascii) {
      out = std::ranges::copy (idna_prefix, out).out;
      any_needed = true;
    }
    out = std::ranges::copy (segment, out).out;
    if (in == end) {
      break;
    }

    (*out++) = '.';
    view = subrange{std::next (in), end} | find_dot;
  }

  return {std::move (out), any_needed};
}

template <std::ranges::input_range Range>
  requires std::is_same_v<
    std::remove_cv_t<typename std::ranges::range_value_t<Range>>, char32_t>
std::size_t puny_encoded_size (Range&& range) {
  details::ro_sink_container<char> sink;
  return puny_encoded (std::forward<Range> (range), std::back_inserter (sink))
             .any_non_ascii
           ? sink.size ()
           : std::size_t{0};
}

template <std::output_iterator<char8_t> OutputIterator>
struct puny_decoded_result {
  OutputIterator out;
  bool any_encoded = false;
};

template <std::output_iterator<char8_t> OutputIterator>
puny_decoded_result (OutputIterator,
                     bool) -> puny_decoded_result<OutputIterator>;

template <std::ranges::bidirectional_range Range,
          std::output_iterator<char8_t> OutputIterator>
  requires std::is_same_v<std::ranges::range_value_t<Range>, char>
std::variant<std::error_code, puny_decoded_result<OutputIterator>>
puny_decoded (Range&& range, OutputIterator out) {
  using std::ranges::subrange;
  auto const last = std::ranges::end (range);
  bool any_encoded = false;

  auto const find_dot = std::views::take_while (is_not_dot);
  auto view = subrange{std::ranges::begin (range), last} | find_dot;
  auto first = std::ranges::begin (view);

  for (;;) {
    if (starts_with (view, idna_prefix)) {
      any_encoded = true;
      std::ranges::advance (first, idna_prefix.size ());
      auto decode_result =
        punycode::decode (subrange{first, std::ranges::end (view)});
      using success_type =
        std::variant_alternative_t<1U, decltype (decode_result)>;
      if (auto const* const dr = std::get_if<success_type> (&decode_result)) {
        first = std::move (dr->in);
        out = std::ranges::copy (
                dr->str | icubaby::views::transcode<char32_t, char8_t>, out)
                .out;
      } else if (auto const* const erc =
                   std::get_if<std::error_code> (&decode_result)) {
        return *erc;
      } else {
        return make_error_code (std::errc::invalid_argument);
      }
    } else {
      auto const& copy_result = std::ranges::copy (view, out);
      first = std::move (copy_result.in);
      out = std::move (copy_result.out);
    }

    if (first == last) {
      break;
    }
    first = std::next (first);
    view = subrange{first, last} | find_dot;
    *(out++) = '.';
  }
  return puny_decoded_result{out, any_encoded};
}

template <std::ranges::bidirectional_range Range>
  requires std::is_same_v<std::ranges::range_value_t<Range>, char>
std::variant<std::error_code, std::size_t> puny_decoded_size (Range&& range) {
  details::ro_sink_container<char> sink;
  auto out = std::back_inserter (sink);
  auto result = puny_decoded (std::forward<Range> (range), out);
  if (auto const* const decoded =
        std::get_if<puny_decoded_result<decltype (out)>> (&result)) {
    return decoded->any_encoded ? sink.size () : std::size_t{0};
  }
  if (auto const* const erc = std::get_if<std::error_code> (&result)) {
    return *erc;
  }
  return make_error_code (std::errc::invalid_argument);
}

}  // end namespace details

template <typename VectorContainer>
  requires std::contiguous_iterator<typename VectorContainer::iterator> &&
           std::is_same_v<typename VectorContainer::value_type, char>
parts encode (VectorContainer& store, parts const& p) {
  using pft = std::underlying_type_t<parts_field>;
  std::bitset<static_cast<pft> (parts_field::last)> needs_encoding;

  parts result = p;
  store.clear ();
  auto const convert_to_utf32 = [] (auto const& r) {
    return r | std::views::transform ([] (char const c) { return static_cast<char8_t> (c); }) |
           icubaby::views::transcode<char8_t, char32_t>;
  };

  auto required_size = std::size_t{0};
  details::parts_strings (result, [&required_size, &convert_to_utf32, &needs_encoding] (
                                      std::string_view const str, unsigned const index, parts_field const field) {
    assert (str.data () != nullptr && "String data field was null");
    assert ((index == 0 || field == parts_field::path) && "Unexpected non-zero index");
    auto const extra = field == parts_field::host
                           ? details::puny_encoded_size (convert_to_utf32 (str))
                           : details::pct_encoded_size (str, pctencode_set_from_parts_field (field));
    if (auto const pos = static_cast<pft> (field) + index; pos < needs_encoding.size ()) {
      needs_encoding.set (pos, needs_encoding.test (pos) || extra != 0);
    }
    required_size += extra;
    return str;
  });
  store.reserve (required_size);

  details::parts_strings (result, [&store, &convert_to_utf32, &needs_encoding] (
                                      std::string_view const str, unsigned const index, parts_field const field) {
    assert (str.data () != nullptr && "String data field was null");
    assert ((index == 0 || field == parts_field::path) && "Unexpected non-zero index");
    auto const needs_encoding_pos = static_cast<pft> (field) + index;
    if (needs_encoding_pos < needs_encoding.size () && !needs_encoding.test (needs_encoding_pos)) {
      return str;
    }

    auto const original_size = store.size ();
    if (field == parts_field::host) {
      assert (details::puny_encoded_size (convert_to_utf32 (str)) != 0 &&
              "Host field said it needs to be encoded but doesn't");
      assert (store.capacity () >= original_size + details::puny_encoded_size (convert_to_utf32 (str)) &&
              "Store capacity is insufficient");
      details::puny_encoded (convert_to_utf32 (str), std::back_inserter (store));
      assert (original_size + details::puny_encoded_size (convert_to_utf32 (str)) == store.size () &&
              "Store size was not as expected");
    } else {
      auto const es = pctencode_set_from_parts_field (field);
      assert (field == parts_field::path || needs_pctencode (str, es));
      if (needs_encoding_pos >= needs_encoding.size () && !needs_pctencode (str, es)) {
        return str;
      }
      assert (store.capacity () >= original_size + details::pct_encoded_size (str, es) &&
              "Store capacity is insufficient");
      pctencode (std::begin (str), std::end (str), std::back_inserter (store), es);
      assert (original_size + details::pct_encoded_size (str, es) == store.size () && "Store size was not as expected");
    }
    return std::string_view{store.data () + original_size, store.size () - original_size};
  });
  assert (required_size == store.size () && "Expected store required size does not match actual");
  return result;
}

template <typename VectorContainer>
  requires std::contiguous_iterator<typename VectorContainer::iterator> &&
           std::is_same_v<typename VectorContainer::value_type, char>
std::variant<std::error_code, parts> decode (VectorContainer& store,
                                             parts const& p) {
  using pft = std::underlying_type_t<parts_field>;
  std::bitset<static_cast<pft> (parts_field::last)> needs_encoding;

  parts result = p;
  store.clear ();

  auto required_size = std::size_t{0};
  std::error_code error;
  details::parts_strings (result, [&required_size, &error, &needs_encoding] (
                                      std::string_view const str, unsigned const index, parts_field const field) {
    assert (str.data () != nullptr && "String data field was null");
    assert ((index == 0 || field == parts_field::path) && "Unexpected non-zero index");
    if (error) {
      return str;
    }

    auto extra = std::size_t{0};
    if (field == parts_field::host) {
      auto const pds_result = details::puny_decoded_size (str);
      if (auto const* const size = std::get_if<std::size_t> (&pds_result)) {
        extra = *size;
      } else if (auto const* const erc = std::get_if<std::error_code> (&pds_result)) {
        error = *erc;
      } else {
        error = make_error_code (std::errc::invalid_argument);
      }
    } else {
      extra = details::pct_decoded_size (str);
    }

    if (auto const pos = static_cast<pft> (field) + index; pos < needs_encoding.size ()) {
      needs_encoding.set (pos, needs_encoding.test (pos) || extra != 0);
    }
    required_size += extra;
    return str;
  });
  store.reserve (required_size);
  if (error) {
    return error;
  }

  details::parts_strings (
      result, [&store, &needs_encoding] (std::string_view const str, unsigned const index, parts_field const field) {
        assert (str.data () != nullptr && "String data field was null");
        assert ((index == 0 || field == parts_field::path) && "Unexpected non-zero index");
        assert (static_cast<std::size_t> (field) < needs_encoding.size ());

        auto const needs_encoding_pos = static_cast<pft> (field) + index;
        if (needs_encoding_pos < needs_encoding.size () && !needs_encoding.test (needs_encoding_pos)) {
          return str;
        }

        auto const original_size = store.size ();
        if (field == parts_field::host) {
          details::puny_decoded (str, std::back_inserter (store));
        } else {
          if (needs_encoding_pos >= needs_encoding.size () && details::pct_decoded_size (str) == 0) {
            return str;
          }
          assert (details::pct_decoded_size (str) > 0);
          std::ranges::copy (str | views::pctdecode, std::back_inserter (store));
        }
        return std::string_view{store.data () + original_size, store.size () - original_size};
      });
  assert (required_size == store.size () && "Expected store required size does not match actual");
  return result;
}

}  // end namespace uri

#endif  // URI_PARTS_HPP
