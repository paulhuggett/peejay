//===- include/peejay/schema/schema.hpp -------------------*- mode: C++ -*-===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_SCHEMA_HPP
#define PEEJAY_SCHEMA_HPP

#include <functional>
#include <regex>

#include "peejay/json/dom.hpp"

namespace peejay::schema {

template <typename T> constexpr bool is_type(element const &el) {
  return std::holds_alternative<T>(el);
}

constexpr bool is_number(element const &el) {
  return is_type<std::int64_t>(el) || is_type<double>(el);
}
constexpr bool is_integer(element const &el) {
  if (is_type<std::int64_t>(el)) {
    return true;
  }
  if (auto const *const d = std::get_if<double>(&el)) {
    return *d == std::rint(*d);
  }
  return false;
}

class checker {
public:
  static std::error_code check_type(u8string const &type_name, element const &instance);
  static std::error_code check_type(element const &type_name, element const &instance);

  static std::error_code string_constraints(object const &schema, u8string const &s);
  static std::error_code object_constraints(object const &schema, object const &obj);
  static std::error_code check(element const &schema, element const &instance);

  std::error_code root(element const &schema, element const &instance);

private:
  u8string base_uri_;
  element const *root_ = nullptr;
  std::optional<object const *> defs_;
};

inline std::error_code check(element const &schema, element const &instance) {
  return checker{}.root(schema, instance);
}

}  // end namespace peejay::schema

#endif  // PEEJAY_SCHEMA_HPP
