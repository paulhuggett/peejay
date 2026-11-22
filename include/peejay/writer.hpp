//===- include/peejay/writer.hpp --------------------------*- mode: C++ -*-===//
//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
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
/// \file writer.hpp
/// \brief Provides peejay::writer, a backend for the PJ JSON parser which
///   writes the values that it receives as JSON text.

#ifndef PEEJAY_WRITER_HPP
#define PEEJAY_WRITER_HPP

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "icubaby/icubaby.hpp"
#include "peejay/parser.hpp"

namespace peejay {

/// \brief A PJ JSON parser backend which writes the values that it receives as JSON text.
///
/// The output is always legal JSON and contains only ASCII characters: any character in a string or key which is
/// not ASCII is written as a `\\uXXXX` escape (using a UTF-16 surrogate pair for characters outside the Basic
/// Multilingual Plane). Quotation marks, reverse solidi, and control characters are escaped as well.
///
/// The writer has two modes, chosen by the `indent` constructor argument:
/// - compact (indent is 0): no whitespace at all is written.
/// - pretty-print (indent is greater than 0): each array and object member is written on its own line and is
///   indented by `indent` spaces for each level of nesting. A key is followed by a colon and a single space. Empty
///   arrays and objects are written as `[]` and `{}`.
///
/// No newline is written after the top-level value.
///
/// \tparam OutputIterator  The type of the output iterator to which the JSON text is written.
/// \tparam Policies  A type satisfying the policies concept which configures the parser.
template <std::output_iterator<char> OutputIterator, policies Policies = default_policies> class writer {
public:
  /// The parser policies used with this backend.
  using policies = std::remove_reference_t<Policies>;

  /// \param out  The output iterator to which the JSON text is written.
  /// \param indent  The number of spaces by which each level of nesting is indented. Zero selects compact output with
  ///   no whitespace.
  explicit constexpr writer(OutputIterator out, std::size_t indent = 0) : out_{std::move(out)}, indent_{indent} {}

  /// Called at the end of the parse.
  /// \returns The output iterator, one past the last character written.
  [[nodiscard]] constexpr OutputIterator result() const { return out_; }

  /// Called when a string value has been parsed.
  /// \param sv  The string value as UTF-8.
  /// \returns An empty error code, or error::bad_unicode_code_point if \p sv is not well-formed UTF-8. In the latter
  ///   case, each ill-formed sequence is written as U+FFFD REPLACEMENT CHARACTER.
  std::error_code string_value(std::u8string_view const& sv) {
    separator();
    return string(sv);
  }
  /// Called when an integer value has been parsed.
  /// \param v  The integer value.
  /// \returns An empty (success) error code.
  std::error_code integer_value(policies::integer_type v) {
    separator();
    // digits10 is one less than the maximum number of digits. Add one more for a sign.
    std::array<char, std::numeric_limits<typename policies::integer_type>::digits10 + 2> buffer{};
    auto const res = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
    assert(res.ec == std::errc{} && "buffer is too small");
    out_ = std::copy(buffer.data(), res.ptr, out_);
    return {};
  }
  /// Called when a floating-point value has been parsed. The value is written using the shortest representation
  /// which converts back to the same value. A whole number may be written without a decimal point or exponent (for
  /// example, 1.0 is written as "1"), so it is read as an integer if the output is parsed again.
  /// \param v  The floating-point value.
  /// \returns An empty error code, or error::number_out_of_range if \p v is infinite or NaN, neither of which can be
  ///   represented in JSON. Nothing is written in that case.
  std::error_code float_value(policies::float_type v)
    requires(!no_float<typename policies::float_type>)
  {
    if (!std::isfinite(v)) {
      return error::number_out_of_range;
    }
    separator();
    std::array<char, 64> buffer{};
    auto const res = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
    assert(res.ec == std::errc{} && "buffer is too small");
    out_ = std::copy(buffer.data(), res.ptr, out_);
    return {};
  }
  /// Called when a boolean value has been parsed.
  /// \param b  The boolean value.
  /// \returns An empty (success) error code.
  std::error_code boolean_value(bool b) {
    separator();
    put(b ? "true" : "false");
    return {};
  }
  /// Called when a null value has been parsed.
  /// \returns An empty (success) error code.
  std::error_code null_value() {
    separator();
    put("null");
    return {};
  }

  /// Called at the start of an array.
  /// \returns An empty error code, or error::nesting_too_deep if more than policies::max_stack_depth arrays and
  ///   objects would be open.
  std::error_code begin_array() { return begin_container('['); }
  /// Called at the end of an array.
  /// \returns An empty (success) error code.
  std::error_code end_array() { return end_container(']'); }

  /// Called at the start of an object.
  /// \returns An empty error code, or error::nesting_too_deep if more than policies::max_stack_depth arrays and
  ///   objects would be open.
  std::error_code begin_object() { return begin_container('{'); }
  /// Called when an object key has been parsed.
  /// \param sv  The key as UTF-8.
  /// \returns An empty error code, or error::bad_unicode_code_point if \p sv is not well-formed UTF-8. In the latter
  ///   case, each ill-formed sequence is written as U+FFFD REPLACEMENT CHARACTER.
  std::error_code key(std::u8string_view const& sv) {
    assert(depth_ > 0 && !after_key_ && "a key must be a member of an object");
    member_prefix();
    auto const err = string(sv);
    put(':');
    if (indent_ > 0) {
      put(' ');
    }
    after_key_ = true;
    return err;
  }
  /// Called at the end of an object.
  /// \returns An empty (success) error code.
  std::error_code end_object() { return end_container('}'); }

private:
  /// The maximum number of arrays and objects that may be open at once.
  static constexpr auto max_depth = static_cast<std::size_t>(policies::max_stack_depth);

  OutputIterator out_;
  /// The number of spaces per level of indentation. Zero selects compact output.
  std::size_t indent_;
  /// The number of arrays and objects that are currently open.
  std::size_t depth_ = 0;
  /// Bit n is set if the array or object at depth n+1 has at least one member.
  std::bitset<max_depth> non_empty_{};
  /// True if a key has just been written so that the next value needs no separator.
  bool after_key_ = false;

  void put(char const c) { *out_++ = c; }
  void put(std::string_view const& str) { out_ = std::copy(str.begin(), str.end(), out_); }

  /// Starts a new line indented for nesting depth \p depth. Does nothing in compact mode.
  void newline(std::size_t const depth) {
    if (indent_ == 0) {
      return;
    }
    put('\n');
    out_ = std::fill_n(out_, depth * indent_, ' ');
  }

  /// Writes whatever must precede a member of the innermost array or object: a comma if it is not the first member
  /// followed, in pretty-print mode, by a new line.
  void member_prefix() {
    assert(depth_ > 0);
    auto const index = depth_ - 1;
    if (non_empty_[index]) {
      put(',');
    }
    non_empty_.set(index);
    newline(depth_);
  }

  /// Writes whatever must precede a value.
  void separator() {
    if (after_key_) {
      // The value of an object member. The key has already been followed by a colon.
      after_key_ = false;
    } else if (depth_ > 0) {
      // An array member.
      member_prefix();
    }
  }

  std::error_code begin_container(char const open) {
    if (depth_ >= max_depth) {
      return error::nesting_too_deep;
    }
    separator();
    put(open);
    non_empty_.reset(depth_);
    ++depth_;
    return {};
  }

  std::error_code end_container(char const close) {
    assert(depth_ > 0 && !after_key_ && "unbalanced end of array or object");
    --depth_;
    if (non_empty_[depth_]) {
      newline(depth_);
    }
    put(close);
    return {};
  }

  /// If \p cu has a two-character escape sequence (such as \\n), returns the character that follows the reverse
  /// solidus. Otherwise returns '\0'.
  static constexpr char short_escape(char16_t const cu) noexcept {
    switch (cu) {
    case u'"': return '"';
    case u'\\': return '\\';
    case u'\b': return 'b';
    case u'\f': return 'f';
    case u'\n': return 'n';
    case u'\r': return 'r';
    case u'\t': return 't';
    default: return '\0';
    }
  }

  /// Writes a single UTF-16 code unit of a string, escaping it if necessary.
  void code_unit(char16_t const cu) {
    if (auto const esc = short_escape(cu); esc != '\0') {
      put('\\');
      put(esc);
    } else if (cu < 0x20 || cu > 0x7F) {
      // A control character or a character that is not ASCII.
      constexpr std::string_view hex = "0123456789abcdef";
      put("\\u");
      for (auto shift = 12; shift >= 0; shift -= 4) {
        put(hex[(static_cast<unsigned>(cu) >> static_cast<unsigned>(shift)) & 0xFU]);
      }
    } else {
      put(static_cast<char>(cu));
    }
  }

  /// Writes a quoted string.
  std::error_code string(std::u8string_view const& sv) {
    put('"');
    // Converting to UTF-16 gives the code units needed for \u escapes, including surrogate pairs.
    icubaby::t8_16 utf16;
    std::array<char16_t, icubaby::max_output_bytes<icubaby::char8, char16_t>> buffer{};
    auto const code_units = [this, &buffer](char16_t const* const end) {
      for (char16_t const* p = buffer.data(); p != end; ++p) {
        this->code_unit(*p);
      }
    };
    for (auto const c : sv) {
      code_units(utf16(c, buffer.data()));
    }
    code_units(utf16.end_cp(buffer.data()));
    put('"');
    if (!utf16.well_formed()) {
      return error::bad_unicode_code_point;
    }
    return {};
  }
};

/// Deduces the output iterator type of a writer constructed with no indent argument. The default policies are used.
template <std::output_iterator<char> OutputIterator> writer(OutputIterator) -> writer<OutputIterator>;
/// Deduces the output iterator type of a writer constructed with an indent argument. The default policies are used.
template <std::output_iterator<char> OutputIterator> writer(OutputIterator, std::size_t) -> writer<OutputIterator>;

/// Creates a writer backend. This is a convenience function which deduces the output iterator type while allowing
/// the policies to be given explicitly.
///
/// \tparam Policies  A type satisfying the policies concept which configures the parser.
/// \tparam OutputIterator  The type of the output iterator to which the JSON text is written.
/// \param out  The output iterator to which the JSON text is written.
/// \param indent  The number of spaces by which each level of nesting is indented. Zero selects compact output with
///   no whitespace.
/// \returns A writer backend.
template <policies Policies = default_policies, std::output_iterator<char> OutputIterator>
constexpr writer<OutputIterator, Policies> make_writer(OutputIterator out, std::size_t indent = 0) {
  return writer<OutputIterator, Policies>{std::move(out), indent};
}

}  // end namespace peejay

#endif  // PEEJAY_WRITER_HPP
