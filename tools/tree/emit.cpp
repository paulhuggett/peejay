//===- tools/tree/emit.cpp ------------------------------------------------===//
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
#include "emit.hpp"

namespace {

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

// helper type for the visitor
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <typename... Ts>
overloaded (Ts...) -> overloaded<Ts...>;

void emit (std::ostream& os, indent const i, peejay::dom::element const& el) {
  auto const emit_object = [&] (peejay::dom::object const& obj) {
    os << "{\n";
    auto const* separator = "";
    indent const next_indent = i.next ();
    for (auto const& kvp : obj) {
      os << separator << next_indent << '"' << kvp.first << "\": ";
      emit (os, next_indent, kvp.second);
      separator = ",\n";
    }
    os << '\n' << i << "}";
  };
  auto const emit_array = [&] (peejay::dom::array const& arr) {
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
      overloaded{[&os] (std::string const& s) { os << '"' << s << '"'; },
                 [&os] (int64_t v) { os << v; },
                 [&os] (uint64_t v) { os << v; }, [&os] (double v) { os << v; },
                 [&os] (bool b) { os << (b ? "true" : "false"); },
                 [&os] (peejay::dom::null) { os << "null"; }, emit_array,
                 emit_object, [] (peejay::dom::mark) { assert (false); }},
      el);
}

}  // end anonymous namespace

void emit (std::ostream& os, peejay::dom::element const& root) {
  emit (os, indent{}, root);
  os << '\n';
}
