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
///
/// The Document Object Model (DOM) is a data representation of the objects
/// that comprise the structure and content of a JSON document.
#ifndef PEEJAY_DOM_HPP
#define PEEJAY_DOM_HPP

#include <unordered_map>
#include <vector>

#include "peejay/almost_equal.hpp"
#include "peejay/json.hpp"
#include "peejay/small_vector.hpp"

namespace peejay {

//*      _                   _    *
//*  ___| |___ _ __  ___ _ _| |_  *
//* / -_) / -_) '  \/ -_) ' \  _| *
//* \___|_\___|_|_|_\___|_||_\__| *
//*                               *

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
using array = std::shared_ptr<small_vector<element, 8>>;
using variant = std::variant<std::int64_t, double, bool, null, u8string, array,
                             object, mark>;

struct element : variant {
  using variant::variant;

  bool operator== (element const &rhs) const;

  /// Evaluate a JSON pointer (RFC6901).
  element *eval_pointer (u8string_view s);

private:
  static constexpr std::pair<u8string, u8string_view::size_type> next_token (
      u8string_view s, u8string_view::size_type token_start);
  static element *apply_token (element *el, u8string const &token);
  /// Converts a u8stringview to an unsigned integer.
  static constexpr std::optional<unsigned> stoui (u8string_view s);
};

// operator==
// ~~~~~~~~~~
inline bool element::operator== (element const &rhs) const {
  if (this->index () != rhs.index ()) {
    return false;
  }
  if (this->valueless_by_exception ()) {
    return true;
  }
  return std::visit (
      [&rhs] (auto const &lhs) {
        using T = std::decay_t<decltype (lhs)>;
        bool resl = false;
        if constexpr (std::is_same_v<T, object>) {
          resl = *lhs == *std::get<object> (rhs);
        } else if constexpr (std::is_same_v<T, array>) {
          resl = *lhs == *std::get<array> (rhs);
        } else if constexpr (std::is_floating_point_v<T>) {
          resl = almost_equal (lhs, std::get<T> (rhs));
        } else {
          resl = lhs == std::get<T> (rhs);
        }
        return resl;
      },
      *this);
}

// eval pointer
// ~~~~~~~~~~~~
inline element *element::eval_pointer (u8string_view const s) {
  if (s.length () == 0) {
    return this;
  }
  if (s[0] != '/') {
    return nullptr;
  }
  element *el = this;
  auto token_start = u8string_view::size_type{1};
  do {
    u8string token;
    std::tie (token, token_start) = element::next_token (s, token_start);
    el = element::apply_token (el, token);
    if (el == nullptr) {
      return nullptr;
    }
  } while (token_start != std::string::npos);
  return el;
}

// apply token
// ~~~~~~~~~~~
inline element *element::apply_token (element *el, u8string const &token) {
  if (auto *const obj = std::get_if<object> (el)) {
    auto const pos = (*obj)->find (token);
    if (pos == (*obj)->end ()) {
      return nullptr;  // property not found.
    }
    el = &pos->second;
  } else if (auto *const arr = std::get_if<array> (el)) {
    if (token == u8"-") {
      // the (nonexistent) member after the last array element
      // TODO(paul): implement support.
      return nullptr; // Not supported ATM.
    }
    if (std::optional<unsigned> const index = element::stoui (token)) {
      if (index >= (*arr)->size ()) {
        return nullptr;  // Fail (index out of range)
      }
      el = &((**arr)[*index]);
    } else {
      return nullptr;  // Not an integer or '-'
    }
  } else {
    return nullptr;  // Not an object or an array.
  }
  return el;
}

// next token
// ~~~~~~~~~~
constexpr std::pair<u8string, u8string_view::size_type> element::next_token (
    u8string_view s, u8string_view::size_type token_start) {
  auto const token_end = s.find ('/', token_start);
  u8string_view const token_view = s.substr (
      token_start, token_end != u8string_view::npos ? token_end - token_start
                                                    : u8string_view::npos);
  u8string token{std::begin (token_view), std::end (token_view)};
  if (auto const pos = token.find (u8"~1"); pos != u8string::npos) {
    token.replace (pos, 2, u8"/");
  }
  if (auto const pos = token.find (u8"~0"); pos != u8string::npos) {
    token.replace (pos, 2, u8"~");
  }
  return {token, token_end != u8string::npos ? token_end + 1 : u8string::npos};
}

// stoui
// ~~~~~
constexpr std::optional<unsigned> element::stoui (u8string_view s) {
  if (s.length () == 0) {
    return {};
  }
  auto res = 0U;
  for (auto const c : s) {
    if (!std::isdigit (static_cast<int> (c))) {
      return {};
    }
    res = res * 10U + (c - '0');
  }
  return res;
}

//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
/// \brief A PJ JSON parser backend which constructs a DOM using instances of
///   peejay::element.
template <std::size_t StackSize>
class dom {
public:
  static constexpr std::size_t stack_size = StackSize;

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
  std::error_code integer_value (std::int64_t v);
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
  [[nodiscard]] std::size_t elements_until_mark () const noexcept;

  using stack_type = stack<element, arrayvec<element, stack_size>>;
  std::unique_ptr<stack_type> stack_ = std::make_unique<stack_type> ();
};

template <std::size_t StackSize = 1024>
dom () -> dom<StackSize>;

// string
// ~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::string_value (icubaby::u8string_view const &s) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (icubaby::u8string{s});
  return error::none;
}

// integer
// ~~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::integer_value (std::int64_t v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// double
// ~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::double_value (double v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// boolean
// ~~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::boolean_value (bool v) {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (v);
  return error::none;
}

// null
// ~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::null_value () {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (null{});
  return error::none;
}

// begin array
// ~~~~~~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::begin_array () {
  if (stack_->size () >= stack_size) {
    return error::dom_nesting_too_deep;
  }
  stack_->emplace (mark{});
  return error::none;
}

// end array
// ~~~~~~~~~
template <std::size_t StackSize>
std::error_code dom<StackSize>::end_array () {
  auto arr = std::make_shared<array::element_type> ();
  std::size_t const size = this->elements_until_mark ();
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
template <std::size_t StackSize>
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
template <std::size_t StackSize>
std::size_t dom<StackSize>::elements_until_mark () const noexcept {
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

template <std::size_t Size1, std::size_t Size2>
constexpr bool operator== (typename dom<Size1>::element const &lhs,
                           typename dom<Size2>::element const &rhs) {
  return static_cast<typename dom<Size1>::variant const &> (lhs) ==
         static_cast<typename dom<Size2>::variant const &> (rhs);
}
#if !PEEJAY_CXX20
template <std::size_t Size1, std::size_t Size2>
constexpr bool operator!= (typename dom<Size1>::element const &lhs,
                           typename dom<Size2>::element const &rhs) {
  return !operator== (lhs, rhs);
}
#endif  // !PEEJAY_CXX20

}  // end namespace peejay

#endif  // PEEJAY_DOM_HPP
