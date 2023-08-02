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
  struct element *parent = nullptr;

  element (element const &rhs) = delete;
  element (element &&rhs) noexcept
      : variant (std::move (static_cast<variant &&> (rhs))) {
    adjust_parents ();
  }

  element &operator= (element const &rhs) = delete;
  element &operator= (element &&rhs) noexcept {
    if (this != &rhs) {
      static_cast<variant *> (this)->operator= (
          std::move (static_cast<variant &&> (rhs)));
      adjust_parents ();
    }
    return *this;
  }
  bool operator== (element const &rhs) const;

  /// Evaluate a JSON pointer (RFC6901).
  element *eval_pointer (u8string_view s);
  std::optional<variant> eval_relative_pointer (u8string_view s);

private:
  void adjust_parents () {
    if (auto *const arr = std::get_if<array> (this)) {
      for (element &member : **arr) {
        member.parent = this;
      }
    } else if (auto const *obj = std::get_if<object> (this)) {
      for (auto &kvp : **obj) {
        kvp.second.parent = this;
      }
    }
  }

  static std::pair<u8string, u8string_view::size_type> next_token (
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

inline std::optional<std::pair<u8string_view, unsigned>> decimal (
    u8string_view s) {
  auto prefix = 0U;
  auto pos = u8string_view::size_type{0};
  auto const len = s.length ();
  for (; std::isdigit (s[pos]) && pos < len; ++pos) {
    prefix = prefix * 10U + static_cast<unsigned> (s[pos] - '0');
  }
  if (pos == 0) {
    return {};
  }
  s.remove_prefix (pos);
  return std::make_pair (s, prefix);
}

// The ABNF syntax of a Relative JSON Pointer is:
//
//  relative-json-pointer =  non-negative-integer [index-manipulation]
//                           <json-pointer>
//  relative-json-pointer =/ non-negative-integer "#"
//  index-manipulation    =  ("+" / "-") non-negative-integer
//  non-negative-integer      =  %x30 / %x31-39 *( %x30-39 )
//           ; "0", or digits without a leading "0"
//
// where <json-pointer> follows the production defined in RFC 6901, Section 3
// ("Syntax").
//
// [pbh: I think this may be wrong. One of the examples from the "Relative JSON
// Pointers" specification (draft-bhutton-relative-json-pointer-00) is "0-1#".
// If I'm reading the grammar correctly, then this is not legal because the
// production containing '#' doesn't allow for index-manipulation.]
inline std::optional<variant> element::eval_relative_pointer (u8string_view s) {
  // Evaluation begins by processing the non-negative-integer prefix. This can
  // be found by taking the longest continuous sequence of decimal digits
  // available, starting from the beginning of the string, taking the decimal
  // numerical value. If this value is more than zero, then the following steps
  // are repeated that number of times:
  //
  // - If the current referenced value is the root of the document, then
  //   evaluation fails (see below).
  // - If the referenced value is an item within an array, then the new
  //   referenced value is that array.
  // - If the referenced value is an object member within an object, then the
  //   new referenced value is that object.

  if (s.empty () || !std::isdigit (s.front ())) {
    return {};
  }
  auto const d1 = decimal (s);
  if (!d1) {
    return {};
  }
  auto prefix = 0U;
  std::tie (s, prefix) = *d1;

  auto current = this;
  for (; prefix > 0U; --prefix) {
    if (current == nullptr) {
      return {};  // we've reached the root: fail.
    }
    if (std::holds_alternative<array> (*current->parent) ||
        std::holds_alternative<object> (*current->parent)) {
      current = current->parent;
    }
  }

  if (current == nullptr) {
    return {};
  }
  if (s.length () == 0) {
    return *current;
  }

  // If the next character is a plus ("+") or minus ("-"), followed by another
  // continuous sequence of decimal digits, the following steps are taken using
  // the decimal numeric value of that plus or minus sign and decimal sequence:
  //
  // - If the current referenced value is not an item of an array, then
  //   evaluation fails (see below).
  // - If the referenced value is an item of an array, then the new referenced
  //   value is the item of the array indexed by adding the decimal value (which
  //   may be negative), to the index of the current referenced value.
  if (s[0] == '+' || s[0] == '-') {
    bool const positive = s[0] == '+';
    s.remove_prefix (1);
    auto const d2 = decimal (s);
    if (!d2) {
      return {};
    }
    auto offset = 0U;
    std::tie (s, offset) = *d2;

    if (auto const *const arr = std::get_if<array> (current->parent)) {
      auto index = current - &(*arr)->front ();
      auto new_index = index + (positive ? index : -index);
      using udiff_type = std::make_unsigned_t<decltype (index)>;
      if (new_index < 0 ||
          static_cast<udiff_type> (new_index) >= (*arr)->size ()) {
        return {};
      }
      current = &(**arr)[static_cast<udiff_type> (new_index)];
    } else {
      return {};  // the current referenced value is not an item of an array.
    }
  }

  if (s == u8string_view{u8"#"}) {
    // Otherwise (when the remainder of the Relative JSON Pointer is the
    // character '#'), the final result is determined as follows:
    //
    // - If the current referenced value is the root of the document, then
    //   evaluation fails (see below).
    // - If the referenced value is an item within an array, then the final
    //   evaluation result is the value's index position within the array.
    // - If the referenced value is an object member within an object, then
    //   the new referenced value is the corresponding member name.
    assert (current->parent != nullptr);
    if (current->parent != nullptr) {
      if (auto const *const arr = std::get_if<array> (current->parent)) {
        auto index = current - &(*arr)->front ();
        return index;
      }
      if (auto const *const obj = std::get_if<object> (current->parent)) {
        auto const end = (*obj)->end ();
        if (auto const pos = std::find_if (
                (*obj)->begin (), end,
                [current] (auto const &kvp) { return &kvp.second == current; });
            pos != end) {
          return pos->first;
        }
      }
      return {};
    }
    return *current;
  }

  auto const *const resl = current->eval_pointer (s);
  if (resl == nullptr) {
    return {};
  }
  return *resl;
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
inline std::pair<u8string, u8string_view::size_type> element::next_token (
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
    res = res * 10U + static_cast<unsigned> (c - '0');
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
  element &top = stack_->emplace (std::move (arr));
  for (element &member : *std::get<array> (top)) {
    member.parent = &top;
  }
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
  element &top = stack_->emplace (std::move (obj));
  for (auto &kvp : *std::get<object> (top)) {
    kvp.second.parent = &top;
  }
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
