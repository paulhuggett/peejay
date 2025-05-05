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
#include <optional>

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
  static bool consume(parser<Backend> &parser, std::optional<char32_t> ch) {
    if (!ch) {
      parser.set_error(error::expected_array_member);
      return true;
    }
    auto const c = *ch;
    switch (parser.stack_.top()) {
    case state::array_start: assert(c == '['); return start(parser);
    case state::array_first_object:
      // Match this character and consume whitespace before the object (or close
      // bracket).
      if (whitespace(parser, c)) {
        return false;
      }
      if (c == ']') {
        return end_array(parser);
      }
      [[fallthrough]];
    case state::array_object:
      parser.stack_.top() = state::array_comma;
      parser.push_root_matcher();
      return false;
    case state::array_comma: return comma(parser, c);
    default: unreachable(); break;
    }
    return true;
  }

private:
  static bool start(parser<Backend> &parser) {
    if (parser.set_error(parser.backend().begin_array())) {
      return true;
    }
    parser.stack_.top() = state::array_first_object;
    // Consume the open bracket.
    return true;
  }
  static bool end_array(parser<Backend> &parser) {
    parser.set_error(parser.backend().end_array());
    parser.pop();
    return true;
  }
  static bool comma(parser<Backend> &parser, char32_t c) {
    if (whitespace(parser, c)) {
      return false;
    }
    if (c == ',') {
      parser.stack_.top() = state::array_object;
      return true;
    }
    if (c == ']') {
      return end_array(parser);
    }
    parser.set_error(error::expected_array_member);
    return true;
  }
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_ARRAY_HPP
