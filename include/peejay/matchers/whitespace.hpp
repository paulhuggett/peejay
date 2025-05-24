//===- include/peejay/matchers/whitespace.hpp -------------*- mode: C++ -*-===//
//*           _     _ _                                  *
//* __      _| |__ (_) |_ ___  ___ _ __   __ _  ___ ___  *
//* \ \ /\ / / '_ \| | __/ _ \/ __| '_ \ / _` |/ __/ _ \ *
//*  \ V  V /| | | | | ||  __/\__ \ |_) | (_| | (_|  __/ *
//*   \_/\_/ |_| |_|_|\__\___||___/ .__/ \__,_|\___\___| *
//*                               |_|                    *
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
#ifndef PEEJAY_MATCHERS_WHITESPACE_HPP
#define PEEJAY_MATCHERS_WHITESPACE_HPP

#include "peejay/concepts.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

template <backend Backend> class whitespace_matcher {
public:
  using parser_type = parser<Backend>;

  static bool whitespace(parser_type &p, char32_t c) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      p.push_whitespace_matcher();
      return true;
    }
    return false;
  }

  static bool consume(parser_type &parser, char32_t c) {
    switch (parser.stack_.top()) {
      // Handles the LF part of a Windows-style CR/LF pair.
    case state::whitespace_crlf:
      if (whitespace_matcher::crlf(parser, c)) {
        return true;
      }
      [[fallthrough]];
    case state::whitespace_start: return whitespace_matcher::body(parser, c);
    default: unreachable(); break;
    }
  }

  static void eof(parser_type &parser) { parser.pop(); }

private:
  static bool body(parser_type &parser, char32_t c) {
    switch (c) {
    case ' ':
    case '\t':
      // TODO(paul) tab expansion.
      break;
    case '\n': parser.advance_row(); break;
    case '\r':
      parser.advance_row();
      parser.stack_.top() = state::whitespace_crlf;
      break;
    default:
      // Stop, pop this matcher, and retry with the same character.
      parser.pop();
      return false;
    }
    return true;  // Consume this character.
  }

  /// Processes the second character of a Windows-style CR/LF pair. Returns true
  /// if the character shoud be treated as whitespace.
  static bool crlf(parser_type &parser, char32_t c) {
    parser.stack_.top() = state::whitespace_start;
    if (c != '\n') {
      return false;
    }
    parser.reset_column();
    return true;
  }
};

template <backend Backend> bool whitespace(parser<Backend> &p, char32_t c) {
  return whitespace_matcher<Backend>::whitespace(p, c);
}

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_WHITESPACE_HPP
