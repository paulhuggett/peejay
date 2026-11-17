//===- include/peejay/matchers/string.hpp -----------------*- mode: C++ -*-===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
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
/// \file string.hpp
/// \brief Provides the matcher for a JSON string.
#ifndef PEEJAY_MATCHERS_STRING_HPP
#define PEEJAY_MATCHERS_STRING_HPP

#include <system_error>
#include <type_traits>

// Local includes
#include "peejay/concepts.hpp"
#include "peejay/details/portab.hpp"
#include "peejay/details/string_accumulator.hpp"
#include "peejay/error.hpp"

#ifndef PEEJAY_DETAILS_PARSER_HPP
#include "peejay/parser.hpp"
#endif
#ifndef PEEJAY_DETAILS_STATES_HPP
#include "peejay/details/states.hpp"
#endif

namespace peejay::details {

/// Matches a string. The opening quote has already been consumed when the matcher is pushed. Once the closing quote
/// is consumed, the string is passed to the backend's key() or string_value() member function.
///
/// The characters of the string are accumulated, and its escape sequences decoded, by string_accumulator. That
/// class depends only on the policies' max_length, so parsers whose backends have the same policies share its code.
/// This class is the thin layer which connects it to the parser and the backend.
///
/// \tparam Backend  The parser backend type.
template <backend Backend> class string_matcher {
public:
  /// The type of the owning parser.
  using parser_type = parser<Backend>;
  /// The parser policies.
  using policies = std::remove_reference_t<Backend>::policies;

  /// \param is_key  True if this string is an object's property name, false if it is a value.
  constexpr explicit string_matcher(bool const is_key) noexcept : is_key_{is_key} {}

  /// Consumes a code unit.
  ///
  /// \param parser  The owning parser instance.
  /// \param code_unit  A UTF-8 code unit from the input.
  /// \returns True if \p code_unit was consumed, false if it should be passed to the matcher which is now on top
  ///   of the parse stack.
  bool consume(parser_type& parser, char8_t code_unit);
  /// Signals the end of the input. The string is incomplete so error::expected_close_quote is raised.
  /// \param parser  The owning parser instance.
  void eof(parser_type &parser);

private:
  /// The type of the object which accumulates the string.
  using accumulator = string_accumulator<policies::max_length>;

  /// True if this string is an object's property name, false otherwise.
  bool is_key_;
  /// The string seen so far.
  accumulator acc_;
};

// consume
// ~~~~~~~
template <backend Backend>
PEEJAY_ALWAYS_INLINE bool string_matcher<Backend>::consume(parser_type& parser, char8_t const code_unit) {
  auto const current = parser.get_state();
  // Most characters are ordinary ones in the body of the string: handle those without a call.
  if (current == state::string_normal_char && acc_.append_plain(code_unit)) {
    return true;
  }
  auto const code = acc_.consume(current, code_unit);
  if (accumulator::is_state(code)) {
    // Every character which leaves the state unchanged was handled by the fast path above.
    parser.set_state(static_cast<state>(code));
    return true;
  }
  if (auto const err = static_cast<error>(code); err != error::none) {
    return parser.set_error_and_pop(err);
  }
  // The closing quote: pass the string to the backend.
  auto &backend = parser.backend();
  auto const result = acc_.str();
  parser.set_error(is_key_ ? backend.key(result) : backend.string_value(result));
  parser.pop();  // unconditionally pop this matcher.
  return true;
}

// eof
// ~~~
template <backend Backend> void string_matcher<Backend>::eof(parser_type &parser) {
  parser.set_error_and_pop(error::expected_close_quote);
}

}  // end namespace peejay::details

#endif  // PEEJAY_MATCHERS_STRING_HPP
