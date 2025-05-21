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
  friend constexpr bool operator==(null /*unused*/, null /*unused*/) noexcept { return true; }
};

template <typename T> struct is_unique_pointer : std::false_type {};
template <typename T> struct is_unique_pointer<std::unique_ptr<T>> : std::true_type {};
template <typename T> static constexpr inline bool is_unique_pointer_v = is_unique_pointer<T>::value;

//*      _                   _    *
//*  ___| |___ _ __  ___ _ _| |_  *
//* / -_) / -_) '  \/ -_) ' \  _| *
//* \___|_\___|_|_|_\___|_||_\__| *
//*                               *
template <policy PJPolicies> class element {
public:
  using integer_type = typename PJPolicies::integer_type;
  using float_type = typename PJPolicies::float_type;
  using string = std::basic_string<typename PJPolicies::char_type>;  // TODO: here be allocations
  using object = std::unordered_map<string, element>;              // TODO: here be allocations
  using array = std::vector<element>;                              // TODO: here be allocations

  using simple_types = type_list::concat<type_list::type_list<null, bool, integer_type, string>,
                                         std::conditional_t<std::is_same_v<float_type, no_float_type>,
                                                            type_list::type_list<>, type_list::type_list<float_type>>>;
  using composite_types = type_list::type_list<array, object>;
  using member_types = type_list::concat<simple_types, composite_types>;

  template <typename MemberType, typename... Args>
    requires type_list::has_type_v<member_types, MemberType>
  static constexpr element make(Args &&...args) {
    if constexpr (std::is_same_v<MemberType, array>) {
      return element{tag{}, std::in_place_type_t<array_ptr>{}, std::make_unique<array>(std::forward<Args>(args)...)};
    } else if constexpr (std::is_same_v<MemberType, object>) {
      return element{tag{}, std::in_place_type_t<object_ptr>{}, std::make_unique<object>(std::forward<Args>(args)...)};
    } else {
      return element{tag{}, std::in_place_type_t<MemberType>{}, std::forward<Args>(args)...};
    }
  }

  element(element const &rhs) = delete;
  constexpr element(element &&rhs) noexcept = default;
  ~element() noexcept = default;

  element &operator=(element const &rhs) = delete;
  constexpr element &operator=(element &&rhs) noexcept = default;

  friend constexpr bool operator==(element const &lhs, element const &rhs) {
    if (auto const lindex = lhs.var_.index(); lindex == std::variant_npos || lindex != rhs.var_.index()) {
      return false;
    }
    return variant_equal(lhs, rhs);
  }

  /// Checks if the element holds a value of type \p MemberType.
  /// \tparam MemberType one of the possible member types. See element::member_types.
  /// \returns true if the element currently holds the alternative MemberType, false otherwise.
  template <typename MemberType>
    requires type_list::has_type_v<member_types, MemberType>
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
    requires type_list::has_type_v<member_types, MemberType>
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
    requires type_list::has_type_v<member_types, MemberType>
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

  type_list::to_variant<type_list::concat<simple_types, internal_composite_types>> var_;

  /// The 'tag' type is here simply to be the first argument for the forwarding constructor below. This
  /// prevents that ctor from being too "greedy" and matching arbitrary argument lists.
  struct tag {};
  template <typename... Args> constexpr explicit element(tag, Args &&...args) : var_{std::forward<Args>(args)...} {}

  template <typename T> static constexpr bool equal(T const &lhs, T const &rhs) {
    if constexpr (is_unique_pointer_v<T>) {
      // This index refers to one of the pointer members so dereference before comparison.
      assert(lhs != nullptr && rhs != nullptr);
      // TODO(paul): This is a recursive comparison.
      return *lhs == *rhs;
    } else {
      return lhs == rhs;
    }
  }

  static constexpr bool equal(float_type a, float_type b) noexcept {
    // This function is based on the code in this StackOverflow posting <https://stackoverflow.com/a/32334103>.
    PEEJAY_CLANG_DIAG_PUSH
    PEEJAY_CLANG_DIAG_NO_FLOAT_EQUAL
    // These defaults are arbitrary.
    constexpr float_type epsilon = 16 * std::numeric_limits<float_type>::epsilon();
    static_assert(epsilon < float_type{1.0});
    constexpr float_type abs_th = std::numeric_limits<float_type>::min();
    if (a == b) {
      return true;
    }
    auto const norm = std::min((std::abs(a) + std::abs(b)), std::numeric_limits<float_type>::max());
    return std::abs(a - b) < std::max(abs_th, epsilon * norm);
    PEEJAY_CLANG_DIAG_POP
  }

  template <std::size_t Index = 0> static constexpr bool variant_equal(element const &lhs, element const &rhs) {
    assert(!lhs.var_.valueless_by_exception() && lhs.var_.index() == rhs.var_.index() && lhs.var_.index() >= Index);
    if constexpr (Index >= std::variant_size_v<decltype(lhs.var_)>) {
      return false;
    } else {
      if (lhs.var_.index() == Index) {
        return equal(std::get<Index>(lhs.var_), std::get<Index>(rhs.var_));
      }
      return variant_equal<Index + 1>(lhs, rhs);
    }
  }
};

template <typename Policies>
concept dom_policies = requires(Policies &&p) {
  requires std::unsigned_integral<decltype(p.max_array_size)>;
  requires std::unsigned_integral<decltype(p.max_object_size)>;
};
struct default_dom_policies {
  static std::size_t const max_array_size = 50;
  static std::size_t const max_object_size = 50;
};

//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
/// \brief A PJ JSON parser backend which constructs a DOM using instances of
///   peejay::dom::element.
template <policy PJPolicies = default_policies, dom_policies DOMPolicies = default_dom_policies> class dom {
public:
  using policies = std::remove_reference_t<PJPolicies>;
  using element = ::peejay::dom::element<PJPolicies>;

  using char_type = typename PJPolicies::char_type;
  using integer_type = typename PJPolicies::integer_type;
  using float_type = typename PJPolicies::float_type;
  using string_view = std::basic_string_view<char_type>;
  using string = element::string;
  using array = element::array;
  using object = element::object;

  constexpr dom() = default;
  dom(dom const &) = delete;
  constexpr dom(dom &&) noexcept = default;
  ~dom() noexcept = default;

  dom &operator=(dom const &) = delete;
  constexpr dom &operator=(dom &&) noexcept = default;

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
  std::stack<element, arrayvec<element, PJPolicies::max_stack_depth>> stack_;
};

enum class dom_error : int {
  none,
  too_many_array_members,
  too_many_object_members,
};

// ******************
// * error category *
// ******************
/// The error category object for PJ errors
class dom_error_category final : public std::error_category {
public:
  /// Returns a pointer to a C string naming the error category.
  ///
  /// \returns The string "PJ JSON Parser".
  constexpr char const *name() const noexcept override { return "PJ JSON Parser"; }

  /// Returns a string describing the given error in the PJ category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  constexpr std::string message(int const err) const override {
    switch (static_cast<dom_error>(err)) {
    case dom_error::none: return "none";
    case dom_error::too_many_array_members: return "Too many array members for DOM";
    case dom_error::too_many_object_members: return "Too many object members for DOM";
    default: break;
    }
    return "unknown PJ DOM error code";
  }
};

/// \param e  The error value to be converted.
/// \returns  A std::error_code which encapsulates the error \p e.
inline std::error_code make_error_code(dom_error const e) noexcept {
  static dom_error_category const cat;
  return {static_cast<int>(e), cat};
}

//===----------------------------------------------------------------------===//
//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
// result
// ~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies>
std::optional<element<PJPolicies>> dom<PJPolicies, DOMPolicies>::result() noexcept {
  if (stack_.empty()) {
    return {std::nullopt};
  }
  // The stack will contain only the root element unless
  // there was an error which interrupted parsing.
  auto result = std::move(stack_.top());
  // Empty the stack in case any of the dom methods are subsequently called.
  while (!stack_.empty()) {
    stack_.pop();
  }
  return result;
}

// float value
// ~~~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies>
std::error_code dom<PJPolicies, DOMPolicies>::float_value(float_type v) {
  if constexpr (std::same_as<float_type, no_float_type>) {
    // Floating point support is disabled so do nothing (the function will never be called anyway).
    return {};
  } else {
    return this->record<float_type>(v);
  }
}

// begin array
// ~~~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies> std::error_code dom<PJPolicies, DOMPolicies>::begin_array() {
  stack_.emplace(element::template make<array>());
  return {};
}
// end array
// ~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies> std::error_code dom<PJPolicies, DOMPolicies>::end_array() {
  assert(!stack_.empty() && stack_.top().template holds<array>() &&
         "The element on top of the stack must be of type array.");
  return this->end_composite();
}

// begin object
// ~~~~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies> std::error_code dom<PJPolicies, DOMPolicies>::begin_object() {
  stack_.emplace(element::template make<object>());
  return {};
}
// key
// ~~~
template <policy PJPolicies, dom_policies DOMPolicies>
std::error_code dom<PJPolicies, DOMPolicies>::key(string_view const &s) {
  key_ = s;
  return {};
}
// end object
// ~~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies> std::error_code dom<PJPolicies, DOMPolicies>::end_object() {
  assert(!stack_.empty() && stack_.top().template holds<object>() &&
         "The element on top of the stack must be of type object.");
  return this->end_composite();
}

// record
// ~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies>
template <typename MemberType, typename... Args>
std::error_code dom<PJPolicies, DOMPolicies>::record(Args &&...args) {
  return this->record(element::template make<MemberType>(std::forward<Args>(args)...));
}

template <policy PJPolicies, dom_policies DOMPolicies>
std::error_code dom<PJPolicies, DOMPolicies>::record(element &&el) {
  assert(stack_.size() < PJPolicies::max_stack_depth);
  if (stack_.empty()) {
    stack_.emplace(std::move(el));
    return {};
  }
  // We're inside a composite object of some kind. Add this object to it.
  auto &top = stack_.top();
  assert((top.template holds<array>() || top.template holds<object>()) && "Type of top-of-stack was unexpected");
  if (auto *const arr = top.template get_if<array>()) {
    if (arr->size() >= DOMPolicies::max_array_size) {
      return make_error_code(dom_error::too_many_array_members);
    }
    arr->emplace_back(std::move(el));
  } else if (auto *const obj = top.template get_if<object>()) {
    if (obj->size() >= DOMPolicies::max_object_size) {
      return make_error_code(dom_error::too_many_object_members);
    }
    obj->insert_or_assign(std::move(key_), std::move(el));
  } else {
    assert(false && "Type of top-of-stack was unexpected");
  }
  return {};
}

// end composite
// ~~~~~~~~~~~~~
template <policy PJPolicies, dom_policies DOMPolicies> std::error_code dom<PJPolicies, DOMPolicies>::end_composite() {
  if (stack_.size() < 2) {
    return {};
  }
  auto el = std::move(stack_.top());
  stack_.pop();
  return this->record(std::move(el));
}

}  // end namespace peejay::dom

template <> struct std::is_error_code_enum<peejay::dom::dom_error> : std::true_type {};

#endif  // PEEJAY_DOM_HPP
