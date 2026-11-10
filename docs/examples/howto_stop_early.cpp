// Copyright © 2026 Paul Bowen-Huggett
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "peejay/null.hpp"

namespace search {

// An error code that the backend uses to stop the parse.
enum class status { found = 1 };

class category final : public std::error_category {
public:
  char const* name() const noexcept override { return "search"; }
  std::string message(int) const override { return "found"; }
};

// Found by argument-dependent lookup when a status is converted to std::error_code.
inline std::error_code make_error_code(status const e) {
  static category const cat;
  return {static_cast<int>(e), cat};
}

}  // end namespace search

template <> struct std::is_error_code_enum<search::status> : std::true_type {};

namespace {

// Finds the value of the first top-level "version" property. The null
// backend supplies the functions that we don't need to customize.
class find_version : public peejay::null<> {
public:
  std::optional<policies::integer_type> result() const noexcept { return version_; }

  std::error_code begin_object() { return this->enter(); }
  std::error_code end_object() { return this->leave(); }
  std::error_code begin_array() { return this->enter(); }
  std::error_code end_array() { return this->leave(); }

  std::error_code key(std::u8string_view const k) {
    is_version_ = depth_ == 1 && k == u8"version";
    return {};
  }
  std::error_code integer_value(policies::integer_type const v) {
    if (is_version_) {
      version_ = v;
      return search::status::found;  // Stop parsing: we have what we need.
    }
    return {};
  }

private:
  std::error_code enter() {
    ++depth_;
    is_version_ = false;
    return {};
  }
  std::error_code leave() {
    --depth_;
    return {};
  }

  unsigned depth_ = 0;
  bool is_version_ = false;
  std::optional<policies::integer_type> version_;
};

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  peejay::parser<find_version> p;
  // Parsing stops at the "version" value, so the rest of the input isn't examined.
  auto const version = p.input(u8R"({"name": "web", "version": 3, "data": [1, 2, 3, ...)"sv).eof();
  if (std::error_code const err = p.last_error(); err && err != search::status::found) {
    std::cerr << "error: " << err.message() << '\n';
    return EXIT_FAILURE;
  }
  if (version) {
    std::cout << "version " << *version << '\n';
  }
  return EXIT_SUCCESS;
}
