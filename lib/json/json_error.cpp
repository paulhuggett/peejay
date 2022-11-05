//===- lib/json/json_error.cpp --------------------------------------------===//
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
#include "json/json_error.hpp"

// ******************
// * error category *
// ******************
peejay::error_category::error_category () noexcept = default;

char const* peejay::error_category::name () const noexcept {
  return "peejay json parser category";
}

std::string peejay::error_category::message (int const err) const {
  auto* result = "unknown peejay::error_category error";
  switch (static_cast<error> (err)) {
  case error::none: result = "none"; break;
  case error::bad_unicode_code_point: result = "bad UNICODE code point"; break;
  case error::expected_array_member: result = "expected array member"; break;
  case error::expected_close_quote: result = "expected close quote"; break;
  case error::expected_colon: result = "expected colon"; break;
  case error::expected_digits: result = "expected digits"; break;
  case error::expected_object_member: result = "expected object member"; break;
  case error::expected_string: result = "expected string"; break;
  case error::expected_token: result = "expected token"; break;
  case error::invalid_escape_char: result = "invalid escape character"; break;
  case error::invalid_hex_char:
    result = "invalid hexadecimal escape character";
    break;
  case error::number_out_of_range: result = "number out of range"; break;
  case error::unexpected_extra_input: result = "unexpected extra input"; break;
  case error::unrecognized_token: result = "unrecognized token"; break;
  case error::nesting_too_deep: result = "objects are too deeply nested"; break;
  }
  return result;
}

std::error_category const& peejay::get_error_category () noexcept {
  static peejay::error_category const cat;
  return cat;
}
