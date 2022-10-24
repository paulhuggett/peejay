//===- include/json/utility.hpp ---------------------------*- mode: C++ -*-===//
//*        _   _ _ _ _          *
//*  _   _| |_(_) (_) |_ _   _  *
//* | | | | __| | | | __| | | | *
//* | |_| | |_| | | | |_| |_| | *
//*  \__,_|\__|_|_|_|\__|\__, | *
//*                      |___/  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_UTILITY_HPP
#define PEEJAY_UTILITY_HPP

#include <string>

namespace peejay {

bool is_valid (std::string const& str);

}  // end namespace peejay

#endif  // PEEJAY_UTILITY_HPP
