// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <iostream>
#include <string_view>

#include "peejay/rules/non_terminal_rule.hpp"

using namespace std::string_view_literals;
using policies = peejay::default_policies;

struct point {
  int x = 0;
  int y = 0;
};

// Reads {"x": ..., "y": ...} into a point.
class point_rule final : public peejay::rules::object_rule<policies, 2> {
public:
  using value_type = point;
  explicit point_rule(point* const pt) { this->set_properties({{u8"x"sv, &pt->x}, {u8"y"sv, &pt->y}}); }
};

int main() {
  point pt;
  peejay::rules::context<policies, point_rule> ctx{&pt};
  auto p = ctx.make_parser();
  p.input(u8R"({"x": 3, "y": -4})"sv).eof();
  if (!p.has_error()) {
    std::cout << pt.x << ", " << pt.y << '\n';  // 3, -4
  }
}
