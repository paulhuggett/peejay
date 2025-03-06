//===- unittests/schema/callbacks.hpp ---------------------*- mode: C++ -*-===//
//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#ifndef PEEJAY_UNITTESTS_CALLBACKS_HPP
#define PEEJAY_UNITTESTS_CALLBACKS_HPP

#include <gmock/gmock.h>

#include <string_view>
#include <system_error>

#include "peejay/json/json.hpp"
#include "peejay/json/small_vector.hpp"

template <typename Parser> Parser &input(Parser &parser, peejay::u8string_view const &str) {
  auto const op = [](peejay::char8 const c) { return static_cast<std::byte>(c); };
#if PEEJAY_HAVE_CONCEPTS && PEEJAY_HAVE_RANGES
  return parser.input(str | std::views::transform(op));
#else
  peejay::small_vector<std::byte, 16> vec;
  std::transform(std::begin(str), std::end(str), std::back_inserter(vec), op);
  return parser.input(std::begin(vec), std::end(vec));
#endif  // PEEJAY_HAVE_CONCEPTS && PEEJAY_HAVE_RANGES
}

template <typename IntegerType> class json_callbacks_base {
public:
  using integer_type = IntegerType;

  json_callbacks_base() = default;
  json_callbacks_base(json_callbacks_base const &) = delete;
  json_callbacks_base(json_callbacks_base &&) noexcept = delete;

  virtual ~json_callbacks_base() noexcept = default;

  json_callbacks_base &operator=(json_callbacks_base const &) = delete;
  json_callbacks_base &operator=(json_callbacks_base &&) noexcept = delete;

  virtual std::error_code string_value(peejay::u8string_view const &) = 0;
  virtual std::error_code integer_value(std::make_signed_t<IntegerType>) = 0;
  virtual std::error_code double_value(double) = 0;
  virtual std::error_code boolean_value(bool) = 0;
  virtual std::error_code null_value() = 0;

  virtual std::error_code begin_array() = 0;
  virtual std::error_code end_array() = 0;

  virtual std::error_code begin_object() = 0;
  virtual std::error_code key(peejay::u8string_view const &) = 0;
  virtual std::error_code end_object() = 0;
};

template <typename IntegerType> class mock_json_callbacks : public json_callbacks_base<IntegerType> {
public:
  mock_json_callbacks() = default;
  mock_json_callbacks(mock_json_callbacks const &) = delete;
  mock_json_callbacks(mock_json_callbacks &&) noexcept = delete;

  ~mock_json_callbacks() noexcept override = default;

  mock_json_callbacks &operator=(mock_json_callbacks const &) = delete;
  mock_json_callbacks &operator=(mock_json_callbacks &&) = delete;

  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, string_value, (peejay::u8string_view const &));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, integer_value, (std::make_signed_t<IntegerType>));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, double_value, (double));
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
  MOCK_METHOD(std::error_code, key, (peejay::u8string_view const &));
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, end_object, ());
};

template <typename T> class callbacks_proxy {
public:
  static constexpr void result() noexcept {}

  explicit callbacks_proxy(T &original) : original_(original) {}
  callbacks_proxy(callbacks_proxy const &) = default;
  callbacks_proxy(callbacks_proxy &&) noexcept = default;

  ~callbacks_proxy() noexcept = default;

  callbacks_proxy &operator=(callbacks_proxy const &) = delete;
  callbacks_proxy &operator=(callbacks_proxy &&) noexcept = delete;

  std::error_code string_value(peejay::u8string_view const &s) { return original_.string_value(s); }
  std::error_code integer_value(std::make_signed_t<typename T::integer_type> const v) {
    return original_.integer_value(v);
  }
  std::error_code double_value(double v) { return original_.double_value(v); }
  std::error_code boolean_value(bool v) { return original_.boolean_value(v); }
  std::error_code null_value() { return original_.null_value(); }

  std::error_code begin_array() { return original_.begin_array(); }
  std::error_code end_array() { return original_.end_array(); }

  std::error_code begin_object() { return original_.begin_object(); }
  std::error_code key(peejay::u8string_view const &s) { return original_.key(s); }
  std::error_code end_object() { return original_.end_object(); }

private:
  T &original_;
};

template <typename T> callbacks_proxy(testing::StrictMock<T> &) -> callbacks_proxy<T>;
template <typename T> callbacks_proxy(T &) -> callbacks_proxy<T>;

template <typename T> PEEJAY_CXX20REQUIRES(std::is_arithmetic_v<T>) peejay::u8string to_u8string(T v) {
  std::string s = std::to_string(v);
  peejay::u8string resl;
  resl.reserve(s.size());
  auto const op = [](char c) { return static_cast<peejay::char8>(c); };
#if __cpp_lib_ranges
  std::ranges::transform(s, std::back_inserter(resl), op);
#else
  std::transform(std::begin(s), std::end(s), std::back_inserter(resl), op);
#endif
  return resl;
}

class json_out_callbacks {
public:
  [[nodiscard]] constexpr peejay::u8string const &result() const noexcept { return out_; }

  std::error_code string_value(peejay::u8string_view const &s) {
    return this->append(u8'"' + peejay::u8string{std::begin(s), std::end(s)} + u8'"');
  }
  std::error_code integer_value(std::int64_t v) { return this->append(to_u8string(v)); }
  std::error_code double_value(double v) { return this->append(to_u8string(v)); }
  std::error_code boolean_value(bool v) { return this->append(v ? u8"true" : u8"false"); }
  std::error_code null_value() {
    using namespace std::string_view_literals;
    return this->append(u8"null"sv);
  }

  std::error_code begin_array() { return this->append(u8'['); }
  std::error_code end_array() { return this->append(u8']'); }

  std::error_code begin_object() { return this->append(u8'{'); }
  std::error_code key(peejay::u8string_view const &s) { return this->string_value(s); }
  std::error_code end_object() { return this->append(u8'}'); }

private:
  template <typename StringType> std::error_code append(StringType const &s) {
    if (out_.length() > 0) {
      out_ += ' ';
    }
    out_ += s;
    return {};
  }

  peejay::u8string out_;
};

#endif  // PEEJAY_UNITTESTS_CALLBACKS_HPP
