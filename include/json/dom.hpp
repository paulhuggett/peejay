//===- include/json/dom.hpp -------------------------------*- mode: C++ -*-===//
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
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json/json.hpp"

namespace peejay {

namespace details {

template <typename Tp, class Container = std::deque<Tp>>
class stack {
public:
  using container_type = Container;
  using value_type = typename container_type::value_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using size_type = typename container_type::size_type;
  static_assert (std::is_same_v<Tp, value_type>);

  static_assert (std::is_nothrow_default_constructible<container_type>::value);
  static_assert (std::is_nothrow_move_constructible<container_type>::value);
  static_assert (std::is_nothrow_move_assignable<container_type>::value);

  stack () : stack (Container ()) {}
  explicit stack (Container const &cont) : c_{cont} {}
  explicit stack (Container &&cont) : c_{std::move (cont)} {}

  stack (stack const &) = default;
  stack (stack &&) noexcept = default;

  stack &operator= (stack const &) = default;
  stack &operator= (stack &&) noexcept = default;

  bool empty () const { return c_.empty (); }
  size_type size () const { return c_.size (); }
  reference top () { return c_.back (); }
  const_reference top () const { return c_.back (); }

  template <typename... Args>
  decltype (auto) emplace (Args &&...args) {
    return c_.emplace_back (std::forward<Args> (args)...);
  }

  void pop () { c_.pop_back (); }

  template <typename Function>
  size_t find_if (Function fn) const {
    auto rend = std::rend (c_);
    auto pos = std::find_if (std::rbegin (c_), rend, fn);
    if (pos == rend) {
      return 0;
    }
    return std::distance (pos.base (), std::end (c_));
  }

private:
  container_type c_;
};

}  // end namespace details

class dom {
public:
  struct element;
  struct null {
    bool operator== (null) const noexcept { return true; }
  };
  struct mark {
    bool operator== (mark) const noexcept { return true; }
  };

  using variant = std::variant<int64_t, uint64_t, double, bool, null,
                               std::string, std::vector<element>,
                               std::unordered_map<std::string, element>, mark>;
  struct element : variant {
#if 1
    using variant::variant;
#else
    template <typename Ty>
    element (Ty &&t) : variant (std::forward<Ty> (t)) {
    }

    element (element const &) = delete;
    element (element &&) noexcept = default;
    element &operator= (element const &) = delete;
    element &operator= (element &&) noexcept = default;
#endif

    constexpr bool operator== (dom::element const &other) const {
      return static_cast<variant const &> (*this) ==
             static_cast<variant const &> (other);
    }
    constexpr bool operator!= (dom::element const &other) const {
      return !operator== (other);
    }
  };

  using object = std::unordered_map<std::string, element>;
  using array = std::vector<element>;

  dom ();
#if 1
  dom (dom const &) = delete;
  dom (dom &&) noexcept = default;

  dom &operator= (dom const &) = delete;
  dom &operator= (dom &&) noexcept = default;
#endif

  element const &result () const noexcept {
    return stack_.top ();
  }

  std::error_code string_value (std::string_view const &s);
  std::error_code int64_value (int64_t v);
  std::error_code uint64_value (uint64_t v);
  std::error_code double_value (double v);
  std::error_code boolean_value (bool v);
  std::error_code null_value ();

  std::error_code begin_array ();
  std::error_code end_array ();

  std::error_code begin_object () {
    return this->begin_array ();
  }
  std::error_code key (std::string_view const &s) {
    return this->string_value (s);
  }
  std::error_code end_object ();

private:
  static decltype (auto) initial_container () {
    std::vector<element> cont;
    cont.reserve (512);
    return cont;
  }
  size_t elements_until_mark () const noexcept {
    return stack_.find_if (
        [] (element const &v) { return std::holds_alternative<mark> (v); });
  }
  details::stack<element, std::vector<element>> stack_;
};

}  // end namespace peejay

#endif  // PEEJAY_TREE_DOM_HPP
