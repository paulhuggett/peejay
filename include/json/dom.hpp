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
#include <optional>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#include "json/arrayvec.hpp"
#include "json/json.hpp"

namespace peejay {

namespace details {

template <typename Tp, class Container = arrayvec<Tp, 256>>
class stack {
public:
  using container_type = Container;
  using value_type = typename container_type::value_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using size_type = typename container_type::size_type;
  static_assert (std::is_same_v<Tp, value_type>);

  stack () noexcept (std::is_nothrow_default_constructible_v<Container>)
      : stack (Container ()) {}
  explicit stack (Container const &cont) noexcept (
      std::is_nothrow_move_assignable_v<Container>)
      : c_{cont} {}
  explicit stack (Container &&cont) noexcept (
      std::is_nothrow_move_constructible_v<Container>)
      : c_{std::move (cont)} {}

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

    auto const result = std::distance (pos.base (), std::end (c_));
    assert (result >= 0);
    return static_cast<std::make_unsigned_t<decltype (result)>> (result);
  }

private:
  container_type c_;
};

}  // end namespace details

enum class dom_error_code : int {
  none,
  nesting_too_deep,
};

// ******************
// * error category *
// ******************
class dom_error_category final : public std::error_category {
public:
  dom_error_category () noexcept = default;
  char const* name () const noexcept override;
  std::string message (int error) const override;
};

std::error_category const& get_dom_error_category () noexcept;

inline std::error_code make_error_code (dom_error_code const e) noexcept {
  return {static_cast<int> (e), get_dom_error_category ()};
}


class dom {
public:
  struct element;
  struct null {
    bool operator== (null) const noexcept { return true; }
#if !PEEJAY_CXX20
    bool operator!= (null) const noexcept { return false; }
#endif  // !PEEJAY_CXX20
  };
  struct mark {
    bool operator== (mark) const noexcept { return true; }
#if !PEEJAY_CXX20
    bool operator!= (mark) const noexcept { return false; }
#endif  // !PEEJAY_CXX20
  };
  using variant = std::variant<int64_t, uint64_t, double, bool, null,
                               std::string, std::vector<element>,
                               std::unordered_map<std::string, element>, mark>;

  struct element : variant {
    using variant::variant;
    friend constexpr bool operator== (element const &lhs, element const &rhs);
    friend constexpr bool operator!= (element const &lhs, element const &rhs);
  };

  using object = std::unordered_map<std::string, element>;
  using array = std::vector<element>;

  dom () = default;
  dom (dom const &) = delete;
  dom (dom &&) noexcept = default;
  ~dom () noexcept = default;

  dom &operator= (dom const &) = delete;
  dom &operator= (dom &&) noexcept = default;

  std::optional<element> result () noexcept {
    if (stack_->empty ()) {
      return {std::nullopt};
    }
    return {std::move (stack_->top ())};
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
  size_t elements_until_mark () const noexcept {
    return stack_->find_if (
        [] (element const &v) { return std::holds_alternative<mark> (v); });
  }
  static constexpr size_t stack_size = 1024;
  using stack_type = details::stack<element, arrayvec<element, stack_size>>;
  std::unique_ptr<stack_type> stack_= std::make_unique<stack_type> ();
};

constexpr bool operator== (dom::element const &lhs, dom::element const &rhs) {
  return static_cast<dom::variant const &> (lhs) ==
         static_cast<dom::variant const &> (rhs);
}
constexpr bool operator!= (dom::element const &lhs, dom::element const &rhs) {
  return !operator== (lhs, rhs);
}

}  // end namespace peejay

namespace std {

template <>
struct is_error_code_enum<peejay::dom_error_code> : std::true_type {};

}  // end namespace std

#endif  // PEEJAY_TREE_DOM_HPP
