//===- include/peejay/dom.hpp -----------------------------*- mode: C++ -*-===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
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
/// \file dom.hpp
/// \brief Provides a simple document-object-module backend for the PJ JSON
///   parser.
///
/// The Document Object Model (DOM) is a data representation of the objects
/// that comprise the structure and content of a JSON document.
#ifndef PEEJAY_DOM_HPP
#define PEEJAY_DOM_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "peejay/details/type_list.hpp"
#include "peejay/parser.hpp"

namespace peejay::dom {

struct null {
  // All instances of null compare equal.
  constexpr bool operator==(null /*unused*/) const noexcept { return true; }
};

//*      _                   _    *
//*  ___| |___ _ __  ___ _ _| |_  *
//* / -_) / -_) '  \/ -_) ' \  _| *
//* \___|_\___|_|_|_\___|_||_\__| *
//*                               *
template <policy Policies> class element {
public:
  using integer_type = typename Policies::integer_type;
  using float_type = typename Policies::float_type;
  using string = std::basic_string<typename Policies::char_type>;  // TODO: here be allocations
  using object = std::unordered_map<string, element>;              // TODO: here be allocations
  using array = std::vector<element>;                              // TODO: here be allocations

  using simple_types = type_list::concat<type_list::type_list<null, bool, integer_type, string>,
                                         std::conditional_t<std::is_same_v<float_type, no_float_type>,
                                                            type_list::type_list<>, type_list::type_list<float_type>>>;
  using composite_types = type_list::type_list<array, object>;
  using member_types = type_list::concat<simple_types, composite_types>;

  template <typename MemberType, typename... Args>
    requires(type_list::has_type_v<member_types, MemberType>)
  static constexpr element make(Args &&...args) {
    if constexpr (std::is_same_v<MemberType, array>) {
      return {tag{}, std::in_place_type_t<array_ptr>{}, std::forward<Args>(args)...};
    } else if constexpr (std::is_same_v<MemberType, object>) {
      return {tag{}, std::in_place_type_t<object_ptr>{}, std::forward<Args>(args)...};
    } else {
      return {tag{}, std::in_place_type_t<MemberType>{}, std::forward<Args>(args)...};
    }
  }

  element(element const &rhs) = delete;
  constexpr element(element &&rhs) noexcept = default;
  ~element() noexcept = default;

  element &operator=(element const &rhs) = delete;
  constexpr element &operator=(element &&rhs) noexcept = default;

  constexpr bool operator==(element const &other) const;

  /// Checks if the element holds a value of type \p MemberType.
  /// \tparam MemberType one of the possible member types. See element::member_types.
  /// \returns true if the element currently holds the alternative MemberType, false otherwise.
  template <typename MemberType>
    requires(type_list::has_type_v<member_types, MemberType>)
  constexpr bool holds() const noexcept {
    if constexpr (std::is_same_v<object, MemberType>) {
      return std::holds_alternative<object_ptr>(var_);
    } else if constexpr (std::is_same_v<array, MemberType>) {
      return std::holds_alternative<array_ptr>(var_);
    } else {
      return std::get_if<MemberType>(&var_);
    }
  }

  /// If the element holds a value of type \p MemberType, returns a pointer to the stored value. Otherwise, returns a
  /// null pointer.
  /// \tparam MemberType one of the possible member types. See element::member_types.
  /// \return A pointer to the value stored in the element or null pointer on error.
  template <typename MemberType>
    requires(type_list::has_type_v<member_types, MemberType>)
  constexpr auto const *get_if() const noexcept {
    if constexpr (std::is_same_v<object, MemberType>) {
      auto const *const obj = std::get_if<object_ptr>(&var_);
      return obj != nullptr ? obj->get() : nullptr;
    } else if constexpr (std::is_same_v<array, MemberType>) {
      auto const *const arr = std::get_if<array_ptr>(&var_);
      return arr != nullptr ? arr->get() : nullptr;
    } else {
      return std::get_if<MemberType>(&var_);
    }
  }

  template <typename MemberType>
    requires(type_list::has_type_v<member_types, MemberType>)
  constexpr auto *get_if() noexcept {
    if constexpr (std::is_same_v<object, MemberType>) {
      auto *const obj = std::get_if<object_ptr>(&var_);
      return obj != nullptr ? obj->get() : nullptr;
    } else if constexpr (std::is_same_v<array, MemberType>) {
      auto *const arr = std::get_if<array_ptr>(&var_);
      return arr != nullptr ? arr->get() : nullptr;
    } else {
      return std::get_if<MemberType>(&var_);
    }
  }

private:
  using object_ptr = std::unique_ptr<object>;  // TODO: here be allocations
  using array_ptr = std::unique_ptr<array>;    // TODO: here be allocations
  using internal_composite_types = type_list::type_list<array_ptr, object_ptr>;
  static_assert(composite_types::size == internal_composite_types::size,
                "There must be a one-to-one correspondence between internal and public composite types");

  struct tag {};
  template <typename... Args> constexpr element(tag, Args &&...args) : var_{std::forward<Args>(args)...} {}

  type_list::to_variant<type_list::concat<simple_types, internal_composite_types>> var_;
};

//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
/// \brief A PJ JSON parser backend which constructs a DOM using instances of
///   peejay::dom::element.
template <policy Policies> class dom {
public:
  using policies = std::remove_reference_t<Policies>;
  using element = ::peejay::dom::element<Policies>;

  using char_type = typename Policies::char_type;
  using integer_type = typename Policies::integer_type;
  using float_type = typename Policies::float_type;
  using string_view = std::basic_string_view<char_type>;
  using string = element::string;
  using array = element::array;
  using object = element::object;

  dom() = default;
  dom(dom const &) = delete;
  dom(dom &&) noexcept = default;
  ~dom() noexcept = default;

  dom &operator=(dom const &) = delete;
  dom &operator=(dom &&) noexcept = default;

  std::optional<element> result() noexcept;

  std::error_code string_value(string_view const &v) { return this->record<string>(v); }
  std::error_code integer_value(std::make_signed_t<integer_type> v) { return this->record<integer_type>(v); }
  std::error_code float_value(float_type v);
  std::error_code boolean_value(bool v) { return this->record<bool>(v); }
  std::error_code null_value() { return this->record<null>(); }

  std::error_code begin_array();
  std::error_code end_array();

  std::error_code begin_object();
  std::error_code key(string_view const &s);
  std::error_code end_object();

private:
  template <typename MemberType, typename... Args> std::error_code record(Args &&...args);
  std::error_code record(element &&el);

  std::error_code end_composite();

  string key_;
  std::stack<element, arrayvec<element, Policies::max_stack_depth>> stack_;
};

//===----------------------------------------------------------------------===//
//*      _                   _    *
//*  ___| |___ _ __  ___ _ _| |_  *
//* / -_) / -_) '  \/ -_) ' \  _| *
//* \___|_\___|_|_|_\___|_||_\__| *
//*                               *
// operator==
// ~~~~~~~~~~
template <policy Policies> constexpr bool element<Policies>::operator==(element const &rhs) const {
  if (var_.index() != rhs.var_.index()) {
    return false;
  }
  if (var_.valueless_by_exception()) {
    return true;
  }
  return std::visit(
      [&rhs](auto const &lhs) {
        using T = std::decay_t<decltype(lhs)>;
        bool resl = false;
        if constexpr (std::is_same_v<T, object_ptr>) {
          resl = *lhs == *std::get<object_ptr>(rhs.var_);
        } else if constexpr (std::is_same_v<T, array_ptr>) {
          resl = *lhs == *std::get<array_ptr>(rhs.var_);
        } else if constexpr (std::is_floating_point_v<T>) {
          // resl = almost_equal(lhs, std::get<T>(rhs.var_));
          resl = lhs == std::get<T>(rhs.var_);  // TODO: comparing floats: are you mad?
        } else {
          resl = lhs == std::get<T>(rhs.var_);
        }
        return resl;
      },
      var_);
}

//===----------------------------------------------------------------------===//
//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
// result
// ~~~~~~
template <policy Policies> std::optional<element<Policies>> dom<Policies>::result() noexcept {
  if (stack_.empty()) {
    return {std::nullopt};
  }
  // The stack will contain only the root element unless
  // there was an error which interrupted parsing.
  auto result = std::move(stack_.top());
  // The stack in case any of the dom methods are subsequently called.
  while (!stack_.empty()) {
    stack_.pop();
  }
  return result;
}

// float value
// ~~~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::float_value(float_type v) {
  if constexpr (std::same_as<float_type, no_float_type>) {
    // Floating point support is disabled so do nothing (the function will never be called anyway).
    return {};
  } else {
    return this->record<float_type>(v);
  }
}

// begin array
// ~~~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::begin_array() {
  stack_.emplace(element::template make<array>());
  return {};
}
// end array
// ~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::end_array() {
  assert(!stack_.empty() && stack_.top().template holds<array>() &&
         "The element on top of the stack must be of type array.");
  return this->end_composite();
}

// begin object
// ~~~~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::begin_object() {
  stack_.emplace(element::template make<object>());
  return {};
}
// key
// ~~~
template <policy Policies> std::error_code dom<Policies>::key(string_view const &s) {
  key_ = s;
  return {};
}
// end object
// ~~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::end_object() {
  assert(!stack_.empty() && stack_.top().template holds<object>() &&
         "The element on top of the stack must be of type object.");
  return this->end_composite();
}

// record
// ~~~~~~
template <policy Policies>
template <typename MemberType, typename... Args>
std::error_code dom<Policies>::record(Args &&...args) {
  using el = dom<Policies>::element;
  return this->record(el::template make<MemberType>(std::forward<Args>(args)...));
}

template <policy Policies> std::error_code dom<Policies>::record(element &&el) {
  if (stack_.size() >= Policies::max_stack_depth) {
    return error::nesting_too_deep;  // dom_nesting_too_deep;
  }
  if (stack_.empty()) {
    stack_.emplace(std::move(el));
    return {};
  }
  // We're inside a composite object of some kind. Add this object to it.
  auto &top = stack_.top();
  if (auto *const arr = top.template get_if<array>()) {
    arr->emplace_back(std::move(el));
  } else if (auto *const obj = top.template get_if<object>()) {
    obj->insert_or_assign(std::move(key_), std::move(el));
  } else {
    assert(false && "Type of top-of-stack was unexpected");
  }
  return {};
}

// end composite
// ~~~~~~~~~~~~~
template <policy Policies> std::error_code dom<Policies>::end_composite() {
  if (stack_.size() < 2) {
    return {};
  }
  auto el = std::move(stack_.top());
  stack_.pop();
  return this->record(std::move(el));
}

}  // end namespace peejay::dom

#endif  // PEEJAY_DOM_HPP
