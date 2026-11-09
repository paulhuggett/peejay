//===- include/peejay/rules/terminals.hpp -----------------*- mode: C++ -*-===//
//*  _                      _             _      *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*                                              *
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
/// \file terminals.hpp
/// \brief Declares rules for the handling of terminals in the grammar (e.g. integers and
/// strings).
///
/// Each of these rules accepts a single value of one type, stores it in the object that it was given, and pops
/// itself from the parse stack. Any other event produces an error.
#ifndef PEEJAY_RULES_TERMINALS_HPP
#define PEEJAY_RULES_TERMINALS_HPP

#include "peejay/rules/rule.hpp"

// Standard library
#include <utility>

namespace peejay::rules {

/// A rule which accepts a boolean value.
///
/// \tparam Policies  The parser policies.
template <policy Policies = peejay::default_policies> class bool_rule final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = bool;

  /// \param value  The location to which the value will be written.
  explicit constexpr bool_rule(value_type* const value) noexcept : value_{value} {}
  /// Stores \p value and pops this rule.
  /// \param context  The parse context.
  /// \param value  The boolean value.
  /// \returns Success.
  std::error_code boolean_value(context_base<Policies>* const context, value_type const value) override {
    *value_ = value;
    return this->pop(context);
  }
  /// \returns The name of this rule.
  [[nodiscard]] char const* name() const noexcept override { return "bool"; }

private:
  /// The location to which the value will be written.
  value_type* value_;
};

/// A rule which accepts an integer value and assigns it to an object of type \p T. A value which \p T cannot represent
/// produces peejay::error::number_out_of_range.
///
/// The parser produces values of the policy's integer_type, so a type which is wider than integer_type can receive
/// only those values that integer_type can represent.
///
/// \tparam Policies  The parser policies.
/// \tparam T  The type of the value stored by this rule. Defaults to the policy's integer_type.
template <policy Policies = default_policies, integer_target T> class integer_rule final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = T;

  /// \param value  The location to which the value will be written.
  explicit constexpr integer_rule(value_type* const value) noexcept : value_{value} {}
  /// \returns The name of this rule.
  [[nodiscard]] char const* name() const noexcept override {
    return std::is_signed_v<value_type> ? "integer" : "unsigned-integer";
  }
  /// Stores \p value and pops this rule.
  /// \param context  The parse context.
  /// \param value  The integer value.
  /// \returns peejay::error::number_out_of_range if \p value cannot be represented by value_type, otherwise success.
  std::error_code integer_value(context_base<Policies>* const context, Policies::integer_type const value) override {
    if (!std::in_range<value_type>(value)) {
      return peejay::error::number_out_of_range;
    }
    *value_ = static_cast<value_type>(value);
    return this->pop(context);
  }

private:
  /// The location to which the value will be written.
  value_type* value_;
};

/// A rule which accepts a number and stores it as a floating-point value. JSON does not distinguish between integers
/// and floating-point numbers, so a number written without a fraction or exponent (which the parser reports as an
/// integer) is also accepted and converted. A large integer may be rounded by that conversion.
///
/// \tparam Policies  The parser policies.
template <policy Policies = default_policies> class float_rule final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = Policies::float_type;

  /// \param value  The location to which the value will be written.
  explicit constexpr float_rule(value_type* const value) noexcept : value_{value} {}
  /// \returns The name of this rule.
  [[nodiscard]] char const* name() const noexcept override { return "float"; }
  /// Stores \p value and pops this rule.
  /// \param context  The parse context.
  /// \param value  The floating-point value.
  /// \returns Success.
  std::error_code float_value(context_base<Policies>* const context, value_type const value) override {
    *value_ = value;
    return this->pop(context);
  }
  /// Converts \p value to value_type and yields the resulting floating-point number.
  /// \param context  The parse context.
  /// \param value  The integer value.
  /// \returns error::unexpected_number if floating-point support is disabled by the policies, otherwise success.
  std::error_code integer_value([[maybe_unused]] context_base<Policies>* const context,
                                [[maybe_unused]] Policies::integer_type const value) override {
    if constexpr (!std::is_same_v<value_type, no_float_type>) {
      return this->float_value(context, static_cast<value_type>(value));
    } else {
      return error::unexpected_number;
    }
  }

private:
  /// The location to which the value will be written.
  value_type* value_;
};

/// A rule which accepts a string value and assigns it to an object of type \p String.
///
/// \tparam Policies  The parser policies.
/// \tparam String  The type of the value stored by this rule: std::u8string (the default) or arrayvec<char8_t, Size>.
template <policy Policies = default_policies, string_target String> class string_rule final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = String;

  /// \param value  The location to which the value will be written.
  explicit constexpr string_rule(value_type* const value) noexcept : value_{value} {}
  /// \returns The name of this rule.
  [[nodiscard]] char const* name() const noexcept override { return "string"; }
  /// Stores \p value and pops this rule.
  /// \param context  The parse context.
  /// \param value  The string value.
  /// \returns peejay::error::string_too_long if \p value does not fit in value_type (in which case the stored value
  ///   is unchanged), otherwise success.
  std::error_code string_value(context_base<Policies>* const context, std::u8string_view const value) override {
    if constexpr (std::same_as<value_type, std::u8string>) {
      *value_ = value;
    } else {
      if (value.size() > value_->max_size()) {
        return peejay::error::string_too_long;
      }
      value_->assign(value.begin(), value.end());
    }
    return this->pop(context);
  }

private:
  /// The location to which the value will be written.
  value_type* value_;
};

}  // end namespace peejay::rules

#endif  // PEEJAY_RULES_TERMINALS_HPP
