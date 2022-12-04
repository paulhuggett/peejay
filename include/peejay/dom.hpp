//===- include/peejay/dom.hpp -----------------------------*- mode: C++ -*-===//
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
#ifndef PEEJAY_DOM_HPP
#define PEEJAY_DOM_HPP

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#include "peejay/arrayvec.hpp"
#include "peejay/json.hpp"

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

  ~stack () noexcept = default;

  stack (stack const &) = default;
  stack (stack &&) noexcept = default;

  stack &operator= (stack const &) = default;
  stack &operator= (stack &&) noexcept = default;

  [[nodiscard]] bool empty () const { return c_.empty (); }
  [[nodiscard]] size_type size () const { return c_.size (); }
  [[nodiscard]] reference top () { return c_.back (); }
  [[nodiscard]] const_reference top () const { return c_.back (); }

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

struct element;
struct null {
  bool operator== (null /*unused*/) const noexcept { return true; }
#if !PEEJAY_CXX20
  bool operator!= (null /*unused*/) const noexcept { return false; }
#endif  // !PEEJAY_CXX20
};

struct mark {
  bool operator== (mark /*unused*/) const noexcept { return true; }
#if !PEEJAY_CXX20
  bool operator!= (mark /*unused*/) const noexcept { return false; }
#endif  // !PEEJAY_CXX20
};
using variant = std::variant<int64_t, uint64_t, double, bool, null, u8string,
                             std::vector<element>,
                             std::unordered_map<u8string, element>, mark>;

struct element : variant {
  using variant::variant;
};

using object = std::unordered_map<u8string, element>;
using array = std::vector<element>;

template <size_t StackSize>
class dom {
public:
  static constexpr size_t stack_size = StackSize;

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

  std::error_code string_value (u8string_view const &s);
  std::error_code int64_value (int64_t v);
  std::error_code uint64_value (uint64_t v);
  std::error_code double_value (double v);
  std::error_code boolean_value (bool v);
  std::error_code null_value ();

  std::error_code begin_array ();
  std::error_code end_array ();

  std::error_code begin_object () { return this->begin_array (); }
  std::error_code key (u8string_view const &s) {
    return this->string_value (s);
  }
  std::error_code end_object ();

private:
  [[nodiscard]] size_t elements_until_mark () const noexcept {
    return stack_->find_if (
        [] (element const &v) { return std::holds_alternative<mark> (v); });
  }
  using stack_type = details::stack<element, arrayvec<element, stack_size>>;
  std::unique_ptr<stack_type> stack_ = std::make_unique<stack_type> ();
};

template <size_t StackSize = 1024>
dom () -> dom<StackSize>;

// string
// ~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::string_value (icubaby::u8string_view const &s) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (icubaby::u8string{s});
  return error::none;
}

// int64
// ~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::int64_value (int64_t v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// uint64
// ~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::uint64_value (uint64_t v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// double
// ~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::double_value (double v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// boolean
// ~~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::boolean_value (bool v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// null
// ~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::null_value () {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (null{});
  return error::none;
}

// begin array
// ~~~~~~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::begin_array () {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (mark{});
  return error::none;
}

// end array
// ~~~~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::end_array () {
  array arr;
  size_t const size = this->elements_until_mark ();
  arr.reserve (size);
  for (;;) {
    auto &top = stack_->top ();
    if (std::holds_alternative<mark> (top)) {
      stack_->pop ();
      break;
    }
    arr.emplace_back (std::move (top));
    stack_->pop ();
  }
  assert (arr.size () == size);
  std::reverse (std::begin (arr), std::end (arr));
  stack_->emplace (std::move (arr));
  return error::none;
}

// end object
// ~~~~~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::end_object () {
  assert (this->elements_until_mark () % 2U == 0U);
  typename object::size_type const size = this->elements_until_mark () / 2U;
  object obj{size};
  for (;;) {
    element value = std::move (stack_->top ());
    stack_->pop ();
    if (std::holds_alternative<mark> (value)) {
      break;
    }
    auto &key = stack_->top ();
    assert (std::holds_alternative<u8string> (key));
    obj.try_emplace (std::move (std::get<icubaby::u8string> (key)),
                     std::move (value));
    stack_->pop ();
  }
  // The presence of duplicate keys can mean that we end up with fewer entries
  // in the map than there were key/value pairs on the stack.
  assert (obj.size () <= size);
  stack_->emplace (std::move (obj));
  return error::none;
}

template <size_t Size1, size_t Size2>
constexpr bool operator== (typename dom<Size1>::element const &lhs,
                           typename dom<Size2>::element const &rhs) {
  return static_cast<typename dom<Size1>::variant const &> (lhs) ==
         static_cast<typename dom<Size2>::variant const &> (rhs);
}
#if !PEEJAY_CXX20
template <size_t Size1, size_t Size2>
constexpr bool operator!= (typename dom<Size1>::element const &lhs,
                           typename dom<Size2>::element const &rhs) {
  return !operator== (lhs, rhs);
}
#endif  // !PEEJAY_CXX20

}  // end namespace peejay

#endif  // PEEJAY_DOM_HPP
