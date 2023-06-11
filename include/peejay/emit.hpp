//===- include/peejay/emit.hpp ----------------------------*- mode: C++ -*-===//
//*                 _ _    *
//*   ___ _ __ ___ (_) |_  *
//*  / _ \ '_ ` _ \| | __| *
//* |  __/ | | | | | | |_  *
//*  \___|_| |_| |_|_|\__| *
//*                        *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_EMIT_HPP
#define PEEJAY_EMIT_HPP

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>

#if PEEJAY_HAVE_SPAN
#include <span>
#endif

#if PEEJAY_CXX20 && defined(__has_include) && __has_include(<bit>)
#include <bit>
#endif

#include "peejay/dom.hpp"

namespace peejay {

namespace emit_details {

// to char pointer
// ~~~~~~~~~~~~~~~
template <typename T>
constexpr char const* to_char_pointer (T&& x) noexcept {
  return pointer_cast<char const*> (
      to_address (std::forward<T> (x)));
}

// ident
// ~~~~~
class indent {
public:
  constexpr indent () noexcept = default;
  explicit constexpr indent (size_t const depth) noexcept : depth_{depth} {}

  /// Writes the indentation sequence to the output stream \p os.
  /// \param os  An output stream instance.
  /// \returns \p os.
  template <typename OStream>
  OStream& write (OStream& os) const {
    static std::array<char const, 2> whitespace{{' ', ' '}};
    for (auto ctr = size_t{0}; ctr < depth_; ++ctr) {
      os.write (whitespace.data (), whitespace.size ());
    }
    return os;
  }
  /// Returns an increased indentation instance.
  [[nodiscard]] constexpr indent next () const noexcept {
    return indent{depth_ + 1U};
  }

private:
  size_t depth_ = 0;
};

inline std::ostream& operator<< (std::ostream& os, indent const& i) {
  return i.write (os);
}

// to hex
// ~~~~~~
constexpr char to_hex (unsigned v) noexcept {
  assert (v < 0x10 && "Individual hex values must be < 0x10");
  constexpr auto letter_threshold = 10;
  return static_cast<char> (
      v + ((v < letter_threshold) ? '0' : 'A' - letter_threshold));
}

// break char
// ~~~~~~~~~~
inline u8string_view::const_iterator break_char (
    u8string_view::const_iterator first,
    u8string_view::const_iterator last) noexcept {
  return std::find_if (first, last, [] (char8 const c) {
    return c < static_cast<char8> (char_set::space) ||
           c == static_cast<char8> (char_set::quotation_mark) ||
           c == static_cast<char8> (char_set::reverse_solidus);
  });
}

// convu8
// ~~~~~~
inline std::string convu8 (u8string const& str) {
  std::string result;
  result.reserve (str.size ());
  auto const op = [] (char8 c) { return static_cast<char> (c); };
#if __cpp_lib_ranges
  std::ranges::transform (str, std::back_inserter (result), op);
#else
  std::transform (std::begin (str), std::end (str), std::back_inserter (result),
                  op);
#endif
  return result;
}

// emit object
// ~~~~~~~~~~~
template <typename OStream>
void emit_object (OStream& os, indent i, object const& obj) {
  assert (obj);
  if (obj == nullptr || obj->empty ()) {
    os << "{}";
    return;
  }
  os << "{\n";
  auto const* separator = "";
  indent const next_indent = i.next ();
  for (auto const& [key, value] : *obj) {
    os << separator << next_indent << '"' << convu8 (key) << "\": ";
    emit (os, next_indent, value);
    separator = ",\n";
  }
  os << '\n' << i << "}";
}

// emit array
// ~~~~~~~~~~
template <typename OStream>
void emit_array (OStream& os, indent i, array const& arr) {
  assert (arr);
  if (arr == nullptr || arr->empty ()) {
    os << "[]";
    return;
  }
  os << "[\n";
  auto const* separator = "";
  indent const next_indent = i.next ();
  for (auto const& v : *arr) {
    os << separator << next_indent;
    emit (os, next_indent, v);
    separator = ",\n";
  }
  os << '\n' << i << "]";
}

// emit string view
// ~~~~~~~~~~~~~~~~
template <typename OStream>
OStream& emit_string_view (OStream& os, u8string_view const& str) {
  os << '"';
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto first = std::begin (str);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = std::end (str);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto pos = first;
  while ((pos = break_char (pos, last)) != last) {
    os.write (to_char_pointer (first), std::distance (first, pos));
    os << '\\';
    switch (*pos) {
    case char_set::quotation_mark: os << '"'; break;
    case char_set::reverse_solidus: os << '\\'; break;
    case char_set::backspace: os << 'b'; break;
    case char_set::form_feed: os << 'f'; break;
    case char_set::line_feed: os << 'n'; break;
    case char_set::carriage_return: os << 'r'; break;
    case char_set::character_tabulation: os << 't'; break;
    default: {
      auto const c = static_cast<std::byte> (*pos);
      auto const high_nibble = ((c & std::byte{0xF0}) >> 4);
      auto const low_nibble = c & std::byte{0x0F};
      std::array<char, 5> hex{
          {'u', '0', '0', to_hex (std::to_integer<unsigned> (high_nibble)),
           to_hex (std::to_integer<unsigned> (low_nibble))}};
      os.write (hex.data (), hex.size ());
    } break;
    }
    std::advance (pos, 1);
    first = pos;
  }
  assert (pos == last);
  if (first != last) {
    os.write (to_char_pointer (first), std::distance (first, last));
  }
  os << '"';
  return os;
}

// Helper type for the visitor
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// Explicit deduction guide
template <typename... Ts>
overloaded (Ts...) -> overloaded<Ts...>;

// emit
// ~~~~
template <typename OStream>
OStream& emit (OStream& os, indent const i, element const& el) {
  std::visit (
      overloaded{
          [&os] (u8string const& s) { emit_string_view (os, s); },
          [&os] (int64_t v) { os << v; }, [&os] (uint64_t v) { os << v; },
          [&os] (double v) { os << v; },
          [&os] (bool b) { os << (b ? "true" : "false"); },
          [&os] (null) { os << "null"; },
          [&os, i] (array const& arr) { emit_array (os, i, arr); },
          [&os, i] (object const& obj) { emit_object (os, i, obj); },
          [] (mark) { assert (false); }},
      el);
  return os;
}

}  // namespace emit_details

template <typename OStream>
OStream& emit (OStream& os, std::optional<element> const& root) {
  if (root) {
    emit (os, emit_details::indent{}, *root);
  }
  os << '\n';
  return os;
}

}  // end namespace peejay

#endif  // PEEJAY_EMIT_HPP
