// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <iostream>
#include <string>
#include <string_view>

#include "peejay/rules/non_terminal_rule.hpp"

namespace {

using namespace std::string_view_literals;
using policies = peejay::default_policies;

enum class level { debug, info, warning, error };

// Accepts one of four strings and stores the matching level.
class level_rule final : public peejay::rules::enum_rule<policies, level, 4> {
public:
  explicit level_rule(level* const v)
      : enum_rule{v,
                  {
                      {u8"debug"sv, level::debug},
                      {u8"info"sv, level::info},
                      {u8"warning"sv, level::warning},
                      {u8"error"sv, level::error},
                  }} {}
  [[nodiscard]] char const* name() const noexcept override { return "level"; }
};

struct logger {
  std::string name;
  level threshold = level::info;
};

// The rule for a property of type level is listed after the number of properties.
class logger_rule final : public peejay::rules::object_rule<policies, 2, level_rule> {
public:
  using value_type = logger;

  explicit logger_rule(logger* const l) : object_rule{{u8"name"sv, &l->name}, {u8"level"sv, &l->threshold}} {}
  [[nodiscard]] char const* name() const noexcept override { return "logger"; }
};

void show(std::u8string_view const input) {
  logger l;
  peejay::rules::context<policies, logger_rule> ctx{&l};
  auto p = ctx.make_parser();
  p.input(input).eof();
  if (p.has_error()) {
    std::cout << p.last_error().message() << '\n';
    return;
  }
  std::cout << l.name << ": level " << static_cast<int>(l.threshold) << '\n';
}

}  // end anonymous namespace

int main() {
  show(u8R"({"name": "net", "level": "warning"})"sv);
  show(u8R"({"name": "net", "level": "loud"})"sv);
}
