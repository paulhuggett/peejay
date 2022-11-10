//===- include/peejay/null.hpp ----------------------------*- mode: C++ -*-===//
//*              _ _  *
//*  _ __  _   _| | | *
//* | '_ \| | | | | | *
//* | | | | |_| | | | *
//* |_| |_|\__,_|_|_| *
//*                   *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//

#ifndef PEEJAY_NULL_HPP
#define PEEJAY_NULL_HPP

#include <cstdint>
#include <string_view>
#include <system_error>

namespace peejay {

class null {
public:
  constexpr void result () const noexcept {
    // The null output produces no result at all.
  }

  std::error_code string_value (std::string_view const &) const noexcept {
    return {};
  }
  std::error_code int64_value (std::int64_t) const noexcept { return {}; }
  std::error_code uint64_value (std::uint64_t) const noexcept { return {}; }
  std::error_code double_value (double) const noexcept { return {}; }
  std::error_code boolean_value (bool) const noexcept { return {}; }
  std::error_code null_value () const noexcept { return {}; }

  std::error_code begin_array () const noexcept { return {}; }
  std::error_code end_array () const noexcept { return {}; }

  std::error_code begin_object () const noexcept { return {}; }
  std::error_code key (std::string_view const &) const noexcept { return {}; }
  std::error_code end_object () const noexcept { return {}; }
};

}  // end namespace peejay

#endif  // PEEJAY_NULL_HPP
