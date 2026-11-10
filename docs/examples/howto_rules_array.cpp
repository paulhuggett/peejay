// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "peejay/rules/non_terminal_rule.hpp"

namespace {

using namespace std::string_view_literals;
using policies = peejay::default_policies;

struct address {
  std::string name;
  std::uint16_t port = 0;
};

// Parses one {"name": ..., "port": ...} object.
class address_rule final : public peejay::rules::object_rule<policies, 2> {
public:
  using value_type = address;

  explicit address_rule(address* const a) { this->set_properties({{u8"name"sv, &a->name}, {u8"port"sv, &a->port}}); }
  [[nodiscard]] char const* name() const noexcept override { return "address"; }
};

// Parses an array of at most three addresses. address_rule parses each member.
using addresses_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<address>, address_rule, 3>;

void show(std::u8string_view const input) {
  std::vector<address> addresses;
  peejay::rules::context<policies, addresses_rule> ctx{&addresses};
  auto p = ctx.make_parser();
  p.input(input).eof();

  std::cout << (p.has_error() ? p.last_error().message() : "OK") << ':';
  for (address const& a : addresses) {
    std::cout << ' ' << a.name << '=' << a.port;
  }
  std::cout << '\n';
}

}  // end anonymous namespace

int main() {
  show(u8R"([{"name": "http", "port": 80}, {"name": "ssh", "port": 22}])"sv);
  show(u8R"([])"sv);
  show(u8R"([{"name": "a", "port": 1}, {"name": "b", "port": 2}, {"name": "c", "port": 3}, {"name": "d"}])"sv);
  show(u8R"([{"name": "http", "port": 80}, {"name": "ssh"}])"sv);
}
