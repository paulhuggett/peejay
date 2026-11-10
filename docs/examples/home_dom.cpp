// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <iostream>
#include <string_view>

#include "peejay/dom.hpp"

using namespace std::string_view_literals;
using dom_backend = peejay::dom::dom<>;
using element = dom_backend::element;

int main() {
  peejay::parser<dom_backend> p;
  auto const root = p.input(u8R"({"name": "web", "port": 8080})"sv).eof();
  if (!p.has_error()) {
    if (auto const port = root->get_object_element<element::integer_type>(u8"port")) {
      std::cout << port->get() << '\n';  // 8080
    }
  }
}
