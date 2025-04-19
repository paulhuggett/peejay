//===- include/peejay/null.hpp ----------------------------*- mode: C++ -*-===//
//*              _ _  *
//*  _ __  _   _| | | *
//* | '_ \| | | | | | *
//* | | | | |_| | | | *
//* |_| |_|\__,_|_|_| *
//*                   *
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
/// \file null.hpp
/// \brief Provides peejay::null, a backend for the PJ JSON parser which simply
///   discards all of its input.

#ifndef PEEJAY_NULL_HPP
#define PEEJAY_NULL_HPP

#include <string_view>
#include <system_error>

#include "peejay/parser.hpp"

namespace peejay {

/// A PJ JSON parser backend which simply discards all of its input.
template <policy Policies = default_policies> class null {
public:
  using policies = std::remove_reference_t<Policies>;
  using string_view = std::basic_string_view<typename policies::char_type>;

  static constexpr void result() noexcept {
    // The null output produces no result at all.
  }

  static std::error_code string_value(string_view const &sv) noexcept {
    (void)sv;
    return {};
  }
  static std::error_code integer_value(policies::integer_type v) noexcept {
    (void)v;
    return {};
  }
  static std::error_code float_value(policies::float_type v) noexcept {
    (void)v;
    return {};
  }
  static std::error_code boolean_value(bool b) noexcept {
    (void)b;
    return {};
  }
  static std::error_code null_value() noexcept { return {}; }

  static std::error_code begin_array() noexcept { return {}; }
  static std::error_code end_array() noexcept { return {}; }

  static std::error_code begin_object() noexcept { return {}; }
  static std::error_code key(string_view const &sv) noexcept {
    (void)sv;
    return {};
  }
  static std::error_code end_object() noexcept { return {}; }
};

}  // end namespace peejay

#endif  // PEEJAY_NULL_HPP
