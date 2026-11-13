// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "peejay/rules/non_terminal_rule.hpp"

namespace {

using namespace std::string_view_literals;
using policies = peejay::default_policies;

struct server {
  std::string host;
  std::uint16_t port = 0;
  // The defaults for the optional properties.
  std::uint16_t timeout = 30;
  bool verbose = false;
};

// "host" and "port" are required. "timeout" and "verbose" are optional.
class server_rule final : public peejay::rules::object_rule<policies, 2, 2> {
public:
  using value_type = server;

  explicit server_rule(server* const s)
      : object_rule{{{u8"host"sv, &s->host}, {u8"port"sv, &s->port}},
                    {{u8"timeout"sv, &s->timeout}, {u8"verbose"sv, &s->verbose}}} {}
  [[nodiscard]] char const* name() const noexcept override { return "server"; }
};

void show(std::u8string_view const input) {
  server s;
  peejay::rules::context<policies, server_rule> ctx{&s};
  auto p = ctx.make_parser();
  p.input(input).eof();
  if (p.has_error()) {
    std::cout << p.last_error().message() << '\n';
    return;
  }
  std::cout << s.host << ':' << s.port << " timeout=" << s.timeout << " verbose=" << std::boolalpha << s.verbose
            << '\n';
}

}  // end anonymous namespace

int main() {
  show(u8R"({"host": "alpha", "port": 80, "timeout": 5, "verbose": true})"sv);
  show(u8R"({"host": "beta", "port": 443})"sv);
  show(u8R"({"host": "gamma", "timeout": 5})"sv);
  show(u8R"({"host": "delta", "port": 22, "retries": 3})"sv);
}
