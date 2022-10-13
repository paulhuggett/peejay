//===- include/json/dom_types.hpp -------------------------*- mode: C++ -*-===//
//*      _                   _                          *
//*   __| | ___  _ __ ___   | |_ _   _ _ __   ___  ___  *
//*  / _` |/ _ \| '_ ` _ \  | __| | | | '_ \ / _ \/ __| *
//* | (_| | (_) | | | | | | | |_| |_| | |_) |  __/\__ \ *
//*  \__,_|\___/|_| |_| |_|  \__|\__, | .__/ \___||___/ *
//*                              |___/|_|               *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PEEJAY_DOM_TYPES_HPP
#define PEEJAY_DOM_TYPES_HPP

#include <cstdint>
#include <string_view>
#include <system_error>

namespace json {

class null_output {
public:
  using result_type = void;
  constexpr result_type result () noexcept {}

  std::error_code string_value (std::string_view const &) noexcept {
    return {};
  }
  std::error_code int64_value (std::int64_t) noexcept { return {}; }
  std::error_code uint64_value (std::uint64_t) noexcept { return {}; }
  std::error_code double_value (double) noexcept { return {}; }
  std::error_code boolean_value (bool) noexcept { return {}; }
  std::error_code null_value () noexcept { return {}; }

  std::error_code begin_array () noexcept { return {}; }
  std::error_code end_array () noexcept { return {}; }

  std::error_code begin_object () noexcept { return {}; }
  std::error_code key (std::string_view const &) noexcept { return {}; }
  std::error_code end_object () noexcept { return {}; }
};

}  // end namespace json

#endif  // PEEJAY_DOM_TYPES_HPP
