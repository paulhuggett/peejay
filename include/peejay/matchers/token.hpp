//===- include/peejay/matchers/token.hpp ------------------*- mode: C++ -*-===//
//*  _        _               *
//* | |_ ___ | | _____ _ __   *
//* | __/ _ \| |/ / _ \ '_ \  *
//* | || (_) |   <  __/ | | | *
//*  \__\___/|_|\_\___|_| |_| *
//*                           *
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
/// \file token.hpp
/// \brief Provides the matcher for the keywords "true", "false", and "null".
#ifndef PEEJAY_MATCHERS_TOKEN_HPP
#define PEEJAY_MATCHERS_TOKEN_HPP

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

/// \brief A matcher which checks for a specific keyword such as "true",
///   "false", or "null".
///
/// Once a token has been recognized, the matcher invokes the relevant backend function ---
/// boolean_value() or null_value() --- then removes itself from the parser's parse stack and
/// self-destructs.
///
/// \tparam Backend  The parser callback structure.
template <backend Backend> class token_matcher {
public:
  /// \param t  Which of the tokens is to be consumed. The first character of the token has already been consumed.
  constexpr explicit token_matcher(token const t) noexcept : token_{t} {
    using namespace std::literals::string_view_literals;
    switch (t) {
    case token::true_token: text_ = u8"rue"sv; break;
    case token::false_token: text_ = u8"alse"sv; break;
    case token::null_token: text_ = u8"ull"sv; break;
    default: unreachable(); break;
    }
  }

  /// Consumes a code unit. If it does not match the next character of the token, error::unrecognized_token is
  /// raised.
  ///
  /// \param parser  The owning parser instance.
  /// \param code_unit  A UTF-8 code unit from the input.
  /// \returns True.
  bool consume(parser<Backend>& parser, char8_t const code_unit) {
    assert(!parser.has_error() && "The parser should not be in an error state here");
    assert(!text_.empty() && "Input text must not be empty");
    if (text_.empty() || code_unit != text_.front()) {
      return parser.set_error_and_pop(error::unrecognized_token);
    }
    text_.remove_prefix(1);
    if (text_.empty()) {
      this->end(parser);
    }
    return true;
  }

  /// Signals the end of the input. The token is incomplete so error::unrecognized_token is raised.
  /// \param parser  The owning parser instance.
  void eof(parser<Backend> &parser) { parser.set_error_and_pop(error::unrecognized_token); }

private:
  /// The remaining characters of the keyword to be matched. The input sequence must exactly match this
  /// string or an unrecognized token error is raised. Once all of the
  /// characters are matched, the backend function derived from token_ is called.
  std::u8string_view text_;
  /// The token being matched. This determines the backend function that is called once the complete token text
  /// has been matched.
  token token_;

  /// When called, we have successfully processed the input text to the end of the specified token.
  /// Call the relevant backend function to signal the token then self-destruct this object,
  /// removing it from the parse stack.
  /// \param parser The owning parser instance
  void end(parser<Backend>& parser) const {
    auto& backend = parser.backend();
    std::error_code err{};
    switch (token_) {
    case token::true_token: err = backend.boolean_value(true); break;
    case token::false_token: err = backend.boolean_value(false); break;
    case token::null_token: err = backend.null_value(); break;
    default: unreachable(); break;
    }
    parser.set_error(err);
    // Unconditionally pop this matcher. This instance is destroyed.
    parser.pop();
  }
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_TOKEN_HPP
