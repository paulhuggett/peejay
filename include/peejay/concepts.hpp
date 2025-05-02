//===- include/peejay/concepts.hpp ------------------------*- mode: C++ -*-===//
//*                                 _        *
//*   ___ ___  _ __   ___ ___ _ __ | |_ ___  *
//*  / __/ _ \| '_ \ / __/ _ \ '_ \| __/ __| *
//* | (_| (_) | | | | (_|  __/ |_) | |_\__ \ *
//*  \___\___/|_| |_|\___\___| .__/ \__|___/ *
//*                          |_|             *
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
#ifndef PEEJAY_CONCEPTS_HPP
#define PEEJAY_CONCEPTS_HPP

#include <concepts>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "peejay/details/type_list.hpp"

namespace peejay {

/// A type used to indicate that floating point support is not enabled.
struct no_float_type {};

/// The concept no_float<T> is satisfied if T is no_float_type.
template <typename T>
concept no_float = std::same_as<T, no_float_type>;

/// The concept character<T> is satisfied if T is one of the built-in character types that could be used to store UTF-8
/// encoded text. char8_t is the preferred type, but plain char and its variants are allowed to support code that
/// requires them.
template <typename T>
concept character = type_list::has_type_v<type_list::type_list<char8_t, char, signed char, unsigned char>, T>;

template <typename Policy>
concept policy = requires(Policy &&p) {
  /// The maximum length of a string allowed in the JSON input. A buffer of this size is allocated within the parser
  /// instance.
  requires std::unsigned_integral<decltype(p.max_length)>;

  /// The maximum depth to which we allow the parse stack to grow. This should be given a value
  /// sufficient for any reasonable input. The setting is intended to enable the memory required
  /// for the parse stack to be fixed.
  requires requires { requires std::integral<decltype(p.max_stack_depth)> && requires { p.max_stack_depth >= 2; }; };

  /// This boolean value determines whether the library tracks the line and column position in the
  /// input. These values can be used for reporting error to the user but can be disabled if they
  /// are not required.
  requires std::same_as<std::remove_cv_t<decltype(p.pos_tracking)>, bool>;

  /// The type used for floating point values or no_float_type. The latter indicates that floating point
  /// numbers should not be allowed in the input or passed to the backend.
  requires std::floating_point<typename Policy::float_type> || no_float<typename Policy::float_type>;

  /// The type used for integer values. The unsigned type will also be derived when necessary.
  requires std::signed_integral<typename Policy::integer_type>;
  requires character<typename Policy::char_type>;
};

template <typename Backend>
concept backend = requires(Backend &&be) {
  /// There must be a member type "policies" which conforms to the "policy" concept.
  requires policy<typename std::remove_reference_t<Backend>::policies>;

  /// Returns the result of the parse. If the parse was successful, this
  /// function is called by parser<>::eof() which will return its result. The result type is defined by the backend
  /// object and not constrained here.
  be.result();

  /// Called when a JSON string has been parsed.
  {
    be.string_value(std::basic_string_view<typename std::remove_reference_t<Backend>::policies::char_type>{})
  } -> std::convertible_to<std::error_code>;
  /// Called when an integer value has been parsed.
  {
    be.integer_value(std::make_signed_t<typename std::remove_reference_t<Backend>::policies::integer_type>{})
  } -> std::convertible_to<std::error_code>;
  requires requires {
    requires no_float<typename std::remove_reference_t<Backend>::policies::float_type> || requires {
      {
        /// Called when a floating-point value has been parsed.
        be.float_value(typename std::remove_reference_t<Backend>::policies::float_type{})
      } -> std::convertible_to<std::error_code>;
    };
  };
  /// Called when a boolean value has been parsed
  { be.boolean_value(bool{}) } -> std::convertible_to<std::error_code>;
  /// Called when a null value has been parsed.
  { be.null_value() } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an array. Subsequent event notifications are
  /// for members of this array until a matching call to end_array().
  { be.begin_array() } -> std::convertible_to<std::error_code>;
  /// Called indicate that an array has been completely parsed. This will always
  /// follow an earlier call to begin_array().
  { be.end_array() } -> std::convertible_to<std::error_code>;
  /// Called to notify the start of an object. Subsequent event notifications
  /// are for members of this object until a matching call to end_object().
  { be.begin_object() } -> std::convertible_to<std::error_code>;
  /// Called when an object key string has been parsed.
  {
    be.key(std::basic_string_view<typename std::remove_reference_t<Backend>::policies::char_type>{})
  } -> std::convertible_to<std::error_code>;
  /// Called to indicate that an object has been completely parsed. This will
  /// always follow an earlier call to begin_object().
  { be.end_object() } -> std::convertible_to<std::error_code>;
};

}  // end namespace peejay

#endif  // PEEJAY_CONCEPTS_HPP
