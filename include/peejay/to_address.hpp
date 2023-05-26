//===- include/peejay/to_address.hpp ----------------------*- mode: C++ -*-===//
//*  _                    _     _                    *
//* | |_ ___     __ _  __| | __| |_ __ ___  ___ ___  *
//* | __/ _ \   / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
//* | || (_) | | (_| | (_| | (_| | | |  __/\__ \__ \ *
//*  \__\___/   \__,_|\__,_|\__,_|_|  \___||___/___/ *
//*                                                  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_TO_ADDRESS_HPP
#define PEEJAY_TO_ADDRESS_HPP

#include <memory>

#include "peejay/portab.hpp"

namespace peejay {

#if defined(__cpp_lib_to_address)
template <typename T>
constexpr T* to_address (T* p) noexcept {
  return std::to_address (p);
}
template <typename T>
constexpr auto to_address (T const& p) noexcept {
  return std::to_address (p);
}
#else
template <typename T>
constexpr T* to_address (T* const p) noexcept {
  static_assert (!std::is_function_v<T>);
  return p;
}
template <typename T>
constexpr auto to_address (T const& p) noexcept {
  return to_address (p.operator->());
}
#endif  // defined(__cpp_lib_to_address)

}  // end namespace peejay

#endif  // PEEJAY_TO_ADDRESS_HPP
