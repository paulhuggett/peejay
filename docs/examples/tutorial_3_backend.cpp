// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <system_error>

#include "peejay/json.hpp"

namespace {

struct statistics {
  std::size_t strings = 0;
  std::size_t numbers = 0;
  std::size_t keys = 0;
  std::size_t max_depth = 0;
};

// A backend which gathers statistics about a JSON document.
class stats_backend {
public:
  // The compile-time configuration of the parser.
  using policies = peejay::default_policies;

  // Called by parser<>::eof(). Its return value is returned by eof().
  statistics result() const noexcept { return stats_; }

  std::error_code string_value(std::u8string_view) noexcept {
    ++stats_.strings;
    return {};
  }
  std::error_code integer_value(policies::integer_type) noexcept {
    ++stats_.numbers;
    return {};
  }
  std::error_code float_value(policies::float_type) noexcept {
    ++stats_.numbers;
    return {};
  }
  std::error_code boolean_value(bool) const noexcept { return {}; }
  std::error_code null_value() const noexcept { return {}; }

  std::error_code begin_array() noexcept { return this->enter(); }
  std::error_code end_array() noexcept { return this->leave(); }

  std::error_code begin_object() noexcept { return this->enter(); }
  std::error_code key(std::u8string_view) noexcept {
    ++stats_.keys;
    return {};
  }
  std::error_code end_object() noexcept { return this->leave(); }

private:
  std::error_code enter() noexcept {
    ++depth_;
    stats_.max_depth = std::max(stats_.max_depth, depth_);
    return {};
  }
  std::error_code leave() noexcept {
    --depth_;
    return {};
  }

  std::size_t depth_ = 0;
  statistics stats_;
};

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  peejay::parser<stats_backend> p;
  statistics const stats = p.input(u8R"({"a": [1, 2.5, "x"], "b": {"c": "y"}})"sv).eof();
  if (std::error_code const err = p.last_error()) {
    std::cerr << "error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "strings: " << stats.strings << '\n'
            << "numbers: " << stats.numbers << '\n'
            << "keys: " << stats.keys << '\n'
            << "max depth: " << stats.max_depth << '\n';
  return EXIT_SUCCESS;
}
