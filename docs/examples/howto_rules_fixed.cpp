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

// Nothing here allocates memory: each arrayvec holds its elements inside itself.
using sensor_id = peejay::arrayvec<char8_t, 8>;
using readings = peejay::arrayvec<std::int16_t, 4>;

struct sensor {
  sensor_id id;
  readings values;
};

// A string held in an arrayvec needs its own string_rule.
using id_rule = peejay::rules::string_rule<policies, sensor_id>;
// Accepts at most 4 readings, which is the capacity of the arrayvec.
using readings_rule =
    peejay::rules::homogeneous_array_rule<policies, readings, peejay::rules::integer_rule<policies, std::int16_t>, 4>;

class sensor_rule final : public peejay::rules::object_rule<policies, 2, id_rule, readings_rule> {
public:
  using value_type = sensor;

  explicit sensor_rule(sensor* const s) : object_rule{{u8"id"sv, &s->id}, {u8"values"sv, &s->values}} {}
  [[nodiscard]] char const* name() const noexcept override { return "sensor"; }
};

void show(std::u8string_view const input) {
  sensor s;
  peejay::rules::context<policies, sensor_rule> ctx{&s};
  auto p = ctx.make_parser();
  p.input(input).eof();
  if (p.has_error()) {
    std::cout << p.last_error().message() << '\n';
    return;
  }
  std::cout << std::string(s.id.begin(), s.id.end()) << ':';
  for (std::int16_t const v : s.values) {
    std::cout << ' ' << v;
  }
  std::cout << '\n';
}

}  // end anonymous namespace

int main() {
  show(u8R"({"id": "t-01", "values": [21, -4, 19]})"sv);
  show(u8R"({"id": "thermometer", "values": []})"sv);
  show(u8R"({"id": "t-01", "values": [1, 2, 3, 4, 5]})"sv);
  show(u8R"({"id": "t-01", "values": [40000]})"sv);
}
