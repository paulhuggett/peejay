//===- lib/uri/rule.cpp ---------------------------------------------------===//
//*             _       *
//*  _ __ _   _| | ___  *
//* | '__| | | | |/ _ \ *
//* | |  | |_| | |  __/ *
//* |_|   \__,_|_|\___| *
//*                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "uri/rule.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>

namespace uri {

bool rule::done() const {
  if (!tail_ || !tail_->empty()) {
    return false;
  }
  // Run all of the acceptor functions that were gathered on the grammar path
  // that we matched.
  std::for_each(std::begin(acceptors_), std::end(acceptors_),
                [](acceptor_container::value_type const& a) { std::invoke(std::get<0>(a), std::get<1>(a)); });
  return true;
}

rule::matched_result rule::matched(char const* name, rule const& in) const {
  assert(!tail_ || in.tail_);
  static constexpr auto trace = false;
  if constexpr (trace) {
    std::cout << (tail_ ? '*' : '-') << ' ' << std::quoted(name);
  }

  if (tail_) {
    std::string_view const str = in.tail_->substr(0, in.tail_->length() - tail_->length());
    if constexpr (trace) {
      std::cout << ' ' << std::quoted(str) << '\n';
    }
    return std::make_tuple(str, acceptors_);
  }

  if constexpr (trace) {
    std::cout << '\n';
  }
  return {};
}

}  // end namespace uri
