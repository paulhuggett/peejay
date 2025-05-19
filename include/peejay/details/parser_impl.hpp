//===- include/peejay/details/parser_impl.hpp -------------*- mode: C++ -*-===//
//*                                   _                 _  *
//*  _ __   __ _ _ __ ___  ___ _ __  (_)_ __ ___  _ __ | | *
//* | '_ \ / _` | '__/ __|/ _ \ '__| | | '_ ` _ \| '_ \| | *
//* | |_) | (_| | |  \__ \  __/ |    | | | | | | | |_) | | *
//* | .__/ \__,_|_|  |___/\___|_|    |_|_| |_| |_| .__/|_| *
//* |_|                                          |_|       *
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
#ifndef PEEJAY_DETAILS_PARSER_IMPL_HPP
#define PEEJAY_DETAILS_PARSER_IMPL_HPP

#include "peejay/error.hpp"

namespace peejay {

// push
// ~~~~
template <backend Backend> bool parser<Backend>::push(details::state next_state) {
  assert(storage_.holds() == type_list::npos && "A terminal instance should be destroyed before pushing a new state");
  if (stack_.size() >= policies::max_stack_depth) {
    set_error_and_pop(error::nesting_too_deep);
    return false;
  }
  stack_.emplace(next_state);
  if constexpr (policies::pos_tracking) {
    if (get_group(next_state) != details::group::whitespace) {
      matcher_pos_ = pos_;
    }
  }
  return true;
}

// push terminal
// ~~~~~~~~~~~~~
template <backend Backend>
template <details::state NextState, typename... Args>
void parser<Backend>::push_terminal(Args &&...args) {
  if (this->push(NextState)) {
    storage_.template emplace<details::group_to_matcher_t<get_group(NextState), Backend>>(std::forward<Args>(args)...);
  }
}

// pop
// ~~~
template <backend Backend> void parser<Backend>::pop() {
  using enum details::group;

  switch (get_group(stack_.top())) {
  case number: storage_.template destroy<number_matcher>(); break;
  case string: storage_.template destroy<string_matcher>(); break;
  case token: storage_.template destroy<token_matcher>(); break;
  default: break;
  }
  stack_.pop();
  if constexpr (policies::pos_tracking) {
    if (!stack_.empty() && get_group(stack_.top()) != whitespace) {
      matcher_pos_ = pos_;
    }
  }
}

// eof
// ~~~
template <backend Backend> decltype(auto) parser<Backend>::eof() {
  while (!stack_.empty() && !this->last_error()) {
    this->consume_code_point(std::nullopt);
  }
  // Pop any states that remained on the stack following an error.
  while (!stack_.empty()) {
    stack_.pop();
  }
  // Re-prime the stack in case the user calls input() again.
  this->init_stack();
  // Finally, extract the result from the backend object.
  return this->backend().result();
}

// consume code point
// ~~~~~~~~~~~~~~~~~~
template <backend Backend> void parser<Backend>::consume_code_point(std::optional<char32_t> code_point) {
  bool match = false;
  using enum details::group;

  while (!match && !this->has_error()) {
    switch (get_group(stack_.top())) {
    // Matchers with no additional state.
    case array: match = details::group_to_matcher_t<array, Backend>::consume(*this, code_point); break;
    case object: match = details::group_to_matcher_t<object, Backend>::consume(*this, code_point); break;
    case eof: match = details::group_to_matcher_t<eof, Backend>::consume(*this, code_point); break;
    case root: match = details::group_to_matcher_t<root, Backend>::consume(*this, code_point); break;
    case whitespace: match = details::group_to_matcher_t<whitespace, Backend>::consume(*this, code_point); break;
    // The matchers that maintain state.
    case number:
      match = storage_.template get<details::group_to_matcher_t<number, Backend>>().consume(*this, code_point);
      break;
    case string:
      match = storage_.template get<details::group_to_matcher_t<string, Backend>>().consume(*this, code_point);
      break;
    case token:
      match = storage_.template get<details::group_to_matcher_t<token, Backend>>().consume(*this, code_point);
      break;
    default:
      assert(false && "Unknown state group");
      unreachable();
      break;
    }
  }
}

}  // end namespace peejay

#endif  // PEEJAY_DETAILS_PARSER_IMPL_HPP
