//===- tools/tree/dom.cpp -------------------------------------------------===//
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
#include "dom.hpp"

// end array
// ~~~~~~~~~
std::error_code dom_tree::end_array () {
  array arr;
  for (;;) {
    auto& top = stack_.top ();
    if (std::holds_alternative<mark> (top)) {
      stack_.pop ();
      break;
    }
    arr.push_back (std::move (top));
    stack_.pop ();
  }
  std::reverse (std::begin (arr), std::end (arr));
  stack_.push (element{std::move (arr)});
  return {};
}

// end object
// ~~~~~~~~~~
std::error_code dom_tree::end_object () {
  object obj;
  for (;;) {
    element const value = std::move (stack_.top ());
    stack_.pop ();
    if (std::holds_alternative<mark> (value)) {
      break;
    }
    auto const& key = stack_.top ();
    assert (std::holds_alternative<std::string> (key));
    obj[std::get<std::string> (key)] = value;
    stack_.pop ();
  }
  stack_.push (element{std::move (obj)});
  return {};
}
