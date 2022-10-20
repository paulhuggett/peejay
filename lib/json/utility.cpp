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
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "json/utility.hpp"

#include "json/dom_types.hpp"
#include "json/json.hpp"

namespace peejay {

bool is_valid (std::string const& str) {
  parser p{null_output{}};
  p.input (str).eof ();
  return !p.has_error ();
}

}  // end namespace peejay
