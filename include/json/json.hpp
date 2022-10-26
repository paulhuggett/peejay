//===- include/json/json.hpp ------------------------------*- mode: C++ -*-===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_JSON_HPP
#define PEEJAY_JSON_HPP

// Standard library
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <memory>
#include <optional>
#include <ostream>
#include <stack>
#include <string>
#include <tuple>
#include <variant>

// C++20 standard library
#if __cplusplus >= 202002L
#include <concepts>
#include <span>
#endif

#include "json/json_error.hpp"
#include "json/utf.hpp"

namespace peejay {

template <typename T>
constexpr std::optional<
    typename std::remove_const_t<typename std::remove_reference_t<T>>>
just (T &&t) {
  return {std::forward<T> (t)};
}
template <typename T>
constexpr std::optional<T> nothing () {
  return {std::nullopt};
}

/// The monadic "bind" operator for std::optional<T>. If \p t has no value then
/// returns an empty optional<> where the type of the return is derived from the
/// return type of \p f.  If \p t has a value then returns the result of calling
/// \p f.
///
/// \tparam T  The input type wrapped by a std::optional<>.
/// \tparam Function  A callable object whose signature is of the form `std::optional<U> f(T t)`.
template <typename T, typename Function>
inline auto operator>>= (std::optional<T> const &t, Function f)
    -> decltype (f (*t)) {
  if (t) {
    return f (*t);
  }
  return std::optional<typename decltype (f (*t))::value_type>{};
}

#if __cplusplus >= 202002L
template <typename T>
concept notifications = requires (T &&v) {
  /// Returns the result of the parse. If the parse was successful, this
  /// function is called by parser<>::eof() which will return its result.
  {v.result ()};

  /// Called when a JSON string has been parsed.
  {
    v.string_value (std::string_view{})
    } -> std::convertible_to<std::error_code>;
  /// Called when an integer value has been parsed.
  { v.int64_value (std::int64_t{}) } -> std::convertible_to<std::error_code>;
  /// Called when an unsigned integer value has been parsed.
  { v.uint64_value (std::uint64_t{}) } -> std::convertible_to<std::error_code>;
  /// Called when a floating-point value has been parsed.
  { v.double_value (double{}) } -> std::convertible_to<std::error_code>;
  /// Called when a boolean value has been parsed
  { v.boolean_value (bool{}) } -> std::convertible_to<std::error_code>;
  /// Called when a null value has been parsed.
  { v.null_value () } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an array. Subsequent event notifications are
  /// for members of this array until a matching call to Callbacks::end_array().
  { v.begin_array () } -> std::convertible_to<std::error_code>;
  /// Called indicate that an array has been completely parsed. This will always
  /// follow an earlier call to begin_array().
  { v.end_array () } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an object. Subsequent event notifications
  /// are for members of this object until a matching call to
  /// Callbacks::end_object().
  { v.begin_object () } -> std::convertible_to<std::error_code>;
  /// Called when an object key string has been parsed.
  { v.key (std::string_view{}) } -> std::convertible_to<std::error_code>;
  /// Called to indicate that an object has been completely parsed. This will
  /// always follow an earlier call to begin_object().
  { v.end_object () } -> std::convertible_to<std::error_code>;
};
#endif  // __cplusplus >= 202002L

/// \brief JSON parser implementation details.
namespace details {

template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
class matcher;

template <typename Callbacks>
class false_token_matcher;
template <typename Callbacks>
class null_token_matcher;
template <typename Callbacks>
class number_matcher;
template <typename Callbacks>
class root_matcher;
template <typename Callbacks>
class string_matcher;
template <typename Callbacks>
class true_token_matcher;
template <typename Callbacks>
class whitespace_matcher;

template <typename Callbacks>
struct singleton_storage;

/// deleter is intended for use as a unique_ptr<> Deleter. It enables
/// unique_ptr<> to be used with a mixture of heap-allocated and
/// placement-new-allocated objects.
template <typename T>
class deleter {
public:
  /// \param d True if the managed object should be deleted; false, if it only the
  /// detructor should be called.
  constexpr explicit deleter (bool const d = true) noexcept : delete_{d} {}
  void operator() (T *const p) const noexcept {
    if (delete_) {
      delete p;
    }
  }

private:
  bool delete_;
};

}  // end namespace details

struct row {
  explicit constexpr operator unsigned () const noexcept { return x; }
  unsigned x;
};
struct column {
  explicit constexpr operator unsigned () const noexcept { return y; }
  unsigned y;
};

struct coord {
  constexpr coord () noexcept = default;
  constexpr coord (struct column x, struct row y) noexcept
      : row{y}, column{x} {}
  constexpr coord (struct row y, struct column x) noexcept
      : row{y}, column{x} {}

#if __cplusplus >= 202002L
  // https://github.com/llvm/llvm-project/issues/55919
  _Pragma ("GCC diagnostic push")
  _Pragma ("GCC diagnostic ignored \"-Wzero-as-null-pointer-constant\"")
  constexpr auto operator<=> (coord const &) const noexcept = default;
  _Pragma ("GCC diagnostic pop")
#else
  constexpr bool operator== (coord const &rhs) const noexcept {
    return std::make_pair (row, column) == std::make_pair (rhs.row, rhs.column);
  }
  constexpr bool operator!= (coord const &rhs) const noexcept {
    return !operator== (rhs);
  }
  constexpr bool operator<(coord const &rhs) const noexcept {
    return std::make_pair (row, column) < std::make_pair (rhs.row, rhs.column);
  }
  constexpr bool operator<= (coord const &rhs) const noexcept {
    return std::make_pair (row, column) <= std::make_pair (rhs.row, rhs.column);
  }
  constexpr bool operator> (coord const &rhs) const noexcept {
    return std::make_pair (row, column) > std::make_pair (rhs.row, rhs.column);
  }
  constexpr bool operator>= (coord const &rhs) const noexcept {
    return std::make_pair (row, column) >= std::make_pair (rhs.row, rhs.column);
  }
#endif  // __cplusplus >= 202002L
  unsigned row = 1U;
  unsigned column = 1U;
};

enum class extensions : unsigned {
  none = 0U,
  bash_comments = 1U << 0U,
  single_line_comments = 1U << 1U,
  multi_line_comments = 1U << 2U,
  array_trailing_comma = 1U << 3U,
  object_trailing_comma = 1U << 4U,
  all = ~none,
};

constexpr extensions operator| (extensions a, extensions b) noexcept {
  return static_cast<extensions> (static_cast<unsigned> (a) |
                                  static_cast<unsigned> (b));
}

//-MARK:parser
/// \tparam Callbacks A type meeting the notifications<> requirements.
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
class parser {
  friend class details::matcher<Callbacks>;
  friend class details::root_matcher<Callbacks>;
  friend class details::whitespace_matcher<Callbacks>;

public:
  explicit parser (extensions extensions = extensions::none)
      : parser (Callbacks{}, extensions) {}
  template <typename OtherCallbacks>
  CXX20REQUIRES (notifications<OtherCallbacks>)
  explicit parser (OtherCallbacks &&callbacks,
                   extensions extensions = extensions::none);
  parser (parser const &) = delete;
  parser (parser &&) noexcept (std::is_nothrow_constructible_v<Callbacks>) =
      default;

  parser &operator= (parser const &) = delete;
  parser &operator= (parser &&) noexcept (
      std::is_nothrow_move_assignable_v<Callbacks>) = default;

  ///@{
  /// Parses a chunk of JSON input. This function may be called repeatedly with
  /// portions of the source data (for example, as the data is received from an
  /// external source). Once all of the data has been received, call the
  /// parser::eof() method.

  /// \param src The data to be parsed.
  parser &input (std::string const &src) {
    return this->input (std::begin (src), std::end (src));
  }
  parser &input (std::string_view const &src) {
    return this->input (std::begin (src), std::end (src));
  }
#if __cplusplus >= 202002L
  /// \param span The span of UTF-8 code units to be parsed.
  template <size_t Extent>
  parser &input (std::span<char, Extent> const &span) {
    return this->input (std::begin (span), std::end (span));
  }
  /// \param span The span of UTF-8 code units to be parsed.
  template <size_t Extent>
  parser &input (std::span<char const, Extent> const &span) {
    return this->input (std::begin (span), std::end (span));
  }
#endif  // __cplusplus >= 202002L
  /// \param first The element in the half-open range of UTF-8 code-units to be parsed.
  /// \param last The end of the range of UTF-8 code-units to be parsed.
  template <typename InputIterator>
  CXX20REQUIRES (std::input_iterator<InputIterator>)
  parser &input (InputIterator first, InputIterator last);
  ///@}

  /// Informs the parser that the complete input stream has been passed by calls
  /// to parser<>::input().
  ///
  /// \returns If the parse completes successfully, Callbacks::result()
  /// is called and its result returned.
  decltype (auto) eof ();

  ///@{

  /// \returns True if the parser has signalled an error.
  constexpr bool has_error () const noexcept {
    return static_cast<bool> (error_);
  }
  /// \returns The error code held by the parser.
  constexpr std::error_code const &last_error () const noexcept {
    return error_;
  }

  ///@{
  constexpr Callbacks &callbacks () noexcept {
    return callbacks_;
  }
  constexpr Callbacks const &callbacks () const noexcept {
    return callbacks_;
  }
  ///@}

  /// \param flag  A selection of bits from the parser_extensions enum.
  /// \returns True if any of the extensions given by \p flag are enabled by the parser.
  constexpr bool extension_enabled (extensions const flag) const noexcept {
    using ut = std::underlying_type_t<extensions>;
    return (static_cast<ut> (extensions_) & static_cast<ut> (flag)) != 0U;
  }

  /// Returns the parser's position in the input text.
  constexpr coord coordinate () const noexcept {
    return coordinate_;
  }

private:
  using matcher = details::matcher<Callbacks>;
  using pointer = std::unique_ptr<matcher, details::deleter<matcher>>;

  ///@{
  /// \brief Managing the column and row number (the "coordinate").

  /// Increments the column number.
  void advance_column () noexcept {
    ++coordinate_.column;
  }

  /// Increments the row number and resets the column.
  void advance_row () noexcept {
    // The column number is set to 0. This is because the outer parse loop
    // automatically advances the column number for each character consumed.
    // This happens after the row is advanced by a matcher's consume() function.
    coordinate_.column = 0U;
    ++coordinate_.row;
  }

  /// Resets the column count but does not affect the row number.
  void reset_column () noexcept {
    coordinate_.column = 0U;
  }
  ///@}

  /// Records an error for this parse. The parse will stop as soon as a non-zero
  /// error code is recorded. An error may be reported at any time during the
  /// parse; all subsequent text is ignored.
  ///
  /// \param err  The json error code to be stored in the parser.
  bool set_error (std::error_code const err) noexcept {
    assert (!error_ || err);
    error_ = err;
    return this->has_error ();
  }
  ///@}

  pointer make_root_matcher (bool object_key = false);
  pointer make_whitespace_matcher ();

  template <typename Matcher, typename... Args>
  CXX20REQUIRES ((std::derived_from<Matcher, matcher>))
  pointer make_terminal_matcher (Args &&...args);

  /// Preallocated storage for "terminal" matchers. These are the matchers,
  /// such as numbers or strings which can't have child objects.
  details::singleton_storage<Callbacks> singletons_;

  /// The maximum depth to which we allow the parse stack to grow. This value
  /// should be sufficient for any reasonable input: its intention is to prevent
  /// bogus (attack) inputs from causing the parser's memory consumption to grow
  /// uncontrollably.
  static constexpr std::size_t max_stack_depth_ = 200;
  /// The parse stack.
  std::stack<pointer> stack_;
  std::error_code error_;

  /// Each instance of the string matcher uses this string object to record its
  /// output. This avoids having to create a new instance each time we scan a
  /// string.
  std::string string_;

  /// The column and row number of the parse within the input stream.
  coord coordinate_;
  extensions extensions_;
  [[no_unique_address]] Callbacks callbacks_;
};

template <typename Callbacks>
parser(Callbacks) -> parser<Callbacks>;


template <typename Callbacks>
CXX20REQUIRES (notifications<std::remove_reference_t<Callbacks>>)
inline parser<std::remove_reference_t<Callbacks>>
make_parser (Callbacks &&callbacks, extensions const extensions = extensions::none) {
  return parser<std::remove_reference_t<Callbacks>>{std::forward<Callbacks> (callbacks), extensions};
}

namespace details {

enum char_set : char {
  cr = '\x0D',
  hash = '#',
  lf = '\x0A',
  slash = '/',
  space = '\x20',
  star = '*',
  tab = '\x09',
};
constexpr bool isspace (char const c) noexcept {
  return c == char_set::tab || c == char_set::lf || c == char_set::cr ||
         c == char_set::space;
}

/// The base class for the various state machines ("matchers") which implement
/// the various productions of the JSON grammar.
//-MARK:matcher
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
class matcher {
public:
  using pointer = std::unique_ptr<matcher, deleter<matcher>>;

  matcher (matcher const &) = delete;
  virtual ~matcher () noexcept = default;
  matcher &operator= (matcher const &) = delete;

  /// Called for each character as it is consumed from the input.
  ///
  /// \param parser The owning parser instance.
  /// \param ch If true, the character to be consumed. A value of nothing indicates
  /// end-of-file.
  /// \returns A pair consisting of a matcher pointer and a boolean. If non-null, the
  /// matcher is pushed onto the parse stack; if null the same matcher object is
  /// used to process the next character. The boolean value is false if the same
  /// character must be passed to the next consume() call; true indicates that
  /// the character was correctly matched by this consume() call.
  virtual std::pair<pointer, bool> consume (parser<Callbacks> &parser,
                                            std::optional<char> ch) = 0;

  /// \returns True if this matcher has completed (and reached it's "done" state). The
  /// parser will pop this instance from the parse stack before continuing.
  bool is_done () const noexcept { return state_ == done; }

protected:
  explicit constexpr matcher (int const initial_state) noexcept
      : state_{initial_state} {}

  matcher (matcher &&) noexcept = default;
  matcher &operator= (matcher &&) noexcept = default;

  constexpr int get_state () const noexcept { return state_; }
  void set_state (int const s) noexcept { state_ = s; }

  ///@{
  /// \brief Errors

  /// \returns True if the parser is in an error state.
  bool set_error (parser<Callbacks> &parser,
                  std::error_code const &err) noexcept {
    bool const has_error = parser.set_error (err);
    if (has_error) {
      set_state (done);
    }
    return has_error;
  }
  ///@}

  pointer make_root_matcher (parser<Callbacks> &parser,
                             bool object_key = false) {
    return parser.make_root_matcher (object_key);
  }
  pointer make_whitespace_matcher (parser<Callbacks> &parser) {
    return parser.make_whitespace_matcher ();
  }

  template <typename Matcher, typename... Args>
  pointer make_terminal_matcher (parser<Callbacks> &parser, Args &&...args) {
    return parser.template make_terminal_matcher<Matcher, Args...> (
        std::forward<Args> (args)...);
  }

  /// The value to be used for the "done" state in the each of the matcher state
  /// machines.
  static constexpr auto done = 1;

private:
  int state_;
};

//*  _       _             *
//* | |_ ___| |_____ _ _   *
//* |  _/ _ \ / / -_) ' \  *
//*  \__\___/_\_\___|_||_| *
//*                        *
/// A matcher which checks for a specific keyword such as "true", "false", or
/// "null".
/// \tparam Callbacks  The parser callback structure.
//-MARK:token matcher
template <typename Callbacks, typename DoneFunction>
CXX20REQUIRES ((notifications<Callbacks> &&
                std::invocable<DoneFunction, parser<Callbacks> &>))
class token_matcher : public matcher<Callbacks> {
public:
  /// \param text  The string to be matched.
  /// \param done  The function called when the source string is matched.
  explicit token_matcher (char const *text, DoneFunction done) noexcept
      : matcher<Callbacks> (start_state), text_{text}, done_{done} {}
  token_matcher (token_matcher const &) = delete;
  token_matcher (token_matcher &&) noexcept = default;

  token_matcher &operator= (token_matcher const &) = delete;
  token_matcher &operator= (token_matcher &&) noexcept = default;

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
    last_state,
  };

  /// The keyword to be matched. The input sequence must exactly match this
  /// string or an unrecognized token error is raised. Once all of the
  /// characters are matched, complete() is called.
  char const *text_;

  /// This function is called once the complete token text has been matched.
  [[no_unique_address]] DoneFunction const done_;
};

template <typename Callbacks, typename DoneFunction>
CXX20REQUIRES ((notifications<Callbacks> &&
                std::invocable<DoneFunction, parser<Callbacks> &>))
std::pair<typename matcher<Callbacks>::pointer, bool> token_matcher<
    Callbacks, DoneFunction>::consume (parser<Callbacks> &parser,
                                       std::optional<char> ch) {
  bool match = true;
  switch (this->get_state ()) {
  case start_state:
    if (!ch || *ch != *text_) {
      this->set_error (parser, error_code::unrecognized_token);
    } else {
      ++text_;
      if (*text_ == '\0') {
        // We've run out of input text, so ensure that the next character isn't
        // alpha-numeric.
        this->set_state (last_state);
      }
    }
    break;
  case last_state:
    if (ch) {
      if (std::isalnum (*ch) != 0) {
        this->set_error (parser, error_code::unrecognized_token);
        return {nullptr, true};
      }
      match = false;
    }
    this->set_error (parser, done_ (parser));
    this->set_state (done_state);
    break;
  case done_state:
  default: assert (false); break;
  }
  return {nullptr, match};
}

//*   __      _           _       _             *
//*  / _|__ _| |___ ___  | |_ ___| |_____ _ _   *
//* |  _/ _` | (_-</ -_) |  _/ _ \ / / -_) ' \  *
//* |_| \__,_|_/__/\___|  \__\___/_\_\___|_||_| *
//*                                             *

//-MARK:false token
template <typename Callbacks>
struct false_complete {
  std::error_code operator() (parser<Callbacks> &p) const {
    return p.callbacks ().boolean_value (false);
  }
};

template <typename Callbacks>
class false_token_matcher
    : public token_matcher<Callbacks, false_complete<Callbacks>> {
public:
  false_token_matcher ()
      : token_matcher<Callbacks, false_complete<Callbacks>> ("false", {}) {}
};

//*  _                  _       _             *
//* | |_ _ _ _  _ ___  | |_ ___| |_____ _ _   *
//* |  _| '_| || / -_) |  _/ _ \ / / -_) ' \  *
//*  \__|_|  \_,_\___|  \__\___/_\_\___|_||_| *
//*                                           *

//-MARK:true token
template <typename Callbacks>
struct true_complete {
  std::error_code operator() (parser<Callbacks> &p) const {
    return p.callbacks ().boolean_value (true);
  }
};

template <typename Callbacks>
class true_token_matcher
    : public token_matcher<Callbacks, true_complete<Callbacks>> {
public:
  true_token_matcher ()
      : token_matcher<Callbacks, true_complete<Callbacks>> ("true", {}) {}
};

//*           _ _   _       _             *
//*  _ _ _  _| | | | |_ ___| |_____ _ _   *
//* | ' \ || | | | |  _/ _ \ / / -_) ' \  *
//* |_||_\_,_|_|_|  \__\___/_\_\___|_||_| *
//*                                       *

//-MARK:null token
template <typename Callbacks>
struct null_complete {
  std::error_code operator() (parser<Callbacks> &p) const {
    return p.callbacks ().null_value ();
  }
};

template <typename Callbacks>
class null_token_matcher
    : public token_matcher<Callbacks, null_complete<Callbacks>> {
public:
  null_token_matcher ()
      : token_matcher<Callbacks, null_complete<Callbacks>> ("null", {}) {}
};

//*                 _              *
//*  _ _ _  _ _ __ | |__  ___ _ _  *
//* | ' \ || | '  \| '_ \/ -_) '_| *
//* |_||_\_,_|_|_|_|_.__/\___|_|   *
//*                                *
// Grammar (from RFC 7159, March 2014)
//     number = [ minus ] int [ frac ] [ exp ]
//     decimal-point = %x2E       ; .
//     digit1-9 = %x31-39         ; 1-9
//     e = %x65 / %x45            ; e E
//     exp = e [ minus / plus ] 1*DIGIT
//     frac = decimal-point 1*DIGIT
//     int = zero / ( digit1-9 *DIGIT )
//     minus = %x2D               ; -
//     plus = %x2B                ; +
//     zero = %x30                ; 0
//-MARK:number
template <typename Callbacks>
class number_matcher final : public matcher<Callbacks> {
public:
  number_matcher () noexcept : matcher<Callbacks> (leading_minus_state) {}
  number_matcher (number_matcher const &) = delete;
  number_matcher (number_matcher &&) noexcept = default;

  number_matcher &operator= (number_matcher const &) = delete;
  number_matcher &operator= (number_matcher &&) noexcept = default;

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  bool in_terminal_state () const;

  bool do_leading_minus_state (parser<Callbacks> &parser, char c);
  /// Implements the first character of the 'int' production.
  bool do_integer_initial_digit_state (parser<Callbacks> &parser, char c);
  bool do_integer_digit_state (parser<Callbacks> &parser, char c);
  bool do_frac_state (parser<Callbacks> &parser, char c);
  bool do_frac_digit_state (parser<Callbacks> &parser, char c);
  bool do_exponent_sign_state (parser<Callbacks> &parser, char c);
  bool do_exponent_digit_state (parser<Callbacks> &parser, char c);

  void complete (parser<Callbacks> &parser);
  void number_is_float ();

  void make_result (parser<Callbacks> &parser);

  enum state {
    done_state = matcher<Callbacks>::done,
    leading_minus_state,
    integer_initial_digit_state,
    integer_digit_state,
    frac_state,
    frac_initial_digit_state,
    frac_digit_state,
    exponent_sign_state,
    exponent_initial_digit_state,
    exponent_digit_state,
  };

  bool is_neg_ = false;
  bool is_integer_ = true;
  std::uint64_t int_acc_ = 0;

  struct {
    double frac_part = 0.0;
    double frac_scale = 1.0;
    double whole_part = 0.0;

    bool exp_is_negative = false;
    unsigned exponent = 0;
  } fp_acc_;
};

// number is float
// ~~~~~~~~~~~~~~~
template <typename Callbacks>
void number_matcher<Callbacks>::number_is_float () {
  if (is_integer_) {
    fp_acc_.whole_part = static_cast<double> (int_acc_);
    is_integer_ = false;
  }
}

// in terminal state
// ~~~~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::in_terminal_state () const {
  switch (this->get_state ()) {
  case integer_digit_state:
  case frac_state:
  case frac_digit_state:
  case exponent_digit_state:
  case done_state: return true;
  default: return false;
  }
}

// leading minus state
// ~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_leading_minus_state (
    parser<Callbacks> &parser, char c) {
  bool match = true;
  if (c == '-') {
    this->set_state (integer_initial_digit_state);
    is_neg_ = true;
  } else if (c >= '0' && c <= '9') {
    this->set_state (integer_initial_digit_state);
    match = do_integer_initial_digit_state (parser, c);
  } else {
    // minus MUST be followed by the 'int' production.
    this->set_error (parser, error_code::number_out_of_range);
  }
  return match;
}

// frac state
// ~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_frac_state (parser<Callbacks> &parser,
                                               char const c) {
  bool match = true;
  if (c == '.') {
    this->set_state (frac_initial_digit_state);
  } else if (c == 'e' || c == 'E') {
    this->set_state (exponent_sign_state);
  } else if (c >= '0' && c <= '9') {
    // digits are definitely not part of the next token so we can issue an error
    // right here.
    this->set_error (parser, error_code::number_out_of_range);
  } else {
    // the 'frac' production is optional.
    match = false;
    this->complete (parser);
  }
  return match;
}

// frac digit
// ~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_frac_digit_state (parser<Callbacks> &parser,
                                                     char const c) {
  assert (this->get_state () == frac_initial_digit_state ||
          this->get_state () == frac_digit_state);

  bool match = true;
  if (c == 'e' || c == 'E') {
    this->number_is_float ();
    if (this->get_state () == frac_initial_digit_state) {
      this->set_error (parser, error_code::unrecognized_token);
    } else {
      this->set_state (exponent_sign_state);
    }
  } else if (c >= '0' && c <= '9') {
    this->number_is_float ();
    fp_acc_.frac_part = fp_acc_.frac_part * 10 + (c - '0');
    fp_acc_.frac_scale *= 10;

    this->set_state (frac_digit_state);
  } else {
    if (this->get_state () == frac_initial_digit_state) {
      this->set_error (parser, error_code::unrecognized_token);
    } else {
      match = false;
      this->complete (parser);
    }
  }
  return match;
}

// exponent sign state
// ~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_exponent_sign_state (
    parser<Callbacks> &parser, char c) {
  bool match = true;
  this->number_is_float ();
  this->set_state (exponent_initial_digit_state);
  switch (c) {
  case '+': fp_acc_.exp_is_negative = false; break;
  case '-': fp_acc_.exp_is_negative = true; break;
  default: match = this->do_exponent_digit_state (parser, c); break;
  }
  return match;
}

// complete
// ~~~~~~~~
template <typename Callbacks>
void number_matcher<Callbacks>::complete (parser<Callbacks> &parser) {
  this->set_state (done_state);
  this->make_result (parser);
}

// exponent digit
// ~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_exponent_digit_state (
    parser<Callbacks> &parser, char const c) {
  assert (this->get_state () == exponent_digit_state ||
          this->get_state () == exponent_initial_digit_state);
  assert (!is_integer_);

  bool match = true;
  if (c >= '0' && c <= '9') {
    fp_acc_.exponent = fp_acc_.exponent * 10U + static_cast<unsigned> (c - '0');
    this->set_state (exponent_digit_state);
  } else {
    if (this->get_state () == exponent_initial_digit_state) {
      this->set_error (parser, error_code::unrecognized_token);
    } else {
      match = false;
      this->complete (parser);
    }
  }
  return match;
}

// do integer initial digit state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_integer_initial_digit_state (
    parser<Callbacks> &parser, char const c) {
  assert (this->get_state () == integer_initial_digit_state);
  assert (is_integer_);
  if (c == '0') {
    this->set_state (frac_state);
  } else if (c >= '1' && c <= '9') {
    assert (int_acc_ == 0);
    int_acc_ = static_cast<unsigned> (c - '0');
    this->set_state (integer_digit_state);
  } else {
    this->set_error (parser, error_code::unrecognized_token);
  }
  return true;
}

// do integer digit state
// ~~~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
bool number_matcher<Callbacks>::do_integer_digit_state (
    parser<Callbacks> &parser, char const c) {
  assert (this->get_state () == integer_digit_state);
  assert (is_integer_);

  bool match = true;
  if (c == '.') {
    this->set_state (frac_initial_digit_state);
    number_is_float ();
  } else if (c == 'e' || c == 'E') {
    this->set_state (exponent_sign_state);
    number_is_float ();
  } else if (c >= '0' && c <= '9') {
    std::uint64_t const new_acc =
        int_acc_ * 10U + static_cast<unsigned> (c - '0');
    if (new_acc < int_acc_) {  // Did this overflow?
      this->set_error (parser, error_code::number_out_of_range);
    } else {
      int_acc_ = new_acc;
    }
  } else {
    match = false;
    this->complete (parser);
  }
  return match;
}

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
number_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                    std::optional<char> ch) {
  bool match = true;
  if (ch) {
    char const c = *ch;
    switch (this->get_state ()) {
    case leading_minus_state:
      match = this->do_leading_minus_state (parser, c);
      break;
    case integer_initial_digit_state:
      match = this->do_integer_initial_digit_state (parser, c);
      break;
    case integer_digit_state:
      match = this->do_integer_digit_state (parser, c);
      break;
    case frac_state: match = this->do_frac_state (parser, c); break;
    case frac_initial_digit_state:
    case frac_digit_state: match = this->do_frac_digit_state (parser, c); break;
    case exponent_sign_state:
      match = this->do_exponent_sign_state (parser, c);
      break;
    case exponent_initial_digit_state:
    case exponent_digit_state:
      match = this->do_exponent_digit_state (parser, c);
      break;
    case done_state:
    default: assert (false); break;
    }
  } else {
    assert (!parser.has_error ());
    if (!this->in_terminal_state ()) {
      this->set_error (parser, error_code::expected_digits);
    }
    this->complete (parser);
  }
  return {nullptr, match};
}

// make result
// ~~~~~~~~~~~
template <typename Callbacks>
void number_matcher<Callbacks>::make_result (parser<Callbacks> &parser) {
  if (parser.has_error ()) {
    return;
  }
  assert (this->in_terminal_state ());

  if (is_integer_) {
    constexpr auto min = std::numeric_limits<std::int64_t>::min ();
    constexpr auto umin = static_cast<std::uint64_t> (min);

    if (is_neg_) {
      if (int_acc_ > umin) {
        this->set_error (parser, error_code::number_out_of_range);
        return;
      }

      this->set_error (
          parser,
          parser.callbacks ().int64_value (
              (int_acc_ == umin) ? min
                                 : -static_cast<std::int64_t> (int_acc_)));
      return;
    }
    this->set_error (parser, parser.callbacks ().uint64_value (int_acc_));
    return;
  }

  auto xf = (fp_acc_.whole_part + fp_acc_.frac_part / fp_acc_.frac_scale);
  auto exp = std::pow (10, fp_acc_.exponent);
  if (std::isinf (exp)) {
    this->set_error (parser, error_code::number_out_of_range);
    return;
  }
  if (fp_acc_.exp_is_negative) {
    exp = 1.0 / exp;
  }

  xf *= exp;
  if (is_neg_) {
    xf = -xf;
  }

  if (std::isinf (xf) || std::isnan (xf)) {
    this->set_error (parser, error_code::number_out_of_range);
    return;
  }
  this->set_error (parser, parser.callbacks ().double_value (xf));
}

//*     _       _            *
//*  __| |_ _ _(_)_ _  __ _  *
//* (_-<  _| '_| | ' \/ _` | *
//* /__/\__|_| |_|_||_\__, | *
//*                   |___/  *
//-MARK:string
template <typename Callbacks>
class string_matcher final : public matcher<Callbacks> {
public:
  explicit string_matcher (std::string *const str, bool object_key) noexcept
      : matcher<Callbacks> (start_state),
        is_object_key_{object_key},
        app_{str} {
    assert (str != nullptr);
    str->clear ();
  }
  string_matcher (string_matcher const &) = delete;
  string_matcher (string_matcher &&) noexcept = default;

  string_matcher &operator= (string_matcher const &) = delete;
  string_matcher &operator= (string_matcher &&) noexcept = default;

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
    normal_char_state,
    escape_state,
    hex1_state,
    hex2_state,
    hex3_state,
    hex4_state,
  };

  class appender {
  public:
    explicit appender (std::string *const result) noexcept : result_{result} {
      assert (result != nullptr);
    }
    bool append32 (char32_t code_point);
    bool append16 (char16_t cu);
    std::string *result () { return result_; }
    bool has_high_surrogate () const noexcept { return high_surrogate_ != 0; }

  private:
    std::string *const result_;
    char16_t high_surrogate_ = 0;
  };

  std::tuple<state, std::error_code> consume_normal_state (
      parser<Callbacks> &parser, char32_t code_point, appender &app);

  static std::optional<unsigned> hex_value (char32_t c, unsigned value);

  static std::optional<std::tuple<unsigned, state>> consume_hex_state (
      unsigned hex, enum state state, char32_t code_point);

  static std::tuple<state, error_code> consume_escape_state (
      char32_t code_point, appender &app);
  bool is_object_key_;
  utf8_decoder decoder_;
  appender app_;
  unsigned hex_ = 0U;
};

// append32
// ~~~~~~~~
template <typename Callbacks>
bool string_matcher<Callbacks>::appender::append32 (char32_t const code_point) {
  if (this->has_high_surrogate ()) {
    // A high surrogate followed by something other than a low surrogate.
    return false;
  }
  code_point_to_utf8<char> (code_point, std::back_inserter (*result_));
  return true;
}

// append16
// ~~~~~~~~
template <typename Callbacks>
bool string_matcher<Callbacks>::appender::append16 (char16_t const cu) {
  bool ok = true;
  if (is_utf16_high_surrogate (cu)) {
    if (!this->has_high_surrogate ()) {
      high_surrogate_ = cu;
    } else {
      // A high surrogate following another high surrogate.
      ok = false;
    }
  } else if (is_utf16_low_surrogate (cu)) {
    if (!this->has_high_surrogate ()) {
      // A low surrogate following by something other than a high surrogate.
      ok = false;
    } else {
      std::array<char16_t, 2> const surrogates{{high_surrogate_, cu}};
      auto [_, code_point] =
          utf16_to_code_point (std::begin (surrogates), std::end (surrogates));
      code_point_to_utf8 (code_point, std::back_inserter (*result_));
      high_surrogate_ = 0;
    }
  } else {
    if (this->has_high_surrogate ()) {
      // A high surrogate followed by something other than a low surrogate.
      ok = false;
    } else {
      auto const code_point = static_cast<char32_t> (cu);
      code_point_to_utf8 (code_point, std::back_inserter (*result_));
    }
  }
  return ok;
}

// consume normal state
// ~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
auto string_matcher<Callbacks>::consume_normal_state (parser<Callbacks> &parser,
                                                      char32_t code_point,
                                                      appender &app)
    -> std::tuple<state, std::error_code> {
  state next_state = normal_char_state;
  std::error_code error;

  if (code_point == '"') {
    if (app.has_high_surrogate ()) {
      error = error_code::bad_unicode_code_point;
    } else {
      // Consume the closing quote character.
      if (is_object_key_) {
        error = parser.callbacks ().key (*app.result ());
      } else {
        error = parser.callbacks ().string_value (*app.result ());
      }
    }
    next_state = done_state;
  } else if (code_point == '\\') {
    next_state = escape_state;
  } else if (code_point <= 0x1F) {
    // Control characters U+0000 through U+001F MUST be escaped.
    error = error_code::bad_unicode_code_point;
  } else {
    if (!app.append32 (code_point)) {
      error = error_code::bad_unicode_code_point;
    }
  }

  return std::make_tuple (next_state, error);
}

// hex value [static]
// ~~~~~~~~~
template <typename Callbacks>
std::optional<unsigned> string_matcher<Callbacks>::hex_value (
    char32_t const c, unsigned const value) {
  auto digit = 0U;
  if (c >= '0' && c <= '9') {
    digit = static_cast<unsigned> (c) - '0';
  } else if (c >= 'a' && c <= 'f') {
    digit = static_cast<unsigned> (c) - 'a' + 10;
  } else if (c >= 'A' && c <= 'F') {
    digit = static_cast<unsigned> (c) - 'A' + 10;
  } else {
    return {std::nullopt};
  }
  return just (16 * value + digit);
}

// consume hex state [static]
// ~~~~~~~~~~~~~~~~~
template <typename Callbacks>
auto string_matcher<Callbacks>::consume_hex_state (unsigned const hex,
                                                   enum state const state,
                                                   char32_t const code_point)
    -> std::optional<std::tuple<unsigned, enum state>> {

      return hex_value (code_point, hex) >>=
             [state] (unsigned value) {
               assert (value <= std::numeric_limits<std::uint16_t>::max ());
               auto next_state = state;
               switch (state) {
               case hex1_state: next_state = hex2_state; break;
               case hex2_state: next_state = hex3_state; break;
               case hex3_state: next_state = hex4_state; break;
               case hex4_state: next_state = normal_char_state; break;

               case start_state:
               case normal_char_state:
               case escape_state:
               case done_state:
                 assert (false);
                 return nothing<std::tuple<unsigned, enum state>> ();
               }

               return just (std::make_tuple (value, next_state));
             };
    }

// consume escape state [static]
// ~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
auto string_matcher<Callbacks>::consume_escape_state (char32_t code_point,
                                                      appender &app)
    -> std::tuple<state, error_code> {
  auto decode = [] (char32_t cp) {
    state next_state = normal_char_state;
    switch (cp) {
    case '"': cp = '"'; break;
    case '\\': cp = '\\'; break;
    case '/': cp = '/'; break;
    case 'b': cp = '\b'; break;
    case 'f': cp = '\f'; break;
    case 'n': cp = '\n'; break;
    case 'r': cp = '\r'; break;
    case 't': cp = '\t'; break;
    case 'u': next_state = hex1_state; break;
    default: return nothing<std::tuple<char32_t, state>> ();
    }
    return just (std::make_tuple (cp, next_state));
  };

  auto append = [&app] (std::tuple<char32_t, state> const &s) {
    auto const [cp, next_state] = s;
    assert (next_state == normal_char_state || next_state == hex1_state);
    if (next_state == normal_char_state && !app.append32 (cp)) {
      return std::optional<state>{std::nullopt};
    }
    return just (next_state);
  };

  std::optional<state> const x = decode (code_point) >>= append;
  return x ? std::make_tuple (*x, error_code::none)
           : std::make_tuple (normal_char_state,
                              error_code::invalid_escape_char);
}

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
string_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                    std::optional<char> ch) {
  if (!ch) {
    this->set_error (parser, error_code::expected_close_quote);
    return {nullptr, true};
  }

  if (std::optional<char32_t> const code_point =
          decoder_.get (static_cast<std::uint8_t> (*ch))) {
    switch (this->get_state ()) {
    // Matches the opening quote.
    case start_state:
      if (*code_point == '"') {
        assert (!app_.has_high_surrogate ());
        this->set_state (normal_char_state);
      } else {
        this->set_error (parser, error_code::expected_token);
      }
      break;
    case normal_char_state: {
      auto const normal_resl =
          string_matcher::consume_normal_state (parser, *code_point, app_);
      this->set_state (std::get<0> (normal_resl));
      this->set_error (parser, std::get<std::error_code> (normal_resl));
    } break;

    case escape_state: {
      auto const escape_resl =
          string_matcher::consume_escape_state (*code_point, app_);
      this->set_state (std::get<0> (escape_resl));
      this->set_error (parser, std::get<1> (escape_resl));
    } break;

    case hex1_state: hex_ = 0; [[fallthrough]];
    case hex2_state:
    case hex3_state:
    case hex4_state: {
      std::optional<std::tuple<unsigned, state>> const hex_resl =
          string_matcher::consume_hex_state (
              hex_, static_cast<state> (this->get_state ()), *code_point);
      if (!hex_resl) {
        this->set_error (parser, error_code::invalid_hex_char);
        break;
      }
      hex_ = std::get<0> (*hex_resl);
      state const next_state = std::get<1> (*hex_resl);
      this->set_state (next_state);
      // We're done with the hex characters and are switching back to the
      // "normal" state. The means that we can add the accumulated code-point
      // (in hex_) to the string.
      if (next_state == normal_char_state &&
          !app_.append16 (static_cast<char16_t> (hex_))) {
        this->set_error (parser, error_code::bad_unicode_code_point);
      }
    } break;

    case done_state:
    default: assert (false); break;
    }
  }
  return {nullptr, true};
}

//*                          *
//*  __ _ _ _ _ _ __ _ _  _  *
//* / _` | '_| '_/ _` | || | *
//* \__,_|_| |_| \__,_|\_, | *
//*                    |__/  *
//-MARK:array
template <typename Callbacks>
class array_matcher final : public matcher<Callbacks> {
public:
  array_matcher () noexcept : matcher<Callbacks> (start_state) {}

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
    first_object_state,
    object_state,
    comma_state,
  };

  void end_array (parser<Callbacks> &parser);
};

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
array_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                   std::optional<char> ch) {
  if (!ch) {
    this->set_error (parser, error_code::expected_array_member);
    return {nullptr, true};
  }
  char const c = *ch;
  switch (this->get_state ()) {
  case start_state:
    assert (c == '[');
    if (this->set_error (parser, parser.callbacks ().begin_array ())) {
      break;
    }
    this->set_state (first_object_state);
    // Match this character and consume whitespace before the object (or close
    // bracket).
    return {this->make_whitespace_matcher (parser), true};

  case first_object_state:
    if (c == ']') {
      this->end_array (parser);
      break;
    }
    [[fallthrough]];
  case object_state:
    this->set_state (comma_state);
    return {this->make_root_matcher (parser), false};
    break;
  case comma_state:
    if (isspace (c)) {
      // just consume whitespace before a comma.
      return {this->make_whitespace_matcher (parser), false};
    }
    switch (c) {
    case ',':
      this->set_state (
          (parser.extension_enabled (extensions::array_trailing_comma))
              ? first_object_state
              : object_state);
      return {this->make_whitespace_matcher (parser), true};
    case ']': this->end_array (parser); break;
    default: this->set_error (parser, error_code::expected_array_member); break;
    }
    break;
  case done_state:
  default: assert (false); break;
  }
  return {nullptr, true};
}

// end array
// ~~~~~~~~~
template <typename Callbacks>
void array_matcher<Callbacks>::end_array (parser<Callbacks> &parser) {
  this->set_error (parser, parser.callbacks ().end_array ());
  this->set_state (done_state);
}

//*      _     _        _    *
//*  ___| |__ (_)___ __| |_  *
//* / _ \ '_ \| / -_) _|  _| *
//* \___/_.__// \___\__|\__| *
//*         |__/             *
//-MARK:object
template <typename Callbacks>
class object_matcher final : public matcher<Callbacks> {
public:
  object_matcher () noexcept : matcher<Callbacks> (start_state) {}

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
    first_key_state,
    key_state,
    colon_state,
    value_state,
    comma_state,
  };

  void end_object (parser<Callbacks> &parser);
};

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
object_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                    std::optional<char> ch) {
  if (this->get_state () == done_state) {
    assert (parser.last_error ());
    return {nullptr, true};
  }
  if (!ch) {
    this->set_error (parser, error_code::expected_object_member);
    return {nullptr, true};
  }
  char const c = *ch;
  switch (this->get_state ()) {
  case start_state:
    assert (c == '{');
    this->set_state (first_key_state);
    if (this->set_error (parser, parser.callbacks ().begin_object ())) {
      break;
    }
    return {this->make_whitespace_matcher (parser), true};
  case first_key_state:
    // We allow either a closing brace (to end the object) or a property name.
    if (c == '}') {
      this->end_object (parser);
      break;
    }
    [[fallthrough]];
  case key_state:
    // Match a property name then expect a colon.
    this->set_state (colon_state);
    return {this->make_root_matcher (parser, true /*object key?*/), false};
  case colon_state:
    if (isspace (c)) {
      // just consume whitespace before the colon.
      return {this->make_whitespace_matcher (parser), false};
    }
    if (c == ':') {
      this->set_state (value_state);
    } else {
      this->set_error (parser, error_code::expected_colon);
    }
    break;
  case value_state:
    this->set_state (comma_state);
    return {this->make_root_matcher (parser), false};
  case comma_state:
    if (isspace (c)) {
      // just consume whitespace before the comma.
      return {this->make_whitespace_matcher (parser), false};
    }
    if (c == ',') {
      // Strictly conforming JSON requires a property name following a comma but
      // we have an extension to allow an trailing comma which may be followed
      // by the object's closing brace.
      this->set_state (
          (parser.extension_enabled (extensions::object_trailing_comma))
              ? first_key_state
              : key_state);
      // Consume the comma and any whitespace before the close brace or property
      // name.
      return {this->make_whitespace_matcher (parser), true};
    }
    if (c == '}') {
      this->end_object (parser);
    } else {
      this->set_error (parser, error_code::expected_object_member);
    }
    break;
  case done_state:
  default: assert (false); break;
  }
  // No change of matcher. Consume the input character.
  return {nullptr, true};
}

// end object
// ~~~~~~~~~~~
template <typename Callbacks>
void object_matcher<Callbacks>::end_object (parser<Callbacks> &parser) {
  this->set_error (parser, parser.callbacks ().end_object ());
  this->set_state (done_state);
}

//*             *
//* __ __ _____ *
//* \ V  V (_-< *
//*  \_/\_//__/ *
//*             *
/// This matcher consumes whitespace and updates the row number in response to
/// the various combinations of CR and LF. Supports #, //, and /* style comments
/// as an extension.
//-MARK:whitespace
template <typename Callbacks>
class whitespace_matcher final : public matcher<Callbacks> {
public:
  whitespace_matcher () noexcept : matcher<Callbacks> (body_state) {}
  whitespace_matcher (whitespace_matcher const &) = delete;
  whitespace_matcher (whitespace_matcher &&) noexcept = default;

  whitespace_matcher &operator= (whitespace_matcher const &) = delete;
  whitespace_matcher &operator= (whitespace_matcher &&) noexcept = default;

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    /// Normal whitespace scanning. The "body" is the whitespace being consumed.
    body_state,
    /// Handles the LF part of a Windows-style CR/LF pair.
    crlf_state,
    /// Consumes the contents of a single-line comment.
    single_line_comment_state,
    comment_start_state,
    /// Consumes the contents of a multi-line comment.
    multi_line_comment_body_state,
    /// Entered when checking for the second character of the '*/' pair.
    multi_line_comment_ending_state,
    /// Handles the LF part of a Windows-style CR/LF pair inside a multi-line
    /// comment.
    multi_line_comment_crlf_state,
  };

  std::pair<typename matcher<Callbacks>::pointer, bool> consume_body (
      parser<Callbacks> &parser, char c);

  std::pair<typename matcher<Callbacks>::pointer, bool> consume_comment_start (
      parser<Callbacks> &parser, char c);

  std::pair<typename matcher<Callbacks>::pointer, bool>
  multi_line_comment_body (parser<Callbacks> &parser, char c);

  void cr (parser<Callbacks> &parser, state next) {
    assert (this->get_state () == multi_line_comment_body_state ||
            this->get_state () == body_state);
    parser.advance_row ();
    this->set_state (next);
  }
  void lf (parser<Callbacks> &parser) { parser.advance_row (); }

  /// Processes the second character of a Windows-style CR/LF pair. Returns true
  /// if the character shoud be treated as whitespace.
  bool crlf (parser<Callbacks> &parser, char c) {
    if (c != details::char_set::lf) {
      return false;
    }
    parser.reset_column ();
    return true;
  }
};

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
whitespace_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                        std::optional<char> ch) {
  if (!ch) {
    this->set_state (done_state);
  } else {
    char const c = *ch;
    switch (this->get_state ()) {
    // Handles the LF part of a Windows-style CR/LF pair.
    case crlf_state:
      this->set_state (body_state);
      if (crlf (parser, c)) {
        break;
      }
      [[fallthrough]];
    case body_state: return this->consume_body (parser, c);
    case comment_start_state: return this->consume_comment_start (parser, c);

    case multi_line_comment_ending_state:
      assert (parser.extension_enabled (extensions::multi_line_comments));
      this->set_state (c == details::char_set::slash
                           ? body_state
                           : multi_line_comment_body_state);
      break;

    case multi_line_comment_crlf_state:
      this->set_state (multi_line_comment_body_state);
      if (crlf (parser, c)) {
        break;
      }
      [[fallthrough]];
    case multi_line_comment_body_state:
      return this->multi_line_comment_body (parser, c);
    case single_line_comment_state:
      assert (parser.extension_enabled (extensions::bash_comments) ||
              parser.extension_enabled (extensions::single_line_comments) ||
              parser.extension_enabled (extensions::multi_line_comments));
      if (c == details::char_set::cr || c == details::char_set::lf) {
        // This character marks a bash/single-line comment end. Go back to
        // normal whitespace handling. Retry with the same character.
        this->set_state (body_state);
        return {nullptr, false};
      }
      // Just consume the character.
      break;

    case done_state:
    default: assert (false); break;
    }
  }
  return {nullptr, true};
}

// consume body
// ~~~~~~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
whitespace_matcher<Callbacks>::consume_body (parser<Callbacks> &parser,
                                             char c) {
  auto const stop_retry = [this] () {
    // Stop, pop this matcher, and retry with the same character.
    this->set_state (done_state);
    return std::pair<typename matcher<Callbacks>::pointer, bool>{nullptr,
                                                                 false};
  };

  using details::char_set;
  switch (c) {
  case char_set::space: break;  // Just consume.
  case char_set::tab:
    // TODO: tab expansion.
    break;
  case char_set::cr: this->cr (parser, crlf_state); break;
  case char_set::lf: this->lf (parser); break;
  case char_set::hash:
    if (!parser.extension_enabled (extensions::bash_comments)) {
      return stop_retry ();
    }
    this->set_state (single_line_comment_state);
    break;
  case char_set::slash:
    if (!parser.extension_enabled (extensions::single_line_comments) &&
        !parser.extension_enabled (extensions::multi_line_comments)) {
      return stop_retry ();
    }
    this->set_state (comment_start_state);
    break;
  default: return stop_retry ();
  }
  return {nullptr, true};  // Consume this character.
}

// consume comment start
// ~~~~~~~~~~~~~~~~~~~~~
/// We've already seen an initial slash ('/') which could mean one of three
/// things:
///   - the start of a single-line // comment
///   - the start of a multi-line /* */ comment
///   - just a random / character.
/// This function handles the character after that initial slash to determine
/// which of the three it is.
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
whitespace_matcher<Callbacks>::consume_comment_start (parser<Callbacks> &parser,
                                                      char c) {
  using details::char_set;
  if (c == char_set::slash &&
      parser.extension_enabled (extensions::single_line_comments)) {
    this->set_state (single_line_comment_state);
  } else if (c == char_set::star &&
             parser.extension_enabled (extensions::multi_line_comments)) {
    this->set_state (multi_line_comment_body_state);
  } else {
    this->set_error (parser, error_code::expected_token);
  }
  return {nullptr, true};  // Consume this character.
}

// multi line comment body
// ~~~~~~~~~~~~~~~~~~~~~~~
/// Similar to consume_body() except that the commented characters are consumed
/// as well as whitespace. We're looking to see a star ('*') character which may
/// indicate the end of the multi-line comment.
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
whitespace_matcher<Callbacks>::multi_line_comment_body (
    parser<Callbacks> &parser, char c) {
  using details::char_set;
  assert (parser.extension_enabled (extensions::multi_line_comments));
  assert (this->get_state () == multi_line_comment_body_state);
  switch (c) {
  case char_set::star:
    // This could be a standalone star character or be followed by a slash
    // to end the multi-line comment.
    this->set_state (multi_line_comment_ending_state);
    break;
  case char_set::cr: this->cr (parser, multi_line_comment_crlf_state); break;
  case char_set::lf: this->lf (parser); break;
  case char_set::tab: break;  // TODO: tab expansion.
  default: break;             // Just consume.
  }
  return {nullptr, true};  // Consume this character.
}

//*           __  *
//*  ___ ___ / _| *
//* / -_) _ \  _| *
//* \___\___/_|   *
//*               *
//-MARK:eof
template <typename Callbacks>
class eof_matcher final : public matcher<Callbacks> {
public:
  eof_matcher () noexcept : matcher<Callbacks> (start_state) {}

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
  };
};

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
eof_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                 std::optional<char> const ch) {
  if (ch) {
    this->set_error (parser, error_code::unexpected_extra_input);
  } else {
    this->set_state (done_state);
  }
  return {nullptr, true};
}

//*               _                _      _             *
//*  _ _ ___  ___| |_   _ __  __ _| |_ __| |_  ___ _ _  *
//* | '_/ _ \/ _ \  _| | '  \/ _` |  _/ _| ' \/ -_) '_| *
//* |_| \___/\___/\__| |_|_|_\__,_|\__\__|_||_\___|_|   *
//*                                                     *
//-MARK:root
template <typename Callbacks>
class root_matcher final : public matcher<Callbacks> {
public:
  explicit constexpr root_matcher (bool const is_object_key = false) noexcept
      : matcher<Callbacks> (start_state), object_key_{is_object_key} {}

  std::pair<typename matcher<Callbacks>::pointer, bool> consume (
      parser<Callbacks> &parser, std::optional<char> ch) override;

private:
  enum state {
    done_state = matcher<Callbacks>::done,
    start_state,
    new_token_state,
  };
  bool const object_key_;
};

// consume
// ~~~~~~~
template <typename Callbacks>
std::pair<typename matcher<Callbacks>::pointer, bool>
root_matcher<Callbacks>::consume (parser<Callbacks> &parser,
                                  std::optional<char> ch) {
  if (!ch) {
    this->set_error (parser, error_code::expected_token);
    return {nullptr, true};
  }

  switch (this->get_state ()) {
  case start_state:
    this->set_state (new_token_state);
    return {this->make_whitespace_matcher (parser), false};

  case new_token_state: {
    if (object_key_ && *ch != '"') {
      this->set_error (parser, error_code::expected_string);
      // Don't return here in order to allow the switch default to produce a
      // different error code for a bad token.
    }
    this->set_state (done_state);
    switch (*ch) {
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return {this->template make_terminal_matcher<number_matcher<Callbacks>> (
                  parser),
              false};
    case '"':
      return {this->template make_terminal_matcher<string_matcher<Callbacks>> (
                  parser, &parser.string_, object_key_),
              false};
    case 't':
      return {
          this->template make_terminal_matcher<true_token_matcher<Callbacks>> (
              parser),
          false};
    case 'f':
      return {
          this->template make_terminal_matcher<false_token_matcher<Callbacks>> (
              parser),
          false};
    case 'n':
      return {
          this->template make_terminal_matcher<null_token_matcher<Callbacks>> (
              parser),
          false};
    case '[':
      return {typename matcher<Callbacks>::pointer (
                  new array_matcher<Callbacks> ()),
              false};
    case '{':
      return {typename matcher<Callbacks>::pointer (
                  new object_matcher<Callbacks> ()),
              false};

    default:
      this->set_error (parser, error_code::expected_token);
      return {nullptr, true};
    }
  } break;
  case done_state:
  default: assert (false); break;
  }
  assert (false);  // unreachable.
  return {nullptr, true};
}

//*     _           _     _                 _                          *
//*  __(_)_ _  __ _| |___| |_ ___ _ _    __| |_ ___ _ _ __ _ __ _ ___  *
//* (_-< | ' \/ _` | / -_)  _/ _ \ ' \  (_-<  _/ _ \ '_/ _` / _` / -_) *
//* /__/_|_||_\__, |_\___|\__\___/_||_| /__/\__\___/_| \__,_\__, \___| *
//*           |___/                                         |___/      *
//-MARK:singleton storage
template <typename Callbacks>
struct singleton_storage {
  template <typename T>
  struct storage {
    using type = typename std::aligned_storage_t<sizeof (T), alignof (T)>;
  };

  typename storage<eof_matcher<Callbacks>>::type eof;
  typename storage<whitespace_matcher<Callbacks>>::type trailing_ws;
  typename storage<root_matcher<Callbacks>>::type root;

  std::variant<details::number_matcher<Callbacks>,
               details::string_matcher<Callbacks>,
               details::true_token_matcher<Callbacks>,
               details::false_token_matcher<Callbacks>,
               details::null_token_matcher<Callbacks>,
               details::whitespace_matcher<Callbacks>>
      terminals_;
};

}  // end namespace details

// (ctor)
// ~~~~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
template <typename OtherCallbacks>
CXX20REQUIRES (notifications<OtherCallbacks>)
parser<Callbacks>::parser (OtherCallbacks &&callbacks,
                           extensions const extensions)
    : extensions_{extensions},
      callbacks_{std::forward<OtherCallbacks> (callbacks)} {
  using mpointer = typename matcher::pointer;
  using deleter = typename mpointer::deleter_type;
  // The EOF matcher is placed at the bottom of the stack to ensure that the
  // input JSON ends after a single top-level object.
  stack_.push (mpointer (new (&singletons_.eof)
                             details::eof_matcher<Callbacks>{},
                         deleter{false}));
  // We permit whitespace after the top-level object.
  stack_.push (mpointer (new (&singletons_.trailing_ws)
                             details::whitespace_matcher<Callbacks>{},
                         deleter{false}));
  stack_.push (this->make_root_matcher ());
}

// make root matcher
// ~~~~~~~~~~~~~~~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
auto parser<Callbacks>::make_root_matcher (bool object_key) -> pointer {
  using root_matcher = details::root_matcher<Callbacks>;
  return pointer (new (&singletons_.root) root_matcher (object_key),
                  typename pointer::deleter_type{false});
}

// make whitespace matcher
// ~~~~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
auto parser<Callbacks>::make_whitespace_matcher () -> pointer {
  using whitespace_matcher = details::whitespace_matcher<Callbacks>;
  return this->make_terminal_matcher<whitespace_matcher> ();
}

// make terminal matcher
// ~~~~~~~~~~~~~~~~~~~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
template <typename Matcher, typename... Args>
CXX20REQUIRES ((std::derived_from<Matcher, details::matcher<Callbacks>>))
auto parser<Callbacks>::make_terminal_matcher (Args &&...args) -> pointer {
  Matcher &m = singletons_.terminals_.template emplace<Matcher> (
      std::forward<Args> (args)...);
  return pointer{&m, details::deleter<matcher>{false}};
}

// input
// ~~~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
template <typename InputIterator>
CXX20REQUIRES (std::input_iterator<InputIterator>)
auto parser<Callbacks>::input (InputIterator first, InputIterator last)
    -> parser & {
  static_assert (
      std::is_same_v<typename std::remove_cv_t<typename std::iterator_traits<
                         InputIterator>::value_type>,
                     char>,
      "iterator value_type must be char (with optional cv)");
  if (error_) {
    return *this;
  }
  while (first != last) {
    assert (!stack_.empty ());
    auto &handler = stack_.top ();
    auto res = handler->consume (*this, just (*first));
    if (handler->is_done ()) {
      if (error_) {
        break;
      }
      stack_.pop ();  // release the topmost matcher object.
    }

    if (res.first != nullptr) {
      if (stack_.size () > max_stack_depth_) {
        // We've already hit the maximum allowed parse stack depth. Reject this
        // new matcher.
        assert (!error_);
        error_ = make_error_code (error_code::nesting_too_deep);
        break;
      }

      stack_.push (std::move (res.first));
    }
    // If we're matching this character, advance the column number and increment
    // the iterator.
    if (res.second) {
      // Increment the column number if this is _not_ a UTF-8 continuation
      // character.
      if (is_utf_char_start (*first)) {
        this->advance_column ();
      }
      ++first;
    }
  }
  return *this;
}

// eof
// ~~~
template <typename Callbacks>
CXX20REQUIRES (notifications<Callbacks>)
decltype (auto) parser<Callbacks>::eof () {
  while (!stack_.empty () && !has_error ()) {
    auto &handler = stack_.top ();
    auto res = handler->consume (*this, std::optional<char>{std::nullopt});
    assert (handler->is_done ());
    assert (res.second);
    stack_.pop ();  // release the topmost matcher object.
  }
  return this->callbacks ().result ();
}

}  // namespace peejay

#endif  // PEEJAY_JSON_HPP
