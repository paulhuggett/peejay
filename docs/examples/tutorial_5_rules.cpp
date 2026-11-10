// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "peejay/rules/non_terminal_rule.hpp"

namespace {

using namespace std::string_view_literals;
using policies = peejay::default_policies;

// The values that the program needs from the document.
struct config {
  std::string name;
  std::uint16_t port = 0;
  bool debug = false;
  std::vector<std::string> hosts;
};

// Parses an array of strings into a std::vector<std::string>.
using hosts_rule = peejay::rules::homogeneous_array_rule<policies, std::vector<std::string>>;

// Parses an object with exactly the four properties of a config.
class config_rule final : public peejay::rules::object_rule<policies, 4, hosts_rule> {
public:
  using value_type = config;

  explicit config_rule(config* const c) {
    this->set_properties({
        {u8"name"sv, &c->name},
        {u8"port"sv, &c->port},
        {u8"debug"sv, &c->debug},
        {u8"hosts"sv, &c->hosts},
    });
  }
  [[nodiscard]] char const* name() const noexcept override { return "config"; }
};

bool parse(std::u8string_view const input, config& c) {
  peejay::rules::context<policies, config_rule> ctx{&c};
  auto p = ctx.make_parser();
  p.input(input).eof();
  if (std::error_code const err = p.last_error()) {
    std::cout << "error: " << err.message() << '\n';
    return false;
  }
  return true;
}

}  // end anonymous namespace

int main() {
  auto const input = u8R"({
    "name": "web",
    "port": 8080,
    "debug": false,
    "hosts": ["alpha", "beta"]
  })"sv;

  config c;
  if (!parse(input, c)) {
    return EXIT_FAILURE;
  }
  std::cout << "name: " << c.name << '\n';
  std::cout << "port: " << c.port << '\n';
  std::cout << "debug: " << std::boolalpha << c.debug << '\n';
  for (std::string const& host : c.hosts) {
    std::cout << "host: " << host << '\n';
  }

  // Each of these documents is rejected.
  config bad;
  parse(u8R"({"name": "web", "port": 8080, "debug": false})"sv, bad);
  parse(u8R"({"name": "web", "port": 80000, "debug": false, "hosts": []})"sv, bad);
  parse(u8R"({"name": "web", "port": 8080, "debug": "no", "hosts": []})"sv, bad);
  parse(u8R"({"name": "web", "colour": "red"})"sv, bad);
  return EXIT_SUCCESS;
}
