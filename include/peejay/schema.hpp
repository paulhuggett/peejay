//===- include/peejay/schema.hpp --------------------------*- mode: C++ -*-===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_SCHEMA_HPP
#define PEEJAY_SCHEMA_HPP

#include <functional>

#include "peejay/dom.hpp"

namespace peejay {

template <typename T>
using error_or = std::variant<std::error_code, T>;

namespace schema {

template <typename T>
constexpr bool is_type (element const &el) {
  return std::holds_alternative<T> (el);
}

constexpr bool is_number (element const &el) {
  return is_type<std::int64_t> (el) || is_type<double> (el);
}
constexpr bool is_integer (element const &el) {
  if (is_type<std::int64_t> (el)) {
    return true;
  }
  if (auto const *const d = std::get_if<double> (&el)) {
    return *d == std::rint (*d);
  }
  return false;
}
template <typename InputIterator, typename Predicate>
inline error_or<bool> any (InputIterator first, InputIterator last,
                           Predicate pred) {
  for (; first != last; ++first) {
    auto r = pred (*first);
    if (r.valueless_by_exception () ||
        std::holds_alternative<std::error_code> (r)) {
      return r;
    }
    assert (std::holds_alternative<bool> (r));
    if (auto const *const b = std::get_if<bool> (&r); b != nullptr && *b) {
      return r;
    }
  }
  return false;
}
class checker {
public:
  static error_or<bool> check_type (u8string const &type_name,
                                    element const &instance) {
    static std::unordered_map<u8string,
                              std::function<bool (element const &)>> const map{
        {u8"array", is_type<array>},     {u8"boolean", is_type<bool>},
        {u8"integer", is_integer},       {u8"null", is_type<null>},
        {u8"number", is_number},         {u8"object", is_type<object>},
        {u8"string", is_type<u8string>},
    };
    if (auto const pos = map.find (type_name); pos != map.end ()) {
      return pos->second (instance);
    }
    return make_error_code (error::schema_type_name_invalid);
  }
  static error_or<bool> check_type (element const &type_name,
                                    element const &instance) {
    if (auto const *name = std::get_if<u8string> (&type_name)) {
      return check_type (*name, instance);
    }
    return make_error_code (error::schema_type_name_invalid);
  }

  static error_or<bool> check (element const &schema, element const &instance) {
    // A schema or a sub-schema may be either an object or a boolean.
    if (auto const *const b = std::get_if<bool> (&schema)) {
      return *b;
    }
    auto const *const obj = std::get_if<object> (&schema);
    if (obj == nullptr) {
      return make_error_code (error::schema_not_boolean_or_object);
    }
    auto const &map = **obj;
    auto const end = map.end ();

    if (auto const &const_pos = map.find (u8"const"); const_pos != end) {
      if (instance != const_pos->second) {
        return false;
      }
    }
    if (auto const &enum_pos = map.find (u8"enum"); enum_pos != end) {
      if (auto const *const arr = std::get_if<array> (&enum_pos->second)) {
        return any (std::begin (**arr), std::end (**arr),
                    [&instance] (element const &el) -> error_or<bool> {
                      return el == instance;
                    });
      }
      return error::schema_enum_must_be_array;
    }
    if (auto const &type_pos = map.find (u8"type"); type_pos != end) {
      if (auto const *const name = std::get_if<u8string> (&type_pos->second)) {
        return check_type (*name, instance);
      }
      if (auto const *const name_array =
              std::get_if<array> (&type_pos->second)) {
        return any (std::begin (**name_array), std::end (**name_array),
                    [&instance] (element const &type_name) {
                      return check_type (type_name, instance);
                    });
      }
      return error::schema_type_string_or_string_array;
    }

    if (auto const &properties_pos = map.find (u8"properties");
        properties_pos != end) {
      if (auto const *const properties =
              std::get_if<object> (&properties_pos->second)) {
        // TODO(paul): implement this!
        // auto const &pmap = **properties;
      } else {
        // error: properties MUST be an object.
      }
    }
    if (map.find (u8"patternProperties") != end) {
    }
    if (map.find (u8"additionalProperties") != end) {
    }
    if (map.find (u8"propertyNames") != end) {
    }
    return true;
  }
};

inline error_or<bool> check (element const &schema, element const &instance) {
  return checker::check (schema, instance);
}

}  // end namespace schema

}  // end namespace peejay

#endif  // PEEJAY_SCHEMA_HPP
