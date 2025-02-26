//===- include/peejay/schema/error.hpp --------------------*- mode: C++ -*-===//
//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file json_error.hpp
/// \brief Defines the errors that can be returned from the PJ library.
#ifndef PEEJAY_SCHEMA_ERROR_HPP
#define PEEJAY_SCHEMA_ERROR_HPP

#include <cassert>
#include <string>
#include <system_error>

namespace peejay::schema {

enum class error : int {
  none,
  defs_must_be_object,
  enum_must_be_array,
  not_boolean_or_object,
  expected_integer,
  expected_number,
  expected_non_negative_integer,
  expected_string,
  pattern_string,
  properties_must_be_object,
  type_string_or_string_array,
  type_name_invalid,
  validation,
};

// ******************
// * error category *
// ******************
/// The error category object for PJ schema errors
class error_category final : public std::error_category {
public:
  /// Returns a pointer to a C string naming the error category.
  ///
  /// \returns The string "PJ Schema".
  char const* name() const noexcept override { return "PJ Schema"; }
  /// Returns a string describing the given error in the PJ category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  std::string message(int const err) const override {
    switch (static_cast<error>(err)) {
    case error::none: return "none";
    case error::defs_must_be_object: return "schema $defs value must be an object";
    case error::enum_must_be_array: return "schema enum value must be an array";
    case error::expected_integer: return "schema expected an integer";
    case error::expected_number: return "schema expected a number";
    case error::expected_non_negative_integer: return "schema expected a non-negative integer";
    case error::expected_string: return "schema expected a string";
    case error::not_boolean_or_object: return "schema must be boolean or object";
    case error::properties_must_be_object: return "schema properties keyword value must be an object";
    case error::type_name_invalid: return "schema type name invalid";
    case error::type_string_or_string_array: return "schema type constraint was not a string or an array";
    case error::pattern_string: return "schema pattern constraint was not a string";
    case error::validation: return "schema validation failed";
    }
    assert(false && "bad error code");
    return "unknown PJ schema error code";
  }
};

// make error code
// ~~~~~~~~~~~~~~~
/// Converts a peejay::error value to a std::error_code.
///
/// \param e  The peejay::error value to be converted.
/// \returns  A std::error_code which encapsulates the error \p e.
inline std::error_code make_error_code(error const e) noexcept {
  static error_category const cat;
  return {static_cast<int>(e), cat};
}

}  // end namespace peejay::schema

template <> struct std::is_error_code_enum<peejay::schema::error> : std::true_type {};

#endif  // PEEJAY_SCHEMA_ERROR_HPP
