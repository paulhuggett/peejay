//===- include/peejay/json/json.hpp -----------------------*- mode: C++ -*-===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file json.hpp
/// \brief  Implements the PJ JSON Parser.
#ifndef PEEJAY_JSON_HPP
#define PEEJAY_JSON_HPP

#include "peejay/json/arrayvec.hpp"
#include "peejay/json/cbii.hpp"
#include "peejay/json/cprun.hpp"
#include "peejay/json/json_error.hpp"
#include "peejay/json/stack.hpp"

#define ICUBABY_INSIDE_NS peejay
#include "peejay/json/icubaby.hpp"
#undef ICUBABY_INSIDE_NS

// Standard library
#include <cctype>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstring>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <variant>

namespace peejay {

using char8 = icubaby::char8;
using u8string = icubaby::u8string;
using u8string_view = icubaby::u8string_view;

/// A type that is always false. Used to improve the failure mesages from
/// static_assert().
template <typename... T> [[maybe_unused]] constexpr bool always_false = false;

template <typename Policy>
concept policy = std::is_integral_v<decltype(Policy::max_length)> && std::is_integral_v<typename Policy::integer_type>;

template <typename T, typename IntegerType = std::int64_t>
concept backend = requires(T &&v) {
  /// Returns the result of the parse. If the parse was successful, this
  /// function is called by parser<>::eof() which will return its result.
  { v.result() };

  /// Called when a JSON string has been parsed.
  { v.string_value(u8string_view{}) } -> std::convertible_to<std::error_code>;
  /// Called when an integer value has been parsed.
  { v.integer_value(std::make_signed_t<IntegerType>{}) } -> std::convertible_to<std::error_code>;
  /// Called when a floating-point value has been parsed.
  { v.double_value(double{}) } -> std::convertible_to<std::error_code>;
  /// Called when a boolean value has been parsed
  { v.boolean_value(bool{}) } -> std::convertible_to<std::error_code>;
  /// Called when a null value has been parsed.
  { v.null_value() } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an array. Subsequent event notifications are
  /// for members of this array until a matching call to end_array().
  { v.begin_array() } -> std::convertible_to<std::error_code>;
  /// Called indicate that an array has been completely parsed. This will always
  /// follow an earlier call to begin_array().
  { v.end_array() } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an object. Subsequent event notifications
  /// are for members of this object until a matching call to end_object().
  { v.begin_object() } -> std::convertible_to<std::error_code>;
  /// Called when an object key string has been parsed.
  { v.key(u8string_view{}) } -> std::convertible_to<std::error_code>;
  /// Called to indicate that an object has been completely parsed. This will
  /// always follow an earlier call to begin_object().
  { v.end_object() } -> std::convertible_to<std::error_code>;
};

/// \brief JSON parser implementation details.
namespace details {

template <typename Backend, typename Policies>
  requires(backend<Backend>)
class matcher;

template <typename Backend, typename Policies> class root_matcher;
template <typename Backend, typename Policies> class whitespace_matcher;

template <typename Backend, typename Policies> struct singletons;

/// deleter is intended for use as a unique_ptr<> Deleter. It enables
/// unique_ptr<> to be used with a mixture of heap-allocated and
/// placement-new-allocated objects.
template <typename T> class deleter {
public:
  enum class mode : char { do_delete, do_nothing };

  /// \param m One of the three modes: delete the object, call the destructor, or do nothing.
  constexpr explicit deleter(mode const m) noexcept : mode_{m} {}
  void operator()(T *const p) const noexcept {
    switch (mode_) {
    case mode::do_delete: delete p; break;
    case mode::do_nothing:
    default: break;
    }
  }

private:
  mode mode_;
};

template <typename CharType, typename InputIterator> struct iterator_produces {
  static constexpr bool value =
      std::is_same_v<std::decay_t<typename std::iterator_traits<InputIterator>::value_type>, CharType>;
};

}  // end namespace details

class coord {
public:
  class line {
  public:
    explicit constexpr line(unsigned x) noexcept : x_{x} {}
    explicit constexpr operator unsigned() const noexcept { return x_; }
    constexpr bool operator==(line const &rhs) const noexcept = default;

  private:
    unsigned x_;
  };
  class column {
  public:
    explicit constexpr column(unsigned y) noexcept : y_{y} {}
    explicit constexpr operator unsigned() const noexcept { return y_; }
    constexpr bool operator==(column const &rhs) const noexcept = default;

  private:
    unsigned y_;
  };

  constexpr coord() noexcept = default;
  constexpr coord(column x, line y) noexcept : line_{y}, column_{x} {}
  constexpr coord(line y, column x) noexcept : line_{y}, column_{x} {}

  constexpr std::strong_ordering operator<=>(coord const &) const noexcept = default;

  constexpr auto get_line() const noexcept { return line_; }
  constexpr auto get_column() const noexcept { return column_; }

  void next_line() noexcept { ++line_; }
  void next_column() noexcept { ++column_; }

  void reset_column() noexcept {
    // The column number is set to 0. This is because the outer parse loop
    // automatically advances the column number for each character consumed.
    // This happens after the row is advanced by a matcher's consume() function.
    column_ = 0U;
  }

private:
  unsigned line_ = 1U;
  unsigned column_ = 1U;
};

enum class extensions : unsigned {
  none = 0U,
  bash_comments = 1U << 0U,
  single_line_comments = 1U << 1U,
  multi_line_comments = 1U << 2U,
  array_trailing_comma = 1U << 3U,
  object_trailing_comma = 1U << 4U,
  single_quote_string = 1U << 5U,
  leading_plus = 1U << 6U,
  extra_whitespace = 1 << 7U,
  identifier_object_key = 1 << 8U,
  string_escapes = 1 << 9U,
  numbers = 1 << 10U,
  all = ~none,
};

constexpr extensions operator|(extensions a, extensions b) noexcept {
  using ut = std::underlying_type_t<extensions>;
  static_assert(std::is_unsigned_v<ut>, "The extensions type must be unsigned");
  return static_cast<extensions>(static_cast<ut>(a) | static_cast<ut>(b));
}

struct default_policies {
  /// The maximum length of strings or identifiers that will be permitted before
  /// a string_too_long or identifier_too_long error is raised.
  static constexpr size_t max_length = 65535;

  using integer_type = std::int64_t;
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
template <typename Backend, typename Policies = default_policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
class parser {
  friend class details::matcher<Backend, Policies>;
  friend class details::root_matcher<Backend, Policies>;
  friend class details::whitespace_matcher<Backend, Policies>;

public:
  explicit parser(extensions extensions = extensions::none) : parser(Backend{}, extensions) {}

  explicit parser(Backend const &backend, extensions extensions = extensions::none);
  explicit parser(Backend &&backend, extensions extensions = extensions::none);

  parser(parser const &) = delete;
  parser(parser &&rhs) noexcept(std::is_nothrow_move_constructible_v<Backend>);

  ~parser() noexcept = default;

  parser &operator=(parser const &) = delete;
  parser &operator=(parser &&rhs) noexcept(std::is_nothrow_move_assignable_v<Backend>);

  ///@{
  /// Parses a chunk of JSON input. This function may be called repeatedly with
  /// portions of the source data (for example, as the data is received from an
  /// external source). Once all of the data has been received, call the
  /// parser::eof() method.

  /// \param span The span of bytes to be parsed.
  template <std::size_t Extent> parser &input(std::span<std::byte, Extent> const &span) {
    return this->input(std::begin(span), std::end(span));
  }
  /// \param span The span of bytes to be parsed.
  template <std::size_t Extent> parser &input(std::span<std::byte const, Extent> const &span) {
    return this->input(std::begin(span), std::end(span));
  }

  /// \param first The element in the half-open range of bytes to be parsed.
  /// \param last The end of the range of bytes to be parsed.
  template <std::input_iterator InputIterator>
    requires(std::is_same_v<std::decay_t<typename std::iterator_traits<InputIterator>::value_type>, std::byte>)
  parser &input(InputIterator first, InputIterator last);
  template <std::ranges::input_range Range> parser &input(Range const &range) {
    return this->input(range.begin(), range.end());
  }
  ///@}

  /// Informs the parser that the complete input stream has been passed by calls
  /// to parser<>::input().
  ///
  /// \returns If the parse completes successfully, Backend::result()
  /// is called and its result returned.
  decltype(auto) eof();

  ///@{

  /// \returns True if the parser has signalled an error.
  [[nodiscard]] constexpr bool has_error() const noexcept { return static_cast<bool>(error_); }
  /// \returns The error code held by the parser.
  [[nodiscard]] constexpr std::error_code const &last_error() const noexcept { return error_; }

  ///@{
  [[nodiscard]] constexpr Backend &backend() noexcept { return backend_; }
  [[nodiscard]] constexpr Backend const &backend() const noexcept { return backend_; }
  ///@}

  /// \param flag  A selection of bits from the parser_extensions enum.
  /// \returns True if any of the extensions given by \p flag are enabled by the parser.
  [[nodiscard]] constexpr bool extension_enabled(extensions const flag) const noexcept {
    using ut = std::underlying_type_t<extensions>;
    return (to_underlying(extensions_) & static_cast<ut>(flag)) != 0U;
  }

  /// Returns the parser's position in the input text.
  [[nodiscard]] constexpr coord input_pos() const noexcept { return pos_; }
  /// Returns the position of the most recent token in the input text.
  [[nodiscard]] constexpr coord pos() const noexcept { return matcher_pos_; }

private:
  using matcher = details::matcher<Backend, Policies>;
  using pointer = std::unique_ptr<matcher, details::deleter<matcher>>;

  static constexpr auto null_pointer() {
    using deleter = typename pointer::deleter_type;
    return pointer{nullptr, deleter{deleter::mode::do_nothing}};
  }

  void consume_code_point(char32_t code_point);

  ///@{
  /// \brief Managing the column and row number (the "coordinate").

  /// Increments the column number.
  void advance_column() noexcept { pos_.next_column(); }

  /// Increments the row number and resets the column.
  void advance_row() noexcept {
    // The column number is set to 0. This is because the outer parse loop
    // automatically advances the column number for each character consumed.
    // This happens after the row is advanced by a matcher's consume() function.
    this->reset_column();
    pos_.next_line();
  }

  /// Resets the column count but does not affect the row number.
  void reset_column() noexcept { pos_.reset_column(); }
  ///@}

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
  ///@}

  pointer make_root_matcher();
  pointer make_whitespace_matcher();
  pointer make_string_matcher(bool object_key, char32_t enclosing_char);
  pointer make_identifier_matcher();

  void init_stack();

  template <typename Matcher, typename... Args>
    requires(std::derived_from<Matcher, matcher>)
  pointer make_terminal_matcher(Args &&...args) {
    Matcher &m = singletons_.terminals.template emplace<Matcher>(std::forward<Args>(args)...);
    using deleter = typename pointer::deleter_type;
    return pointer{&m, deleter{deleter::mode::do_nothing}};
  }

  /// The parse stack likely contains pointers into the singletons_ object.
  /// After move-construction or move-assignment, this function adjusts those
  /// pointers so that they point to this object rather than the original.
  void reseat_stack_after_move(parser const &rhs) noexcept;

  /// The maximum depth to which we allow the parse stack to grow. This value
  /// should be sufficient for any reasonable input: its intention is to prevent
  /// bogus (attack) inputs from causing the parser's memory consumption to grow
  /// uncontrollably.
  static constexpr std::size_t max_stack_depth_ = 200;

  icubaby::tx_32 utf_;
  /// The parse stack.
  stack<pointer> stack_;
  std::error_code error_;

  /// Preallocated storage for "terminal" matchers. These are the matchers,
  /// such as numbers or strings which can't have child objects.
  details::singletons<Backend, Policies> singletons_;

  /// Each instance of the string and identifier matcher uses this object to
  /// record its output. This avoids having to create a new instance each time
  /// we scan a string.
  std::unique_ptr<arrayvec<char8, Policies::max_length>> str_buffer_;

  /// The column and row number of the parse within the input stream.
  coord pos_;
  coord matcher_pos_;
  extensions extensions_;
  PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE Backend backend_;
};

template <typename Backend, typename Policies = default_policies>
parser(Backend, Policies) -> parser<Backend, Policies>;
template <typename Backend> parser(Backend) -> parser<Backend, default_policies>;

template <typename Policies, typename Backend>
  requires(policy<Policies> && backend<std::remove_reference_t<Backend>, typename Policies::integer_type>)
inline decltype(auto) make_parser(Backend &&backend, extensions const extensions = extensions::none) {
  return parser<std::remove_reference_t<Backend>, Policies>{std::forward<Backend>(backend), extensions};
}

template <typename Backend>
  requires(backend<Backend>)
inline decltype(auto) make_parser(Backend &&backend, extensions const extensions = extensions::none) {
  return parser<std::remove_reference_t<Backend>>{std::forward<Backend>(backend), extensions};
}

enum char_set : char32_t {
  apostrophe = char32_t{0x0027},  // "'"
  asterisk = char32_t{0x002A},    // '*'
  backspace = char32_t{0x0008},   // '\b'
  byte_order_mark = icubaby::byte_order_mark,
  carriage_return = char32_t{0x000D},       // '\r'
  character_tabulation = char32_t{0x0009},  // '\t'
  colon = char32_t{0x003A},                 // ':'
  digit_eight = char32_t{0x0038},           // '8'
  digit_five = char32_t{0x0035},            // '5'
  digit_four = char32_t{0x0034},            // '4'
  digit_nine = char32_t{0x0039},            // '9'
  digit_one = char32_t{0x0031},             // '1'
  digit_seven = char32_t{0x0037},           // '7'
  digit_six = char32_t{0x0036},             // '6'
  digit_three = char32_t{0x0033},           // '3'
  digit_two = char32_t{0x0032},             // '2'
  digit_zero = char32_t{0x0030},            // '0'
  dollar_sign = char32_t{0x0024},           // '$'
  en_quad = char32_t{0x2000},
  form_feed = char32_t{0x000C},               // '\f'
  full_stop = char32_t{0x002E},               // '.'
  hyphen_minus = char32_t{0x002D},            // '-'
  latin_capital_letter_a = char32_t{0x0041},  // 'A'
  latin_capital_letter_e = char32_t{0x0045},  // 'E'
  latin_capital_letter_f = char32_t{0x0046},  // 'F'
  latin_capital_letter_i = char32_t{0x0049},  // 'I'
  latin_capital_letter_n = char32_t{0x004E},  // 'N'
  latin_capital_letter_x = char32_t{0x0058},  // 'X'
  latin_capital_letter_z = char32_t{0x005A},  // 'Z'
  latin_small_letter_a = char32_t{0x0061},    // 'a'
  latin_small_letter_b = char32_t{0x0062},    // 'b'
  latin_small_letter_e = char32_t{0x0065},    // 'e'
  latin_small_letter_f = char32_t{0x0066},    // 'f'
  latin_small_letter_n = char32_t{0x006E},    // 'n'
  latin_small_letter_r = char32_t{0x0072},    // 'r'
  latin_small_letter_t = char32_t{0x0074},    // 't'
  latin_small_letter_u = char32_t{0x0075},    // 'u'
  latin_small_letter_v = char32_t{0x0076},    // 'v'
  latin_small_letter_x = char32_t{0x0078},    // 'x'
  latin_small_letter_z = char32_t{0x007A},    // 'z'
  line_feed = char32_t{0x000A},               // '\n'
  line_separator = char32_t{0x2028},
  low_line = char32_t{0x005f},  // '_'
  no_break_space = char32_t{0x00A0},
  null_char = char32_t{0x0000},
  number_sign = char32_t{0x0023},  // '#'
  paragraph_separator = char32_t{0x2029},
  plus_sign = char32_t{0x002B},        // '+'
  quotation_mark = char32_t{0x0022},   // '"'
  reverse_solidus = char32_t{0x005C},  // '\'
  solidus = char32_t{0x002F},          // '/'
  space = char32_t{0x0020},            // ' '
  vertical_tabulation = char32_t{0x000B},
};

namespace details {

constexpr std::optional<uint_least16_t> digit_offset(char32_t code_point) noexcept {
  if (code_point >= char_set::digit_zero && code_point <= char_set::digit_nine) {
    return static_cast<uint_least16_t>(char_set::digit_zero);
  }
  if (code_point >= char_set::latin_small_letter_a && code_point <= char_set::latin_small_letter_f) {
    return static_cast<uint_least16_t>(char_set::latin_small_letter_a - 10U);
  }
  if (code_point >= char_set::latin_capital_letter_a && code_point <= char_set::latin_capital_letter_f) {
    return static_cast<uint_least16_t>(char_set::latin_capital_letter_a - 10U);
  }
  return std::nullopt;
}

constexpr std::optional<grammar_rule> code_point_grammar_rule(char32_t const code_point) noexcept {
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const end = std::end(code_point_runs);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const it = std::lower_bound(
      std::begin(code_point_runs), end, cprun{code_point, 0, 0},
      [](cprun const &cpr, cprun const &value) { return cpr.code_point + cpr.length < value.code_point; });
  if (it != end && code_point >= it->code_point && code_point < static_cast<char32_t>(it->code_point + it->length)) {
    return static_cast<grammar_rule>(it->rule);
  }
  return {};
}

//*             _      _             *
//*  _ __  __ _| |_ __| |_  ___ _ _  *
//* | '  \/ _` |  _/ _| ' \/ -_) '_| *
//* |_|_|_\__,_|\__\__|_||_\___|_|   *
//*                                  *
/// \brief The base class for the various state machines ("matchers") which
///   implement the productions of the JSON grammar.
template <typename Backend, typename Policies>
  requires(backend<Backend>)
class matcher {
  template <int FirstHexState, int LastHexState, int PostState> friend class hex_consumer;

public:
  using parser_type = parser<Backend, Policies>;
  using pointer = std::unique_ptr<matcher, deleter<matcher>>;

  matcher(matcher const &) = delete;
  virtual ~matcher() noexcept = default;
  matcher &operator=(matcher const &) = delete;

  /// Called for each character as it is consumed from the input.
  ///
  /// \param parser The owning parser instance.
  /// \param ch If true, the character to be consumed. An empty value value indicates
  ///   end-of-file.
  /// \returns A pair consisting of a matcher pointer and a boolean. If non-null, the
  ///   matcher is pushed onto the parse stack; if null the same matcher object
  ///   is used to process the next character. The boolean value is false if the
  ///   same character must be passed to the next consume() call; true indicates
  ///   that the character was correctly matched by this consume() call.
  virtual std::pair<pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) = 0;

  /// \returns True if this matcher has completed (and reached it's "done" state). The
  /// parser will pop this instance from the parse stack before continuing.
  [[nodiscard]] bool is_done() const noexcept { return state_ == done; }

protected:
  explicit constexpr matcher(int const initial_state) noexcept : state_{initial_state} {}

  matcher(matcher &&) noexcept = default;
  matcher &operator=(matcher &&) noexcept = default;

  ///@{
  /// \brief Errors

  /// \returns True if the parser is in an error state.
  bool set_error(parser_type &parser, std::error_code const &err) noexcept {
    bool const has_error = parser.set_error(err);
    if (has_error) {
      state_ = done;
    }
    return has_error;
  }
  ///@}

  pointer make_root_matcher(parser_type &parser) { return parser.make_root_matcher(); }
  pointer make_whitespace_matcher(parser_type &parser) { return parser.make_whitespace_matcher(); }
  pointer make_string_matcher(parser_type &parser, bool object_key, char32_t enclosing_char) {
    return parser.make_string_matcher(object_key, enclosing_char);
  }
  pointer make_identifier_matcher(parser_type &parser) { return parser.make_identifier_matcher(); }

  template <typename Matcher, typename... Args> pointer make_terminal_matcher(parser_type &parser, Args &&...args) {
    return parser.template make_terminal_matcher<Matcher, Args...>(std::forward<Args>(args)...);
  }

  static constexpr auto null_pointer() { return parser_type::null_pointer(); }

  /// The value to be used for the "done" state in the each of the matcher state
  /// machines.
  static constexpr auto done = 1;

  int state_;
};

//*  _       _                                                 *
//* | |_ ___| |_____ _ _    __ ___ _ _  ____  _ _ __  ___ _ _  *
//* |  _/ _ \ / / -_) ' \  / _/ _ \ ' \(_-< || | '  \/ -_) '_| *
//*  \__\___/_\_\___|_||_| \__\___/_||_/__/\_,_|_|_|_\___|_|   *
//*                                                            *
class token_consumer {
public:
  constexpr token_consumer() noexcept = default;
  explicit constexpr token_consumer(u8string_view text) noexcept : text_{text} {}
  enum class result { match, fail, more };

  void set_text(u8string_view text) noexcept { text_ = text; }

  result match(char32_t const code_point) noexcept {
    auto const c = text_.front();
    assert(icubaby::is_code_point_start(c) && is_identifier_cp(static_cast<char32_t>(c)));
    if (code_point != static_cast<char32_t>(c)) {
      return result::fail;
    }
    text_.remove_prefix(1);
    return text_.empty() ? result::match : result::more;
  }

  /// Checks if the given code point is valid in an identifier.
  static constexpr bool is_identifier_cp(char32_t const code_point) noexcept {
    if (code_point >= char_set::digit_zero && code_point <= char_set::digit_nine) {
      return true;
    }
    if (code_point >= char_set::latin_capital_letter_a && code_point <= char_set::latin_capital_letter_z) {
      return true;
    }
    if (code_point >= char_set::latin_small_letter_a && code_point <= char_set::latin_small_letter_z) {
      return true;
    }
    // U+0080 Is where the Latin-1 supplement starts. Consult the table for
    // code points beyond this point.
    if (code_point >= 0x80) {
      if ((to_underlying(code_point_grammar_rule(code_point).value_or(static_cast<grammar_rule>(0))) & idmask) != 0U) {
        return true;
      }
    }
    return false;
  }

private:
  u8string_view text_;
};

//*  _       _             *
//* | |_ ___| |_____ _ _   *
//* |  _/ _ \ / / -_) ' \  *
//*  \__\___/_\_\___|_||_| *
//*                        *
/// \brief A matcher which checks for a specific keyword such as "true",
///   "false", or "null".
/// \tparam Backend  The parser callback structure.
template <typename Backend, typename Policies, typename DoneFunction>
  requires(backend<Backend> && std::invocable<DoneFunction, parser<Backend, Policies> &>)
class token_matcher : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename matcher<Backend, Policies>::parser_type;

  /// \param text  The string to be matched.
  /// \param donefn  The function called when the source string is matched.
  constexpr token_matcher(u8string_view text, DoneFunction donefn) noexcept
      : inherited(start_state), text_{text}, done_{donefn} {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    start_state,
    last_state,
  };

  /// The keyword to be matched. The input sequence must exactly match this
  /// string or an unrecognized token error is raised. Once all of the
  /// characters are matched, done_() is called.
  token_consumer text_;
  /// This function is called once the complete token text has been matched.
  PEEJAY_NO_UNIQUE_ADDRESS_ATTRIBUTE DoneFunction done_;
};

template <typename Backend, typename Policies, typename DoneFunction>
  requires(backend<Backend> && std::invocable<DoneFunction, parser<Backend, Policies> &>)
auto token_matcher<Backend, Policies, DoneFunction>::consume(parser_type &parser, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  bool match = true;
  switch (state_) {
  case start_state:
    assert(!ch || icubaby::is_code_point_start(*ch));
    if (!ch) {
      this->set_error(parser, error::unrecognized_token);
      break;
    }
    switch (text_.match(*ch)) {
    case token_consumer::result::fail: this->set_error(parser, error::unrecognized_token); break;
    case token_consumer::result::match: state_ = last_state; break;
    case token_consumer::result::more:
    default: break;
    }
    break;
  case last_state:
    if (ch) {
      if (token_consumer::is_identifier_cp(*ch)) {
        this->set_error(parser, error::unrecognized_token);
        return {matcher<Backend, Policies>::null_pointer(), true};
      }
      match = false;
    }
    this->set_error(parser, done_(parser));
    state_ = done_state;
    break;
  case done_state:
  default: unreachable(); break;
  }

  return {matcher<Backend, Policies>::null_pointer(), match};
}

//*   __      _           _       _             *
//*  / _|__ _| |___ ___  | |_ ___| |_____ _ _   *
//* |  _/ _` | (_-</ -_) |  _/ _ \ / / -_) ' \  *
//* |_| \__,_|_/__/\___|  \__\___/_\_\___|_||_| *
//*                                             *
/// A function object invoked when the "false" token is matched.
template <typename Backend, typename Policies> struct false_complete {
  std::error_code operator()(parser<Backend, Policies> &p) const { return p.backend().boolean_value(false); }
};

/// Matches a "false" token.
template <typename Backend, typename Policies>
class false_token_matcher : public token_matcher<Backend, Policies, false_complete<Backend, Policies>> {
public:
  false_token_matcher() : token_matcher<Backend, Policies, false_complete<Backend, Policies>>(u8"false", {}) {}
};

//*  _                  _       _             *
//* | |_ _ _ _  _ ___  | |_ ___| |_____ _ _   *
//* |  _| '_| || / -_) |  _/ _ \ / / -_) ' \  *
//*  \__|_|  \_,_\___|  \__\___/_\_\___|_||_| *
//*                                           *
/// A function object invoked when the "true" token is matched.
template <typename Backend, typename Policies> struct true_complete {
  std::error_code operator()(parser<Backend, Policies> &p) const { return p.backend().boolean_value(true); }
};

/// Matches a "true" token.
template <typename Backend, typename Policies>
class true_token_matcher : public token_matcher<Backend, Policies, true_complete<Backend, Policies>> {
public:
  true_token_matcher() : token_matcher<Backend, Policies, true_complete<Backend, Policies>>(u8"true", {}) {}
};

//*           _ _   _       _             *
//*  _ _ _  _| | | | |_ ___| |_____ _ _   *
//* | ' \ || | | | |  _/ _ \ / / -_) ' \  *
//* |_||_\_,_|_|_|  \__\___/_\_\___|_||_| *
//*                                       *
/// A function object invoked when the "null" token is matched.
template <typename Backend, typename Policies> struct null_complete {
  std::error_code operator()(parser<Backend, Policies> &p) const { return p.backend().null_value(); }
};

/// Matches a "null" token.
template <typename Backend, typename Policies>
class null_token_matcher : public token_matcher<Backend, Policies, null_complete<Backend, Policies>> {
public:
  null_token_matcher() : token_matcher<Backend, Policies, null_complete<Backend, Policies>>(u8"null", {}) {}
};

//*  _       __ _      _ _          _       _             *
//* (_)_ _  / _(_)_ _ (_) |_ _  _  | |_ ___| |_____ _ _   *
//* | | ' \|  _| | ' \| |  _| || | |  _/ _ \ / / -_) ' \  *
//* |_|_||_|_| |_|_||_|_|\__|\_, |  \__\___/_\_\___|_||_| *
//*                          |__/                         *
/// A function object invoked when the "Infinity" token is matched.
template <typename Backend, typename Policies> struct infinity_complete {
  std::error_code operator()(parser<Backend, Policies> &p) const {
    return p.backend().double_value(std::numeric_limits<double>::infinity());
  }
};

/// Matches an "Infinity" token.
template <typename Backend, typename Policies>
class infinity_token_matcher : public token_matcher<Backend, Policies, infinity_complete<Backend, Policies>> {
public:
  infinity_token_matcher() : token_matcher<Backend, Policies, infinity_complete<Backend, Policies>>(u8"Infinity", {}) {}
};

//*                   _       _             *
//*  _ _  __ _ _ _   | |_ ___| |_____ _ _   *
//* | ' \/ _` | ' \  |  _/ _ \ / / -_) ' \  *
//* |_||_\__,_|_||_|  \__\___/_\_\___|_||_| *
//*                                         *
/// A function object invoked when the "NaN" token is matched.
template <typename Backend, typename Policies> struct nan_complete {
  std::error_code operator()(parser<Backend, Policies> &p) const {
    return p.backend().double_value(std::numeric_limits<double>::quiet_NaN());
  }
};

/// Matches an "NaN" token.
template <typename Backend, typename Policies>
class nan_token_matcher : public token_matcher<Backend, Policies, nan_complete<Backend, Policies>> {
public:
  nan_token_matcher() : token_matcher<Backend, Policies, nan_complete<Backend, Policies>>(u8"NaN", {}) {}
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
/// Matches a number.
template <typename Backend, typename Policies> class number_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;
  using uinteger_type = std::make_unsigned_t<typename Policies::integer_type>;
  using sinteger_type = std::make_signed_t<typename Policies::integer_type>;

  constexpr number_matcher() noexcept : inherited(leading_minus_state) {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;
  [[nodiscard]] bool in_terminal_state() const;

  bool do_leading_minus_state(parser_type &parser, char32_t c);
  /// Implements the first character of the 'int' production.
  bool do_integer_initial_digit_state(parser_type &parser, char32_t c);
  bool do_integer_digit_state(parser_type &parser, char32_t c);
  bool do_frac_state(parser_type &parser, char32_t c);
  bool do_frac_digit_state(parser_type &parser, char32_t c);
  bool do_exponent_sign_state(parser_type &parser, char32_t c);
  bool do_exponent_digit_state(parser_type &parser, char32_t c);
  bool do_hex_digits_state(parser_type &parser, char32_t c);

  void complete(parser_type &parser);
  void number_is_float();

  void make_result(parser_type &parser);
  std::pair<typename inherited::pointer, bool> end(parser_type &parser);

  enum state {
    done_state = inherited::done,
    leading_minus_state,
    integer_initial_digit_state,
    integer_digit_state,

    frac_state,
    frac_initial_digit_state,
    frac_digit_state,

    exponent_sign_state,
    exponent_initial_digit_state,
    exponent_digit_state,

    initial_hex_digit,
    hex_digits,

    // The same as frac_initial_digit_state but used after
    // a leading dot to enable a lone dot character to be
    // rejected.
    initial_dot_state,

    match_token_infinity_state,
    match_token_nan_state,
    end_token_state,
  };

  token_consumer text_;
  bool is_neg_ = false;
  struct float_accumulator {
    /// Promote from integer.
    explicit constexpr float_accumulator(uinteger_type v) noexcept : whole_part{static_cast<double>(v)} {}
    /// Assign an explicit double.
    explicit constexpr float_accumulator(double v) noexcept : whole_part{v} {}

    void add_digit(unsigned const digit) {
      frac_part = frac_part * 10.0 + digit;
      frac_scale *= 10;
    }

    double frac_part = 0.0;
    double frac_scale = 1.0;
    double whole_part = 0.0;

    bool exp_is_negative = false;
    unsigned exponent = 0U;
  };

  std::variant<uinteger_type, float_accumulator> acc_;
};

// number is float
// ~~~~~~~~~~~~~~~
template <typename Backend, typename Policies> void number_matcher<Backend, Policies>::number_is_float() {
  if (std::holds_alternative<uinteger_type>(acc_)) {
    acc_ = float_accumulator{std::get<uinteger_type>(acc_)};
  }
}

// in terminal state
// ~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies> bool number_matcher<Backend, Policies>::in_terminal_state() const {
  switch (state_) {
  case end_token_state:
  case exponent_digit_state:
  case frac_digit_state:
  case frac_state:
  case hex_digits:
  case integer_digit_state:
  case done_state: return true;
  default: return false;
  }
}

// leading minus state
// ~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_leading_minus_state(parser_type &parser, char32_t c) {
  bool match = true;
  if (c == char_set::hyphen_minus) {
    state_ = integer_initial_digit_state;
    is_neg_ = true;
  } else if (c == char_set::plus_sign) {
    assert(parser.extension_enabled(extensions::leading_plus));
    state_ = integer_initial_digit_state;
  } else if (c >= char_set::digit_zero && c <= char_set::digit_nine) {
    state_ = integer_initial_digit_state;
    match = do_integer_initial_digit_state(parser, c);
  } else if (c == char_set::full_stop) {
    assert(parser.extension_enabled(extensions::numbers));
    state_ = initial_dot_state;
  } else {
    // minus MUST be followed by the 'int' production.
    this->set_error(parser, error::number_out_of_range);
    unreachable();
  }
  return match;
}

// frac state
// ~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_frac_state(parser_type &parser, char32_t const c) {
  bool match = true;
  switch (c) {
  case char_set::full_stop: state_ = frac_initial_digit_state; break;
  case char_set::latin_small_letter_e:
  case char_set::latin_capital_letter_e: state_ = exponent_sign_state; break;
  case char_set::digit_zero:
  case char_set::digit_one:
  case char_set::digit_two:
  case char_set::digit_three:
  case char_set::digit_four:
  case char_set::digit_five:
  case char_set::digit_six:
  case char_set::digit_seven:
  case char_set::digit_eight:
  case char_set::digit_nine:
    // digits are definitely not part of the next token so we can issue an error
    // right here.
    this->set_error(parser, error::number_out_of_range);
    break;
  case char_set::latin_small_letter_x:
  case char_set::latin_capital_letter_x:
    if (parser.extension_enabled(extensions::numbers)) {
      state_ = initial_hex_digit;
    } else {
      this->set_error(parser, error::number_out_of_range);
    }
    break;
  default:
    // the 'frac' production is optional.
    match = false;
    this->complete(parser);
    break;
  }
  return match;
}

// frac digit
// ~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_frac_digit_state(parser_type &parser, char32_t const c) {
  auto const state = (state_ == initial_dot_state) ? frac_initial_digit_state : state_;
  assert(state == frac_initial_digit_state || state == frac_digit_state);

  bool match = true;
  switch (c) {
  case char_set::latin_small_letter_e:
  case char_set::latin_capital_letter_e:
    this->number_is_float();
    if (state == frac_initial_digit_state) {
      this->set_error(parser, error::unrecognized_token);
    } else {
      state_ = exponent_sign_state;
    }
    break;

  case char_set::digit_zero:
  case char_set::digit_one:
  case char_set::digit_two:
  case char_set::digit_three:
  case char_set::digit_four:
  case char_set::digit_five:
  case char_set::digit_six:
  case char_set::digit_seven:
  case char_set::digit_eight:
  case char_set::digit_nine: {
    this->number_is_float();
    std::get<float_accumulator>(acc_).add_digit(c - char_set::digit_zero);
    state_ = frac_digit_state;
  } break;

  default:
    if ((state == frac_initial_digit_state && !parser.extension_enabled(extensions::numbers)) ||
        this->state_ == initial_dot_state) {
      this->set_error(parser, error::unrecognized_token);
    } else {
      match = false;
      this->complete(parser);
    }
    break;
  }
  return match;
}

// exponent sign state
// ~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_exponent_sign_state(parser_type &parser, char32_t c) {
  bool match = true;
  this->number_is_float();
  auto &fp_acc = std::get<float_accumulator>(acc_);
  state_ = exponent_initial_digit_state;
  switch (c) {
  case '+': fp_acc.exp_is_negative = false; break;
  case '-': fp_acc.exp_is_negative = true; break;
  default: match = this->do_exponent_digit_state(parser, c); break;
  }
  return match;
}

// complete
// ~~~~~~~~
template <typename Backend, typename Policies> void number_matcher<Backend, Policies>::complete(parser_type &parser) {
  state_ = done_state;
  this->make_result(parser);
}

// exponent digit
// ~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_exponent_digit_state(parser_type &parser, char32_t const c) {
  assert(state_ == exponent_digit_state || state_ == exponent_initial_digit_state);
  assert(std::holds_alternative<float_accumulator>(acc_));

  bool match = true;
  if (c >= char_set::digit_zero && c <= char_set::digit_nine) {
    auto &fp_acc = std::get<float_accumulator>(acc_);
    fp_acc.exponent = fp_acc.exponent * 10U + static_cast<unsigned>(c - char_set::digit_zero);
    state_ = exponent_digit_state;
  } else {
    if (state_ == exponent_initial_digit_state) {
      this->set_error(parser, error::unrecognized_token);
    } else {
      match = false;
      this->complete(parser);
    }
  }
  return match;
}

// do integer initial digit state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_integer_initial_digit_state(parser_type &parser, char32_t const c) {
  using namespace std::string_view_literals;

  assert(state_ == integer_initial_digit_state);
  assert(std::holds_alternative<uinteger_type>(acc_));
  if (c == char_set::digit_zero) {
    state_ = frac_state;
  } else if (c >= char_set::digit_one && c <= char_set::digit_nine) {
    assert(std::get<uinteger_type>(acc_) == 0);
    std::get<uinteger_type>(acc_) = static_cast<uinteger_type>(c) - char_set::digit_zero;
    state_ = integer_digit_state;
  } else if (c == char_set::latin_capital_letter_i) {
    text_.set_text(u8"nfinity"sv);
    state_ = match_token_infinity_state;
  } else if (c == char_set::latin_capital_letter_n) {
    text_.set_text(u8"aN"sv);
    state_ = match_token_nan_state;
  } else {
    this->set_error(parser, error::unrecognized_token);
  }
  return true;
}

// do integer digit state
// ~~~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_integer_digit_state(parser_type &parser, char32_t const c) {
  assert(state_ == integer_digit_state);
  assert(std::holds_alternative<uinteger_type>(acc_));

  bool match = true;
  if (c == char_set::full_stop) {
    state_ = frac_initial_digit_state;
    number_is_float();
  } else if (c == char_set::latin_small_letter_e || c == char_set::latin_capital_letter_e) {
    state_ = exponent_sign_state;
    number_is_float();
  } else if (c >= '0' && c <= '9') {
    auto &int_acc = std::get<uinteger_type>(acc_);
    auto const new_acc = static_cast<uinteger_type>(int_acc * 10U + static_cast<uinteger_type>(c) - '0');
    if (new_acc < int_acc) {  // Did this overflow?
      this->set_error(parser, error::number_out_of_range);
    }
    int_acc = new_acc;
  } else {
    match = false;
    this->complete(parser);
  }
  return match;
}

// do hex digits state
// ~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
bool number_matcher<Backend, Policies>::do_hex_digits_state(parser_type &parser, char32_t const c) {
  auto const offset = digit_offset(c);
  if (!offset) {
    this->complete(parser);
    return false;
  }

  auto &int_acc = std::get<uinteger_type>(acc_);
  auto const new_acc = static_cast<uinteger_type>(int_acc * 16U + static_cast<uinteger_type>(c) - *offset);
  if (new_acc < int_acc) {  // Did this overflow?
    this->set_error(parser, error::number_out_of_range);
  }
  int_acc = new_acc;
  return true;
}

// end
// ~~~
template <typename Backend, typename Policies>
auto number_matcher<Backend, Policies>::end(parser_type &parser) -> std::pair<typename inherited::pointer, bool> {
  assert(!parser.has_error());
  if (!this->in_terminal_state()) {
    switch (state_) {
    case match_token_infinity_state:
    case match_token_nan_state: this->set_error(parser, error::unrecognized_token); break;
    case frac_initial_digit_state:
      // A trailing dot.
      if (parser.extension_enabled(extensions::numbers)) {
        break;
      }
      [[fallthrough]];
    default: this->set_error(parser, error::expected_digits); break;
    }
  }
  this->complete(parser);
  return {null_pointer(), true};
}

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto number_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (!ch) {
    return this->end(parser);
  }

  bool match = true;
  auto const c = *ch;
  switch (state_) {
  case leading_minus_state: match = this->do_leading_minus_state(parser, c); break;
  case integer_initial_digit_state: match = this->do_integer_initial_digit_state(parser, c); break;
  case integer_digit_state: match = this->do_integer_digit_state(parser, c); break;
  case frac_state: match = this->do_frac_state(parser, c); break;
  case initial_dot_state:
  case frac_initial_digit_state:
  case frac_digit_state: match = this->do_frac_digit_state(parser, c); break;
  case exponent_sign_state: match = this->do_exponent_sign_state(parser, c); break;
  case exponent_initial_digit_state:
  case exponent_digit_state: match = this->do_exponent_digit_state(parser, c); break;
  case initial_hex_digit:
    if (!digit_offset(c)) {
      this->set_error(parser, error::expected_digits);
      break;
    }
    state_ = hex_digits;
    [[fallthrough]];
  case hex_digits: match = this->do_hex_digits_state(parser, c); break;

  case match_token_infinity_state:
  case match_token_nan_state:
    switch (text_.match(*ch)) {
    case token_consumer::result::more: break;
    case token_consumer::result::match:
      acc_ = float_accumulator{state_ == match_token_infinity_state ? std::numeric_limits<double>::infinity()
                                                                    : std::numeric_limits<double>::quiet_NaN()};
      state_ = end_token_state;
      break;
    case token_consumer::result::fail:
    default: this->set_error(parser, error::unrecognized_token); break;
    }
    break;
  case end_token_state:
    if (ch) {
      if (token_consumer::is_identifier_cp(*ch)) {
        this->set_error(parser, error::unrecognized_token);
        return {null_pointer(), true};
      }
      match = false;
    }
    this->complete(parser);
    break;

  case done_state:
  default: unreachable(); break;
  }

  return {null_pointer(), match};
}

// make result
// ~~~~~~~~~~~
template <typename Backend, typename Policies>
void number_matcher<Backend, Policies>::make_result(parser_type &parser) {
  if (parser.has_error()) {
    return;
  }
  assert(this->in_terminal_state());

  if (std::holds_alternative<uinteger_type>(acc_)) {
    constexpr auto min = std::numeric_limits<sinteger_type>::min();
    constexpr auto umin = static_cast<uinteger_type>(min);
    constexpr auto umax = static_cast<uinteger_type>(std::numeric_limits<sinteger_type>::max());

    // TODO: this doesn't automatically promote v. large integers to float.
    // Perhaps it should?
    auto &int_acc = std::get<uinteger_type>(acc_);
    if (is_neg_) {
      if (int_acc > umin) {
        this->set_error(parser, error::number_out_of_range);
      } else {
        this->set_error(parser,
                        parser.backend().integer_value((int_acc == umin) ? min : -static_cast<sinteger_type>(int_acc)));
      }
    } else {
      if (int_acc > umax) {
        this->set_error(parser, error::number_out_of_range);
      } else {
        this->set_error(parser, parser.backend().integer_value(static_cast<sinteger_type>(int_acc)));
      }
    }
    return;
  }

  auto &fp_acc = std::get<float_accumulator>(acc_);
  auto xf = (fp_acc.whole_part + fp_acc.frac_part / fp_acc.frac_scale);
  auto exp = std::pow(10, fp_acc.exponent);
  if (std::isinf(exp)) {
    this->set_error(parser, error::number_out_of_range);
    return;
  }
  if (fp_acc.exp_is_negative) {
    exp = 1.0 / exp;
  }

  xf *= exp;
  if (is_neg_) {
    xf = -xf;
  }

  // Is the fractional part of the float 0 (i.e. could we potentially cast it to
  // one of the integer types)? This enables us to treat input such as "1.0" in
  // the same way as "1".
  if (std::rint(xf) == xf && xf >= static_cast<decltype(xf)>(std::numeric_limits<sinteger_type>::min()) &&
      xf <= static_cast<decltype(xf)>(std::numeric_limits<sinteger_type>::max())) {
    this->set_error(parser, parser.backend().integer_value(static_cast<sinteger_type>(xf)));
    return;
  }

  this->set_error(parser, parser.backend().double_value(xf));
}

//*  _                                                 *
//* | |_  _____ __  __ ___ _ _  ____  _ _ __  ___ _ _  *
//* | ' \/ -_) \ / / _/ _ \ ' \(_-< || | '  \/ -_) '_| *
//* |_||_\___/_\_\ \__\___/_||_/__/\_,_|_|_|_\___|_|   *
//*                                                    *
/// \brief Processing of the four hex digits for the UTF-16 escape sequence
///   used by both string and identifier objects.
///
/// \tparam FirstHexState  The initial hex character state.
/// \tparam LastHexState The final hex character state.
/// \tparam PostState The state to which the matcher will be switched after
///                   the four hex characters have been consumed.
template <int FirstHexState, int LastHexState, int PostState> class hex_consumer {
public:
  static constexpr auto first_hex_state = FirstHexState;
  static constexpr auto last_hex_state = LastHexState;
  static constexpr auto post_hex_state = PostState;

  /// Returns true if we have processed a part of a UTF-16 high/surrogate pair.
  [[nodiscard]] constexpr bool partial() const noexcept { return utf_16_to_8_.partial(); }

  /// Signal the start of a two or four hex digit sequence.
  ///
  /// \p is_utf_16  Pass true if expecting a four digit UTF-16 sequence,
  ///               false if two digit UTF-8 hex digits should follow.
  void start(bool const is_utf_16) noexcept {
    utf16_ = is_utf_16;
    hex_ = 0U;
  }
  /// Processes a code point as part of a hex escape sequence (\\uXXXX) for a
  /// string or identifier.
  ///
  /// \p state  The matcher's current state.
  /// \p out The container to which a completed code point will be appended.
  /// \p code_point  The Unicode code point to be added to the hex sequence.
  /// \returns Either an integer representing the next state that the matcher should assume or an error code.
  template <typename OutputIterator>
  std::variant<std::error_code, std::pair<int, OutputIterator>> consume(int const state, char32_t const code_point,
                                                                        OutputIterator out) noexcept {
    assert(state >= first_hex_state && state <= last_hex_state && "must be one of the hex states");
    auto const offset = digit_offset(code_point);
    if (!offset) {
      return error::invalid_hex_char;
    }
    hex_ = static_cast<uint_least16_t>(16U * hex_ + static_cast<uint_least16_t>(code_point) - *offset);
    if (state < LastHexState) {
      // More hex characters to go.
      return std::make_pair(state + 1, out);
    }
    // Convert the hex characters to either UTF-8 or -16.
    if (utf16_) {
      out = utf_16_to_8_(hex_, out);
      if (!utf_16_to_8_.well_formed()) {
        return error::bad_unicode_code_point;
      }
    } else {
      assert(hex_ <= 0xFF && "Two hex digits can't produce a value > 0xFF!");
      *(out++) = static_cast<char8>(hex_);
    }
    // Reset hex_ for the next sequence.
    hex_ = 0;
    return std::make_pair(PostState, out);
  }

private:
  /// UTF-16 to UTF-8 converter.
  icubaby::t16_8 utf_16_to_8_;
  /// true if expecting a four digit UTF-16 sequence, false if two digit UTF-8
  /// hex digits should follow.
  bool utf16_ = false;
  /// Used to accumulate the code point value from the four hex digits. After
  /// the four digits have been consumed, this UTF-16 code point value is
  /// converted to UTF-8 and added to the output.
  uint_least16_t hex_ = 0U;
};

/// \name hex_consumer<> states
/// The hex_consumer<> template class implements the handling of UTF-16
/// hexadecimal escape codes for both string and identifier productions. To
/// minimize our code size, it's useful to ensure that the template arguments
/// for its instantiations are the same. To acheive this, we predefine the
/// expected first and last hex states for the state machines here. We later
/// statically assert that the values used are consistent.

///@{
constexpr auto first_hex_state = 2;
constexpr auto last_hex_state = 5;
constexpr auto post_hex_state = 6;
///@}

//*     _       _            *
//*  __| |_ _ _(_)_ _  __ _  *
//* (_-<  _| '_| | ' \/ _` | *
//* /__/\__|_| |_|_||_\__, | *
//*                   |___/  *
/// Matches a string.
template <typename Backend, typename Policies> class string_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  string_matcher(arrayvec<char8, Policies::max_length> *const str, bool object_key, char32_t enclosing_char) noexcept
      : inherited(start_state), is_object_key_{object_key}, enclosing_char_{enclosing_char}, str_{str} {
    assert(str != nullptr);
    str->clear();
  }

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    hex1_state,
    hex2_state,
    hex3_state,
    hex4_state,
    normal_char_state,
    start_state,
    escape_state,
    skip_lf_state,
  };

  std::variant<state, std::error_code> consume_normal(parser_type &p, bool is_object_key, char32_t enclosing_char,
                                                      char32_t code_point);

  /// Process a single "normal" (i.e. not part of an escape or hex sequence)
  /// character. This function wraps consume_normal(). That function does the
  /// real work but this wrapper performs any necessary mutations of the state
  /// machine.
  ///
  /// \param p  The parent parser instance.
  /// \param code_point  The Unicode character being processed.
  void normal(parser_type &p, char32_t code_point);

  std::variant<state, error> consume_escape_state(parser_type &parser, char32_t code_point);
  void escape(parser_type &p, char32_t code_point);

  static constexpr bool is_hex_state(enum state const state) noexcept {
    return state == hex1_state || state == hex2_state || state == hex3_state || state == hex4_state;
  }

  bool is_object_key_;
  char32_t enclosing_char_;
  arrayvec<char8, Policies::max_length> *str_;  // output
  hex_consumer<hex1_state, hex4_state, normal_char_state> hex_;
  icubaby::t32_8 utf_32_to_8_;

  static_assert(decltype(hex_)::first_hex_state == first_hex_state);
  static_assert(decltype(hex_)::last_hex_state == last_hex_state);
  static_assert(decltype(hex_)::post_hex_state == post_hex_state);
};

// consume normal
// ~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
auto string_matcher<Backend, Policies>::consume_normal(parser_type &p, bool is_object_key, char32_t enclosing_char,
                                                       char32_t code_point) -> std::variant<state, std::error_code> {
  if (code_point == char_set::reverse_solidus) {
    return escape_state;
  }
  // We processed part of a Unicode UTF-16 code point. The rest needs to be
  // expressed using the '\u' escape.
  if (hex_.partial()) {
    return error::bad_unicode_code_point;
  }
  if (code_point == enclosing_char) {
    // Consume the closing quote character.
    auto &n = p.backend();
    u8string_view const result{str_->data(), str_->size()};
    if (std::error_code const error = is_object_key ? n.key(result) : n.string_value(result)) {
      return error;
    }
    return done_state;
  }
  if (code_point <= 0x1F) {
    // Control characters U+0000 through U+001F MUST be escaped.
    return error::bad_unicode_code_point;
  }

  // Remember this character.
  bool overflow = false;
  auto it = utf_32_to_8_(code_point, checked_back_insert_iterator{str_, &overflow});
  utf_32_to_8_.end_cp(it);
  if (!utf_32_to_8_.well_formed()) {
    return error::bad_unicode_code_point;
  }
  if (overflow) {
    return error::string_too_long;
  }
  return normal_char_state;
}

// normal
// ~~~~~~
template <typename Backend, typename Policies>
void string_matcher<Backend, Policies>::normal(parser_type &p, char32_t code_point) {
  std::visit(
      [this, &p](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::error_code>) {
          this->set_error(p, arg);
        } else if constexpr (std::is_same_v<T, state>) {
          this->state_ = arg;
        } else {
          static_assert(always_false<T>, "non-exhaustive visitor");
        }
      },
      this->consume_normal(p, is_object_key_, enclosing_char_, code_point));
}

// consume escape state
// ~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
auto string_matcher<Backend, Policies>::consume_escape_state(parser_type &parser, char32_t code_point)
    -> std::variant<state, error> {
  state next_state = normal_char_state;
  switch (code_point) {
  case char_set::quotation_mark:
  case char_set::solidus:
  case char_set::reverse_solidus:
    // code points are appended as-is.
    break;
  case char_set::latin_small_letter_b: code_point = char_set::backspace; break;
  case char_set::latin_small_letter_f: code_point = char_set::form_feed; break;
  case char_set::latin_small_letter_n: code_point = char_set::line_feed; break;
  case char_set::latin_small_letter_r: code_point = char_set::carriage_return; break;
  case char_set::latin_small_letter_t: code_point = char_set::character_tabulation; break;
  case char_set::latin_small_letter_u:
    hex_.start(true);  // Signal the start of four hex-digit UTF-16.
    return {hex1_state};

  case char_set::latin_small_letter_x:
    if (parser.extension_enabled(extensions::string_escapes)) {
      hex_.start(false);  // Signal the start of two hex-digit UTF-8.
      return {hex3_state};
    }
    return {error::invalid_escape_char};

  case char_set::apostrophe:
    if (!parser.extension_enabled(extensions::string_escapes)) {
      return {error::invalid_escape_char};
    }
    // code points is appended as-is.
    break;
  case char_set::latin_small_letter_v:
    if (!parser.extension_enabled(extensions::string_escapes)) {
      return {error::invalid_escape_char};
    }
    code_point = char_set::vertical_tabulation;
    break;
  case char_set::digit_zero:
    if (!parser.extension_enabled(extensions::string_escapes)) {
      return {error::invalid_escape_char};
    }
    code_point = char_set::null_char;
    break;

  case char_set::line_feed:
  case char_set::carriage_return:
  case char_set::line_separator:
  case char_set::paragraph_separator:
    if (parser.extension_enabled(extensions::string_escapes)) {
      if (code_point == char_set::carriage_return) {
        // a special state to handle the potential line feed.
        next_state = skip_lf_state;
      }
      // Just consume the character.
      return next_state;
    }
    [[fallthrough]];
  default: return {error::invalid_escape_char};
  }
  assert(next_state == normal_char_state);
  bool overflow = false;
  utf_32_to_8_(code_point, checked_back_insert_iterator(str_, &overflow));
  assert(utf_32_to_8_.well_formed());
  if (overflow) {
    return {error::string_too_long};
  }
  return {next_state};
}

// escape
// ~~~~~~
template <typename Backend, typename Policies>
void string_matcher<Backend, Policies>::escape(parser_type &p, char32_t code_point) {
  std::visit(
      [this, &p](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, error>) {
          this->set_error(p, arg);
        } else if constexpr (std::is_same_v<T, state>) {
          this->state_ = arg;
        } else {
          static_assert(always_false<T>, "non-exhaustive visitor");
        }
      },
      this->consume_escape_state(p, code_point));
}

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto string_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (!ch) {
    this->set_error(parser, error::expected_close_quote);
    return {null_pointer(), true};
  }

  auto const c = *ch;
  bool match = true;
  switch (state_) {
  // Matches the opening quote.
  case start_state:
    if (c == enclosing_char_) {
      state_ = normal_char_state;
    } else {
      this->set_error(parser, error::expected_token);
    }
    break;
  case normal_char_state: this->normal(parser, c); break;
  case escape_state: this->escape(parser, c); break;

  case hex1_state:
  case hex2_state:
  case hex3_state:
  case hex4_state: {
    bool overflow = false;
    auto const v = hex_.consume(state_, c, checked_back_insert_iterator{str_, &overflow});
    if (std::holds_alternative<std::error_code>(v)) {
      assert(v.index() == 0);
      this->set_error(parser, std::get<std::error_code>(v));
    } else if (overflow) {
      this->set_error(parser, error::string_too_long);
    } else {
      state_ = std::get<1>(v).first;
    }
  } break;

  // We saw a reverse solidus (backslash) followed by a carriage return.
  // Silently consume a subsequent line_feed.
  case skip_lf_state:
    assert(parser.extension_enabled(extensions::string_escapes));
    state_ = normal_char_state;
    if (c != char_set::line_feed) {
      match = false;
    }
    break;

  case done_state:
  default: assert(false); break;
  }
  return {null_pointer(), match};
}

//*  _    _         _   _  __ _          *
//* (_)__| |___ _ _| |_(_)/ _(_)___ _ _  *
//* | / _` / -_) ' \  _| |  _| / -_) '_| *
//* |_\__,_\___|_||_\__|_|_| |_\___|_|   *
//*                                      *
/// Matches an identifier.
template <typename Backend, typename Policies> class identifier_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  explicit constexpr identifier_matcher(arrayvec<char8, Policies::max_length> *const str) noexcept
      : inherited(start_state), str_{str} {
    assert(str != nullptr);
    str->clear();
  }

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser,
                                                       std::optional<char32_t> code_point) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    // hexN_state is used to implement the \uXXXX hexadecimal escape
    // productions.
    hex1_state,
    hex2_state,
    hex3_state,
    hex4_state,
    part_state,   // Implements the ECMAScript IdentifierPart rule.
    start_state,  // Implements the ECMAScript IdentifierStart rule.
    u_state,      // Used after a backslash is encountered.
  };

  arrayvec<char8, Policies::max_length> *str_;  // output
  hex_consumer<hex1_state, hex4_state, part_state> hex_;
  icubaby::t32_8 utf_32_to_8_;

  static_assert(decltype(hex_)::first_hex_state == first_hex_state);
  static_assert(decltype(hex_)::last_hex_state == last_hex_state);
  static_assert(decltype(hex_)::post_hex_state == post_hex_state);

  void hex_states(parser_type &parser, char32_t code_point);

  /// \brief Checks if the given code point can begin an identifier.
  ///
  /// The code point \p code_point must be categorized as identifier-start.
  ///
  /// \param code_point  The Unicode code-point to be checked.
  /// \returns True if the code-point can be used as the start of an identifier, false otherwise.
  static constexpr bool is_identifier_start(char32_t code_point) noexcept;
  /// \brief Checks if the given code point can be part of an identifier.
  ///
  /// The code point \p code_point must be categorized as either an identifier-
  /// start or identifier-part.
  ///
  /// \param code_point  The Unicode code-point to be checked.
  /// \returns True if the code-point can be part of an identifier, false otherwise.
  static constexpr bool is_identifier_member(char32_t code_point) noexcept;
};

// hex states
// ~~~~~~~~~~
template <typename Backend, typename Policies>
void identifier_matcher<Backend, Policies>::hex_states(parser_type &parser, char32_t const code_point) {
  auto const state = state_;
  assert(state == hex1_state || state == hex2_state || state == hex3_state || state == hex4_state);
  bool overflow = false;
  auto out = checked_back_insert_iterator{str_, &overflow};
  auto v = hex_.consume(state, code_point, out);
  if (std::holds_alternative<std::error_code>(v)) {
    assert(v.index() == 0);
    this->set_error(parser, std::get<std::error_code>(v));
  } else if (overflow) {
    this->set_error(parser, error::identifier_too_long);
  } else {
    state_ = std::get<1>(v).first;
  }
}

// is identifier start
// ~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
constexpr bool identifier_matcher<Backend, Policies>::is_identifier_start(char32_t const code_point) noexcept {
  if (code_point >= char_set::latin_capital_letter_a && code_point <= char_set::latin_capital_letter_z) {
    return true;
  }
  if (code_point >= char_set::latin_small_letter_a && code_point <= char_set::latin_small_letter_z) {
    return true;
  }
  if (code_point == char_set::dollar_sign || code_point == char_set::low_line) {
    return true;
  }
  // U+0080 Is where the Latin-1 supplement starts. Consult the table for
  // code points beyond this.
  if (code_point >= 0x80 && code_point_grammar_rule(code_point) == grammar_rule::identifier_start) {
    return true;
  }
  return false;
}

// is identifier member
// ~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
constexpr bool identifier_matcher<Backend, Policies>::is_identifier_member(char32_t const code_point) noexcept {
  if (code_point >= char_set::digit_zero && code_point <= char_set::digit_nine) {
    return true;
  }
  if (code_point < 0x80) {
    return is_identifier_start(code_point);
  }
  return (to_underlying(code_point_grammar_rule(code_point).value_or(static_cast<grammar_rule>(0))) & idmask) != 0U;
}

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto identifier_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> code_point)
    -> std::pair<typename inherited::pointer, bool> {
  using return_type = std::pair<typename inherited::pointer, bool>;
  auto consume_and_iterate = return_type{null_pointer(), true};
  auto retry_char_and_iterate = return_type{null_pointer(), false};
  auto install_error = [&](error err) {
    this->set_error(parser, err);
    return return_type{null_pointer(), true};
  };
  auto change_state = [&](enum state new_state) {
    state_ = new_state;
    return return_type{null_pointer(), true};
  };

  if (!code_point) {
    return install_error(error::expected_close_quote);
  }

  char32_t const c = *code_point;
  switch (state_) {
  case start_state:
    if (whitespace_matcher<Backend, Policies>::want_code_point(parser, c)) {
      return {this->make_whitespace_matcher(parser), false};
    }
    if (c == char_set::reverse_solidus) {
      return change_state(u_state);
    }
    if (!is_identifier_start(c)) {
      return install_error(error::bad_identifier);
    }
    state_ = part_state;
    // Record the character.
    break;
  case part_state:
    if (c == char_set::reverse_solidus) {
      return change_state(u_state);
    }
    // We processed part of a Unicode UTF-16 code point. The rest needs to be
    // expressed using the '\u' escape.
    if (hex_.partial()) {
      return install_error(error::bad_unicode_code_point);
    }
    if (!is_identifier_member(c)) {
      // This code point wasn't part of an identifier, so don't consume it.
      if (std::error_code const error = parser.backend().key(u8string_view{str_->data(), str_->size()})) {
        this->set_error(parser, error);
      }
      // TODO(paul) check whether identifier is empty?
      state_ = done_state;
      return retry_char_and_iterate;
    }
    // Record the character.
    break;
  case u_state:
    if (c != char_set::latin_small_letter_u) {
      return install_error(error::expected_token);  // TODO(paul) expected 'u'?
    }
    hex_.start(true);  // Signal the start of four hex-digit UTF-16.
    return change_state(hex1_state);
  case hex1_state:
  case hex2_state:
  case hex3_state:
  case hex4_state: this->hex_states(parser, c); return consume_and_iterate;

  default: assert(false); break;
  }

  if (str_->size() >= Policies::max_length) {
    return install_error(error::identifier_too_long);
  }

  // Remember this character.
  auto it = utf_32_to_8_(c, std::back_inserter(*str_));
  utf_32_to_8_.end_cp(it);
  if (!utf_32_to_8_.well_formed()) {
    this->set_error(parser, error::bad_unicode_code_point);
  }
  return consume_and_iterate;
}

//*                          *
//*  __ _ _ _ _ _ __ _ _  _  *
//* / _` | '_| '_/ _` | || | *
//* \__,_|_| |_| \__,_|\_, | *
//*                    |__/  *
/// Matches an array.
template <typename Backend, typename Policies> class array_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  constexpr array_matcher() noexcept : inherited(start_state) {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    start_state,
    first_object_state,
    object_state,
    comma_state,
  };

  void end_array(parser_type &parser);
};

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto array_matcher<Backend, Policies>::consume(parser_type &p, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (!ch) {
    this->set_error(p, error::expected_array_member);
    return {null_pointer(), true};
  }
  auto const c = *ch;
  switch (state_) {
  case start_state:
    assert(c == '[');
    if (this->set_error(p, p.backend().begin_array())) {
      break;
    }
    state_ = first_object_state;
    // Match this character and consume whitespace before the object (or close
    // bracket).
    return {this->make_whitespace_matcher(p), true};

  case first_object_state:
    if (c == ']') {
      this->end_array(p);
      break;
    }
    [[fallthrough]];
  case object_state:
    state_ = comma_state;
    return {this->make_root_matcher(p), false};
    break;
  case comma_state:
    if (whitespace_matcher<Backend, Policies>::want_code_point(p, c)) {
      // just consume whitespace before a comma.
      return {this->make_whitespace_matcher(p), false};
    }
    switch (c) {
    case ',':
      state_ = p.extension_enabled(extensions::array_trailing_comma) ? first_object_state : object_state;
      return {this->make_whitespace_matcher(p), true};
    case ']': this->end_array(p); break;
    default: this->set_error(p, error::expected_array_member); break;
    }
    break;
  case done_state:
  default: unreachable(); break;
  }
  return {null_pointer(), true};
}

// end array
// ~~~~~~~~~
template <typename Backend, typename Policies> void array_matcher<Backend, Policies>::end_array(parser_type &parser) {
  this->set_error(parser, parser.backend().end_array());
  state_ = done_state;
}

//*      _     _        _    *
//*  ___| |__ (_)___ __| |_  *
//* / _ \ '_ \| / -_) _|  _| *
//* \___/_.__// \___\__|\__| *
//*         |__/             *
/// Matches an object.
template <typename Backend, typename Policies> class object_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  constexpr object_matcher() noexcept : inherited(start_state) {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    start_state,
    first_key_state,
    key_state,
    colon_state,
    value_state,
    comma_state,
  };

  void end_object(parser_type &parser);
  std::pair<typename inherited::pointer, bool> comma(parser_type &parser, char32_t code_point);
  std::pair<typename inherited::pointer, bool> key(parser_type &parser, char32_t code_point);
};

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto object_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (!ch) {
    this->set_error(parser, error::expected_object_member);
    return {null_pointer(), true};
  }
  auto const c = *ch;
  switch (state_) {
  case start_state:
    assert(c == '{');
    state_ = first_key_state;
    if (this->set_error(parser, parser.backend().begin_object())) {
      break;
    }
    return {this->make_whitespace_matcher(parser), true};
  case first_key_state:
    // We allow either a closing brace (to end the object) or a property name.
    if (c == '}') {
      this->end_object(parser);
      break;
    }
    [[fallthrough]];
  case key_state: return this->key(parser, c);
  case colon_state:
    if (whitespace_matcher<Backend, Policies>::want_code_point(parser, c)) {
      // just consume whitespace before the colon.
      return {this->make_whitespace_matcher(parser), false};
    }
    if (c == char_set::colon) {
      state_ = value_state;
    } else {
      this->set_error(parser, error::expected_colon);
    }
    break;
  case value_state: state_ = comma_state; return {this->make_root_matcher(parser), false};
  case comma_state: return this->comma(parser, c);
  case done_state:
  default: unreachable(); break;
  }
  // No change of matcher. Consume the input character.
  return {null_pointer(), true};
}

// key
// ~~~
/// Match a property name then expect a colon.
template <typename Backend, typename Policies>
auto object_matcher<Backend, Policies>::key(parser_type &parser, char32_t code_point)
    -> std::pair<typename inherited::pointer, bool> {
  state_ = colon_state;
  if (code_point == char_set::quotation_mark ||
      (code_point == apostrophe && parser.extension_enabled(extensions::single_quote_string))) {
    return {this->make_string_matcher(parser, true /*object key*/, code_point), false};
  }
  if (parser.extension_enabled(extensions::identifier_object_key)) {
    return {this->make_identifier_matcher(parser), false};
  }

  this->set_error(parser, error::expected_object_key);
  return {null_pointer(), true};
}

// comma
// ~~~~~
template <typename Backend, typename Policies>
auto object_matcher<Backend, Policies>::comma(parser_type &parser, char32_t code_point)
    -> std::pair<typename inherited::pointer, bool> {
  if (whitespace_matcher<Backend, Policies>::want_code_point(parser, code_point)) {
    // just consume whitespace before the comma.
    return {this->make_whitespace_matcher(parser), false};
  }
  if (code_point == ',') {
    // Strictly conforming JSON requires a property name following a comma but
    // we have an extension to allow an trailing comma which may be followed
    // by the object's closing brace.
    state_ = parser.extension_enabled(extensions::object_trailing_comma) ? first_key_state : key_state;
    // Consume the comma and any whitespace before the close brace or property
    // name.
    return {this->make_whitespace_matcher(parser), true};
  }
  if (code_point == '}') {
    this->end_object(parser);
  } else {
    this->set_error(parser, error::expected_object_member);
  }
  // No change of matcher. Consume the input character.
  return {null_pointer(), true};
}

// end object
// ~~~~~~~~~~~
template <typename Backend, typename Policies> void object_matcher<Backend, Policies>::end_object(parser_type &parser) {
  this->set_error(parser, parser.backend().end_object());
  state_ = done_state;
}

//*         _    _ _                             *
//* __ __ _| |_ (_) |_ ___ ____ __  __ _ __ ___  *
//* \ V  V / ' \| |  _/ -_|_-< '_ \/ _` / _/ -_) *
//*  \_/\_/|_||_|_|\__\___/__/ .__/\__,_\__\___| *
//*                          |_|                 *
/// \brief Matches whitespace.
///
/// This matcher consumes whitespace and updates the row number in response to
/// the various combinations of CR and LF. Supports #, //, and /* style comments
/// as an extension.
template <typename Backend, typename Policies> class whitespace_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  constexpr whitespace_matcher() noexcept : inherited(body_state) {}

  /// Returns true if the argument \p code_point potentially represents the
  /// start of a whitespace sequence.
  ///
  /// \p parser  The owning parser instance.
  /// \p code_point  The Unicode code point which is to be tested.
  static constexpr bool want_code_point(parser_type &parser, char32_t code_point) noexcept;

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
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

  std::pair<typename inherited::pointer, bool> consume_body(parser_type &parser, char32_t c);
  std::pair<typename inherited::pointer, bool> consume_comment_start(parser_type &parser, char32_t c);
  std::pair<typename inherited::pointer, bool> multi_line_comment_body(parser_type &parser, char32_t c);
  std::pair<typename inherited::pointer, bool> extra(char32_t c);

  void cr(parser_type &parser, state next) {
    assert(state_ == multi_line_comment_body_state || state_ == body_state);
    parser.advance_row();
    state_ = next;
  }
  void lf(parser_type &parser) { parser.advance_row(); }

  /// Processes the second character of a Windows-style CR/LF pair. Returns true
  /// if the character shoud be treated as whitespace.
  bool crlf(parser_type &parser, char32_t c) {
    if (c != char_set::line_feed) {
      return false;
    }
    parser.reset_column();
    return true;
  }
};

// want code point
// ~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
constexpr bool whitespace_matcher<Backend, Policies>::want_code_point(parser_type &parser,
                                                                      char32_t code_point) noexcept {
  bool result = false;
  switch (code_point) {
  // The following two code points aren't whitespace but potentially introduce
  // a comment which from the parser's POV, counts as whitespace (assuming that
  // the associated extension has been enabled).
  case char_set::number_sign: return parser.extension_enabled(extensions::bash_comments);
  case char_set::solidus:
    return parser.extension_enabled(extensions::multi_line_comments) ||
           parser.extension_enabled(extensions::single_line_comments);
  case char_set::space:
  case char_set::character_tabulation:
  case char_set::carriage_return:
  case char_set::line_feed:
  case char_set::vertical_tabulation:
  case char_set::form_feed:
  case char_set::no_break_space: result = true; break;
  default:
    // The above collection of code points covers everything below 0x100 (which
    // are, by far, the most common). For code points above that, we need to
    // consult the table.
    if (code_point > 0xFF) {
      result = code_point_grammar_rule(code_point) == grammar_rule::whitespace;
    }
    break;
  }
  assert(result == (code_point_grammar_rule(code_point) == grammar_rule::whitespace));
  return result;
}

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
std::pair<typename matcher<Backend, Policies>::pointer, bool> whitespace_matcher<Backend, Policies>::consume(
    parser_type &parser, std::optional<char32_t> ch) {
  if (!ch) {
    switch (state_) {
    case multi_line_comment_body_state:
    case multi_line_comment_ending_state:
    case multi_line_comment_crlf_state: this->set_error(parser, error::unterminated_multiline_comment); break;
    default: state_ = done_state; break;
    }
    return {null_pointer(), true};
  }
  auto const c = *ch;
  switch (state_) {
  // Handles the LF part of a Windows-style CR/LF pair.
  case crlf_state:
    state_ = body_state;
    if (crlf(parser, c)) {
      break;
    }
    [[fallthrough]];
  case body_state: return this->consume_body(parser, c);
  case comment_start_state: return this->consume_comment_start(parser, c);

  case multi_line_comment_ending_state:
    assert(parser.extension_enabled(extensions::multi_line_comments));
    switch (c) {
    // asterisk followed by a second asterisk so don't change state.
    case char_set::asterisk: break;
    // asterisk+solidus (*/) means the end of the comment.
    case char_set::solidus: state_ = body_state; break;
    // some other character. Back to consuming the comment.
    default: state_ = multi_line_comment_body_state; break;
    }
    break;

  case multi_line_comment_crlf_state:
    state_ = multi_line_comment_body_state;
    if (crlf(parser, c)) {
      break;
    }
    [[fallthrough]];
  case multi_line_comment_body_state: return this->multi_line_comment_body(parser, c);
  case single_line_comment_state:
    assert(parser.extension_enabled(extensions::bash_comments) ||
           parser.extension_enabled(extensions::single_line_comments) ||
           parser.extension_enabled(extensions::multi_line_comments));
    if (c == char_set::carriage_return || c == char_set::line_feed) {
      // This character marks a bash/single-line comment end. Go back to
      // normal whitespace handling. Retry with the same character.
      state_ = body_state;
      return {null_pointer(), false};
    }
    // Just consume the character.
    break;

  default: assert(false); break;
  }
  return {null_pointer(), true};
}

// extra
// ~~~~~
template <typename Backend, typename Policies>
auto whitespace_matcher<Backend, Policies>::extra(char32_t c) -> std::pair<typename inherited::pointer, bool> {
  bool is_ws = false;
  switch (c) {
  case char_set::vertical_tabulation:
  case char_set::form_feed:
  case char_set::no_break_space: is_ws = true; break;
  default:
    if (c > 0xFF) {
      is_ws = code_point_grammar_rule(c) == grammar_rule::whitespace;
    }
    break;
  }
  assert(is_ws == (code_point_grammar_rule(c) == grammar_rule::whitespace));
  if (is_ws) {
    return {null_pointer(), true};  // Consume this character.
  }
  state_ = done_state;
  return {null_pointer(), false};  // Retry this character with a different matcher.
}

// consume body
// ~~~~~~~~~~~~
template <typename Backend, typename Policies>
auto whitespace_matcher<Backend, Policies>::consume_body(parser_type &parser, char32_t c)
    -> std::pair<typename inherited::pointer, bool> {
  auto const stop_retry = [this]() {
    // Stop, pop this matcher, and retry with the same character.
    state_ = done_state;
    return std::pair{null_pointer(), false};
  };

  switch (c) {
  case char_set::space:
  case char_set::character_tabulation:  // TODO(paul) tab expansion.
    break;
  case char_set::carriage_return: this->cr(parser, crlf_state); break;
  case char_set::line_feed: this->lf(parser); break;
  case char_set::number_sign:
    if (!parser.extension_enabled(extensions::bash_comments)) {
      return stop_retry();
    }
    state_ = single_line_comment_state;
    break;
  case char_set::solidus:
    if (!parser.extension_enabled(extensions::single_line_comments) &&
        !parser.extension_enabled(extensions::multi_line_comments)) {
      return stop_retry();
    }
    state_ = comment_start_state;
    break;
  default:
    if (parser.extension_enabled(extensions::extra_whitespace)) {
      return this->extra(c);
    }
    return stop_retry();
  }
  return {null_pointer(), true};  // Consume this character.
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
template <typename Backend, typename Policies>
std::pair<typename matcher<Backend, Policies>::pointer, bool>
whitespace_matcher<Backend, Policies>::consume_comment_start(parser_type &parser, char32_t c) {
  if (c == char_set::solidus && parser.extension_enabled(extensions::single_line_comments)) {
    state_ = single_line_comment_state;
  } else if (c == char_set::asterisk && parser.extension_enabled(extensions::multi_line_comments)) {
    state_ = multi_line_comment_body_state;
  } else {
    this->set_error(parser, error::expected_token);
  }
  return {null_pointer(), true};  // Consume this character.
}

// multi line comment body
// ~~~~~~~~~~~~~~~~~~~~~~~
/// Similar to consume_body() except that the commented characters are consumed
/// as well as whitespace. We're looking to see a star ('*') character which may
/// indicate the end of the multi-line comment.
template <typename Backend, typename Policies>
std::pair<typename matcher<Backend, Policies>::pointer, bool>
whitespace_matcher<Backend, Policies>::multi_line_comment_body(parser_type &parser, char32_t c) {
  assert(parser.extension_enabled(extensions::multi_line_comments));
  assert(state_ == multi_line_comment_body_state);
  switch (c) {
  case char_set::asterisk:
    // This could be a standalone star character or be followed by a slash
    // to end the multi-line comment.
    state_ = multi_line_comment_ending_state;
    break;
  case char_set::carriage_return: this->cr(parser, multi_line_comment_crlf_state); break;
  case char_set::line_feed: this->lf(parser); break;
  case char_set::character_tabulation: break;  // TODO(paul) tab expansion.
  default: break;                              // Just consume.
  }
  return {null_pointer(), true};  // Consume this character.
}

//*           __  *
//*  ___ ___ / _| *
//* / -_) _ \  _| *
//* \___\___/_|   *
//*               *
/// Matches the end of the input.
template <typename Backend, typename Policies> class eof_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  constexpr eof_matcher() noexcept : inherited(start_state) {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::state_;
  enum state {
    done_state = inherited::done,
    start_state,
  };
};

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto eof_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> const ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (ch) {
    this->set_error(parser, error::unexpected_extra_input);
  } else {
    state_ = done_state;
  }
  return {inherited::null_pointer(), true};
}

//*               _                _      _             *
//*  _ _ ___  ___| |_   _ __  __ _| |_ __| |_  ___ _ _  *
//* | '_/ _ \/ _ \  _| | '  \/ _` |  _/ _| ' \/ -_) '_| *
//* |_| \___/\___/\__| |_|_|_\__,_|\__\__|_||_\___|_|   *
//*                                                     *
template <typename Backend, typename Policies> class root_matcher final : public matcher<Backend, Policies> {
  using inherited = matcher<Backend, Policies>;

public:
  using parser_type = typename inherited::parser_type;

  constexpr root_matcher() noexcept : inherited(start_state) {}

  std::pair<typename inherited::pointer, bool> consume(parser_type &parser, std::optional<char32_t> ch) override;

private:
  using inherited::null_pointer;
  using inherited::state_;

  enum state {
    done_state = inherited::done,
    start_state,
    new_token_state,
  };
};

// consume
// ~~~~~~~
template <typename Backend, typename Policies>
auto root_matcher<Backend, Policies>::consume(parser_type &parser, std::optional<char32_t> ch)
    -> std::pair<typename inherited::pointer, bool> {
  if (!ch) {
    this->set_error(parser, error::expected_token);
    return {null_pointer(), true};
  }

  using pointer = typename inherited::pointer;
  using deleter = typename pointer::deleter_type;
  switch (state_) {
  case start_state:
    state_ = new_token_state;
    if (whitespace_matcher<Backend, Policies>::want_code_point(parser, *ch)) {
      return {this->make_whitespace_matcher(parser), false};
    }
    [[fallthrough]];
  case new_token_state: {
    state_ = done_state;
    switch (*ch) {
    case char_set::plus_sign:
    case char_set::full_stop:
      if ((*ch == char_set::plus_sign && !parser.extension_enabled(extensions::leading_plus)) ||
          (*ch == char_set::full_stop && !parser.extension_enabled(extensions::numbers))) {
        this->set_error(parser, error::expected_token);
        return {null_pointer(), true};
      }
      [[fallthrough]];

    case '-':
    case char_set::digit_zero:
    case char_set::digit_one:
    case char_set::digit_two:
    case char_set::digit_three:
    case char_set::digit_four:
    case char_set::digit_five:
    case char_set::digit_six:
    case char_set::digit_seven:
    case char_set::digit_eight:
    case char_set::digit_nine:
      return {this->template make_terminal_matcher<number_matcher<Backend, Policies>>(parser), false};
    case '\'':
      if (!parser.extension_enabled(extensions::single_quote_string)) {
        this->set_error(parser, error::expected_token);
        return {null_pointer(), true};
      }
      [[fallthrough]];
    case '"': return {this->make_string_matcher(parser, false /*object key?*/, *ch), false};
    case char_set::latin_capital_letter_i:
      if (parser.extension_enabled(extensions::numbers)) {
        return {this->template make_terminal_matcher<infinity_token_matcher<Backend, Policies>>(parser), false};
      }
      this->set_error(parser, error::expected_token);
      return {null_pointer(), true};

    case char_set::latin_capital_letter_n:
      if (parser.extension_enabled(extensions::numbers)) {
        return {this->template make_terminal_matcher<nan_token_matcher<Backend, Policies>>(parser), false};
      }
      this->set_error(parser, error::expected_token);
      return {null_pointer(), true};

    case 't': return {this->template make_terminal_matcher<true_token_matcher<Backend, Policies>>(parser), false};
    case 'f': return {this->template make_terminal_matcher<false_token_matcher<Backend, Policies>>(parser), false};
    case 'n': return {this->template make_terminal_matcher<null_token_matcher<Backend, Policies>>(parser), false};
    case '[': return {pointer{new array_matcher<Backend, Policies>(), deleter{deleter::mode::do_delete}}, false};
    case '{': return {pointer{new object_matcher<Backend, Policies>(), deleter{deleter::mode::do_delete}}, false};
    default: this->set_error(parser, error::expected_token); return {null_pointer(), true};
    }
  } break;
  default: break;
  }
  unreachable();
}

//*     _           _     _                *
//*  __(_)_ _  __ _| |___| |_ ___ _ _  ___ *
//* (_-< | ' \/ _` | / -_)  _/ _ \ ' \(_-< *
//* /__/_|_||_\__, |_\___|\__\___/_||_/__/ *
//*           |___/                        *
template <typename Backend, typename Policies> struct singletons {
  eof_matcher<Backend, Policies> eof;
  whitespace_matcher<Backend, Policies> trailing_ws;
  root_matcher<Backend, Policies> root;
  std::variant<details::number_matcher<Backend, Policies>, details::string_matcher<Backend, Policies>,
               details::identifier_matcher<Backend, Policies>, details::true_token_matcher<Backend, Policies>,
               details::false_token_matcher<Backend, Policies>, details::null_token_matcher<Backend, Policies>,
               details::infinity_token_matcher<Backend, Policies>, details::nan_token_matcher<Backend, Policies>,
               details::whitespace_matcher<Backend, Policies>>
      terminals;
};

}  // end namespace details

// init stack
// ~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
void parser<Backend, Policies>::init_stack() {
  using mpointer = typename matcher::pointer;
  using deleter = typename mpointer::deleter_type;
  // The EOF matcher is placed at the bottom of the stack to ensure that the
  // input JSON ends after a single top-level object.
  stack_.push(
      mpointer(new (&singletons_.eof) details::eof_matcher<Backend, Policies>{}, deleter{deleter::mode::do_nothing}));
  // We permit whitespace after the top-level object.
  stack_.push(mpointer(new (&singletons_.trailing_ws) details::whitespace_matcher<Backend, Policies>{},
                       deleter{deleter::mode::do_nothing}));
  stack_.push(this->make_root_matcher());
}

// (ctor)
// ~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
parser<Backend, Policies>::parser(Backend &&backend, extensions const extensions)
    : str_buffer_{std::make_unique<arrayvec<char8, Policies::max_length>>()},
      extensions_{extensions},
      backend_{std::move(backend)} {
  this->init_stack();
}

template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
parser<Backend, Policies>::parser(Backend const &backend, extensions const extensions)
    : str_buffer_{std::make_unique<arrayvec<char8, Policies::max_length>>()},
      extensions_{extensions},
      backend_{backend} {
  this->init_stack();
}

template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
parser<Backend, Policies>::parser(parser &&rhs) noexcept(std::is_nothrow_move_constructible_v<Backend>)
    : utf_{std::move(rhs.utf_)},
      stack_{std::move(rhs.stack_)},
      error_{std::move(rhs.error_)},
      singletons_{std::move(rhs.singletons_)},
      str_buffer_{std::move(rhs.str_buffer_)},
      pos_{std::move(rhs.pos_)},
      matcher_pos_{std::move(rhs.matcher_pos_)},
      extensions_{std::move(rhs.extensions_)},
      backend_{std::move(rhs.backend_)} {
  static_assert(std::is_move_constructible_v<decltype(singletons_)>);
  reseat_stack_after_move(rhs);
}

// operator=
// ~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
auto parser<Backend, Policies>::operator=(parser &&rhs) noexcept(std::is_nothrow_move_assignable_v<Backend>)
    -> parser & {
  utf_ = std::move(rhs.utf_);
  stack_ = std::move(rhs.stack_);
  error_ = std::move(rhs.error_);
  singletons_ = std::move(rhs.singletons_);
  str_buffer_ = std::move(rhs.str_buffer_);
  pos_ = std::move(rhs.pos_);
  matcher_pos_ = std::move(rhs.matcher_pos_), extensions_ = std::move(rhs.extensions_);
  backend_ = std::move(rhs.backend_);
  reseat_stack_after_move(rhs);
  return *this;
}

// make root matcher
// ~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
auto parser<Backend, Policies>::make_root_matcher() -> pointer {
  using root_matcher = details::root_matcher<Backend, Policies>;
  using deleter = typename pointer::deleter_type;
  return pointer(new (&singletons_.root) root_matcher(), deleter{deleter::mode::do_nothing});
}

// make whitespace matcher
// ~~~~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
auto parser<Backend, Policies>::make_whitespace_matcher() -> pointer {
  using whitespace_matcher = details::whitespace_matcher<Backend, Policies>;
  return this->make_terminal_matcher<whitespace_matcher>();
}

// make string matcher
// ~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
auto parser<Backend, Policies>::make_string_matcher(bool object_key, char32_t enclosing_char) -> pointer {
  using string_matcher = details::string_matcher<Backend, Policies>;
  return this->template make_terminal_matcher<string_matcher>(str_buffer_.get(), object_key, enclosing_char);
}

// make identifier matcher
// ~~~~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
auto parser<Backend, Policies>::make_identifier_matcher() -> pointer {
  using identifier_matcher = details::identifier_matcher<Backend, Policies>;
  return this->template make_terminal_matcher<identifier_matcher>(str_buffer_.get());
}

// input
// ~~~~~

/// \param first The element in the half-open range of UTF-32 code-units to be parsed.
/// \param last The end of the range of UTF-32 code-units to be parsed.
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
template <std::input_iterator InputIterator>
  requires(std::is_same_v<std::decay_t<typename std::iterator_traits<InputIterator>::value_type>, std::byte>)
parser<Backend, Policies> &parser<Backend, Policies>::input(InputIterator first, InputIterator last) {
  if (error_) {
    return *this;
  }
  std::array<char32_t, 2> code_points{{0}};
  // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
  auto const first_code_point = std::begin(code_points);
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

// consume code point
// ~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
void parser<Backend, Policies>::consume_code_point(char32_t code_point) {
  bool retry = false;
  do {
    assert(!stack_.empty());
    auto &handler = stack_.top();
    auto res = handler->consume(*this, code_point);
    if (error_) {
      return;
    }
    if (handler->is_done()) {
      stack_.pop();  // release the topmost matcher object.
      matcher_pos_ = pos_;
    }

    if (res.first != nullptr) {
      if (stack_.size() > max_stack_depth_)
        PEEJAY_UNLIKELY_ATTRIBUTE {
          // We've already hit the maximum allowed parse stack depth. Reject
          // this new matcher.
          assert(!error_);
          error_ = error::nesting_too_deep;
          return;
        }

      stack_.push(std::move(res.first));
      matcher_pos_ = pos_;
    }
    retry = !res.second;
  } while (retry);
}

// reseat stack after move
// ~~~~~~~~~~~~~~~~~~~~~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
void parser<Backend, Policies>::reseat_stack_after_move(parser const &rhs) noexcept {
  using deleter = typename pointer::deleter_type;
  constexpr auto d = deleter{deleter::mode::do_nothing};

  auto pos = std::begin(stack_);
  auto last = std::end(stack_);
  if (pos == last) {
    return;
  }

  // We know that there is at least one object on the stack.
  std::advance(last, -1);

  // First matcher on the stack is always the EOF checker.
  assert(pos->get() == &rhs.singletons_.eof);
  *pos = pointer(&singletons_.eof, d);
  ++pos;

  if (pos != last) {
    // Next is the "trailing whitespace" matcher.
    assert(pos->get() == &rhs.singletons_.trailing_ws);
    *pos = pointer(&singletons_.trailing_ws, d);
    ++pos;
  }

#ifndef NDEBUG
  // Check that none of the intermediate stack entries point into the
  // singletons object.
  for (auto it2 = pos; it2 != last; ++it2) {
    assert(it2->get() != &rhs.singletons_.eof && it2->get() != &rhs.singletons_.trailing_ws &&
           it2->get() != &rhs.singletons_.root);
    if (!rhs.singletons_.terminals.valueless_by_exception()) {
      std::visit([&it2](auto &v) { assert(it2->get() != &v); }, rhs.singletons_.terminals);
    }
  }
#endif

  auto &top = stack_.top();
  if (top.get() == &rhs.singletons_.root) {
    top = pointer(&singletons_.root, d);
  } else {
    assert(top.get() != &rhs.singletons_.eof && top.get() != &rhs.singletons_.trailing_ws);
    if (!rhs.singletons_.terminals.valueless_by_exception()) {
      std::visit(
          [&top, d, this](auto &v) {
            if (top.get() == &v) {
              using T = std::decay_t<decltype(v)>;
              top = pointer(&std::get<T>(singletons_.terminals), d);
            }
          },
          rhs.singletons_.terminals);
    }
  }
}

// eof
// ~~~
template <typename Backend, typename Policies>
  requires(policy<Policies> && backend<Backend, typename Policies::integer_type>)
decltype(auto) parser<Backend, Policies>::eof() {
  while (!stack_.empty() && !has_error()) {
    auto &handler = stack_.top();
    auto res = handler->consume(*this, std::optional<char>{std::nullopt});
    assert(handler->is_done());
    assert(res.second);
    stack_.pop();  // release the topmost matcher object.
  }
  return this->backend().result();
}

}  // namespace peejay

#endif  // PEEJAY_JSON_HPP
