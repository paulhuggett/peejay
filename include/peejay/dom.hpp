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
/// \file dom.hpp
/// \brief Provides a simple document-object-module backend for the PJ JSON
///   parser.
#ifndef PEEJAY_DOM_HPP
#define PEEJAY_DOM_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>
#include <vector>

#include "peejay/arrayvec.hpp"
#include "peejay/json.hpp"
#include "peejay/stack.hpp"

namespace peejay {

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
using object = std::shared_ptr<std::unordered_map<u8string, element>>;
using array = std::shared_ptr<std::vector<element>>;
using variant = std::variant<int64_t, uint64_t, double, bool, null, u8string,
                             array, object, mark>;
struct element : variant {
  using variant::variant;
};

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
  [[nodiscard]] size_t elements_until_mark () const noexcept;

  using stack_type = stack<element, arrayvec<element, stack_size>>;
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
  auto arr = std::make_shared<array::element_type> ();
  size_t const size = this->elements_until_mark ();
  arr->reserve (size);
  for (;;) {
    auto &top = stack_->top ();
    if (std::holds_alternative<mark> (top)) {
      stack_->pop ();
      break;
    }
    arr->emplace_back (std::move (top));
    stack_->pop ();
  }
  assert (arr->size () == size);
#if __cpp_lib_ranges
  std::ranges::reverse (*arr);
#else
  std::reverse (std::begin (*arr), std::end (*arr));
#endif
  stack_->emplace (std::move (arr));
  return error::none;
}

// end object
// ~~~~~~~~~~
template <size_t StackSize>
std::error_code dom<StackSize>::end_object () {
  assert (this->elements_until_mark () % 2U == 0U);
  typename object::element_type::size_type const size =
      this->elements_until_mark () / 2U;
  auto obj = std::make_shared<object::element_type> (size);
  for (;;) {
    element value = std::move (stack_->top ());
    stack_->pop ();
    if (std::holds_alternative<mark> (value)) {
      break;
    }
    auto &key = stack_->top ();
    assert (std::holds_alternative<u8string> (key));
    obj->try_emplace (std::move (std::get<icubaby::u8string> (key)),
                      std::move (value));
    stack_->pop ();
  }
  // The presence of duplicate keys can mean that we end up with fewer entries
  // in the map than there were key/value pairs on the stack.
  assert (obj->size () <= size);
  stack_->emplace (std::move (obj));
  return error::none;
}

// elements until mark
// ~~~~~~~~~~~~~~~~~~~
template <size_t StackSize>
size_t dom<StackSize>::elements_until_mark () const noexcept {
  auto rend = std::rend (*stack_);
  auto pos = std::find_if (std::rbegin (*stack_), rend, [] (element const &v) {
    return std::holds_alternative<mark> (v);
  });
  if (pos == rend) {
    return 0;
  }

  auto const result = std::distance (pos.base (), std::end (*stack_));
  assert (result >= 0);
  return static_cast<std::make_unsigned_t<decltype (result)>> (result);
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
