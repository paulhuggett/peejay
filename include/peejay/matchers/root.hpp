//===- include/peejay/matchers/root.hpp -------------------*- mode: C++ -*-===//
//*                  _    *
//*  _ __ ___   ___ | |_  *
//* | '__/ _ \ / _ \| __| *
//* | | | (_) | (_) | |_  *
//* |_|  \___/ \___/ \__| *
//*                       *
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
#ifndef PEEJAY_MATCHERS_ROOT_HPP
#define PEEJAY_MATCHERS_ROOT_HPP

#include <optional>

#include "peejay/concepts.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/error.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

template <backend Backend> class root_matcher {
public:
  using policies = typename std::remove_reference_t<Backend>::policies;

  static bool consume(parser<Backend> &parser, std::optional<char32_t> ch) {
    if (!ch) {
      parser.set_error(error::expected_token);
      return true;
    }
    auto const c = *ch;
    switch (parser.stack_.top()) {
    case state::root_start:
      parser.stack_.top() = state::root_new_token;
      if (whitespace_matcher<Backend>::whitespace(parser, c)) {
        return false;
      }
      [[fallthrough]];
    case state::root_new_token:
      parser.pop();
      if (c == '-' || (c >= '0' && c <= '9')) {
        parser.push_number_matcher();
      } else {
        switch (c) {
        case '"': parser.push_string_matcher(false); break;
        case 't': parser.push_token_matcher(token::true_token); break;
        case 'f': parser.push_token_matcher(token::false_token); break;
        case 'n': parser.push_token_matcher(token::null_token); break;
        case '[': parser.push_array_matcher(); break;
        case '{': parser.push_object_matcher(); break;
        default: parser.set_error(error::expected_token); break;
        }
      }
      return false;
    default: unreachable(); break;
    }
    unreachable();
  }
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_ROOT_HPP
