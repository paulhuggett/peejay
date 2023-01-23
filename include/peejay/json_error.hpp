//===- include/peejay/json_error.hpp ----------------------*- mode: C++ -*-===//
//*    _                                             *
//*   (_)___  ___  _ __     ___ _ __ _ __ ___  _ __  *
//*   | / __|/ _ \| '_ \   / _ \ '__| '__/ _ \| '__| *
//*   | \__ \ (_) | | | | |  __/ |  | | | (_) | |    *
//*  _/ |___/\___/|_| |_|  \___|_|  |_|  \___/|_|    *
//* |__/                                             *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_JSON_ERROR_HPP
#define PEEJAY_JSON_ERROR_HPP

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
};

// ******************
// * error category *
// ******************
class error_category final : public std::error_category {
public:
  error_category () noexcept;
  char const* name () const noexcept override;
  std::string message (int error) const override;
};

std::error_category const& get_error_category () noexcept;

inline std::error_code make_error_code (error const e) noexcept {
  return {static_cast<int> (e), get_error_category ()};
}

}  // end namespace peejay

namespace std {

template <>
struct is_error_code_enum<peejay::error> : std::true_type {};

}  // end namespace std

#endif  // PEEJAY_JSON_ERROR_HPP
