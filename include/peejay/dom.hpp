//===- include/peejay/dom.hpp -----------------------------*- mode: C++ -*-===//
//*      _                  *
//*   __| | ___  _ __ ___   *
//*  / _` |/ _ \| '_ ` _ \  *
//* | (_| | (_) | | | | | | *
//*  \__,_|\___/|_| |_| |_| *
//*                         *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
/// \brief Provides a simple document object model backend for the PJ JSON
///   parser.
///
/// The Document Object Model (DOM) is a data representation of the objects
/// that comprise the structure and content of a JSON document.
#ifndef PEEJAY_DOM_HPP
#define PEEJAY_DOM_HPP

#include <functional>
#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "peejay/details/type_list.hpp"
#include "peejay/parser.hpp"

namespace peejay::dom {

namespace details {

/// A custom hasher string_hash with is_transparent to enable heterogeneous lookup in objects.
struct string_hash {
  /// Marks this hasher as transparent, enabling heterogeneous lookup.
  using is_transparent = void;
  /// \param str  A null-terminated string to be hashed.
  /// \returns The hash of \p str.
  [[nodiscard]] size_t operator()(char8_t const* str) const { return std::hash<std::u8string_view>{}(str); }
  /// \param str  The string to be hashed.
  /// \returns The hash of \p str.
  [[nodiscard]] size_t operator()(std::u8string_view const& str) const {
    return std::hash<std::u8string_view>{}(str);
  }
  /// \param str  The string to be hashed.
  /// \returns The hash of \p str.
  [[nodiscard]] size_t operator()(std::u8string const& str) const { return std::hash<std::u8string>{}(str); }
};

}  // end namespace details

/// The type used to represent a JSON null value in the DOM.
struct null {
  /// All instances of null compare equal.
  /// \returns true.
  friend constexpr bool operator==(null /*unused*/, null /*unused*/) noexcept { return true; }
};

/// Yields true if \p T is an instance of std::unique_ptr<>, false otherwise.
/// \tparam T  The type to be checked.
template <typename T> struct is_unique_pointer : std::false_type {};
/// Yields true if \p T is an instance of std::unique_ptr<>, false otherwise.
template <typename T> struct is_unique_pointer<std::unique_ptr<T>> : std::true_type {};
/// A helper variable template for convenient use of is_unique_pointer<T>::value.
template <typename T> static constexpr inline bool is_unique_pointer_v = is_unique_pointer<T>::value;

//*      _                   _    *
//*  ___| |___ _ __  ___ _ _| |_  *
//* / -_) / -_) '  \/ -_) ' \  _| *
//* \___|_\___|_|_|_\___|_||_\__| *
//*                               *
/// \brief A single value in the DOM.
///
/// An element holds exactly one value of one of the types listed in element::member_types: a null, boolean,
/// integer, floating-point number (if enabled), string, array, or object.
///
/// \tparam PJPolicies  The parser policies. These supply the integer and floating-point types.
template <policy PJPolicies> class element {
public:
  /// The type used to represent a JSON null.
  using null = peejay::dom::null;
  /// The type used to represent a JSON boolean.
  using boolean = bool;
  /// The type used to represent a JSON integer.
  using integer_type = typename PJPolicies::integer_type;
  /// The type used to represent a JSON floating-point number.
  using float_type = typename PJPolicies::float_type;
  // TODO: here be allocations in  the following three containers.
  /// The type used to represent a JSON string.
  using string = std::u8string;
  /// The type used to represent a JSON object.
  using object = std::unordered_map<string, element, details::string_hash, std::equal_to<>>;
  /// The type used to represent a JSON array.
  using array = std::vector<element>;

  /// The types of the values held directly by an element. float_type is included only if floating-point support
  /// is enabled.
  using simple_types = type_list::concat<type_list::type_list<null, boolean, integer_type, string>,
                                         std::conditional_t<std::is_same_v<float_type, no_float_type>,
                                                            type_list::type_list<>, type_list::type_list<float_type>>>;
  /// The types of the values which contain other elements.
  using composite_types = type_list::type_list<array, object>;
  /// All of the types of value that may be held by an element.
  using member_types = type_list::concat<simple_types, composite_types>;

  /// Creates an element holding a value of type \p MemberType.
  ///
  /// \tparam MemberType  The type of the value to be held. One of element::member_types.
  /// \tparam Args  The types of the arguments forwarded to the constructor of \p MemberType.
  /// \param args  Arguments forwarded to the constructor of \p MemberType.
  /// \returns A new element.
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
  /// Move constructor.
  /// \param rhs  The element to be moved.
  constexpr element(element &&rhs) noexcept = default;
  /// Destructor.
  ~element() noexcept = default;

  element &operator=(element const &rhs) = delete;
  /// Move assignment.
  /// \param rhs  The element to be moved.
  /// \returns *this.
  constexpr element &operator=(element &&rhs) noexcept = default;

  /// Compares two elements for equality. Elements are equal if they hold values of the same type and those values
  /// are equal. Arrays and objects are compared member-wise. Floating-point values are compared with a small
  /// relative tolerance.
  ///
  /// \param lhs  The first element to be compared.
  /// \param rhs  The second element to be compared.
  /// \returns True if the elements are equal, false otherwise.
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
  [[nodiscard]] constexpr bool holds() const noexcept {
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
  ///
  /// \tparam MemberType one of the possible member types. See element::member_types.
  /// \returns A pointer to the value stored in the element or null pointer on error.
  template <typename MemberType>
    requires type_list::has_type_v<member_types, MemberType>
  [[nodiscard]] constexpr auto const* get_if() const noexcept {
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

  /// If the element holds a value of type \p MemberType, returns a pointer to the stored value. Otherwise, returns a
  /// null pointer.
  ///
  /// \tparam MemberType one of the possible member types. See element::member_types.
  /// \returns A pointer to the value stored in the element or null pointer on error.
  template <typename MemberType>
    requires type_list::has_type_v<member_types, MemberType>
  [[nodiscard]] constexpr auto* get_if() noexcept {
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

  /// A convenience method that allows access to members of an object element.
  ///
  /// If the element represents an object and it has a member with the name given by \p key and that property
  /// is of the type \p Desired, then a reference to the associated value is returned. If any of these
  /// conditions does not hold, an empty optional is returned.
  ///
  /// \tparam Desired  The required type of the value associated with the given key.
  /// \param key The object key whose associated value is to be returned.
  /// \returns An optional reference to the associated value.
  template <typename Desired>
    requires type_list::has_type_v<element::member_types, Desired>
  [[nodiscard]] constexpr std::optional<std::reference_wrapper<Desired const>> get_object_element(
      std::u8string_view const& key) const {
    if (auto const* const obj = this->get_if<object>()) {
      if (auto const pos = obj->find(key); pos != obj->end()) {
        if (auto const* const ptr = pos->second.template get_if<Desired>()) {
          return *ptr;
        }
      }
    }
    return std::nullopt;
  }

  /// A convenience method that allows access to members of an object element.
  ///
  /// If the element represents an object and it has a member with the name given by \p key and that property
  /// is of the type \p Desired, then a reference to the associated value is returned. If any of these
  /// conditions does not hold, an empty optional is returned.
  ///
  /// \tparam Desired  The required type of the value associated with the given key.
  /// \param key The object key whose associated value is to be returned.
  /// \returns An optional reference to the associated value.
  template <typename Desired>
    requires type_list::has_type_v<element::member_types, Desired>
  [[nodiscard]] constexpr std::optional<std::reference_wrapper<Desired>> get_object_element(
      std::u8string_view const& key) {
    if (auto* const obj = this->get_if<object>()) {
      if (auto const pos = obj->find(key); pos != obj->end()) {
        if (auto* const ptr = pos->second.template get_if<Desired>()) {
          return *ptr;
        }
      }
    }
    return std::nullopt;
  }

private:
  using object_ptr = std::unique_ptr<object>;  // TODO: here be allocations
  using array_ptr = std::unique_ptr<array>;    // TODO: here be allocations
  /// The types used to store composite values. Arrays and objects are held by pointer.
  using internal_composite_types = type_list::type_list<array_ptr, object_ptr>;
  static_assert(composite_types::size == internal_composite_types::size,
                "There must be a one-to-one correspondence between internal and public composite types");

  /// The value held by this element.
  type_list::to_variant<type_list::concat<simple_types, internal_composite_types>> var_;

  /// The 'tag' type is here simply to be the first argument for the forwarding constructor below. This
  /// prevents that ctor from being too "greedy" and matching arbitrary argument lists.
  struct tag {};
  template <typename... Args> constexpr explicit element(tag, Args &&...args) : var_{std::forward<Args>(args)...} {}

  /// Compares two values of the same type. Pointers to composite values are dereferenced before comparison.
  ///
  /// \tparam T  The type of the values to be compared.
  /// \param lhs  The first value to be compared.
  /// \param rhs  The second value to be compared.
  /// \returns True if the values are equal, false otherwise.
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

  /// Compares two floating-point values for approximate equality using a combination of relative and absolute
  /// tolerances.
  ///
  /// \param a  The first value to be compared.
  /// \param b  The second value to be compared.
  /// \returns True if the values are approximately equal, false otherwise.
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
    auto const norm = std::min(std::abs(a) + std::abs(b), std::numeric_limits<float_type>::max());
    return std::abs(a - b) < std::max(abs_th, epsilon * norm);
    PEEJAY_CLANG_DIAG_POP
  }

  /// Compares the values held by two elements which hold the same variant alternative. The alternatives are
  /// checked recursively starting at index \p Index.
  ///
  /// \tparam Index  The first variant alternative index to be checked.
  /// \param lhs  The first element to be compared.
  /// \param rhs  The second element to be compared.
  /// \returns True if the values are equal, false otherwise.
  template <std::size_t Index = 0>
    requires(Index <= std::variant_size_v<decltype(element::var_)>)
  static constexpr bool variant_equal(element const& lhs, element const& rhs) {
    assert(!lhs.var_.valueless_by_exception() && lhs.var_.index() == rhs.var_.index() && lhs.var_.index() >= Index);
    if constexpr (Index == std::variant_size_v<decltype(element::var_)>) {
      return false;
    } else {
      if (lhs.var_.index() == Index) {
        return equal(std::get<Index>(lhs.var_), std::get<Index>(rhs.var_));
      }
      return variant_equal<Index + 1>(lhs, rhs);
    }
  }
};

/// The concept dom_policies<Policies> is satisfied by a type which describes the configuration of the DOM backend.
/// It must provide unsigned integral `max_array_size` and `max_object_size` values, which give the maximum number of
/// members permitted in an array and an object respectively.
template <typename Policies>
concept dom_policies = requires(Policies &&p) {
  requires std::unsigned_integral<decltype(p.max_array_size)>;
  requires std::unsigned_integral<decltype(p.max_object_size)>;
};
/// The default DOM policies.
struct default_dom_policies {
  /// The maximum number of members of an array.
  static std::size_t const max_array_size = 50;
  /// The maximum number of members of an object.
  static std::size_t const max_object_size = 50;
};

//*     _            *
//*  __| |___ _ __   *
//* / _` / _ \ '  \  *
//* \__,_\___/_|_|_| *
//*                  *
/// \brief A PJ JSON parser backend which constructs a DOM using instances of
///   peejay::dom::element.
///
/// \tparam PJPolicies  The parser policies.
/// \tparam DOMPolicies  The DOM policies which limit the size of arrays and objects.
template <policy PJPolicies = default_policies, dom_policies DOMPolicies = default_dom_policies> class dom {
public:
  /// The parser policies used with this backend.
  using policies = std::remove_reference_t<PJPolicies>;
  /// The type of the elements from which the DOM is constructed.
  using element = ::peejay::dom::element<PJPolicies>;

  /// The type used to represent a JSON integer.
  using integer_type = PJPolicies::integer_type;
  /// The type used to represent a JSON floating-point number.
  using float_type = PJPolicies::float_type;
  /// The type used to represent a JSON string.
  using string = element::string;
  /// The type used to represent a JSON array.
  using array = element::array;
  /// The type used to represent a JSON object.
  using object = element::object;

  /// Constructs an empty DOM.
  constexpr dom() = default;
  dom(dom const &) = delete;
  /// Move constructor.
  constexpr dom(dom &&) noexcept = default;
  /// Destructor.
  ~dom() noexcept = default;

  dom &operator=(dom const &) = delete;
  /// Move assignment.
  /// \returns *this.
  constexpr dom &operator=(dom &&) noexcept = default;

  /// Called by the parser at the end of the parse to extract the result.
  ///
  /// \returns The root element of the DOM or std::nullopt if no element was recorded.
  std::optional<element> result() noexcept;

  /// Records a string value.
  /// \param v  The string value.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code string_value(std::u8string_view const& v) { return this->record<string>(v); }
  /// Records an integer value.
  /// \param v  The integer value.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code integer_value(std::make_signed_t<integer_type> v) { return this->record<integer_type>(v); }
  /// Records a floating-point value.
  /// \param v  The floating-point value.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code float_value(float_type v);
  /// Records a boolean value.
  /// \param v  The boolean value.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code boolean_value(bool v) { return this->record<bool>(v); }
  /// Records a null value.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code null_value() { return this->record<null>(); }

  /// Begins a new array. Subsequent values are added to this array until end_array() is called.
  /// \returns An empty (success) error code.
  std::error_code begin_array();
  /// Completes the current array and records it in its container (if any).
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code end_array();

  /// Begins a new object. Subsequent key/value pairs are added to this object until end_object() is called.
  /// \returns An empty (success) error code.
  std::error_code begin_object();
  /// Records an object key. The next value recorded will be associated with this key.
  /// \param s  The key.
  /// \returns An empty (success) error code.
  std::error_code key(std::u8string_view const& s);
  /// Completes the current object and records it in its container (if any).
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code end_object();

private:
  /// Creates an element holding a value of type \p MemberType and records it.
  ///
  /// \tparam MemberType  The type of the value to be recorded.
  /// \tparam Args  The types of the arguments forwarded to the constructor of \p MemberType.
  /// \param args  Arguments forwarded to the constructor of \p MemberType.
  /// \returns An error code indicating whether the element could be recorded.
  template <typename MemberType, typename... Args> std::error_code record(Args &&...args);
  /// Records an element. If the stack is empty, the element becomes the root of the DOM. Otherwise it is added to
  /// the array or object on top of the stack.
  ///
  /// \param el  The element to be recorded.
  /// \returns An error code: dom_error::too_many_array_members or dom_error::too_many_object_members if the
  ///   containing array or object is full; otherwise success.
  std::error_code record(element &&el);

  /// Pops the completed array or object from the top of the stack and records it in its container. If the
  /// completed element is the root, it is left on the stack.
  ///
  /// \returns An error code indicating whether the element could be recorded.
  std::error_code end_composite();

  /// The keys of the objects that are being constructed.
  std::stack<string> key_;  // TODO: allocations
  /// The elements that are being constructed.
  std::stack<element, arrayvec<element, PJPolicies::max_stack_depth>> stack_;
};

/// The errors that may be reported by the DOM backend.
enum class dom_error : int {
  /// No error.
  none,
  /// An attempt was made to add more than DOMPolicies::max_array_size members to an array.
  too_many_array_members,
  /// An attempt was made to add more than DOMPolicies::max_object_size members to an object.
  too_many_object_members,
};

// ******************
// * error category *
// ******************
/// The error category object for DOM errors.
class dom_error_category final : public std::error_category {
public:
  /// Returns a pointer to a C string naming the error category.
  ///
  /// \returns The string "PJ JSON DOM".
  [[nodiscard]] constexpr char const* name() const noexcept override { return "PJ JSON DOM"; }

  /// Returns a string describing the given error in the DOM category.
  ///
  /// \param err  An error number which should be one of the values in the
  ///   peejay::dom::dom_error enumeration.
  /// \returns  The message that corresponds to the error \p err.
  [[nodiscard]] std::string message(int const err) const override {
    switch (static_cast<dom_error>(err)) {
    case dom_error::none: return "none";
    case dom_error::too_many_array_members: return "Too many array members for DOM";
    case dom_error::too_many_object_members: return "Too many object members for DOM";
    default: break;
    }
    return "unknown PJ DOM error code";
  }
};

/// Converts a peejay::dom::dom_error value to a std::error_code.
///
/// \param e  The error value to be converted.
/// \returns  A std::error_code which encapsulates the error \p e.
[[nodiscard]] inline std::error_code make_error_code(dom_error const e) noexcept {
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
std::error_code dom<PJPolicies, DOMPolicies>::key(std::u8string_view const& s) {
  key_.emplace(s);
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
    obj->insert_or_assign(std::move(key_.top()), std::move(el));
    key_.pop();
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

/// Enables implicit conversion from peejay::dom::dom_error to std::error_code.
template <> struct std::is_error_code_enum<peejay::dom::dom_error> : std::true_type {};

#endif  // PEEJAY_DOM_HPP
