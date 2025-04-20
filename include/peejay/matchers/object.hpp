//===- include/peejay/matchers/object.hpp -----------------*- mode: C++ -*-===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
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
#ifndef PEEJAY_MATCHERS__OBJECT_HPP
#define PEEJAY_MATCHERS__OBJECT_HPP

#include "peejay/concepts.hpp"
#include "peejay/error.hpp"
#include "peejay/matchers/whitespace.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

template <backend Backend> class object_matcher {
public:
  using parser_type = parser<Backend>;
  static bool consume(parser_type &parser, std::optional<char32_t> ch);

private:
  static void end_object(parser_type &parser);
  static bool comma(parser_type &parser, char32_t code_point);
  static bool key(parser_type &parser, char32_t code_point);
};

// consume
// ~~~~~~~
template <backend Backend> bool object_matcher<Backend>::consume(parser_type &parser, std::optional<char32_t> ch) {
  if (!ch) {
    parser.set_error(error::expected_object_member);
    return true;
  }
  auto const c = *ch;
  switch (parser.stack_.top()) {
  case state::object_start:
    assert(c == '{');
    parser.stack_.top() = state::object_first_key;
    if (parser.set_error(parser.backend().begin_object())) {
      break;
    }
    parser.push_whitespace_matcher();
    return true;
  case state::object_first_key:
    // We allow either a closing brace (to end the object) or a property name.
    if (c == '}') {
      object_matcher::end_object(parser);
      break;
    }
    [[fallthrough]];
  case state::object_key: return object_matcher::key(parser, c);
  case state::object_colon:
    // just consume any whitespace before the colon.
    if (whitespace(parser, c)) {
      return false;
    }
    if (c == ':') {
      parser.stack_.top() = state::object_value;
    } else {
      parser.set_error(error::expected_colon);
    }
    break;
  case state::object_value:
    parser.stack_.top() = state::object_comma;
    parser.push_root_matcher();
    return false;
  case state::object_comma: return object_matcher::comma(parser, c);
  default: unreachable(); break;
  }
  // No change of matcher. Consume the input character.
  return true;
}

// key
// ~~~
/// Match a property name then expect a colon.
template <backend Backend> bool object_matcher<Backend>::key(parser_type &parser, char32_t code_point) {
  parser.stack_.top() = state::object_colon;
  if (code_point == '"') {
    parser.push_string_matcher(true);
    return false;
  }
  parser.set_error(error::expected_object_key);
  return true;
}

// comma
// ~~~~~
template <backend Backend> bool object_matcher<Backend>::comma(parser_type &parser, char32_t code_point) {
  // Consume whitespace before the comma.
  if (whitespace(parser, code_point)) {
    return false;
  }
  if (code_point == ',') {
    // Strictly conforming JSON requires a property name following a comma.
    parser.stack_.top() = state::object_key;
    // Consume the comma and any whitespace before the close brace or property
    // name.
    parser.push_whitespace_matcher();
  } else if (code_point == '}') {
    object_matcher::end_object(parser);
  } else {
    parser.set_error(error::expected_object_member);
  }
  // Consume the input character.
  return true;
}

// end object
// ~~~~~~~~~~~
template <backend Backend> void object_matcher<Backend>::end_object(parser_type &parser) {
  parser.set_error(parser.backend().end_object());
  parser.pop();
}

}  // namespace peejay::details

#endif  // PEEJAY_MATCHERS__OBJECT_HPP
