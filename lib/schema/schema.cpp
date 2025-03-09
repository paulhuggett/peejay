//===- lib/schema/schema.cpp ----------------------------------------------===//
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
#include "peejay/schema/schema.hpp"

#include "peejay/schema/error.hpp"

namespace {

std::error_code bool_to_error(bool b) {
  return b ? peejay::schema::error::none : peejay::schema::error::validation;
}

constexpr bool is_multiple_of(int64_t a, int64_t mo) noexcept {
  return a % mo == 0;
}
bool is_multiple_of(double a, double mo) noexcept {
  auto const t = a / mo;
  return t == std::floor(t);
}
bool is_multiple_of(double a, int64_t mo) noexcept {
  return is_multiple_of(a, static_cast<double>(mo));
}
bool is_multiple_of(int64_t a, double mo) noexcept {
  return is_multiple_of(static_cast<double>(a), mo);
}

template <typename T>
concept numeric = std::integral<T> || std::floating_point<T>;

template <numeric T1, numeric T2>
using math_type =
    std::conditional_t<std::is_floating_point_v<T1> || std::is_floating_point_v<T2>, double, std::int64_t>;

template <std::invocable<double> Predicate> std::error_code check_number(peejay::element const &el, Predicate pred) {
  if (auto const *const integer = std::get_if<std::int64_t>(&el)) {
    return bool_to_error(pred(*integer));
  }
  if (auto const *const fp = std::get_if<double>(&el)) {
    return bool_to_error(pred(*fp));
  }
  return peejay::schema::error::expected_number;
}

template <numeric NumberType> std::error_code number_constraints(peejay::object const &schema, NumberType const num) {
  using namespace peejay;
  auto const end = schema->end();

  // The value of "multipleOf" MUST be a number, strictly greater than 0. A
  // numeric instance is valid only if division by this keyword's value
  // results in an integer.
  if (auto const multipleof_pos = schema->find(u8"multipleOf"); multipleof_pos != end) {
    if (std::error_code const erc =
            check_number(multipleof_pos->second, [num](auto const &v) { return is_multiple_of(num, v); })) {
      return erc;
    }
  }

  // The value of "maximum" MUST be a number, representing an inclusive upper
  // limit for a numeric instance. If the instance is a number, then this
  // keyword validates only if the instance is less than or exactly equal to
  // "maximum".
  if (auto const maximum_pos = schema->find(u8"maximum"); maximum_pos != end) {
    if (std::error_code const erc = check_number(maximum_pos->second, [num](auto const &v) {
          using target_type = math_type<NumberType, std::decay_t<decltype(v)>>;
          return static_cast<target_type>(num) <= static_cast<target_type>(v);
        })) {
      return erc;
    }
  }

  // The value of "exclusiveMaximum" MUST be a number, representing an
  // exclusive upper limit for a numeric instance. If the instance is a
  // number, then the instance is valid only if it has a value strictly less
  // than (not equal to) "exclusiveMaximum".
  if (auto const exmax_pos = schema->find(u8"exclusiveMaximum"); exmax_pos != end) {
    if (std::error_code const erc = check_number(exmax_pos->second, [num](auto const &v) {
          using target_type = math_type<NumberType, std::decay_t<decltype(v)>>;
          return static_cast<target_type>(num) < static_cast<target_type>(v);
        })) {
      return erc;
    }
  }

  // The value of "minimum" MUST be a number, representing an inclusive lower
  // limit for a numeric instance. If the instance is a number, then this
  // keyword validates only if the instance is greater than or exactly equal
  // to "minimum".
  if (auto const minimum_pos = schema->find(u8"minimum"); minimum_pos != end) {
    if (auto const erc = check_number(minimum_pos->second, [num](auto const &v) {
          using target_type = math_type<NumberType, std::decay_t<decltype(v)>>;
          return static_cast<target_type>(num) >= static_cast<target_type>(v);
        })) {
      return erc;
    }
  }

  // The value of "exclusiveMinimum" MUST be a number, representing an
  // exclusive lower limit for a numeric instance. If the instance is a
  // number, then the instance is valid only if it has a value strictly
  // greater than (not equal to) "exclusiveMinimum".
  if (auto const exmin_pos = schema->find(u8"exclusiveMinimum"); exmin_pos != end) {
    if (auto const erc = check_number(exmin_pos->second, [num](auto const &v) {
          using target_type = math_type<NumberType, std::decay_t<decltype(v)>>;
          return static_cast<target_type>(num) > static_cast<target_type>(v);
        })) {
      return erc;
    }
  }

  return {};
}

}  // namespace
namespace peejay {

std::error_code schema::checker::check_type(peejay::u8string const &type_name, peejay::element const &instance) {
  static std::unordered_map<peejay::u8string, std::function<bool(peejay::element const &)>> const map{
      {u8"array", peejay::schema::is_type<peejay::array>},
      {u8"boolean", peejay::schema::is_type<bool>},
      {u8"integer", peejay::schema::is_integer},
      {u8"null", peejay::schema::is_type<peejay::null>},
      {u8"number", peejay::schema::is_number},
      {u8"object", peejay::schema::is_type<peejay::object>},
      {u8"string", peejay::schema::is_type<peejay::u8string>},
  };
  if (auto const pos = map.find(type_name); pos != map.end()) {
    return bool_to_error(pos->second(instance));
  }
  return peejay::schema::error::type_name_invalid;
}

std::error_code schema::checker::check_type(element const &type_name, element const &instance) {
  if (auto const *const name = std::get_if<u8string>(&type_name)) {
    return check_type(*name, instance);
  }
  return error::type_name_invalid;
}

template <std::invocable<std::uint64_t> Predicate>
static std::error_code non_negative_constraint(object const &schema, u8string const &name, Predicate predicate) {
  auto const pos = schema->find(name);
  if (pos == schema->end()) {
    return {};  // key was not found.
  }
  auto const *const v = std::get_if<std::int64_t>(&pos->second);
  if (v == nullptr || *v < 0) {
    return schema::error::expected_non_negative_integer;
  }
  return bool_to_error(predicate(*v));
}

std::error_code schema::checker::string_constraints(object const &schema, u8string const &s) {
  auto const end = schema->end();

  std::optional<u8string::iterator::difference_type> length;
  auto const get_length = [&length, &s]() {
    if (!length) {
      length = icubaby::length(std::begin(s), std::end(s));
    }
    return length.value();
  };

  // The value of this keyword MUST be a non-negative integer. A string
  // instance is valid against this keyword if its length is less than, or
  // equal to, the value of this keyword. The length of a string instance is
  // defined as the number of its characters as defined by RFC 8259.
  if (auto const erc = non_negative_constraint(schema, u8"maxLength",
                                               [&get_length](std::int64_t value) { return get_length() <= value; })) {
    return erc;
  }

  // The value of this keyword MUST be a non-negative integer. A string
  // instance is valid against this keyword if its length is greater than, or
  // equal to, the value of this keyword. The length of a string instance is
  // defined as the number of its characters as defined by RFC 8259. Omitting
  // this keyword has the same behavior as a value of 0.
  if (auto const erc = non_negative_constraint(schema, u8"minLength",
                                               [&get_length](std::int64_t value) { return get_length() >= value; })) {
    return erc;
  }

  if (auto const pattern_pos = schema->find(u8"pattern"); pattern_pos != end) {
    if (auto const *const pattern = std::get_if<u8string>(&pattern_pos->second)) {
      // std::basic_regex<char8> self_regex(*pattern,
      // std::regex_constants::ECMAScript); if
      // (!std::regex_search(*string_inst, self_regex)) {
      //   return false;
      // }
      //  TODO: basic_regex and char8?
    } else {
      return error::pattern_string;
    }
  }
  return {};
}

std::error_code schema::checker::object_constraints(object const &schema, object const &obj) {
  auto const end = schema->end();

  // core 10.3.2.1. properties
  // The value of "properties" MUST be an object. Each value of this object
  // MUST be a valid JSON Schema. Validation succeeds if, for each name that
  // appears in both the instance and as a name within this keyword's value,
  // the child instance for that name successfully validates against the
  // corresponding schema.
  if (auto const properties_pos = schema->find(u8"properties"); properties_pos != end) {
    if (auto const *const properties = std::get_if<object>(&properties_pos->second)) {
      for (auto const &[key, subschema] : **properties) {
        // Does the instance object contain a property with this name?
        if (auto const instance_pos = obj->find(key); instance_pos != obj->end()) {
          // It does, so check the instance property's value against the
          // subschema.
          if (auto const res = check(subschema, instance_pos->second)) {
            return res;
          }
        }
      }
    } else {
      return error::properties_must_be_object;
    }
  }

  // 6.5.1. maxProperties
  // The value of this keyword MUST be a non-negative integer. An object
  // instance is valid against "maxProperties" if its number of properties is
  // less than, or equal to, the value of this keyword.
  if (auto const erc = non_negative_constraint(schema, u8"maxProperties", [&obj](std::int64_t value) {
        return obj->size() <= static_cast<std::uint64_t>(value);
      })) {
    return erc;
  }

  // 6.5.2. minProperties
  // The value of this keyword MUST be a non-negative integer. An object
  // instance is valid against "minProperties" if its number of properties is
  // greater than, or equal to, the value of this keyword. Omitting this
  // keyword has the same behavior as a value of 0.
  if (auto const erc = non_negative_constraint(schema, u8"minProperties", [&obj](std::int64_t value) {
        return obj->size() >= static_cast<std::uint64_t>(value);
      })) {
    return erc;
  }

  if (schema->find(u8"patternProperties") != end) {
  }
  if (schema->find(u8"additionalProperties") != end) {
  }
  if (schema->find(u8"propertyNames") != end) {
  }
  return {};
}

std::error_code schema::checker::check(element const &schema, element const &instance) {
  // A schema or a sub-schema may be either an object or a boolean.
  if (auto const *const b = std::get_if<bool>(&schema)) {
    return *b ? std::error_code{} : make_error_code(error::validation);
  }
  auto const *const schema_obj = std::get_if<object>(&schema);
  if (schema_obj == nullptr) {
    return error::not_boolean_or_object;
  }
  auto const &map = **schema_obj;
  auto const end = map.end();

  if (auto const &const_pos = map.find(u8"const"); const_pos != end) {
    if (instance != const_pos->second) {
      return error::validation;
    }
  }
  if (auto const &enum_pos = map.find(u8"enum"); enum_pos != end) {
    if (auto const *const arr = std::get_if<array>(&enum_pos->second)) {
      if (!std::any_of(std::begin(**arr), std::end(**arr), [&instance](element const &el) { return el == instance; })) {
        return error::validation;
      }
    } else {
      return error::enum_must_be_array;
    }
  }
  if (auto const &type_pos = map.find(u8"type"); type_pos != end) {
    if (auto const *const name = std::get_if<u8string>(&type_pos->second)) {
      if (auto const erc = check_type(*name, instance)) {
        return erc;
      }
    } else if (auto const *const name_array = std::get_if<array>(&type_pos->second)) {
      auto erc = make_error_code(error::validation);
      for (auto it = std::begin(**name_array), na_end = std::end(**name_array); it != na_end && erc; ++it) {
        erc = check_type(*it, instance);
      }
      if (erc) {
        return erc;  // none of the types match.
      }
    } else {
      return error::type_string_or_string_array;
    }
  }

  if (auto const *const integer_inst = std::get_if<std::int64_t>(&instance)) {
    if (auto const erc = number_constraints(*schema_obj, *integer_inst)) {
      return erc;
    }
  }
  if (auto const *const fp_inst = std::get_if<double>(&instance)) {
    if (auto const erc = number_constraints(*schema_obj, *fp_inst)) {
      return erc;
    }
  }

  // If the instance is a string, then check string constraints.
  if (auto const *const string_inst = std::get_if<u8string>(&instance)) {
    if (auto const erc = string_constraints(*schema_obj, *string_inst)) {
      return erc;
    }
  }

  // If the instance is an object, check for the object keywords.
  if (auto const *const object_inst = std::get_if<object>(&instance)) {
    if (auto const erc = object_constraints(*schema_obj, *object_inst)) {
      return erc;
    }
  }
  return {};
}

std::error_code schema::checker::root(element const &schema, element const &instance) {
  root_ = &schema;

  // A schema or a sub-schema may be either an object or a boolean.
  if (auto const *const b = std::get_if<bool>(&schema)) {
    return bool_to_error(*b);
  }
  auto const *const obj = std::get_if<object>(&schema);
  if (obj == nullptr) {
    return error::not_boolean_or_object;
  }

  auto const &map = **obj;
  auto const end = map.end();

  if (auto const base_uri_pos = map.find(u8"$id"); base_uri_pos != end) {
    if (auto const *const base_uri = std::get_if<u8string>(&base_uri_pos->second)) {
      base_uri_ = *base_uri;
    } else {
      return error::expected_string;
    }
  }

  if (auto const defs_pos = map.find(u8"$defs"); defs_pos != end) {
    if (auto const *const defs = std::get_if<object>(&defs_pos->second)) {
      defs_ = defs;
    } else {
      return error::defs_must_be_object;
    }
  }

  return checker::check(schema, instance);
}

}  // end namespace peejay
