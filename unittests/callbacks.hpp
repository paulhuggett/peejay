//===- unittests/callbacks.hpp ----------------------------*- mode: C++ -*-===//
//*            _ _ _                _         *
//*   ___ __ _| | | |__   __ _  ___| | _____  *
//*  / __/ _` | | | '_ \ / _` |/ __| |/ / __| *
//* | (_| (_| | | | |_) | (_| | (__|   <\__ \ *
//*  \___\__,_|_|_|_.__/ \__,_|\___|_|\_\___/ *
//*                                           *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//

#ifndef UNIT_TESTS_PEEJAY_CALLBACKS_HPP
#define UNIT_TESTS_PEEJAY_CALLBACKS_HPP

#include <gmock/gmock.h>

#include <string_view>
#include <system_error>

#include "peejay/portab.hpp"
#include "peejay/utf.hpp"

class json_callbacks_base {
public:
  virtual ~json_callbacks_base () noexcept;

  virtual std::error_code string_value (peejay::u8string_view const &) = 0;
  virtual std::error_code int64_value (std::int64_t) = 0;
  virtual std::error_code uint64_value (std::uint64_t) = 0;
  virtual std::error_code double_value (double) = 0;
  virtual std::error_code boolean_value (bool) = 0;
  virtual std::error_code null_value () = 0;

  virtual std::error_code begin_array () = 0;
  virtual std::error_code end_array () = 0;

  virtual std::error_code begin_object () = 0;
  virtual std::error_code key (peejay::u8string_view const &) = 0;
  virtual std::error_code end_object () = 0;
};

class mock_json_callbacks : public json_callbacks_base {
public:
  ~mock_json_callbacks () noexcept override;

  MOCK_METHOD1 (string_value, std::error_code (peejay::u8string_view const &));
  MOCK_METHOD1 (int64_value, std::error_code (std::int64_t));
  MOCK_METHOD1 (uint64_value, std::error_code (std::uint64_t));
  MOCK_METHOD1 (double_value, std::error_code (double));
  MOCK_METHOD1 (boolean_value, std::error_code (bool));
  MOCK_METHOD0 (null_value, std::error_code ());

  MOCK_METHOD0 (begin_array, std::error_code ());
  MOCK_METHOD0 (end_array, std::error_code ());

  MOCK_METHOD0 (begin_object, std::error_code ());
  MOCK_METHOD1 (key, std::error_code (peejay::u8string_view const &));
  MOCK_METHOD0 (end_object, std::error_code ());
};

template <typename T>
class callbacks_proxy {
public:
  static constexpr void result () noexcept {}

  explicit callbacks_proxy (T &original) : original_ (original) {}
  callbacks_proxy (callbacks_proxy const &) = default;

  std::error_code string_value (peejay::u8string_view const &s) {
    return original_.string_value (s);
  }
  std::error_code int64_value (std::int64_t v) {
    return original_.int64_value (v);
  }
  std::error_code uint64_value (std::uint64_t v) {
    return original_.uint64_value (v);
  }
  std::error_code double_value (double v) { return original_.double_value (v); }
  std::error_code boolean_value (bool v) { return original_.boolean_value (v); }
  std::error_code null_value () { return original_.null_value (); }

  std::error_code begin_array () { return original_.begin_array (); }
  std::error_code end_array () { return original_.end_array (); }

  std::error_code begin_object () { return original_.begin_object (); }
  std::error_code key (peejay::u8string_view const &s) {
    return original_.key (s);
  }
  std::error_code end_object () { return original_.end_object (); }

private:
  T &original_;
};

template <typename T>
callbacks_proxy(testing::StrictMock<T>&) -> callbacks_proxy<T>;
template <typename T>
callbacks_proxy(T&) -> callbacks_proxy<T>;

template <typename T>
PEEJAY_CXX20REQUIRES (std::is_arithmetic_v<T>)
peejay::u8string to_u8string (T v) {
  std::string s = std::to_string (v);
  peejay::u8string resl;
  resl.reserve (s.size ());
  std::transform (std::begin (s), std::end (s), std::back_inserter (resl),
                  [] (char c) { return static_cast<peejay::char8> (c); });
  return resl;
}

class json_out_callbacks {
public:
  constexpr peejay::u8string const &result () const noexcept { return out_; }

  std::error_code string_value (peejay::u8string_view const &s) {
    return this->append (
        u8'"' + peejay::u8string{std::begin (s), std::end (s)} + u8'"');
  }
  std::error_code int64_value (std::int64_t v) {
    return this->append (to_u8string (v));
  }
  std::error_code uint64_value (std::uint64_t v) {
    return this->append (to_u8string (v));
  }
  std::error_code double_value (double v) {
    return this->append (to_u8string (v));
  }
  std::error_code boolean_value (bool v) {
    return this->append (v ? u8"true" : u8"false");
  }
  std::error_code null_value () {
    using namespace std::string_view_literals;
    return this->append (u8"null"sv);
  }

  std::error_code begin_array () { return this->append (u8'['); }
  std::error_code end_array () { return this->append (u8']'); }

  std::error_code begin_object () { return this->append (u8'{'); }
  std::error_code key (peejay::u8string_view const &s) {
    return this->string_value (s);
  }
  std::error_code end_object () { return this->append (u8'}'); }

private:
  template <typename StringType>
  std::error_code append (StringType &&s) {
    if (out_.length () > 0) {
      out_ += ' ';
    }
    out_ += s;
    return {};
  }

  peejay::u8string out_;
};

#endif  // UNIT_TESTS_PEEJAY_CALLBACKS_H
