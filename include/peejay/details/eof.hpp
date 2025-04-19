//===- include/peejay/details/eof.hpp ---------------------*- mode: C++ -*-===//
//*              __  *
//*   ___  ___  / _| *
//*  / _ \/ _ \| |_  *
//* |  __/ (_) |  _| *
//*  \___|\___/|_|   *
//*                  *
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
#ifndef PEEJAY_DETAILS_EOF_HPP
#define PEEJAY_DETAILS_EOF_HPP

#include <optional>

#include "peejay/concepts.hpp"
#include "peejay/details/whitespace.hpp"
#include "peejay/error.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

/// Matches the end of the input.
template <backend Backend> class eof_matcher {
public:
  constexpr static bool consume(parser<Backend> &parser, std::optional<char32_t> ch) {
    if (ch) {
      // Allow whitespace and only whitespace between the top-level object and the end of input.
      if (whitespace(parser, *ch)) {
        return false;
      }
      parser.set_error(error::unexpected_extra_input);
    }
    parser.pop();
    return true;
  }
};

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_EOF_HPP
