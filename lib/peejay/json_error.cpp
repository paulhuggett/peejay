//===- lib/peejay/json_error.cpp ------------------------------------------===//
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
#include "peejay/json_error.hpp"

#include <cassert>

// ******************
// * error category *
// ******************
peejay::error_category::error_category () noexcept = default;

char const* peejay::error_category::name () const noexcept {
  return "PJ JSON parser category";
}

std::string peejay::error_category::message (int const err) const {
  switch (static_cast<error> (err)) {
  case error::none: return "none";
  case error::bad_unicode_code_point: return "bad UNICODE code point";
  case error::dom_nesting_too_deep:
    return "(DOM) object or array contains too many members";
  case error::expected_array_member: return "expected array member";
  case error::expected_close_quote: return "expected close quote";
  case error::expected_colon: return "expected colon";
  case error::expected_digits: return "expected digits";
  case error::expected_object_member: return "expected object member";
  case error::expected_string: return "expected string";
  case error::expected_token: return "expected token";
  case error::invalid_escape_char: return "invalid escape character";
  case error::invalid_hex_char: return "invalid hexadecimal escape character";
  case error::number_out_of_range: return "number out of range";
  case error::unexpected_extra_input: return "unexpected extra input";
  case error::unrecognized_token: return "unrecognized token";
  case error::nesting_too_deep: return "objects are too deeply nested";
  }
  assert (false && "bad error code");
  return "unknown PJ error_category error";
}

std::error_category const& peejay::get_error_category () noexcept {
  static peejay::error_category const cat;
  return cat;
}
