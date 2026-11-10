// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include "peejay/rules/non_terminal_rule.hpp"

namespace {

using namespace std::string_view_literals;
using policies = peejay::default_policies;
using context_base = peejay::rules::context_base<policies>;

// Accepts an integer or null.
class optional_int_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = std::optional<int>;

  explicit optional_int_rule(value_type* const v) noexcept : value_{v} {}
  [[nodiscard]] char const* name() const noexcept override { return "optional-int"; }

  std::error_code integer_value(context_base* const ctx, int const v) override {
    *value_ = v;
    return this->pop(ctx);  // The value is complete.
  }
  std::error_code null_value(context_base* const ctx) override {
    value_->reset();
    return this->pop(ctx);
  }

private:
  value_type* value_;
};

struct point {
  double x = 0.0;
  double y = 0.0;
};

// Accepts an array of exactly two numbers: [x, y].
class point_rule final : public peejay::rules::rule<policies> {
public:
  using value_type = point;
  // The rules that this rule pushes.
  using children = peejay::type_list::type_list<peejay::rules::float_rule<policies>>;

  explicit point_rule(point* const p) noexcept : point_{p} {}
  [[nodiscard]] char const* name() const noexcept override { return "point"; }

  std::error_code begin_array(context_base* const) override { return {}; }
  // Called before each member of the array.
  std::error_code element(context_base* const ctx) override {
    switch (count_++) {
    case 0: return push<peejay::rules::float_rule<policies>>(this, ctx, &point_->x);
    case 1: return push<peejay::rules::float_rule<policies>>(this, ctx, &point_->y);
    default: return peejay::rules::error::too_many_array_elements;
    }
  }
  std::error_code end_array(context_base* const ctx) override {
    if (count_ != 2) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    return this->pop(ctx);
  }

private:
  point* point_;
  unsigned count_ = 0;
};

struct marker {
  std::string label;
  point position;
  std::optional<int> layer;
};

class marker_rule final : public peejay::rules::object_rule<policies, 3, point_rule, optional_int_rule> {
public:
  using value_type = marker;

  explicit marker_rule(marker* const m) {
    this->set_properties({{u8"label"sv, &m->label}, {u8"position"sv, &m->position}, {u8"layer"sv, &m->layer}});
  }
  [[nodiscard]] char const* name() const noexcept override { return "marker"; }
};

void show(std::u8string_view const input) {
  marker m;
  peejay::rules::context<policies, marker_rule> ctx{&m};
  auto p = ctx.make_parser();
  p.input(input).eof();
  if (p.has_error()) {
    std::cout << p.last_error().message() << '\n';
    return;
  }
  std::cout << m.label << " at (" << m.position.x << ", " << m.position.y << ") layer "
            << (m.layer.has_value() ? std::to_string(*m.layer) : "none") << '\n';
}

}  // end anonymous namespace

int main() {
  show(u8R"({"label": "home", "position": [1.5, -2], "layer": 3})"sv);
  show(u8R"({"label": "work", "position": [0, 4], "layer": null})"sv);
  show(u8R"({"label": "x", "position": [1, 2, 3], "layer": null})"sv);
  show(u8R"({"label": "x", "position": [1], "layer": null})"sv);
}
