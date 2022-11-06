//===- lib/json/dom.cpp ---------------------------------------------------===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "json/dom.hpp"

namespace peejay {

// ******************
// * error category *
// ******************
char const* dom_error_category::name () const noexcept {
  return "PJ DOM category";
}

std::string dom_error_category::message (int const error) const {
  switch (static_cast<dom_error> (error)) {
  case dom_error::none: return "none";
  case dom_error::nesting_too_deep:
    return "object or array contains too many members";
  }
  assert (false);
  return "";
}

std::error_category const& get_dom_error_category () noexcept {
  static peejay::dom_error_category const cat;
  return cat;
}

}  // end namespace peejay
