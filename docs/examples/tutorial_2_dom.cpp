// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "peejay/dom.hpp"

namespace {

using dom_backend = peejay::dom::dom<>;
using element = dom_backend::element;

// peejay strings are UTF-8 (char8_t). Copy them into a std::string for printing.
std::string to_string(std::u8string_view const s) {
  return {s.begin(), s.end()};
}

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  auto const config = u8R"({
    "name": "web",
    "port": 8080,
    "debug": false,
    "hosts": ["alpha", "beta"]
  })"sv;

  peejay::parser<dom_backend> p;
  // eof() returns the backend's result: for the DOM backend, the root element.
  std::optional<element> root = p.input(config).eof();
  if (std::error_code const err = p.last_error()) {
    std::cerr << "error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }

  if (auto const name = root->get_object_element<element::string>(u8"name")) {
    std::cout << "name: " << to_string(name->get()) << '\n';
  }
  if (auto const port = root->get_object_element<element::integer_type>(u8"port")) {
    std::cout << "port: " << port->get() << '\n';
  }
  if (auto const debug = root->get_object_element<element::boolean>(u8"debug")) {
    std::cout << "debug: " << std::boolalpha << debug->get() << '\n';
  }
  if (auto const hosts = root->get_object_element<element::array>(u8"hosts")) {
    for (element const& host : hosts->get()) {
      if (auto const* const s = host.get_if<element::string>()) {
        std::cout << "host: " << to_string(*s) << '\n';
      }
    }
  }
  return EXIT_SUCCESS;
}
