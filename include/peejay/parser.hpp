//===- include/peejay/parser.hpp --------------------------*- mode: C++ -*-===//
//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
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
/// \file parser.hpp
/// \brief Provides peejay::parser, the PJ JSON parser.
#ifndef PEEJAY_DETAILS_PARSER_HPP
#define PEEJAY_DETAILS_PARSER_HPP

#include <cassert>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <ranges>
#include <stack>
#include <string>
#include <system_error>
#include <type_traits>
#include <variant>
#include <version>

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>
#endif

// peejay local includes
#include "peejay/concepts.hpp"
#include "peejay/details/arrayvec.hpp"
#include "peejay/details/states.hpp"
#include "peejay/details/variant.hpp"

namespace peejay {

namespace details {

/// The keywords recognized by token_matcher.
enum class token : std::uint8_t { false_token, true_token, null_token };

template <backend Backend> class array_matcher;
template <backend Backend> class eof_matcher;
template <backend Backend> class number_matcher;
template <backend Backend> class object_matcher;
template <backend Backend> class root_matcher;
template <backend Backend> class string_matcher;
template <backend Backend> class token_matcher;
template <backend Backend> class whitespace_matcher;

/// Maps a parse-state group to the type of the matcher which handles it.
///
/// \tparam Group  The state group.
/// \tparam Backend  The parser backend type.
template <group Group, backend Backend> struct group_to_matcher {};
/// Maps group::array to array_matcher.
template <backend Backend> struct group_to_matcher<group::array, Backend> {
  /// The matcher type.
  using type = array_matcher<Backend>;
};
/// Maps group::eof to eof_matcher.
template <backend Backend> struct group_to_matcher<group::eof, Backend> {
  /// The matcher type.
  using type = eof_matcher<Backend>;
};
/// Maps group::number to number_matcher.
template <backend Backend> struct group_to_matcher<group::number, Backend> {
  /// The matcher type.
  using type = number_matcher<Backend>;
};
/// Maps group::object to object_matcher.
template <backend Backend> struct group_to_matcher<group::object, Backend> {
  /// The matcher type.
  using type = object_matcher<Backend>;
};
/// Maps group::root to root_matcher.
template <backend Backend> struct group_to_matcher<group::root, Backend> {
  /// The matcher type.
  using type = root_matcher<Backend>;
};
/// Maps group::string to string_matcher.
template <backend Backend> struct group_to_matcher<group::string, Backend> {
  /// The matcher type.
  using type = string_matcher<Backend>;
};
/// Maps group::token to token_matcher.
template <backend Backend> struct group_to_matcher<group::token, Backend> {
  /// The matcher type.
  using type = token_matcher<Backend>;
};
/// Maps group::whitespace to whitespace_matcher.
template <backend Backend> struct group_to_matcher<group::whitespace, Backend> {
  /// The matcher type.
  using type = whitespace_matcher<Backend>;
};
/// A helper type for convenient use of group_to_matcher<Group, Backend>::type.
template <group Group, backend Backend> using group_to_matcher_t = group_to_matcher<Group, Backend>::type;

}  // end namespace details

/// \brief A position within the input text.
///
/// This primary template is used when position tracking is disabled (see policy::pos_tracking) and has no members.
///
/// \tparam Enabled  True if position tracking is enabled.
template <bool Enabled> struct coord {};
/// \brief A position within the input text: used when position tracking is enabled.
template <> struct coord<true> {
  /// Compares two coordinates.
  friend constexpr std::strong_ordering operator<=>(coord const &, coord const &) noexcept = default;

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
  /// Writes a coordinate to an output stream in the form "(line)".
  /// \param lhs  The coordinate to be written.
  /// \param os  The output stream.
  /// \returns \p os.
  friend std::ostream &operator<<(std::ostream &os, coord<true> const &c) { return os << c.to_string(); }
  /// \returns The coordinate as a string in the form "(line)".
  [[nodiscard]] std::string to_string() const { return std::format("({})", line); }
#else
  /// Writes a coordinate to an output stream in the form "(line)".
  /// \param lhs  The coordinate to be written.
  /// \param os  The output stream.
  /// \returns \p os.
  friend std::ostream &operator<<(std::ostream &os, coord<true> const &c) { return os << c.to_string(); }
  [[nodiscard]] std::string to_string() const { return std::format("({})", line); }
#endif
  /// The line number.
  unsigned line = 1U;
};

/// \brief The default parser policies. These satisfy the peejay::policy concept.
struct default_policies {
  /// The maximum length of strings that will be permitted before
  /// a string_too_long is raised.
  static constexpr std::size_t max_length = 64;
  /// The maximum depth of the parse stack. Exceeding this depth raises error::nesting_too_deep.
  static constexpr std::size_t max_stack_depth = 8;
  /// Should the library track the position (that is, the line number in the
  /// input). This can be important for detailed error reporting but can
  /// be unnecessary in some applications.
  static constexpr bool pos_tracking = true;

  /// Representation of floating-point numbers. Set to peejay::no_float_type
  /// to disable support altogether
  using float_type = double;
  /// Representation of integers. Select a suitable signed type for the
  /// expected input and target hardware.
  using integer_type = int;
};

//*                              *
//*  _ __  __ _ _ _ ___ ___ _ _  *
//* | '_ \/ _` | '_(_-</ -_) '_| *
//* | .__/\__,_|_| /__/\___|_|   *
//* |_|                          *
/// \brief The PJ JSON parser.
///
/// The parser is a push parser: input is supplied (in as many chunks as necessary) by calls to input(). As the
/// input is consumed, the parser calls the member functions of the backend object. Once all of the input has been
/// supplied, eof() completes the parse and returns the backend's result.
///
/// \tparam Backend   A type meeting the backend<> requirements. The backend
///                   instance's member functions are invoked as the parser
///                   encounters the contents of the input file.
template <backend Backend> class parser {
public:
  /// The parser policies, taken from the backend type.
  using policies = std::remove_reference_t<Backend>::policies;

  /// Constructs a parser with a default-constructed backend.
  constexpr parser() : parser(Backend{}) {}
  /// Copy constructor.
  /// \param other  The parser to be copied.
  constexpr parser(parser const &other) = default;
  /// Move constructor.
  /// \param other  The parser to be moved.
  constexpr parser(parser &&other) noexcept = default;
  /// Constructs a parser with a copy of the given backend.
  /// \param backend  The backend to be copied.
  explicit parser(std::remove_reference_t<Backend> &backend) : backend_{backend} { this->init_stack(); }
  /// Constructs a parser from the given backend.
  /// \param backend  The backend to be moved into the parser.
  explicit parser(std::remove_reference_t<Backend> &&backend) : backend_{std::move(backend)} { this->init_stack(); }
  /// Destructor.
  ~parser() noexcept {
    // Pop any states that may remain on the stack.
    while (!stack_.empty()) {
      this->pop();
    }
  }

  /// Copy assignment.
  /// \param other  The parser to be copied.
  /// \returns *this.
  parser &operator=(parser const &other) = default;
  /// Move assignment.
  /// \param other  The parser to be moved.
  /// \returns *this.
  parser &operator=(parser &&other) noexcept = default;

  /// Parses a chunk of JSON input. This function may be called repeatedly with
  /// portions of the source data (for example, as the data is received from an
  /// external source). Once all the data has been received, call the
  /// parser::eof() method. If the parser is in an error state, the input is
  /// ignored.
  ///
  /// \tparam Range An input range that yields char8_t.
  /// \param range  The range of UTF-8 code units to be processed.
  /// \returns *this
  template <std::ranges::input_range Range>
    requires(std::is_same_v<std::ranges::range_value_t<Range>, char8_t>)
  parser& input(Range const& range) {
    if (error_) {
      return *this;
    }
    for (char8_t const cu : range) {
      this->consume_code_unit(cu);
      if (error_) {
        break;
      }
    }
    return *this;
  }

  /// Informs the parser that the complete input stream has been passed by calls
  /// to parser<>::input(). Brings the parser to the completed state to ensure that
  /// complete objects have been consumed. The parser is then reset so that it
  /// may be used to parse a further input stream.
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
  /// \returns The backend object.
  [[nodiscard]] constexpr Backend &backend() & noexcept { return backend_; }
  /// \returns The backend object.
  [[nodiscard]] constexpr Backend const &backend() const & noexcept { return backend_; }
  /// \returns The backend object, moved from the parser.
  [[nodiscard]] constexpr Backend &&backend() && noexcept { return std::move(backend_); }
  ///@}

  ///@{
  /// Returns the parser's position in the input text. If input position tracking is disabled,
  /// via policies::pos_tracking, the returned object has no members.
  /// \returns The current position in the input text.
  [[nodiscard]] constexpr coord<policies::pos_tracking> input_pos() const noexcept { return pos_; }
  /// Returns the position of the most recent token in the input text. If input position tracking is disabled,
  /// via policies::pos_tracking, the returned object has no members.
  /// \returns The position of the most recent token.
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
  /// An existing error cannot be cleared: if the parser is already in an error
  /// state, \p err must also represent an error.
  ///
  /// \param err  The json error code to be stored in the parser.
  /// \returns True if \p err represented an error condition, false otherwise.
  bool set_error(std::error_code const &err) noexcept {
    assert(!error_ || err);
    error_ = err;
    return this->has_error();
  }
  /// Records an error for this parse (see set_error()) and, if \p err represents
  /// an error, pops the top-most matcher from the parse stack.
  ///
  /// \param err  The json error code to be stored in the parser.
  /// \returns True if \p err represented an error condition, false otherwise.
  bool set_error_and_pop(std::error_code const &err) noexcept {
    bool const res = this->set_error(err);
    if (res) {
      this->pop();
    }
    return res;
  }
  /// Increments the row number.
  void advance_row() noexcept {
    if constexpr (policies::pos_tracking) {
      ++pos_.line;
    }
  }

  /// \name Managing the parse stack
  ///@{

  /// Pushes a new state onto the parse stack. If the stack is full, error::nesting_too_deep is
  /// recorded and the current top of stack is popped.
  ///
  /// \param next_state  The state to be pushed.
  /// \returns True if the state was pushed, false if the stack was full.
  bool push(details::state next_state);
  /// Pushes a new state onto the parse stack and constructs the matcher which handles it
  /// (one which carries additional state) in the parser's matcher storage.
  ///
  /// \tparam NextState  The state to be pushed.
  /// \tparam Args  The types of the arguments forwarded to the matcher's constructor.
  /// \param args  Arguments forwarded to the matcher's constructor.
  template <details::state NextState, typename... Args> void push_terminal(Args &&...args);

  /// \returns The state on top of the parse stack.
  /// \pre  The stack must contain at least one element.
  [[nodiscard]] constexpr details::state get_state() const noexcept {
    assert(!stack_.empty());
    return stack_.top();
  }

  /// Replaces the state on top of the parse stack.
  /// \pre  The stack must contain at least one element.
  /// \param state  The new state. This must belong to the same group as the state that it replaces.
  void set_state(details::state const state) noexcept {
    assert(!stack_.empty());
    assert(get_group(stack_.top()) == get_group(state));
    stack_.top() = state;
  }
  /// Remove the top-most match from the parse stack.
  /// \pre  The stack must contain at least one element
  void pop() noexcept;

  /// Pushes a matcher for a number.
  void push_number_matcher() { push_terminal<details::state::number_start>(); }
  /// Pushes a matcher for a string.
  /// \param object_key  True if the string is an object key, false if it is a value.
  void push_string_matcher(bool const object_key) { push_terminal<details::state::string_start>(object_key); }
  /// Pushes a matcher for one of the keywords "true", "false", or "null".
  /// \param t  The keyword to be matched.
  void push_token_matcher(details::token const t) { push_terminal<details::state::token_start>(t); }

  /// Pushes a matcher for any JSON value.
  void push_root_matcher() { push(details::state::root_start); }
  /// Pushes a matcher for whitespace.
  void push_whitespace_matcher() { push(details::state::whitespace_start); }
  /// Pushes a matcher for an array.
  void push_array_matcher() { push(details::state::array_start); }
  /// Pushes a matcher for an object.
  void push_object_matcher() { push(details::state::object_start); }
  /// Pushes a matcher for the end of the input.
  void push_eof_matcher() { push(details::state::eof_start); }
  ///@}

  /// Passes a single code unit to the matcher(s) on the parse stack until it is consumed or an error is raised.
  /// \param code_unit  The UTF-8 code unit to be consumed.
  void consume_code_unit(char8_t code_unit);
  /// Signals the end of the input to the matchers on the parse stack until the stack is empty or an error is
  /// raised.
  void consume_eof();

  /// Initializes the parse stack ready to match a single top-level value.
  void init_stack() {
    // The EOF matcher is placed at the bottom of the stack to ensure that the
    // input JSON ends after a single top-level object.
    this->push_eof_matcher();
    // Match a top-level object.
    this->push_root_matcher();
  }

  /// The parse stack.
  std::stack<details::state, arrayvec<details::state, policies::max_stack_depth>> stack_;
  /// Storage for the active matcher which carries state of its own (if any).
  details::variant<type_list::type_list<string_matcher, number_matcher, token_matcher>> storage_;
  /// The error (if any) raised by the parse.
  std::error_code error_;

  /// The row number of the parse within the input stream.
  [[no_unique_address]] coord<policies::pos_tracking> pos_;
  /// The row number of the most recent token consumed from the input.
  [[no_unique_address]] coord<policies::pos_tracking> matcher_pos_;
  /// The backend object which receives notifications from the parser.
  [[no_unique_address]] Backend backend_;
};

/// Deduction guide for copying or moving a parser.
template <backend Backend> explicit parser(parser<Backend>) -> parser<Backend>;
/// Deduction guide for constructing a parser from a backend.
template <backend Backend> explicit parser(Backend &&backend) -> parser<Backend>;

/// Creates a parser for the given backend.
///
/// \tparam Backend  A type meeting the backend<> requirements.
/// \param backend  The backend object which will receive notifications from the parser. It is forwarded to the
///   parser's constructor.
/// \returns A new parser instance.
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
