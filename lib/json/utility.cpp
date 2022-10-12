//===- lib/json/utility.cpp -----------------------------------------------===//
//*        _   _ _ _ _          *
//*  _   _| |_(_) (_) |_ _   _  *
//* | | | | __| | | | __| | | | *
//* | |_| | |_| | | | |_| |_| | *
//*  \__,_|\__|_|_|_|\__|\__, | *
//*                      |___/  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/json/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "json/utility.hpp"

#include "json/dom_types.hpp"
#include "json/json.hpp"

namespace json {

bool is_valid (std::string const& str) {
  json::parser<json::null_output> p;
  p.input (std::span<char const>{str.data (), str.length ()});
  p.eof ();
  return !p.has_error ();
}

}  // end namespace json
