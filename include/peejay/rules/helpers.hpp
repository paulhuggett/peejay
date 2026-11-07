//===- include/peejay/rules/helpers.hpp -------------------*- mode: C++ -*-===//
//*  _          _                      *
//* | |__   ___| |_ __   ___ _ __ ___  *
//* | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* | | | |  __/ | |_) |  __/ |  \__ \ *
//* |_| |_|\___|_| .__/ \___|_|  |___/ *
//*              |_|                   *
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
/// \file helpers.hpp
/// \brief Provides base classes for rules which parse JSON objects and arrays.
///
/// peejay::rules::object_helper parses an object whose properties are stored in the members of a struct.
/// peejay::rules::homogeneous_array_helper parses an array whose elements are all of the same type and appends them
/// to a container.
#ifndef PEEJAY_RULES_HELPERS_HPP
#define PEEJAY_RULES_HELPERS_HPP

#include "peejay/rules/terminals.hpp"

// Standard library
#include <algorithm>
#include <bitset>
#include <concepts>
#include <ranges>
#include <tuple>
#include <variant>

namespace peejay::rules {

/// \brief This concept is satisfied by a user-defined rule which object_helper can push to parse the value of a
///   property.
///
/// The rule must derive from rule<Policies>, have a member type `value_type`, and be constructible from a pointer to
/// its value_type.
template <typename Rule, typename Policies>
concept user_rule = std::derived_from<Rule, rule<Policies>> && requires { typename Rule::value_type; } &&
                    std::constructible_from<Rule, typename Rule::value_type*>;

namespace details {

/// The number of times that \p T appears in \p Ts.
template <typename T, typename... Ts>
inline constexpr std::size_t count_v = (std::size_t{std::is_same_v<T, Ts>} + ... + std::size_t{0});

}  // end namespace details

/// \brief A base class for rules which parse a JSON object with a fixed set of properties.
///
/// A subclass passes its properties to set_properties() from its constructor. Each property pairs an object key
/// with a pointer to the location where the key's value will be stored. When the key is encountered, the matching
/// rule is pushed to parse its value: one of the terminal rules for a built-in type, or one of \p UserRules.
///
/// Every property must be present in the object. A key which is not one of the properties produces
/// error::unrecognized_property and an object which is missing a property produces error::incomplete_object.
///
/// \tparam Policies  The parser policies.
/// \tparam Elements  The maximum number of properties.
/// \tparam UserRules  Additional rules for property values. A property may hold a pointer to the value_type of any of
///   these rules. The value_types must be distinct from each other and from the built-in property types. These rules
///   must be complete types, so a rule cannot name itself here. A recursive grammar needs a rule derived directly from
///   rule<>.
template <policy Policies, std::size_t Elements, user_rule<Policies>... UserRules>
class object_helper : public rule<Policies> {
  /// Pointers to the types handled by the built-in terminal rules.
  using builtin_pointers = type_list::type_list<bool*, typename Policies::integer_type*,
                                                std::make_unsigned_t<typename Policies::integer_type>*,
                                                typename Policies::float_type*, std::u8string*, std::string*>;
  /// Pointers to the types handled by UserRules.
  using user_pointers = type_list::type_list<typename UserRules::value_type*...>;

  static_assert((!type_list::has_type_v<builtin_pointers, typename UserRules::value_type*> && ...),
                "a UserRules value_type must not be one of the built-in property types");
  static_assert(((details::count_v<typename UserRules::value_type, typename UserRules::value_type...> == 1) && ...),
                "each UserRules value_type must be unique");

public:
  /// The parser policies.
  using policies = Policies;
  /// The rules that this rule may push.
  using children = type_list::concat<builtin_rules<Policies>, type_list::type_list<UserRules...>>;
  /// An object property: a key and a pointer to the location where its value will be stored. A default-constructed
  /// property holds std::monostate and never matches a key.
  using property =
      std::pair<std::u8string_view,
                type_list::to_variant<type_list::concat<type_list::type_list<std::monostate>,
                                                        type_list::concat<builtin_pointers, user_pointers>>>>;

  /// \returns The name of this rule.
  [[nodiscard]] constexpr char const* name() const noexcept override { return "object"; }
  /// Called at the start of the object.
  /// \param context  The parse context.
  /// \returns Success.
  std::error_code begin_object(context_base<Policies>* const context) override {
    (void)context;
    return {};
  }
  /// Called for each key in the object. If \p k matches one of the properties, pushes the rule which will parse its
  /// value.
  /// \param context  The parse context.
  /// \param k  The object key.
  /// \returns error::unrecognized_property if \p k is not one of the properties, peejay::error::nesting_too_deep if
  ///   there is no space for the rule, otherwise success.
  std::error_code key(context_base<Policies>* const context, std::u8string_view const k) override {
    // Unused entries hold std::monostate and an empty key: they must not match an empty key in the input.
    auto const pred = [&k](property const& entry) {
      return !std::holds_alternative<std::monostate>(entry.second) && entry.first == k;
    };
    if (auto const pos = std::ranges::find_if(required_, pred); pos != std::ranges::end(required_)) {
      seen_.set(static_cast<std::size_t>(std::distance(std::ranges::begin(required_), pos)));
      return std::visit(
          [this, context]<typename T0>(T0& arg) {
            if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
              assert(false && "should never find a monostate entry");
              return std::error_code{};
            } else if constexpr (type_list::has_type_v<user_pointers, std::decay_t<T0>>) {
              using user_rule_type = std::tuple_element_t<type_list::index_of_v<user_pointers, std::decay_t<T0>>,
                                                          std::tuple<UserRules...>>;
              return this->template push<user_rule_type>(this, context, arg);
            } else {
              return this->push_terminal(this, context, arg);
            }
          },
          pos->second);
    }
    return peejay::rules::error::unrecognized_property;
  }
  /// Called at the end of the object. Pops this rule if all of the properties were seen.
  /// \param context  The parse context.
  /// \returns error::incomplete_object if any of the properties were not seen, otherwise success.
  std::error_code end_object(context_base<Policies>* const context) override {
    if (!seen_.all()) {
      return peejay::rules::error::incomplete_object;
    }
    return this->pop(context);
  }

protected:
  /// Rules are not destroyed through this type: they are discarded without running their destructors.
  ~object_helper() noexcept = default;

  /// Sets the properties which the object must contain.
  ///
  /// \param first_required  The first of the properties.
  /// \param last_required  The end of the properties. There must be no more than \p Elements of them.
  template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    requires std::is_same_v<std::iter_value_t<Iterator>, property>
  void set_properties(Iterator first_required, Sentinel last_required) {
    seen_.reset();
    auto n_required = std::ranges::distance(first_required, last_required);
    assert(n_required >= 0 && n_required <= std::ssize(required_));
    // Mark the unused entries as seen so that seen_.all() is true once every property has been found.
    auto const last = std::ssize(seen_);
    for (; n_required < last; ++n_required) {
      seen_.set(static_cast<std::size_t>(n_required));
    }
    auto pos = std::ranges::copy(first_required, last_required, required_.begin()).out;
    std::fill(pos, required_.end(), property{});
  }
  /// Sets the properties which the object must contain.
  ///
  /// \param properties  The properties. There must be no more than \p Elements of them.
  void set_properties(std::initializer_list<property> properties) {
    this->set_properties(std::begin(properties), std::end(properties));
  }
  /// Sets the properties which the object must contain.
  ///
  /// \param range  The properties. There must be no more than \p Elements of them.
  template <std::ranges::forward_range RequiredRange>
    requires std::is_same_v<std::ranges::range_value_t<RequiredRange>, property>
  void set_properties(RequiredRange const& range) {
    this->set_properties(std::begin(range), std::end(range));
  }

private:
  /// The properties. Unused entries hold std::monostate.
  std::array<property, Elements> required_;
  /// The properties which have been seen. The bits for unused entries are set.
  std::bitset<Elements> seen_;
};

/// \brief This concept is satisfied by a container to which homogeneous_array_helper can append elements.
///
/// The container must have a member type `value_type` and a member function `emplace_back()` which
/// default-constructs a new element at the end of the container and returns a reference to it. std::vector<>,
/// std::deque<>, and std::list<> all satisfy this concept.
template <typename Container>
concept emplace_back_container = requires(Container& c) {
  typename Container::value_type;
  { c.emplace_back() } -> std::same_as<typename Container::value_type&>;
};

/// \brief A rule which parses a JSON array whose elements are all of the same type.
///
/// An element is appended to the container for each member of the array and \p Rule is pushed to parse its value.
/// The rule can be used directly or as one of object_helper's UserRules.
///
/// \tparam Policies  The parser policies.
/// \tparam Container  The container to which an element is appended for each member of the array.
/// \tparam Rule  The rule used to parse each array member.
/// \tparam MaxElements  The maximum number of elements allowed in the container or 0 for no limit. If not 0,
///   Container must also be a sized range.
template <policy Policies, emplace_back_container Container, typename Rule, std::size_t MaxElements = 0U>
  requires(MaxElements == 0 || std::ranges::sized_range<Container>)
class homogeneous_array_helper final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = Container;
  /// The rules that this rule may push.
  using children = type_list::type_list<Rule>;

  /// \param container  The container to which the array's elements are appended.
  explicit constexpr homogeneous_array_helper(Container* const container) noexcept : container_{container} {}
  /// \returns The name of this rule.
  [[nodiscard]] constexpr char const* name() const noexcept override { return "homogeneous-array"; }
  /// Called at the start of the array.
  /// \param context  The parse context.
  /// \returns Success.
  std::error_code begin_array(context_base<Policies>* const context) override {
    (void)context;
    return {};
  }
  /// Called before each member of the array. Appends an element for that member.
  /// \param context  The parse context.
  /// \returns error::too_many_array_elements if the container is already full, peejay::error::nesting_too_deep if
  ///   there is no space for the rule, otherwise success.
  std::error_code element(context_base<Policies>* const context) override {
    if constexpr (MaxElements != 0U) {
      if (container_->size() >= MaxElements) {
        return error::too_many_array_elements;
      }
    }
    return this->template push<Rule>(this, context, &container_->emplace_back());
  }
  /// Called at the end of the array. Pops this rule.
  /// \param context  The parse context.
  /// \returns Success.
  std::error_code end_array(context_base<Policies>* const context) override { return this->pop(context); }

private:
  /// The container to which the array's elements are appended.
  Container* container_;
};

}  // end namespace peejay::rules

#endif  // PEEJAY_RULES_HELPERS_HPP
