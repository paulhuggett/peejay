//===- include/peejay/parser.hpp --------------------------*- mode: C++ -*-===//
//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
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
#ifndef PEEJAY_DETAILS_PARSER_HPP
#define PEEJAY_DETAILS_PARSER_HPP

#include <cassert>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <stack>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>

#include "peejay/concepts.hpp"
#include "peejay/details/arrayvec.hpp"
#include "peejay/details/states.hpp"
#include "peejay/icubaby.hpp"

namespace peejay {

namespace details {

enum class token : std::uint8_t { false_token, true_token, null_token };

template <backend Backend> class array_matcher;
template <backend Backend> class eof_matcher;
template <backend Backend> class number_matcher;
template <backend Backend> class object_matcher;
template <backend Backend> class root_matcher;
template <backend Backend> class string_matcher;
template <backend Backend> class token_matcher;
template <backend Backend> class whitespace_matcher;

template <details::group Group, backend Backend> struct group_to_matcher {};
template <backend Backend> struct group_to_matcher<details::group::array, Backend> {
  using type = array_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::eof, Backend> {
  using type = eof_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::number, Backend> {
  using type = number_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::object, Backend> {
  using type = object_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::root, Backend> {
  using type = root_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::string, Backend> {
  using type = string_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::token, Backend> {
  using type = token_matcher<Backend>;
};
template <backend Backend> struct group_to_matcher<details::group::whitespace, Backend> {
  using type = whitespace_matcher<Backend>;
};
template <details::group Group, backend Backend> using group_to_matcher_t = group_to_matcher<Group, Backend>::type;

}  // end namespace details

template <bool> struct coord {};
template <> struct coord<true> {
  friend constexpr std::strong_ordering operator<=>(coord const &, coord const &) noexcept = default;
  friend std::ostream &operator<<(coord<true> const &lhs, std::ostream &os) { return os << lhs.to_string(); }
  [[nodiscard]] std::string to_string() const { return std::format("({}:{})", line, column); }
  unsigned line = 1U;
  unsigned column = 1U;
};

struct default_policies {
  /// The maximum length of strings that will be permitted before
  /// a string_too_long is raised.
  static constexpr std::size_t max_length = 64;
  static constexpr std::size_t max_stack_depth = 8;
  /// Should the library track the position (that is, the line and column number
  /// in the input). This can be important for detailed error reporting but can
  /// be unnecessary in some applications.
  static constexpr bool pos_tracking = true;

  using float_type = double;
  using integer_type = std::int64_t;
  using char_type = char8_t;
};

//*                              *
//*  _ __  __ _ _ _ ___ ___ _ _  *
//* | '_ \/ _` | '_(_-</ -_) '_| *
//* | .__/\__,_|_| /__/\___|_|   *
//* |_|                          *
/// \tparam Backend   A type meeting the backend<> requirements. The backend
///                   instance's member functions are invoked as the parser
///                   encounters the contents of the input file.
/// \tparam Policies  A type which contains a collection of policies which
///                   control the behaviour of the parser.
template <backend Backend> class parser {
public:
  using policies = typename std::remove_reference_t<Backend>::policies;

  constexpr parser() : parser(Backend{}) {}
  constexpr parser(parser const &other) = default;
  constexpr parser(parser &&other) noexcept = default;
  explicit parser(std::remove_reference_t<Backend> &backend) : backend_{backend} { this->init_stack(); }
  explicit parser(std::remove_reference_t<Backend> &&backend) : backend_{std::move(backend)} { this->init_stack(); }
  constexpr ~parser() noexcept = default;

  parser &operator=(parser const &other) = default;
  parser &operator=(parser &&other) noexcept = default;

  /// Parses a chunk of JSON input. This function may be called repeatedly with
  /// portions of the source data (for example, as the data is received from an
  /// external source). Once all of the data has been received, call the
  /// parser::eof() method.
  template <std::ranges::input_range Range>
    requires(std::is_same_v<typename std::ranges::range_value_t<Range>, typename parser<Backend>::policies::char_type>)
  parser &input(Range const &range) {
    if (error_) {
      return *this;
    }
    std::array<char32_t, 2> code_points{{0}};
    auto first = std::begin(range);                         // NOLINT(llvm-qualified-auto, readability-qualified-auto)
    auto const last = std::end(range);                      // NOLINT(llvm-qualified-auto, readability-qualified-auto)
    auto const first_code_point = std::begin(code_points);  // NOLINT(llvm-qualified-auto, readability-qualified-auto)
    while (first != last && !error_) {
      auto const last_code_point = utf_(*first, first_code_point);
      ++first;
      std::for_each(first_code_point, last_code_point, [this](char32_t const code_point) {
        if (!error_) {
          this->consume_code_point(code_point);
        }
        if (!error_) {
          this->advance_column();
        }
      });
    }
    return *this;
  }

  /// Informs the parser that the complete input stream has been passed by calls
  /// to parser<>::input(). Brings the parser to the completed state to ensure that
  /// complete objects have been consumed.
  ///
  /// \returns The result of calling the backend object's result() member.
  decltype(auto) eof();

  ///@{

  /// \returns True if the parser has signalled an error.
  [[nodiscard]] constexpr bool has_error() const noexcept { return bool{error_}; }
  /// \returns The error code held by the parser.
  [[nodiscard]] constexpr std::error_code const &last_error() const noexcept { return error_; }
  ///@}

  ///@{
  [[nodiscard]] constexpr Backend &backend() & noexcept { return backend_; }
  [[nodiscard]] constexpr Backend const &backend() const & noexcept { return backend_; }
  [[nodiscard]] constexpr Backend &&backend() && noexcept { return std::move(backend_); }
  ///@}

  ///@{
  /// Returns the parser's position in the input text. If input position tracking is disabled,
  /// via Policies::pos_tracking, the returned object has no members.
  [[nodiscard]] constexpr coord<policies::pos_tracking> input_pos() const noexcept { return pos_; }
  /// Returns the position of the most recent token in the input text. If input position tracking is disabled,
  /// via Policies::pos_tracking, the returned object has no members.
  [[nodiscard]] constexpr coord<policies::pos_tracking> pos() const noexcept { return matcher_pos_; }
  ///@}

private:
  using array_matcher = details::array_matcher<Backend>;
  using eof_matcher = details::eof_matcher<Backend>;
  using number_matcher = details::number_matcher<Backend>;
  using root_matcher = details::root_matcher<Backend>;
  using string_matcher = details::string_matcher<Backend>;
  using object_matcher = details::object_matcher<Backend>;
  using token_matcher = details::token_matcher<Backend>;
  using whitespace_matcher = details::whitespace_matcher<Backend>;

  friend array_matcher;
  friend eof_matcher;
  friend number_matcher;
  friend object_matcher;
  friend root_matcher;
  friend string_matcher;
  friend token_matcher;
  friend whitespace_matcher;

  /// Records an error for this parse. The parse will stop as soon as a non-zero
  /// error code is recorded. An error may be reported at any time during the
  /// parse; all subsequent text is ignored.
  ///
  /// \param err  The json error code to be stored in the parser.
  bool set_error(std::error_code const &err) noexcept {
    assert(!error_ || err);
    error_ = err;
    return this->has_error();
  }
  ///@{
  /// \brief Managing the column and row number.

  /// Resets the column count but does not affect the row number.
  void reset_column() noexcept {
    if constexpr (policies::pos_tracking) {
      pos_.column = 0U;
    }
  }

  /// Increments the column number.
  void advance_column() noexcept {
    if constexpr (policies::pos_tracking) {
      ++pos_.column;
    }
  }

  /// Increments the row number and resets the column.
  void advance_row() noexcept {
    if constexpr (policies::pos_tracking) {
      // The column number is set to 0. This is because the outer parse loop
      // automatically advances the column number for each character consumed.
      // This happens after the row is advanced by a matcher's consume() function.
      pos_.column = 0U;
      ++pos_.line;
    }
  }
  ///@}

  ///@{
  /// \brief Managing the parse stack.
  void push(details::state next_state);
  template <details::state NextState, typename... Args> void push_terminal(Args &&...args);

  void set_state(details::state state) {
    assert(!stack_.empty());
    assert(get_group(stack_.top()) == get_group(state));
    stack_.top() = state;
  }
  void pop();

  void push_number_matcher() { push_terminal<details::state::number_start>(); }
  void push_string_matcher(bool const object_key) { push_terminal<details::state::string_start>(object_key); }
  void push_token_matcher(details::token const t) { push_terminal<details::state::token_start>(t); }

  void push_root_matcher() { push(details::state::root_start); }
  void push_whitespace_matcher() { push(details::state::whitespace_start); }
  void push_array_matcher() { push(details::state::array_start); }
  void push_object_matcher() { push(details::state::object_start); }
  void push_eof_matcher() { push(details::state::eof_start); }
  ///@}

  void consume_code_point(std::optional<char32_t> code_point);

  void init_stack() {
    // The EOF matcher is placed at the bottom of the stack to ensure that the
    // input JSON ends after a single top-level object.
    this->push_eof_matcher();
    // Match a top-level object.
    this->push_root_matcher();
  }

  icubaby::t8_32 utf_;

  /// The parse stack.
  std::stack<details::state, arrayvec<details::state, policies::max_stack_depth>> stack_;
  std::variant<std::monostate, string_matcher, number_matcher, token_matcher> storage_;
  std::error_code error_;

  /// The column and row number of the parse within the input stream.
  [[no_unique_address]] coord<policies::pos_tracking> pos_;
  /// The column and row number of the most recent token consumed from the input.
  [[no_unique_address]] coord<policies::pos_tracking> matcher_pos_;
  [[no_unique_address]] Backend backend_;
};

template <backend Backend> explicit parser(parser<Backend>) -> parser<Backend>;
template <backend Backend> explicit parser(Backend &&backend) -> parser<Backend>;

template <backend Backend> decltype(auto) make_parser(Backend &&backend) {
  return parser<Backend>(std::forward<Backend>(backend));
}

}  // end namespace peejay

#include "peejay/details/parser_impl.hpp"
#include "peejay/matchers/array.hpp"
#include "peejay/matchers/eof.hpp"
#include "peejay/matchers/number.hpp"
#include "peejay/matchers/object.hpp"
#include "peejay/matchers/root.hpp"
#include "peejay/matchers/string.hpp"
#include "peejay/matchers/token.hpp"
#include "peejay/matchers/whitespace.hpp"

#endif  // PEEJAY_DETAILS_PARSER_HPP
