//===- include/peejay/emit.hpp ----------------------------*- mode: C++ -*-===//
//*                 _ _    *
//*   ___ _ __ ___ (_) |_  *
//*  / _ \ '_ ` _ \| | __| *
//* |  __/ | | | | | | |_  *
//*  \___|_| |_| |_|_|\__| *
//*                        *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file emit.hpp
/// \brief The function peejay::emit() recursively writes a peejay::element
///   DOM as JSON.
#ifndef PEEJAY_EMIT_HPP
#define PEEJAY_EMIT_HPP

#include <ios>

#if PEEJAY_HAVE_SPAN
#include <span>
#endif

#if PEEJAY_CXX20 && defined(__has_include) && __has_include(<bit>)
#include <bit>
#endif

#include "peejay/dom.hpp"

namespace peejay {

namespace emit_details {

// to char pointer
// ~~~~~~~~~~~~~~~
template <typename T>
constexpr char const* to_char_pointer (T&& x) noexcept {
  return pointer_cast<char const> (to_address (std::forward<T> (x)));
}

// ident
// ~~~~~
/// Represents an indentation.
///
/// An indentation has a specific depth (which starts at zero) and a specified
/// number of spaces per level.
class indent {
public:
  /// \brief Constructs an indent instance with zero indentation.
  /// \param spaces  The number of spaces making up one indentation level.
  constexpr explicit indent (std::streamsize spaces) noexcept
      : spaces_{spaces} {}

  /// Writes the indentation sequence to the output stream \p os.
  /// \tparam OStream  A type which implements write() and operator<< for
  ///   strings, integers, and doubles.
  /// \param os  An output stream instance.
  /// \returns \p os.
  template <typename OStream>
  OStream& write (OStream& os) const {
    static std::array<char const, 4> whitespace{{' ', ' ', ' ', ' '}};
    assert (spaces_ <= std::numeric_limits<std::streamsize>::max ());
    auto s = depth_ * spaces_;
    while (s > 0) {
      auto const v =
          std::min (static_cast<std::streamsize> (whitespace.size ()), s);
      os.write (whitespace.data (), v);
      s -= v;
    }
    return os;
  }
  /// Returns an increased indentation instance.
  [[nodiscard]] constexpr indent next () const noexcept {
    return indent{spaces_, depth_ + 1U};
  }

private:
  /// \brief Constructs an indent instance which represents an indentation of
  ///   \p depth stops.
  /// \param spaces  The number of spaces making up one indentation level.
  /// \param depth  The indentation depth.
  constexpr indent (std::streamsize const spaces, unsigned const depth) noexcept
      : spaces_{spaces}, depth_{depth} {}

  /// The number of spaces to use for indentation.
  std::streamsize spaces_ = 2;
  unsigned depth_ = 0;  ///< The indentation depth.
};

/// \brief Writes an indent to an output stream.
///
/// \tparam OStream  A type which implements write() and operator<< for
///   strings, integers, and doubles.
/// \param os  The output stream to which an indent will be written.
/// \param i  An indent.
/// \returns  \p os.
template <typename OStream>
inline OStream& operator<< (OStream& os, indent const& i) {
  return i.write (os);
}

// to hex
// ~~~~~~
/// \brief Converts a value from 0..15 to its hexadecimal character equivalent
///   '0'..'F'.
/// \param v  The value to be converted must be less than 16.
/// \return  The hexadecimal character corresponding to \p v.
constexpr char to_hex (unsigned v) noexcept {
  assert (v < 0x10 && "Individual hex values must be < 0x10");
  constexpr auto letter_threshold = 10;
  return static_cast<char> (
      v + ((v < letter_threshold) ? '0' : 'A' - letter_threshold));
}

// break char
// ~~~~~~~~~~
inline u8string_view::const_iterator break_char (
    u8string_view::const_iterator first,
    u8string_view::const_iterator last) noexcept {
  return std::find_if (first, last, [] (char8 const c) {
    return c < static_cast<char8> (char_set::space) ||
           c == static_cast<char8> (char_set::quotation_mark) ||
           c == static_cast<char8> (char_set::reverse_solidus);
  });
}

// convu8
// ~~~~~~
#if !PEEJAY_HAVE_CONCEPTS || !PEEJAVE_HAVE_RANGES
inline std::string convu8 (u8string const& str) {
  std::string result;
  result.reserve (str.size ());
  std::transform (std::begin (str), std::end (str), std::back_inserter (result),
                  [] (char8 const c) { return static_cast<char> (c); });
  return result;
}
#endif  // !PEEJAY_HAVE_CONCEPTS || !PEEJAVE_HAVE_RANGES

// emit object
// ~~~~~~~~~~~
/// \brief Writes a DOM object instance \p obj to output stream \p os.
///
/// The function writes the key and values of the object, including traversing
/// any nested objects or arrays.
///
/// \tparam OStream  A type which implements write() and operator<< for
///   strings, integers, and doubles.
/// \param os  The output stream to which an indent will be written.
/// \param i  An indent.
/// \param obj  The object to be written.
template <typename OStream>
void emit_object (OStream& os, indent i, object const& obj) {
  assert (obj);
  if (obj == nullptr || obj->empty ()) {
    os << "{}";
    return;
  }
  os << "{\n";
  auto const* separator = "";
  indent const next_indent = i.next ();
  for (auto const& [key, value] : *obj) {
    os << separator << next_indent << '"';
#if PEEJAY_HAVE_CONCEPTS && PEEJAVE_HAVE_RANGES
    std::ranges::copy (key | std::transform ([] (char8 const c) {
                         return static_cast<char> (c);
                       }),
                       std::ostream_iterator<char> (os));
#else
    os << convu8 (key);
#endif
    os << "\": ";
    emit (os, next_indent, value);
    separator = ",\n";
  }
  os << '\n' << i << "}";
}

// emit array
// ~~~~~~~~~~
/// \brief Writes a DOM array instance \p arr to output stream \p os.
///
/// The function writes the values of the array including traversing any
/// nested objects or arrays.
///
/// \tparam OStream  A type which implements write() and operator<< for
///   strings, integers, and doubles.
/// \param os  The output stream to which an indent will be written.
/// \param i  An indent.
/// \param arr  The array to be written.
template <typename OStream>
void emit_array (OStream& os, indent i, array const& arr) {
  assert (arr);
  if (arr == nullptr || arr->empty ()) {
    os << "[]";
    return;
  }
  os << "[\n";
  auto const* separator = "";
  indent const next_indent = i.next ();
  for (auto const& v : *arr) {
    os << separator << next_indent;
    emit (os, next_indent, v);
    separator = ",\n";
  }
  os << '\n' << i << "]";
}

// emit string view
// ~~~~~~~~~~~~~~~~
/// \brief Writes a string \p str to output stream \p os.
///
/// Any necessary escaping is performed.
///
/// \tparam OStream  A type which implements write() and operator<< for
///   strings, integers, and doubles.
/// \param os  The output stream to which an indent will be written.
/// \param str  The string to be written.
template <typename OStream>
void emit_string_view (OStream& os, u8string_view const& str) {
  os << '"';
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto first = std::begin (str);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto const last = std::end (str);
  // NOLINTNEXTLINE(llvm-qualified-auto,readability-qualified-auto)
  auto pos = first;
  while ((pos = break_char (pos, last)) != last) {
    os.write (to_char_pointer (first), std::distance (first, pos));
    os << '\\';
    switch (*pos) {
    case char_set::quotation_mark: os << '"'; break;
    case char_set::reverse_solidus: os << '\\'; break;
    case char_set::backspace: os << 'b'; break;
    case char_set::form_feed: os << 'f'; break;
    case char_set::line_feed: os << 'n'; break;
    case char_set::carriage_return: os << 'r'; break;
    case char_set::character_tabulation: os << 't'; break;
    default: {
      auto const c = static_cast<std::byte> (*pos);
      auto const high_nibble = ((c & std::byte{0xF0}) >> 4);
      auto const low_nibble = c & std::byte{0x0F};
      std::array<char, 5> hex{
          {'u', '0', '0', to_hex (std::to_integer<unsigned> (high_nibble)),
           to_hex (std::to_integer<unsigned> (low_nibble))}};
      os.write (hex.data (), hex.size ());
    } break;
    }
    std::advance (pos, 1);
    first = pos;
  }
  assert (pos == last);
  if (first != last) {
    os.write (to_char_pointer (first), std::distance (first, last));
  }
  os << '"';
}

// Helper type for the visitor
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// Explicit deduction guide
template <typename... Ts>
overloaded (Ts...) -> overloaded<Ts...>;

// emit
// ~~~~
template <typename OStream>
OStream& emit (OStream& os, indent const i, element const& el) {
  std::visit (
      overloaded{
          [&os] (u8string const& s) { emit_string_view (os, s); },
          [&os] (int64_t v) { os << v; }, [&os] (uint64_t v) { os << v; },
          [&os] (double v) { os << v; },
          [&os] (bool b) { os << (b ? "true" : "false"); },
          [&os] (null) { os << "null"; },
          [&os, i] (array const& arr) { emit_array (os, i, arr); },
          [&os, i] (object const& obj) { emit_object (os, i, obj); },
          [] (mark) { assert (false); }},
      el);
  return os;
}

}  // namespace emit_details

/// Writes the DOM tree given by \p root to the output stream \p os.
///
/// \tparam OStream  A type which implements write() and operator<< for
///   strings, integers, and doubles.
/// \param os  The output stream to which output will be written.
/// \param root  An optional peejay::element which contains the DOM tree to be
///   emitted.
/// \param spaces  The number of space characters that make up an indentation
///   level.
/// \returns  \p os.
template <typename OStream>
OStream& emit (OStream& os, std::optional<element> const& root,
               std::streamsize spaces = 2) {
  if (root) {
    emit (os, emit_details::indent{spaces}, *root);
  }
  os << '\n';
  return os;
}

}  // end namespace peejay

#endif  // PEEJAY_EMIT_HPP
