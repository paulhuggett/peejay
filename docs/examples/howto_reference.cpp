// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "peejay/null.hpp"

namespace {

// Collects every object key in the document.
class key_collector : public peejay::null<> {
public:
  std::error_code key(std::u8string_view const k) {
    // k refers to the parser's internal buffer: copy it if it must outlive this call.
    keys.emplace_back(k.begin(), k.end());
    return {};
  }
  std::vector<std::string> keys;
};

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  key_collector collector;
  // Passing an lvalue makes a parser<key_collector&>: it holds a reference to
  // collector rather than a copy.
  auto p = peejay::make_parser(collector);
  p.input(u8R"({"id": 1, "tags": {"colour": "red", "size": "L"}})"sv).eof();
  if (std::error_code const err = p.last_error()) {
    std::cerr << "error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  for (std::string const& k : collector.keys) {
    std::cout << k << '\n';
  }
  return EXIT_SUCCESS;
}
