//===- include/peejay/error.hpp ---------------------------*- mode: C++ -*-===//
//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
/// \file error.hpp
/// \brief Defines peejay::error, the error codes raised by the PJ JSON parser, and
///   its integration with std::error_code.
#ifndef PEEJAY_ERROR_HPP
#define PEEJAY_ERROR_HPP

#include <cassert>
#include <string>
#include <system_error>
#include <type_traits>

namespace peejay {

/// The errors that may be reported by the PJ JSON parser.
enum class error : int {
  /// No error.
  none,
  /// A string contained an invalid UTF-16 escape sequence or an unescaped control character.
  bad_unicode_code_point,
  /// An array member was expected but not found.
  expected_array_member,
  /// The input ended within a string.
  expected_close_quote,
  /// A colon was expected between an object's key and its value.
  expected_colon,
  /// A digit was expected in a number.
  expected_digits,
  /// A string was expected for an object key.
  expected_object_key,
  /// An object member was expected but not found.
  expected_object_member,
  /// A value was expected but not found.
  expected_token,
  /// A backslash in a string was followed by an unrecognized escape character.
  invalid_escape_char,
  /// A \\u escape in a string contained a character that is not a hexadecimal digit.
  invalid_hex_char,
  /// Arrays and/or objects are nested more deeply than the policy's max_stack_depth permits.
  nesting_too_deep,
  /// A number could not be represented by the policy's integer or floating-point types, or the input ended
  /// part-way through a number.
  number_out_of_range,
  /// Non-whitespace input followed the top-level value.
  unexpected_extra_input,
  /// The input contained a token that was not recognized.
  unrecognized_token,
  /// A string exceeded the policy's max_length.
  string_too_long,
};

// ******************
// * error category *
// ******************
/// The error category object for PJ errors.
class error_category final : public std::error_category {
public:
  /// Returns a pointer to a C string naming the error category.
  ///
  /// \returns The string "PJ JSON Parser".
  [[nodiscard]] constexpr char const* name() const noexcept override { return "PJ JSON Parser"; }

  /// Returns a string describing the given error in the PJ category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  [[nodiscard]] std::string message(int const err) const override {
    switch (static_cast<error>(err)) {
    case error::none: return "none";
    case error::bad_unicode_code_point: return "bad UNICODE code point";
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
    case error::string_too_long: return "string too long";
    default: break;
    }
    return "unknown PJ error code";
  }
};

// make error code
// ~~~~~~~~~~~~~~~
/// Converts a peejay::error value to a std::error_code.
///
/// \param e  The peejay::error value to be converted.
/// \returns  A std::error_code which encapsulates the error \p e.
[[nodiscard]] inline std::error_code make_error_code(error const e) noexcept {
  static error_category const cat;
  return {static_cast<int>(e), cat};
}

}  // end namespace peejay

/// Enables implicit conversion from peejay::error to std::error_code.
template <> struct std::is_error_code_enum<peejay::error> : std::true_type {};

#endif  // PEEJAY_ERROR_HPP
