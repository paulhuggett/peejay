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

constexpr bool is_multiple_of (int64_t a, int64_t mo) noexcept {
  return a / mo * mo == a;
}
inline bool is_multiple_of (double a, double mo) noexcept {
  auto const t = a / mo;
  return t == std::floor (t);
}
inline bool is_multiple_of (double a, int64_t mo) noexcept {
  return is_multiple_of (a, static_cast<double> (mo));
}
inline bool is_multiple_of (int64_t a, double mo) noexcept {
  return is_multiple_of (static_cast<double> (a), mo);
}

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
class checker {
public:
  static std::error_code bool_to_error (bool b) {
    if (b) {
      return {};
    }
    return error::schema_validation;
  }

  static std::error_code check_type (u8string const &type_name,
                                     element const &instance) {
    static std::unordered_map<u8string,
                              std::function<bool (element const &)>> const map{
        {u8"array", is_type<array>},     {u8"boolean", is_type<bool>},
        {u8"integer", is_integer},       {u8"null", is_type<null>},
        {u8"number", is_number},         {u8"object", is_type<object>},
        {u8"string", is_type<u8string>},
    };
    if (auto const pos = map.find (type_name); pos != map.end ()) {
      return bool_to_error (pos->second (instance));
    }
    return error::schema_type_name_invalid;
  }
  static std::error_code check_type (element const &type_name,
                                     element const &instance) {
    if (auto const *const name = std::get_if<u8string> (&type_name)) {
      return check_type (*name, instance);
    }
    return error::schema_type_name_invalid;
  }

  template <typename Predicate>
  static std::error_code check_number (element const &el, Predicate pred) {
    if (auto const *const integer = std::get_if<std::int64_t> (&el)) {
      return bool_to_error (pred (*integer));
    }
    if (auto const *const fp = std::get_if<double> (&el)) {
      return bool_to_error (pred (*fp));
    }
    return error::schema_expected_number;
  }

  template <typename T1, typename T2>
  using math_type = std::conditional_t<std::is_floating_point_v<T1> ||
                                           std::is_floating_point_v<T2>,
                                       double, std::int64_t>;

  template <typename NumberType>
  static std::error_code number_constraints (object const &schema,
                                             NumberType const num) {
    auto const end = schema->end ();

    // The value of "multipleOf" MUST be a number, strictly greater than 0. A
    // numeric instance is valid only if division by this keyword's value
    // results in an integer.
    if (auto const multipleof_pos = schema->find (u8"multipleOf");
        multipleof_pos != end) {
      if (std::error_code const erc = check_number (
              multipleof_pos->second,
              [num] (auto const &v) { return is_multiple_of (num, v); })) {
        return erc;
      }
    }

    // The value of "maximum" MUST be a number, representing an inclusive upper
    // limit for a numeric instance. If the instance is a number, then this
    // keyword validates only if the instance is less than or exactly equal to
    // "maximum".
    if (auto const maximum_pos = schema->find (u8"maximum");
        maximum_pos != end) {
      if (std::error_code const erc =
              check_number (maximum_pos->second, [num] (auto const &v) {
                using target_type =
                    math_type<NumberType, std::decay_t<decltype (v)>>;
                return static_cast<target_type> (num) <=
                       static_cast<target_type> (v);
              })) {
        return erc;
      }
    }

    // The value of "exclusiveMaximum" MUST be a number, representing an
    // exclusive upper limit for a numeric instance. If the instance is a
    // number, then the instance is valid only if it has a value strictly less
    // than (not equal to) "exclusiveMaximum".
    if (auto const exmax_pos = schema->find (u8"exclusiveMaximum");
        exmax_pos != end) {
      if (std::error_code const erc =
              check_number (exmax_pos->second, [num] (auto const &v) {
                using target_type =
                    math_type<NumberType, std::decay_t<decltype (v)>>;
                return static_cast<target_type> (num) <
                       static_cast<target_type> (v);
              })) {
        return erc;
      }
    }

    // The value of "minimum" MUST be a number, representing an inclusive lower
    // limit for a numeric instance. If the instance is a number, then this
    // keyword validates only if the instance is greater than or exactly equal
    // to "minimum".
    if (auto const minimum_pos = schema->find (u8"minimum");
        minimum_pos != end) {
      if (auto const erc =
              check_number (minimum_pos->second, [num] (auto const &v) {
                using target_type =
                    math_type<NumberType, std::decay_t<decltype (v)>>;
                return static_cast<target_type> (num) >=
                       static_cast<target_type> (v);
              })) {
        return erc;
      }
    }

    // The value of "exclusiveMinimum" MUST be a number, representing an
    // exclusive lower limit for a numeric instance. If the instance is a
    // number, then the instance is valid only if it has a value strictly
    // greater than (not equal to) "exclusiveMinimum".
    if (auto const exmin_pos = schema->find (u8"exclusiveMinimum");
        exmin_pos != end) {
      if (auto const erc =
              check_number (exmin_pos->second, [num] (auto const &v) {
                using target_type =
                    math_type<NumberType, std::decay_t<decltype (v)>>;
                return static_cast<target_type> (num) >
                       static_cast<target_type> (v);
              })) {
        return erc;
      }
    }

    return {};
  }

  template <typename Predicate>
  static std::error_code non_negative_constraint (object const &schema,
                                                  u8string const &name,
                                                  Predicate predicate) {
    auto const pos = schema->find (name);
    if (pos == schema->end ()) {
      return {};  // key was not found.
    }
    auto const *const v = std::get_if<std::int64_t> (&pos->second);
    if (v == nullptr || *v < 0) {
      return error::schema_expected_non_negative_integer;
    }
    return bool_to_error (predicate (*v));
  }

  static std::error_code string_constraints (object const &schema,
                                             u8string const &s) {
    auto const end = schema->end ();

    std::optional<u8string::iterator::difference_type> length;
    auto const get_length = [&length, &s] () {
      if (!length) {
        length = icubaby::length (std::begin (s), std::end (s));
      }
      return length.value ();
    };

    // The value of this keyword MUST be a non-negative integer. A string
    // instance is valid against this keyword if its length is less than, or
    // equal to, the value of this keyword. The length of a string instance is
    // defined as the number of its characters as defined by RFC 8259.
    if (auto const erc = non_negative_constraint (
            schema, u8"maxLength", [&get_length] (std::int64_t value) {
              return get_length () <= value;
            })) {
      return erc;
    }

    // The value of this keyword MUST be a non-negative integer. A string
    // instance is valid against this keyword if its length is greater than, or
    // equal to, the value of this keyword. The length of a string instance is
    // defined as the number of its characters as defined by RFC 8259. Omitting
    // this keyword has the same behavior as a value of 0.
    if (auto const erc = non_negative_constraint (
            schema, u8"minLength", [&get_length] (std::int64_t value) {
              return get_length () >= value;
            })) {
      return erc;
    }

    if (auto const pattern_pos = schema->find (u8"pattern");
        pattern_pos != end) {
      if (auto const *const pattern =
              std::get_if<u8string> (&pattern_pos->second)) {
        // std::basic_regex<char8> self_regex(*pattern,
        // std::regex_constants::ECMAScript); if
        // (!std::regex_search(*string_inst, self_regex)) {
        //   return false;
        // }
        //  TODO: basic_regex and char8?
      } else {
        return error::schema_pattern_string;
      }
    }
    return {};
  }

  static std::error_code object_constraints (object const &schema,
                                             object const &obj) {
    auto const end = schema->end ();

    // core 10.3.2.1. properties
    // The value of "properties" MUST be an object. Each value of this object
    // MUST be a valid JSON Schema. Validation succeeds if, for each name that
    // appears in both the instance and as a name within this keyword's value,
    // the child instance for that name successfully validates against the
    // corresponding schema.
    if (auto const properties_pos = schema->find (u8"properties");
        properties_pos != end) {
      if (auto const *const properties =
              std::get_if<object> (&properties_pos->second)) {
        for (auto const &[key, subschema] : **properties) {
          // Does the instance object contain a property with this name?
          if (auto const instance_pos = obj->find (key);
              instance_pos != obj->end ()) {
            // It does, so check the instance property's value against the
            // subschema.
            if (auto const res = check (subschema, instance_pos->second)) {
              return res;
            }
          }
        }
      } else {
        return error::schema_properties_must_be_object;
      }
    }

    // 6.5.1. maxProperties
    // The value of this keyword MUST be a non-negative integer. An object
    // instance is valid against "maxProperties" if its number of properties is
    // less than, or equal to, the value of this keyword.
    if (auto const erc = non_negative_constraint (
            schema, u8"maxProperties", [&obj] (std::int64_t value) {
              return obj->size () <= static_cast<std::uint64_t> (value);
            })) {
      return erc;
    }

    // 6.5.2. minProperties
    // The value of this keyword MUST be a non-negative integer. An object
    // instance is valid against "minProperties" if its number of properties is
    // greater than, or equal to, the value of this keyword. Omitting this
    // keyword has the same behavior as a value of 0.
    if (auto const erc = non_negative_constraint (
            schema, u8"minProperties", [&obj] (std::int64_t value) {
              return obj->size () >= static_cast<std::uint64_t> (value);
            })) {
      return erc;
    }

    if (schema->find (u8"patternProperties") != end) {
    }
    if (schema->find (u8"additionalProperties") != end) {
    }
    if (schema->find (u8"propertyNames") != end) {
    }
    return {};
  }
  static std::error_code check (element const &schema,
                                element const &instance) {
    // A schema or a sub-schema may be either an object or a boolean.
    if (auto const *const b = std::get_if<bool> (&schema)) {
      return *b ? std::error_code{}
                : make_error_code (error::schema_validation);
    }
    auto const *const schema_obj = std::get_if<object> (&schema);
    if (schema_obj == nullptr) {
      return error::schema_not_boolean_or_object;
    }
    auto const &map = **schema_obj;
    auto const end = map.end ();

    if (auto const &const_pos = map.find (u8"const"); const_pos != end) {
      if (instance != const_pos->second) {
        return error::schema_validation;
      }
    }
    if (auto const &enum_pos = map.find (u8"enum"); enum_pos != end) {
      if (auto const *const arr = std::get_if<array> (&enum_pos->second)) {
        if (!std::any_of (
                std::begin (**arr), std::end (**arr),
                [&instance] (element const &el) { return el == instance; })) {
          return error::schema_validation;
        }
      } else {
        return error::schema_enum_must_be_array;
      }
    }
    if (auto const &type_pos = map.find (u8"type"); type_pos != end) {
      if (auto const *const name = std::get_if<u8string> (&type_pos->second)) {
        if (auto const erc = check_type (*name, instance)) {
          return erc;
        }
      } else if (auto const *const name_array =
                     std::get_if<array> (&type_pos->second)) {
        auto erc = make_error_code (error::schema_validation);
        for (auto it = std::begin (**name_array),
                  na_end = std::end (**name_array);
             it != na_end && erc; ++it) {
          erc = check_type (*it, instance);
        }
        if (erc) {
          return erc;  // none of the types match.
        }
      } else {
        return error::schema_type_string_or_string_array;
      }
    }

    if (auto const *const integer_inst =
            std::get_if<std::int64_t> (&instance)) {
      if (auto const erc = number_constraints (*schema_obj, *integer_inst)) {
        return erc;
      }
    }
    if (auto const *const fp_inst = std::get_if<double> (&instance)) {
      if (auto const erc = number_constraints (*schema_obj, *fp_inst)) {
        return erc;
      }
    }

    // If the instance is a string, then check string constraints.
    if (auto const *const string_inst = std::get_if<u8string> (&instance)) {
      if (auto const erc = string_constraints (*schema_obj, *string_inst)) {
        return erc;
      }
    }

    // If the instance is an object, check for the object keywords.
    if (auto const *const object_inst = std::get_if<object> (&instance)) {
      if (auto const erc = object_constraints (*schema_obj, *object_inst)) {
        return erc;
      }
    }
    return {};
  }

  std::error_code root (element const &schema, element const &instance) {
    root_ = &schema;

    // A schema or a sub-schema may be either an object or a boolean.
    if (auto const *const b = std::get_if<bool> (&schema)) {
      return bool_to_error (*b);
    }
    auto const *const obj = std::get_if<object> (&schema);
    if (obj == nullptr) {
      return error::schema_not_boolean_or_object;
    }

    auto const &map = **obj;
    auto const end = map.end ();

    if (auto const base_uri_pos = map.find (u8"$id"); base_uri_pos != end) {
      if (auto const *const base_uri =
              std::get_if<u8string> (&base_uri_pos->second)) {
        base_uri_ = *base_uri;
      } else {
        return error::schema_expected_string;
      }
    }

    if (auto const defs_pos = map.find (u8"$defs"); defs_pos != end) {
      if (auto const *const defs = std::get_if<object> (&defs_pos->second)) {
        defs_ = defs;
      } else {
        return error::schema_defs_must_be_object;
      }
    }

    return checker::check (schema, instance);
  }

  u8string base_uri_;
  element const *root_;
  std::optional<object const *> defs_;
};

inline std::error_code check (element const &schema, element const &instance) {
  return checker{}.root (schema, instance);
}

}  // end namespace peejay::schema

#endif  // PEEJAY_SCHEMA_HPP
