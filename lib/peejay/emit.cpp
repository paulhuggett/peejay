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
#include <span>

using namespace peejay;

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
  constexpr indent next () const noexcept { return indent{depth_ + 1U}; }

private:
  size_t depth_ = 0;
};

std::ostream& operator<< (std::ostream& os, indent const& i) {
  return i.write (os);
}

constexpr char to_hex (unsigned v) noexcept {
  assert (v < 0x10);
  return static_cast<char> (v + ((v < 10) ? '0' : 'A' - 10));
}

void emit_string_view (std::ostream& os, std::string_view const& str) {
  os << '"';
  auto first = std::begin (str);
  auto const last = std::end (str);
  auto pos = first;
  while ((pos = std::find_if (first, last, [] (char const c) {
            return c < ' ' || c == '"' || c == '\\';
          })) != last) {
    assert (pos >= first);
    os.write (to_address (first), std::distance (first, pos));
    os << '\\';
    switch (*pos) {
    case '"': os << '"'; break;    // quotation mark  U+0022
    case '\\': os << '\\'; break;  // reverse solidus U+005C
    case 0x08: os << 'b'; break;   // backspace       U+0008
    case 0x0C: os << 'f'; break;   // form feed       U+000C
    case 0x0A: os << 'n'; break;   // line feed       U+000A
    case 0x0D: os << 'r'; break;   // carriage return U+000D
    case 0x09: os << 't'; break;   // tab             U+0009
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
    first = pos + 1;
  }
  if (first != last) {
    assert (last > first);
    os.write (to_address (first), std::distance (first, last));
  }
  os << '"';
}

// helper type for the visitor
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <typename... Ts>
overloaded (Ts...) -> overloaded<Ts...>;

void emit (std::ostream& os, indent const i, element const& el) {
  auto const emit_object = [&os, i] (object const& obj) {
    if (obj.empty ()) {
      os << "{}";
      return;
    }
    os << "{\n";
    auto const* separator = "";
    indent const next_indent = i.next ();
    for (auto const& [key, value] : obj) {
      os << separator << next_indent << '"' << key << "\": ";
      emit (os, next_indent, value);
      separator = ",\n";
    }
    os << '\n' << i << "}";
  };
  auto const emit_array = [&os, i] (array const& arr) {
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
      overloaded{[&os] (std::string const& s) { emit_string_view (os, s); },
                 [&os] (int64_t v) { os << v; },
                 [&os] (uint64_t v) { os << v; }, [&os] (double v) { os << v; },
                 [&os] (bool b) { os << (b ? "true" : "false"); },
                 [&os] (null) { os << "null"; }, emit_array, emit_object,
                 [] (mark) { assert (false); }},
      el);
}

}  // end anonymous namespace

namespace peejay {

void emit (std::ostream& os, std::optional<element> const& root) {
  if (root) {
    emit (os, indent{}, *root);
  }
  os << '\n';
}

}  // end namespace peejay
