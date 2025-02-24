//===- include/peejay/json/json_error.hpp -----------------*- mode: C++ -*-===//
//*    _                                             *
//*   (_)___  ___  _ __     ___ _ __ _ __ ___  _ __  *
//*   | / __|/ _ \| '_ \   / _ \ '__| '__/ _ \| '__| *
//*   | \__ \ (_) | | | | |  __/ |  | | | (_) | |    *
//*  _/ |___/\___/|_| |_|  \___|_|  |_|  \___/|_|    *
//* |__/                                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file json_error.hpp
/// \brief Defines the errors that can be returned from the PJ library.
#ifndef PEEJAY_JSON_ERROR_HPP
#define PEEJAY_JSON_ERROR_HPP

#include <cassert>
#include <string>
#include <system_error>

namespace peejay {

enum class error : int {
  none,
  bad_identifier,
  bad_unicode_code_point,
  dom_nesting_too_deep,
  expected_array_member,
  expected_close_quote,
  expected_colon,
  expected_digits,
  expected_object_key,
  expected_object_member,
  expected_token,
  invalid_escape_char,
  invalid_hex_char,
  nesting_too_deep,
  number_out_of_range,
  unexpected_extra_input,
  unrecognized_token,
  unterminated_multiline_comment,
  identifier_too_long,
  string_too_long,

  schema_defs_must_be_object,
  schema_enum_must_be_array,
  schema_not_boolean_or_object,
  schema_expected_integer,
  schema_expected_number,
  schema_expected_non_negative_integer,
  schema_expected_string,
  schema_pattern_string,
  schema_properties_must_be_object,
  schema_type_string_or_string_array,
  schema_type_name_invalid,
  schema_validation,
};

// ******************
// * error category *
// ******************
/// The error category object for PJ errors
class error_category final : public std::error_category {
public:
  /// Returns a pointer to a C string naming the error category.
  ///
  /// \returns The string "PJ JSON Parser".
  char const* name() const noexcept override { return "PJ JSON Parser"; }
  /// Returns a string describing the given error in the PJ category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  std::string message(int const err) const override {
    switch (static_cast<error>(err)) {
    case error::none: return "none";
    case error::bad_identifier: return "bad identifier";
    case error::bad_unicode_code_point: return "bad UNICODE code point";
    case error::dom_nesting_too_deep: return "(DOM) object or array contains too many members";
    case error::expected_array_member: return "expected array member";
    case error::expected_close_quote: return "expected close quote";
    case error::expected_colon: return "expected colon";
    case error::expected_digits: return "expected digits";
    case error::expected_object_member: return "expected object member";
    case error::expected_object_key: return "expected object key";
    case error::expected_token: return "expected token";
    case error::invalid_escape_char: return "invalid escape character";
    case error::invalid_hex_char: return "invalid hexadecimal escape character";
    case error::number_out_of_range: return "number out of range";
    case error::unexpected_extra_input: return "unexpected extra input";
    case error::unrecognized_token: return "unrecognized token";
    case error::nesting_too_deep: return "objects are too deeply nested";
    case error::unterminated_multiline_comment: return "unterminated /* comment";
    case error::identifier_too_long: return "identifier too long";
    case error::string_too_long: return "string too long";

    case error::schema_defs_must_be_object: return "schema $defs value must be an object";
    case error::schema_enum_must_be_array: return "schema enum value must be an array";
    case error::schema_expected_integer: return "schema expected an integer";
    case error::schema_expected_number: return "schema expected a number";
    case error::schema_expected_non_negative_integer: return "schema expected a non-negative integer";
    case error::schema_expected_string: return "schema expected a string";
    case error::schema_not_boolean_or_object: return "schema must be boolean or object";
    case error::schema_properties_must_be_object: return "schema properties keyword value must be an object";
    case error::schema_type_name_invalid: return "schema type name invalid";
    case error::schema_type_string_or_string_array: return "schema type constraint was not a string or an array";
    case error::schema_pattern_string: return "schema pattern constraint was not a string";
    case error::schema_validation: return "schema validation failed";
    }
    assert(false && "bad error code");
    return "unknown PJ error code";
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

}  // end namespace peejay

template <> struct std::is_error_code_enum<peejay::error> : std::true_type {};

#endif  // PEEJAY_JSON_ERROR_HPP
