//===- lib/peejay/emit.cpp ------------------------------------------------===//
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
#include "peejay/emit.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <span>

namespace {

#if PEEJAY_CXX20
using std::to_address;
#else
template <typename T>
constexpr T* to_address (T* const p) noexcept {
  return p;
}
#endif  // PEEJAY_CXX20

class indent {
public:
  constexpr indent () noexcept = default;
  explicit constexpr indent (size_t const depth) noexcept : depth_{depth} {}
  std::ostream& write (std::ostream& os) const {
    static std::array<char const, 2> whitespace{{' ', ' '}};
    for (auto ctr = size_t{0}; ctr < depth_; ++ctr) {
      os.write (whitespace.data (), whitespace.size ());
    }
    return os;
  }
  [[nodiscard]] constexpr indent next () const noexcept {
    return indent{depth_ + 1U};
  }

private:
  size_t depth_ = 0;
};

std::ostream& operator<< (std::ostream& os, indent const& i) {
  return i.write (os);
}

constexpr char to_hex (unsigned v) noexcept {
  assert (v < 0x10);
  constexpr auto letter_threshold = 10;
  return static_cast<char> (
      v + ((v < letter_threshold) ? '0' : 'A' - letter_threshold));
}

std::ostream& emit_string_view (std::ostream& os,
                                peejay::u8string_view const& str) {
  os << '"';
  auto first = std::begin (
      str);  // NOLINT(llvm-qualified-auto,readability-qualified-auto)
  auto const last =
      std::end (str);  // NOLINT(llvm-qualified-auto,readability-qualified-auto)
  peejay::u8string_view::const_iterator pos;
  while ((pos = std::find_if (first, last, [] (char const c) {
            return c < ' ' || c == '"' || c == '\\';
          })) != last) {
    assert (pos >= first);
    os.write (reinterpret_cast<char const*> (to_address (first)),
              std::distance (first, pos));
    os << '\\';
    using peejay::char_set;
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
    first = pos;
    std::advance (first, 1);
  }
  if (first != last) {
    assert (last > first);
    os.write (reinterpret_cast<char const*> (to_address (first)),
              std::distance (first, last));
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

std::string convu8 (peejay::u8string const& str) {
  std::string result;
  result.reserve (str.size ());
  std::transform (std::begin (str), std::end (str), std::back_inserter (result),
                  [] (peejay::char8 c) { return static_cast<char> (c); });
  return result;
}

std::ostream& emit (std::ostream& os, indent const i,
                    peejay::element const& el) {
  auto const emit_object = [&os, i] (peejay::object const& obj) {
    if (obj.empty ()) {
      os << "{}";
      return;
    }
    os << "{\n";
    auto const* separator = "";
    indent const next_indent = i.next ();
    for (auto const& [key, value] : obj) {
      os << separator << next_indent << '"' << convu8 (key) << "\": ";
      emit (os, next_indent, value);
      separator = ",\n";
    }
    os << '\n' << i << "}";
  };
  auto const emit_array = [&os, i] (peejay::array const& arr) {
    if (arr.empty ()) {
      os << "[]";
      return;
    }
    os << "[\n";
    auto const* separator = "";
    indent const next_indent = i.next ();
    for (auto const& v : arr) {
      os << separator << next_indent;
      emit (os, next_indent, v);
      separator = ",\n";
    }
    os << '\n' << i << "]";
  };
  std::visit (
      overloaded{
          [&os] (peejay::u8string const& s) { emit_string_view (os, s); },
          [&os] (int64_t v) { os << v; }, [&os] (uint64_t v) { os << v; },
          [&os] (double v) { os << v; },
          [&os] (bool b) { os << (b ? "true" : "false"); },
          [&os] (peejay::null) { os << "null"; }, emit_array, emit_object,
          [] (peejay::mark) { assert (false); }},
      el);
  return os;
}

}  // end anonymous namespace

namespace peejay {

std::ostream& emit (std::ostream& os, std::optional<element> const& root) {
  if (root) {
    emit (os, indent{}, *root);
  }
  os << '\n';
  return os;
}

}  // end namespace peejay
