//===- include/peejay/rules/errors.hpp --------------------*- mode: C++ -*-===//
//*                                *
//*   ___ _ __ _ __ ___  _ __ ___  *
//*  / _ \ '__| '__/ _ \| '__/ __| *
//* |  __/ |  | | | (_) | |  \__ \ *
//*  \___|_|  |_|  \___/|_|  |___/ *
//*                                *
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
/// \file errors.hpp
/// \brief Declares the errors that can be issued by the rules sub-library.
///
/// These are returned by the members of the peejay::rules namespace and are reported by the
/// parser in the same way as any other error returned by a backend.
#ifndef PEEJAY_RULES_ERRORS_HPP
#define PEEJAY_RULES_ERRORS_HPP

#include <system_error>

namespace peejay::rules {

/// The errors that can be issued by the rules sub-library.
enum class error : int {
  none,                   ///< No error.
  unexpected_null,        ///< A null value was encountered by a rule which does not accept one.
  unexpected_boolean,     ///< A boolean value was encountered by a rule which does not accept one.
  unexpected_number,      ///< A number was encountered by a rule which does not accept one.
  unexpected_string,      ///< A string was encountered by a rule which does not accept one.
  unexpected_array,       ///< An array was encountered by a rule which does not accept one.
  unexpected_end_array,   ///< The end of an array was encountered by a rule which was not parsing an array.
  unexpected_object,      ///< An object was encountered by a rule which does not accept one.
  unexpected_object_key,  ///< An object key was encountered by a rule which was not parsing an object.
  unexpected_end_object,  ///< The end of an object was encountered by a rule which was not parsing an object.

  unrecognized_property,    ///< An object key did not match any of the properties expected by the rule.
  incomplete_object,        ///< An object ended before all of its required properties had been seen.
  too_many_array_elements,  ///< An array contained more elements than the rule's maximum.
  unrecognized_enumerator,  ///< A string did not match any of the enumerators expected by the rule.
  no_active_rule,           ///< An event was received after the context's parse had ended.
};

/// The error category for peejay::rules::error values.
class error_category final : public std::error_category {
public:
  /// \returns The name of this error category.
  char const* name() const noexcept override { return "peejay rule"; }
  /// \param err  An error value. This should be one of the peejay::rules::error enumerators.
  /// \returns A description of the error \p err.
  std::string message(int const err) const override {
    auto const* result = "unknown error";
    switch (static_cast<error>(err)) {
    case error::none: result = "none"; break;
    case error::unexpected_null: result = "unexpected null"; break;
    case error::unexpected_boolean: result = "unexpected boolean"; break;
    case error::unexpected_number: result = "unexpected number"; break;
    case error::unexpected_string: result = "unexpected string"; break;
    case error::unexpected_array: result = "unexpected array"; break;
    case error::unexpected_end_array: result = "unexpected end of array"; break;
    case error::unexpected_object: result = "unexpected object"; break;
    case error::unexpected_object_key: result = "unexpected object key"; break;
    case error::unexpected_end_object: result = "unexpected end object"; break;
    case error::unrecognized_property: result = "unrecognized property"; break;
    case error::incomplete_object: result = "incomplete object"; break;
    case error::too_many_array_elements: result = "too many array elements"; break;
    case error::unrecognized_enumerator: result = "unrecognized enumerator"; break;
    case error::no_active_rule: result = "no active rule"; break;
    default: break;
    }
    return result;
  }
};

/// Converts a peejay::rules::error value to a std::error_code.
///
/// \param e  The error value to be converted.
/// \returns A std::error_code with value \p e and category peejay::rules::error_category.
inline std::error_code make_error_code(error const e) noexcept {
  static error_category const cat;
  static_assert(std::is_same_v<std::underlying_type_t<error>, int>, "The underlying type of import error must be int");
  return {static_cast<int>(e), cat};
}

}  // end namespace peejay::rules

/// Allows peejay::rules::error values to be implicitly converted to std::error_code.
template <> struct std::is_error_code_enum<peejay::rules::error> : std::true_type {};

#endif  // PEEJAY_RULES_ERRORS_HPP
