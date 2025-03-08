//===- lib/uri/punycode.cpp -----------------------------------------------===//
//*                                        _       *
//*  _ __  _   _ _ __  _   _  ___ ___   __| | ___  *
//* | '_ \| | | | '_ \| | | |/ __/ _ \ / _` |/ _ \ *
//* | |_) | |_| | | | | |_| | (_| (_) | (_| |  __/ *
//* | .__/ \__,_|_| |_|\__, |\___\___/ \__,_|\___| *
//* |_|                |___/                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "uri/punycode.hpp"

#include <cstdint>
#include <limits>

namespace uri::punycode {

char const* error_category::name() const noexcept {
  return "punycode decode";
}
std::string error_category::message(int error) const {
  switch (static_cast<decode_error_code>(error)) {
  case decode_error_code::bad_input: return "bad input";
  case decode_error_code::overflow: return "overflow";
  case decode_error_code::none: return "unknown error";
  default: return "unknown error";
  }
}
std::error_code make_error_code(decode_error_code const e) {
  static error_category category;
  return {static_cast<int>(e), category};
}

}  // end namespace uri::punycode
