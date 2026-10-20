//===- include/peejay/matchers/eof.hpp --------------------*- mode: C++ -*-===//
//*              __  *
//*   ___  ___  / _| *
//*  / _ \/ _ \| |_  *
//* |  __/ (_) |  _| *
//*  \___|\___/|_|   *
//*                  *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
/// \file eof.hpp
/// \brief Provides the matcher for the end of the input.
#ifndef PEEJAY_MATCHERS_EOF_HPP
#define PEEJAY_MATCHERS_EOF_HPP

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

/// Matches the end of the input. Only whitespace is permitted between the top-level value and the end of the input.
///
/// \tparam Backend  The parser backend type.
template <backend Backend> class eof_matcher {
public:
  /// Consumes a code unit. Whitespace is consumed; any other input raises error::unexpected_extra_input.
  ///
  /// \param parser  The owning parser instance.
  /// \param code_unit  A UTF-8 code unit from the input.
  /// \returns True if \p code_unit was consumed, false if it should be passed to the matcher which is now on top
  ///   of the parse stack.
  constexpr static bool consume(parser<Backend>& parser, char8_t const code_unit) {
    // Allow whitespace and only whitespace between the top-level object and the end of input.
    if (whitespace(parser, code_unit)) {
      return false;
    }
    return parser.set_error_and_pop(error::unexpected_extra_input);
  }

  /// Signals the end of the input. The matcher is popped from the parse stack.
  /// \param parser  The owning parser instance.
  constexpr static void eof(parser<Backend> &parser) { parser.pop(); }
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_EOF_HPP
