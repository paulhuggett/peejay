// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "peejay/dom.hpp"

namespace {

struct policies : peejay::default_policies {
  using integer_type = std::int64_t;
};
// Raise the DOM's limits on the number of members of an array or object.
struct dom_policies {
  static constexpr std::size_t max_array_size = 10'000;
  static constexpr std::size_t max_object_size = 1'000;
};

using dom_backend = peejay::dom::dom<policies, dom_policies>;
using element = dom_backend::element;

std::string to_string(std::u8string_view const s) {
  return {s.begin(), s.end()};
}

// Writes an indented description of an element and its children.
void dump(element const& el, unsigned const indent = 0) {
  std::cout << std::string(indent * 2U, ' ');
  if (el.holds<element::null>()) {
    std::cout << "null\n";
  } else if (auto const* const b = el.get_if<element::boolean>()) {
    std::cout << "boolean " << std::boolalpha << *b << '\n';
  } else if (auto const* const i = el.get_if<element::integer_type>()) {
    std::cout << "integer " << *i << '\n';
  } else if (auto const* const f = el.get_if<element::float_type>()) {
    std::cout << "float " << *f << '\n';
  } else if (auto const* const s = el.get_if<element::string>()) {
    std::cout << "string \"" << to_string(*s) << "\"\n";
  } else if (auto const* const arr = el.get_if<element::array>()) {
    std::cout << "array (" << arr->size() << " members)\n";
    for (element const& member : *arr) {
      dump(member, indent + 1U);
    }
  } else if (auto const* const obj = el.get_if<element::object>()) {
    std::cout << "object (" << obj->size() << " members)\n";
    // Objects are unordered maps: member order is not preserved.
    for (auto const& [key, value] : *obj) {
      std::cout << std::string((indent + 1U) * 2U, ' ') << '"' << to_string(key) << "\":\n";
      dump(value, indent + 2U);
    }
  }
}

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  peejay::parser<dom_backend> p;
  auto const root = p.input(u8R"({"id": 7, "tags": ["x", null], "ratio": 0.5})"sv).eof();
  if (std::error_code const err = p.last_error()) {
    std::cerr << "error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  dump(*root);
  return EXIT_SUCCESS;
}
