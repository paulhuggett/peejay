//===- include/peejay/error.hpp ---------------------------*- mode: C++ -*-===//
//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
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
#ifndef PEEJAY_ERROR_HPP
#define PEEJAY_ERROR_HPP

#include <cassert>
#include <string>
#include <system_error>
#include <type_traits>

namespace peejay {

enum class error : int {
  none,
  bad_unicode_code_point,
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
  string_too_long,
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
  constexpr char const* name() const noexcept override { return "PJ JSON Parser"; }

  /// Returns a string describing the given error in the PJ category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  std::string message(int const err) const override {
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
inline std::error_code make_error_code(error const e) noexcept {
  static error_category const cat;
  return {static_cast<int>(e), cat};
}

}  // end namespace peejay

template <> struct std::is_error_code_enum<peejay::error> : std::true_type {};

#endif  // PEEJAY_ERROR_HPP
