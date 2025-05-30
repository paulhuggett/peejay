//===- include/peejay/matchers/array.hpp ------------------*- mode: C++ -*-===//
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
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
#ifndef PEEJAY_MATCHERS_ARRAY_HPP
#define PEEJAY_MATCHERS_ARRAY_HPP

#include <cassert>

#include "peejay/concepts.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/error.hpp"
#include "peejay/matchers/whitespace.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {
/// Matches an array.
template <backend Backend> class array_matcher {
public:
  static bool consume(parser<Backend> &parser, char32_t c) {
    switch (parser.stack_.top()) {
    case state::array_start:
      if (parser.set_error_and_pop(parser.backend().begin_array())) {
        return true;  // must return immediately. 'this' has been destroyed.
      }
      parser.set_state(state::array_first_object);
      if (whitespace(parser, c)) {
        return false;
      }
      [[fallthrough]];
    case state::array_first_object:
      if (c == ']') {
        return end_array(parser);
      }
      [[fallthrough]];
    case state::array_object:
      parser.set_state(state::array_comma);
      parser.push_root_matcher();
      return false;
    case state::array_comma: return comma(parser, c);
    default: unreachable(); break;
    }
  }

  static void eof(parser<Backend> &parser) { parser.set_error_and_pop(error::expected_array_member); }

private:
  static bool end_array(parser<Backend> &parser) {
    parser.set_error(parser.backend().end_array());
    parser.pop();  // unconditionally pop this matcher.
    return true;
  }
  static bool comma(parser<Backend> &parser, char32_t c) {
    // There can be whitespace between the end of an object and a subsequent comma or right square bracket.
    if (whitespace(parser, c)) {
      return false;
    }
    switch (c) {
    case ',': parser.set_state(state::array_object); break;
    case ']': return end_array(parser);
    default: return parser.set_error_and_pop(error::expected_array_member);
    }
    return true;
  }
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_ARRAY_HPP
