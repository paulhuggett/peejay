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
#include <regex>

#include "peejay/dom.hpp"

namespace peejay {

template <typename T>
using error_or = std::variant<std::error_code, T>;

namespace schema {

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
    if (auto const *const name = std::get_if<u8string> (&type_name)) {
      return check_type (*name, instance);
    }
    return error::schema_type_name_invalid;
  }

  template <typename Predicate>
  static error_or<bool> check_number (element const &el, Predicate pred) {
    if (auto const *const integer = std::get_if<std::int64_t> (&el)) {
      return pred (*integer);
    }
    if (auto const *const fp = std::get_if<double> (&el)) {
      return pred (*fp);
    }
    return error::schema_expected_number;
  }

  template <typename NumberType>
  static error_or<bool> number_constraints (object const &schema,
                                            NumberType const num) {
    auto const end = schema->end ();

    // The value of "multipleOf" MUST be a number, strictly greater than 0. A
    // numeric instance is valid only if division by this keyword's value
    // results in an integer.
    if (auto const multipleof_pos = schema->find (u8"multipleOf");
        multipleof_pos != end) {
      if (auto const eo = check_number (
              multipleof_pos->second,
              [num] (auto const &v) { return is_multiple_of (num, v); });
          failed (eo)) {
        return eo;
      }
    }

    // The value of "maximum" MUST be a number, representing an inclusive upper
    // limit for a numeric instance. If the instance is a number, then this
    // keyword validates only if the instance is less than or exactly equal to
    // "maximum".
    if (auto const maximum_pos = schema->find (u8"maximum");
        maximum_pos != end) {
      if (auto const eo = check_number (
              maximum_pos->second,
              [num] (auto const &v) {
                return static_cast<std::decay_t<decltype (v)>> (num) <= v;
              });
          failed (eo)) {
        return eo;
      }
    }

    // The value of "exclusiveMaximum" MUST be a number, representing an
    // exclusive upper limit for a numeric instance. If the instance is a
    // number, then the instance is valid only if it has a value strictly less
    // than (not equal to) "exclusiveMaximum".
    if (auto const exmax_pos = schema->find (u8"exclusiveMaximum");
        exmax_pos != end) {
      if (auto const eo = check_number (
              exmax_pos->second,
              [num] (auto const &v) {
                return static_cast<std::decay_t<decltype (v)>> (num) < v;
              });
          failed (eo)) {
        return eo;
      }
    }

    // The value of "minimum" MUST be a number, representing an inclusive lower
    // limit for a numeric instance. If the instance is a number, then this
    // keyword validates only if the instance is greater than or exactly equal
    // to "minimum".
    if (auto const minimum_pos = schema->find (u8"minimum");
        minimum_pos != end) {
      if (auto const eo = check_number (
              minimum_pos->second,
              [num] (auto const &v) {
                return static_cast<std::decay_t<decltype (v)>> (num) >= v;
              });
          failed (eo)) {
        return eo;
      }
    }

    // The value of "exclusiveMinimum" MUST be a number, representing an
    // exclusive lower limit for a numeric instance. If the instance is a
    // number, then the instance is valid only if it has a value strictly
    // greater than (not equal to) "exclusiveMinimum".
    if (auto const exmin_pos = schema->find (u8"exclusiveMinimum");
        exmin_pos != end) {
      if (auto const eo = check_number (
              exmin_pos->second,
              [num] (auto const &v) {
                return static_cast<std::decay_t<decltype (v)>> (num) > v;
              });
          failed (eo)) {
        return eo;
      }
    }

    return true;
  }

  static error_or<bool> string_constraints (object const &schema,
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
    //  defined as the number of its characters as defined by RFC 8259.
    if (auto const maxlength_pos = schema->find (u8"maxLength");
        maxlength_pos != end) {
      auto const *const maxlength =
          std::get_if<std::int64_t> (&maxlength_pos->second);
      if (maxlength == nullptr || *maxlength < 0) {
        return error::schema_expected_non_negative_integer;
      }
      if (get_length () > *maxlength) {
        return false;
      }
    }

    // The value of this keyword MUST be a non-negative integer. A string
    // instance is valid against this keyword if its length is greater than, or
    // equal to, the value of this keyword. The length of a string instance is
    // defined as the number of its characters as defined by RFC 8259. Omitting
    // this keyword has the same behavior as a value of 0.
    if (auto const minlength_pos = schema->find (u8"minLength");
        minlength_pos != end) {
      auto const *const minlength =
          std::get_if<std::int64_t> (&minlength_pos->second);
      if (minlength == nullptr || *minlength < 0) {
        return error::schema_expected_non_negative_integer;
      }
      if (get_length () < *minlength) {
        return false;
      }
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
    return true;
  }

  static bool failed (error_or<bool> const &eo) {
    bool const *const b = std::get_if<bool> (&eo);
    return b == nullptr || !*b;
  }

  static error_or<bool> object_constraints (object const &schema,
                                            object const &obj) {
    auto const end = schema->end ();

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
            if (auto const res = check (subschema, instance_pos->second);
                failed (res)) {
              return res;
            }
          }
        }
      } else {
        return error::schema_properties_must_be_object;
      }
    }
    if (schema->find (u8"patternProperties") != end) {
    }
    if (schema->find (u8"additionalProperties") != end) {
    }
    if (schema->find (u8"propertyNames") != end) {
    }
    return true;
  }
  static error_or<bool> check (element const &schema, element const &instance) {
    // A schema or a sub-schema may be either an object or a boolean.
    if (auto const *const b = std::get_if<bool> (&schema)) {
      return *b;
    }
    auto const *const schema_obj = std::get_if<object> (&schema);
    if (schema_obj == nullptr) {
      return error::schema_not_boolean_or_object;
    }
    auto const &map = **schema_obj;
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

    if (auto const *const integer_inst =
            std::get_if<std::int64_t> (&instance)) {
      if (error_or<bool> const eo =
              number_constraints (*schema_obj, *integer_inst);
          failed (eo)) {
        return eo;
      }
    }
    if (auto const *const fp_inst = std::get_if<double> (&instance)) {
      if (error_or<bool> const eo = number_constraints (*schema_obj, *fp_inst);
          failed (eo)) {
        return eo;
      }
    }

    // If the instance is a string, then check string constraints.
    if (auto const *const string_inst = std::get_if<u8string> (&instance)) {
      if (auto const eo = string_constraints (*schema_obj, *string_inst);
          failed (eo)) {
        return eo;
      }
    }

    // If the instance is an object, check for the object keywords.
    if (auto const *const object_inst = std::get_if<object> (&instance)) {
      if (auto const eo = object_constraints (*schema_obj, *object_inst);
          failed (eo)) {
        return eo;
      }
    }
    return true;
  }

  error_or<bool> root (element const &schema, element const &instance) {
    root_ = &schema;

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
    if (auto const defs_pos = map.find (u8"$defs"); defs_pos != end) {
      if (auto const *const defs = std::get_if<object> (&defs_pos->second)) {
        defs_ = defs;
      } else {
        return error::schema_defs_must_be_object;
      }
    }

    return this->check (schema, instance);
  }

  element const *root_;
  std::optional<object const *> defs_;
};

inline error_or<bool> check (element const &schema, element const &instance) {
  return checker{}.root (schema, instance);
}

}  // end namespace schema

}  // end namespace peejay

#endif  // PEEJAY_SCHEMA_HPP
