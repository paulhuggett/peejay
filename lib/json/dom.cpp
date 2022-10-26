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

// string
// ~~~~~~
std::error_code peejay::dom::string_value (std::string_view const& s) {
  stack_.emplace (std::string{s});
  return {};
}

// int64
// ~~~~~
std::error_code peejay::dom::int64_value (int64_t v) {
  stack_.emplace (v);
  return {};
}

// uint64
// ~~~~~~
std::error_code peejay::dom::uint64_value (uint64_t v) {
  stack_.emplace (v);
  return {};
}

// double
// ~~~~~~
std::error_code peejay::dom::double_value (double v) {
  stack_.emplace (v);
  return {};
}

// boolean
// ~~~~~~~
std::error_code peejay::dom::boolean_value (bool v) {
  stack_.emplace (v);
  return {};
}

// null
// ~~~~
std::error_code peejay::dom::null_value () {
  stack_.emplace (null{});
  return {};
}

// begin array
// ~~~~~~~~~~~
std::error_code peejay::dom::begin_array () {
  stack_.emplace (mark{});
  return {};
}

// end array
// ~~~~~~~~~
std::error_code peejay::dom::end_array () {
  array arr;
  size_t const size = this->elements_until_mark ();
  arr.reserve (size);
  for (;;) {
    auto& top = stack_.top ();
    if (std::holds_alternative<mark> (top)) {
      stack_.pop ();
      break;
    }
    arr.emplace_back (std::move (top));
    stack_.pop ();
  }
  assert (arr.size () == size);
  std::reverse (std::begin (arr), std::end (arr));
  stack_.emplace (std::move (arr));
  return {};
}

// end object
// ~~~~~~~~~~
std::error_code peejay::dom::end_object () {
  assert (this->elements_until_mark () % 2U == 0U);
  object::size_type const size = this->elements_until_mark () / 2U;
  object obj{size};
  for (;;) {
    element value = std::move (stack_.top ());
    stack_.pop ();
    if (std::holds_alternative<mark> (value)) {
      break;
    }
    auto& key = stack_.top ();
    assert (std::holds_alternative<std::string> (key));
    obj.try_emplace (std::move (std::get<std::string> (key)), std::move (value));
    stack_.pop ();
  }
  assert (obj.size () == size);
  stack_.emplace (std::move (obj));
  return {};
}
