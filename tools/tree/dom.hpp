//===- tools/tree/dom.hpp ---------------------------------*- mode: C++ -*-===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_TREE_DOM_HPP
#define PEEJAY_TREE_DOM_HPP

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stack>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json/json.hpp"

struct mark {};
struct element : std::variant<std::string, int64_t, uint64_t, double, bool,
                              std::nullopt_t, std::vector<element>,
                              std::unordered_map<std::string, element>, mark> {
};
using object = std::unordered_map<std::string, element>;
using array = std::vector<element>;

class dom_tree {
public:
  element const &result () const noexcept { return stack_.top (); }

  std::error_code string_value (std::string_view const &s) {
    stack_.push (element{std::string{s}});
    return {};
  }
  std::error_code int64_value (int64_t v) {
    stack_.push (element{v});
    return {};
  }

  std::error_code uint64_value (uint64_t v) {
    stack_.push (element{v});
    return {};
  }
  std::error_code double_value (double v) {
    stack_.push (element{v});
    return {};
  }
  std::error_code boolean_value (bool v) {
    stack_.push (element{v});
    return {};
  }
  std::error_code null_value () {
    stack_.push (element{std::nullopt});
    return {};
  }

  std::error_code begin_array () {
    stack_.push (element{mark{}});
    return {};
  }
  std::error_code end_array ();

  std::error_code begin_object () {
    stack_.push (element{mark{}});
    return {};
  }
  std::error_code key (std::string_view const &s) {
    return this->string_value (s);
  }
  std::error_code end_object ();

private:
  std::stack<element> stack_;
};

#endif  // PEEJAY_TREE_DOM_HPP
