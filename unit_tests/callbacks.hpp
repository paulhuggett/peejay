//===- unit_tests/callbacks.hpp ---------------------------*- mode: C++ -*-===//
//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
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

#ifndef PEEJAY_UNITTESTS_CALLBACKS_HPP
#define PEEJAY_UNITTESTS_CALLBACKS_HPP

#include <gmock/gmock.h>

#include <string_view>
#include <system_error>

#include "peejay/concepts.hpp"
#include "peejay/json.hpp"

template <typename Parser> Parser &input(Parser &parser, std::u8string_view const &str) {
  return parser.input(str);
}

template <std::integral IntegerType, std::floating_point FloatType, peejay::character CharType>
class json_callbacks_base {
public:
  using integer_type = IntegerType;
  using float_type = FloatType;
  using char_type = CharType;
  using string_view = std::basic_string_view<char_type>;

  json_callbacks_base() = default;
  json_callbacks_base(json_callbacks_base const &) = delete;
  json_callbacks_base(json_callbacks_base &&) noexcept = delete;

  virtual ~json_callbacks_base() noexcept = default;

  json_callbacks_base &operator=(json_callbacks_base const &) = delete;
  json_callbacks_base &operator=(json_callbacks_base &&) noexcept = delete;

  virtual std::error_code string_value(string_view const &) = 0;
  virtual std::error_code integer_value(std::make_signed_t<IntegerType>) = 0;
  virtual std::error_code float_value(float_type) = 0;
  virtual std::error_code boolean_value(bool) = 0;
  virtual std::error_code null_value() = 0;

  virtual std::error_code begin_array() = 0;
  virtual std::error_code end_array() = 0;

  virtual std::error_code begin_object() = 0;
  virtual std::error_code key(string_view const &) = 0;
  virtual std::error_code end_object() = 0;
};

template <std::integral IntegerType, std::floating_point FloatType, peejay::character CharType>
class mock_json_callbacks : public json_callbacks_base<IntegerType, FloatType, CharType> {
public:
  using string_view = json_callbacks_base<IntegerType, FloatType, CharType>::string_view;

  mock_json_callbacks() = default;
  mock_json_callbacks(mock_json_callbacks const &) = delete;
  mock_json_callbacks(mock_json_callbacks &&) noexcept = delete;

  ~mock_json_callbacks() noexcept override = default;

  mock_json_callbacks &operator=(mock_json_callbacks const &) = delete;
  mock_json_callbacks &operator=(mock_json_callbacks &&) = delete;

  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, string_value, (string_view const &));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, integer_value, (std::make_signed_t<IntegerType>));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, float_value, (FloatType));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, boolean_value, (bool));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, null_value, ());

  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, begin_array, ());
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, end_array, ());

  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, begin_object, ());
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, key, (string_view const &));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, end_object, ());
};

template <typename T, peejay::policy Policies = peejay::default_policies> class callbacks_proxy {
public:
  using char_type = T::char_type;
  using string_view = std::basic_string_view<char_type>;
  using policies = Policies;

  static constexpr void result() noexcept {}

  explicit callbacks_proxy(T &original) : original_(original) {}
  callbacks_proxy(callbacks_proxy const &) = default;
  callbacks_proxy(callbacks_proxy &&) noexcept = default;

  ~callbacks_proxy() noexcept = default;

  callbacks_proxy &operator=(callbacks_proxy const &) = delete;
  callbacks_proxy &operator=(callbacks_proxy &&) noexcept = delete;

  std::error_code string_value(string_view const &s) { return original_.string_value(s); }
  std::error_code integer_value(std::make_signed_t<typename T::integer_type> const v) {
    return original_.integer_value(v);
  }
  std::error_code float_value(typename T::float_type v) { return original_.float_value(v); }
  std::error_code boolean_value(bool v) { return original_.boolean_value(v); }
  std::error_code null_value() { return original_.null_value(); }

  std::error_code begin_array() { return original_.begin_array(); }
  std::error_code end_array() { return original_.end_array(); }

  std::error_code begin_object() { return original_.begin_object(); }
  std::error_code key(string_view const &s) { return original_.key(s); }
  std::error_code end_object() { return original_.end_object(); }

private:
  T &original_;
};

template <typename T> callbacks_proxy(testing::StrictMock<T> &) -> callbacks_proxy<T>;
template <typename T> callbacks_proxy(T &) -> callbacks_proxy<T>;

template <typename T>
  requires(std::is_arithmetic_v<T>)
static std::u8string to_u8string(T const v) {
  std::string const s = std::to_string(v);
  std::u8string resl;
  resl.reserve(s.size());
  std::ranges::transform(s, std::back_inserter(resl), [](char c) { return static_cast<char8_t>(c); });
  return resl;
}

class json_out_callbacks {
public:
  using policies = peejay::default_policies;
  [[nodiscard]] constexpr std::u8string const &result() const noexcept { return out_; }

  std::error_code string_value(std::u8string_view const &s) {
    return this->append(u8'"' + std::u8string{std::begin(s), std::end(s)} + u8'"');
  }
  std::error_code integer_value(std::int64_t v) { return this->append(to_u8string(v)); }
  std::error_code float_value(double v) { return this->append(to_u8string(v)); }
  std::error_code boolean_value(bool v) {
    using namespace std::string_view_literals;
    return this->append(v ? u8"true"sv : u8"false"sv);
  }
  std::error_code null_value() { return this->append(std::u8string_view{u8"null"}); }
  std::error_code begin_array() { return this->append(std::u8string_view{u8"["}); }
  std::error_code end_array() { return this->append(std::u8string_view{u8"]"}); }

  std::error_code begin_object() { return this->append(std::u8string_view{u8"{"}); }
  std::error_code key(std::u8string_view const &s) { return this->string_value(s); }
  std::error_code end_object() { return this->append(std::u8string_view{u8"}"}); }

private:
  std::error_code append(std::u8string_view const &s) {
    if (out_.length() > 0) {
      out_ += ' ';
    }
    out_ += s;
    return {};
  }
  std::u8string out_;
};

#endif  // PEEJAY_UNITTESTS_CALLBACKS_HPP
