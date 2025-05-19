//===- include/peejay/matchers/token.hpp ------------------*- mode: C++ -*-===//
//*  _        _               *
//* | |_ ___ | | _____ _ __   *
//* | __/ _ \| |/ / _ \ '_ \  *
//* | || (_) |   <  __/ | | | *
//*  \__\___/|_|\_\___|_| |_| *
//*                           *
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
/// \tparam Backend  The parser callback structure.
template <backend Backend> class token_matcher {
public:
  /// \param t  Which of the tokens is to be consumed.
  constexpr explicit token_matcher(token const t) noexcept : token_{t} {
    switch (t) {
    case token::true_token: text_ = u8"rue"; break;
    case token::false_token: text_ = u8"alse"; break;
    case token::null_token: text_ = u8"ull"; break;
    default: unreachable(); break;
    }
  }

  bool consume(parser<Backend> &parser, char32_t ch) {
    assert(!text_.empty() && "Input text must not be empty");
    if (auto const c = text_.front(); ch != static_cast<char32_t>(c)) {
      return parser.set_error_and_pop(error::unrecognized_token);
    }
    text_.remove_prefix(1);
    if (text_.empty()) {
      auto &backend = parser.backend();
      std::error_code err{};
      switch (token_) {
      case token::true_token: err = backend.boolean_value(true); break;
      case token::false_token: err = backend.boolean_value(false); break;
      case token::null_token: err = backend.null_value(); break;
      default: unreachable(); break;
      }
      parser.set_error(err);
      parser.pop();  // unconditionally pop this matcher.
    }
    return true;
  }

  void eof(parser<Backend> &parser) { parser.set_error_and_pop(error::unrecognized_token); }

private:
  /// The keyword to be matched. The input sequence must exactly match this
  /// string or an unrecognized token error is raised. Once all of the
  /// characters are matched, the function derived from which_ is called.
  std::u8string_view text_;
  /// This function is called once the complete token text has been matched.
  token token_;
};

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_TOKEN_HPP
