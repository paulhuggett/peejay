//===- include/uri/pctdecode.hpp --------------------------*- mode: C++ -*-===//
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
#ifndef URI_PCTDECODE_HPP
#define URI_PCTDECODE_HPP

#include <algorithm>
#include <cassert>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <version>

#if !defined(__cpp_lib_ranges)
#error "Need __cpp_lib_ranges to be available"
#elif __cpp_lib_ranges < 201911L
#error "Need __cpp_lib_ranges to __cpp_lib_ranges >= 201911L"
#endif

namespace uri {

namespace details {

inline constexpr std::byte bad = std::byte{0b1'0000};

/// Convert the argument character from a hexadecimal character code
/// (A-F/a-f/0-9) to an integer in the range 0-15. If the input character is
/// not a valid hex character code, returns uri::bad.
template <std::integral ValueT>
constexpr std::byte hex2dec (ValueT const digit) noexcept {
  if (digit >= 'a' && digit <= 'f') {
    return static_cast<std::byte> (static_cast<unsigned> (digit) - ('a' - 10));
  }
  if (digit >= 'A' && digit <= 'F') {
    return static_cast<std::byte> (static_cast<unsigned> (digit) - ('A' - 10));
  }
  if (digit >= '0' && digit <= '9') {
    return static_cast<std::byte> (static_cast<unsigned> (digit) - '0');
  }
  return bad;
}
constexpr bool either_bad (std::byte n1, std::byte n2) noexcept {
  return ((n1 | n2) & bad) != std::byte{0};
}

template <typename Iterator>
Iterator increment (Iterator pos, Iterator end) {
  auto remaining = std::distance (pos, end);
  assert (remaining > 0);
  // Remove 1 character unless we've got a '%' followed by two legal hex
  // characters in which case we remove 3.
  std::advance (pos,
                remaining >= 3 && *pos == '%' &&
                    !either_bad (hex2dec (*(pos + 1)), hex2dec (*(pos + 2)))
                  ? 3
                  : 1);
  return pos;
}

template <typename ReferenceType, typename Iterator, typename ValueType>
ReferenceType deref (Iterator pos, Iterator end, ValueType* const hex) {
  ReferenceType c = *pos;
  if (c != '%' || std::distance (pos, end) < 3) {
    return c;
  }
  auto const nhi = hex2dec (*(pos + 1));
  auto const nlo = hex2dec (*(pos + 2));
  // If either character isn't valid hex, then return the original.
  if (either_bad (nhi, nlo)) {
    return c;
  }
  *hex = static_cast<ValueType> ((nhi << 4) | nlo);
  return *hex;
}

}  // end namespace details

template <std::input_iterator InputIterator>
bool needs_pctdecode (InputIterator first, InputIterator last) {
  return std::find_if (first, last, [] (auto c) { return c == '%'; }) != last;
}

template <std::ranges::input_range View>
  requires std::ranges::forward_range<View>
class pctdecode_view
    : public std::ranges::view_interface<pctdecode_view<View>> {
public:
  class iterator;
  class sentinel;

  pctdecode_view ()
    requires std::default_initializable<View>
  = default;
  constexpr explicit pctdecode_view (View base) : base_{std::move (base)} {}

  template <typename Vp = View>
  constexpr View base () const&
    requires std::copy_constructible<Vp>
  {
    return base_;
  }
  constexpr View base () && { return std::move (base_); }

  constexpr auto begin () const {
    return iterator{*this, std::ranges::begin (base_)};
  }
  constexpr auto end () const {
    if constexpr (std::ranges::common_range<View>) {
      return iterator{*this, std::ranges::end (base_)};
    } else {
      return sentinel{*this};
    }
  }

private:
  [[no_unique_address]] View base_ = View{};
};

template <typename Range>
pctdecode_view (Range&&) -> pctdecode_view<std::views::all_t<Range>>;

template <std::ranges::input_range View>
  requires std::ranges::forward_range<View>
class pctdecode_view<View>::iterator {
public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::ranges::range_value_t<View>;
  using difference_type = std::ranges::range_difference_t<View>;

  iterator ()
    requires std::default_initializable<std::ranges::iterator_t<View>>
  = default;

  constexpr iterator (pctdecode_view const& parent,
                      std::ranges::iterator_t<View> current)
      : parent_{std::addressof (parent)}, pos_{std::move (current)} {}

  constexpr std::ranges::iterator_t<View> const& base () const& noexcept {
    return pos_;
  }
  constexpr std::ranges::iterator_t<View> base () && {
    return std::move (pos_);
  }

  constexpr std::ranges::range_reference_t<View> operator* () const {
    return details::deref<std::ranges::range_reference_t<View>> (
      pos_, std::ranges::end (parent_->base_), &hex_);
  }
  constexpr std::ranges::iterator_t<View> operator->() const
    requires std::copyable<std::ranges::iterator_t<View>>
  {
    return &(**this);
  }

  constexpr iterator& operator++ () {
    pos_ = details::increment (pos_, std::ranges::end (parent_->base_));
    return *this;
  }

  constexpr iterator operator++ (int) {
    auto old = *this;
    ++(*this);
    return old;
  }

  friend constexpr bool operator== (iterator const& x, iterator const& y)
    requires std::equality_comparable<std::ranges::iterator_t<View>>
  {
    return x.pos_ == y.pos_;
  }

  friend constexpr std::ranges::range_rvalue_reference_t<View> iter_move (
    iterator const& it) noexcept (noexcept (std::ranges::iter_move (it.pos_))) {
    return std::ranges::iter_move (it.pos_);
  }

  friend constexpr void iter_swap (
    iterator const& x,
    iterator const& y) noexcept (noexcept (std::ranges::iter_swap (x.pos_,
                                                                   y.pos_)))
    requires std::indirectly_swappable<std::ranges::iterator_t<View>>
  {
    return std::ranges::iter_swap (x.pos_, y.pos_);
  }

private:
  [[no_unique_address]] pctdecode_view const* parent_ = nullptr;
  [[no_unique_address]] std::ranges::iterator_t<View> pos_ =
    std::ranges::iterator_t<View> ();
  mutable std::ranges::range_value_t<View> hex_ = 0;
};

template <std::ranges::input_range View>
  requires std::ranges::forward_range<View>
class pctdecode_view<View>::sentinel {
public:
  sentinel () = default;
  constexpr explicit sentinel (pctdecode_view& parent)
      : end_{std::ranges::end (parent.base_)} {}

  constexpr std::ranges::sentinel_t<View> base () const { return end_; }
  friend constexpr bool operator== (iterator const& x, sentinel const& y) {
    return x.current_ == y.end_;
  }

private:
  std::ranges::sentinel_t<View> end_{};
};

namespace details {

struct pctdecode_range_adaptor {
  template <std::ranges::viewable_range Range>
  constexpr auto operator() (Range&& r) const {
    return pctdecode_view{std::forward<Range> (r)};
  }
};

template <std::ranges::viewable_range Range>
constexpr auto operator| (Range&& r, pctdecode_range_adaptor const& adaptor) {
  return adaptor (std::forward<Range> (r));
}

}  // end namespace details

namespace views {
inline constexpr auto pctdecode = details::pctdecode_range_adaptor{};

// inline constexpr auto pctdecode_lower = uri::views::pctdecode |
// std::views::transform([] (auto c) { return std::tolower (c); });
}  // end namespace views

/// pctdecode_iterator is a forward-iterator which will produce characters from
/// a string-view instance. Each time that it encounters a percent character "%"
/// followed by two hexadecimal digits, the hexadecimal value is decoded. For
/// example, "%20" is the percent-encoding for character 32 which in US-ASCII
/// corresponds to the space character (SP). The uppercase hexadecimal digits
/// 'A' through 'F' are equivalent to the lowercase digits 'a' through 'f',
/// respectively.
///
/// If the two characters following the percent character are _not_ valid
/// hexadecimal digits, the text is left unchanged.
template <typename Iterator>
class pctdecode_iterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type const*;
  using reference = value_type const&;

  constexpr pctdecode_iterator () noexcept = default;
  constexpr pctdecode_iterator (Iterator first, Iterator last)
      : pos_{first}, end_{last} {}

  constexpr bool operator== (pctdecode_iterator const& other) const noexcept {
    assert (end_ == other.end_ &&
            "Comparing iterators that refer to different inputs");
    return pos_ == other.pos_ && end_ == other.end_;
  }

  reference operator* () const {
    return details::deref<reference> (pos_, end_, &hex_);
  }
  pointer operator->() const { return &(**this); }

  pctdecode_iterator& operator++ () {
    pos_ = details::increment (pos_, end_);
    return *this;
  }
  pctdecode_iterator operator++ (int) {
    auto const prev = *this;
    ++(*this);
    return prev;
  }

private:
  Iterator pos_{};
  Iterator end_{};
  mutable value_type hex_ = 0;
};

template <typename Iterator>
pctdecode_iterator (Iterator, Iterator) -> pctdecode_iterator<Iterator>;

template <typename Container>
constexpr auto pctdecode_begin (Container const& c) noexcept {
  return pctdecode_iterator{std::begin (c), std::end (c)};
}
template <typename Container>
constexpr auto pctdecode_end (Container const& c) noexcept {
  return pctdecode_iterator{std::end (c), std::end (c)};
}

namespace details {
template <typename T>
constexpr auto has_begin_end (int)
  -> decltype (begin (std::declval<T&> ()), end (std::declval<T&> ()),
               std::true_type{});

template <typename T>
constexpr auto has_begin_end (...) -> std::false_type;

template <typename T>
inline constexpr bool has_begin_end_v = decltype (has_begin_end<T> (0))::value;
}  // namespace details

template <typename Iterator>
class pctdecoder {
public:
  template <typename Container>
    requires details::has_begin_end_v<Container> && (std::is_same_v<Iterator, typename Container::iterator> ||
                                                     std::is_same_v<Iterator, typename Container::const_iterator>)
  explicit constexpr pctdecoder (Container const& c) noexcept : begin_{pctdecode_begin (c)}, end_{pctdecode_end (c)} {}
  constexpr pctdecoder (Iterator begin, Iterator end) noexcept
      : begin_{pctdecode_iterator (begin, end)},
        end_{pctdecode_iterator (end, end)} {}

  constexpr auto begin () const { return begin_; }
  constexpr auto end () const { return end_; }

private:
  pctdecode_iterator<Iterator> begin_;
  pctdecode_iterator<Iterator> end_;
};

template <typename Container>
pctdecoder (Container) -> pctdecoder<typename Container::const_iterator>;

inline std::string pctdecode (std::string_view s) {
  std::string result;
  result.reserve (s.length ());
  for (auto const& c : pctdecoder{s}) {
    result += c;
  }
  return result;
}

}  // end namespace uri

#endif  // URI_PCTDECODE_HPP
