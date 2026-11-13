//===- include/peejay/rules/non_terminal_rule.hpp ---------*- mode: C++ -*-===//
//*                      _                      _             _  *
//*  _ __   ___  _ __   | |_ ___ _ __ _ __ ___ (_)_ __   __ _| | *
//* | '_ \ / _ \| '_ \  | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | | *
//* | | | | (_) | | | | | ||  __/ |  | | | | | | | | | | (_| | | *
//* |_| |_|\___/|_| |_|  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_| *
//*                                                              *
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
/// \file non_terminal_rule.hpp
/// \brief Provides base classes for rules which parse JSON objects and arrays.
///
/// peejay::rules::object_rule parses an object whose properties are stored in the members of a struct.
/// peejay::rules::homogeneous_array_rule parses an array whose elements are all of the same type and appends them
/// to a container. peejay::rules::enum_rule accepts one of a fixed set of strings and stores the value associated
/// with it.
#ifndef PEEJAY_RULES_NON_TERMINAL_RULE_HPP
#define PEEJAY_RULES_NON_TERMINAL_RULE_HPP

#include "peejay/rules/terminals.hpp"

// Standard library
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <concepts>
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <utility>
#include <variant>

namespace peejay::rules {

/// \brief This concept is satisfied by a user-defined rule which object_rule can push to parse the value of a
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
/// A subclass passes its properties to object_rule's constructor, or to set_properties(). Each property pairs an object
/// key with a pointer to the location where the key's value will be stored. When the key is encountered, the matching
/// rule is pushed to parse its value: one of the terminal rules for a built-in type, or one of \p UserRules. The
/// built-in types are bool, the policy's float_type, std::u8string, std::string, and the integer types listed by
/// builtin_integers. A string held in an arrayvec<char8_t, Size> needs its string_rule to be listed in \p UserRules,
/// for example `string_rule<Policies, arrayvec<char8_t, 16>>`.
///
/// Every required property must be present in the object; an optional property may be absent. A key which is not one
/// of the properties produces error::unrecognized_property and an object which is missing a required property
/// produces error::incomplete_object.
///
/// If necessary, the fields of the final object can be validated by overriding the end_object() member function in
/// the subclass. This override may check the fields and return an error if the check fails otherwise call the
/// base class implementation.
///
/// \tparam Policies  The parser policies.
/// \tparam RequiredElements  The maximum number of required properties.
/// \tparam OptionalElements  The maximum number of optional properties. Must be given explicitly if there are
///   \p UserRules.
/// \tparam UserRules  Additional rules for property values. A property may hold a pointer to the value_type of any of
///   these rules. The value_types must be distinct from each other and from the built-in property types. These rules
///   must be complete types, so a rule cannot name itself here. A recursive grammar needs a rule derived directly from
///   rule<>.
template <policies Policies, std::size_t RequiredElements, std::size_t OptionalElements = 0,
          user_rule<Policies>... UserRules>
class object_rule : public rule<Policies> {
  /// Pointers to the types handled by the built-in terminal rules.
  using builtin_pointers =
      type_list::concat<type_list::type_list<bool*, typename Policies::float_type*, std::u8string*, std::string*>,
                        type_list::transform<builtin_integers<Policies>, std::add_pointer_t>>;
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
    auto v = [this, context]<typename T0>(T0& arg) {
      if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
        assert(false && "should never find a monostate entry");  // LCOV_EXCL_LINE: should be unreachable
        return std::error_code{};
      } else if constexpr (type_list::has_type_v<user_pointers, std::decay_t<T0>>) {
        using user_rule_type =
            std::tuple_element_t<type_list::index_of_v<user_pointers, std::decay_t<T0>>, std::tuple<UserRules...>>;
        return this->template push<user_rule_type>(this, context, arg);
      } else {
        return this->push_terminal(this, context, arg);
      }
    };

    if (auto const pos = std::ranges::find_if(required_, pred); pos != std::ranges::end(required_)) {
      seen_.set(static_cast<std::size_t>(std::distance(std::ranges::begin(required_), pos)));
      return std::visit(v, pos->second);
    }
    if (auto const pos = std::ranges::find_if(optional_, pred); pos != std::ranges::end(optional_)) {
      return std::visit(v, pos->second);
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
  /// Constructs a rule with no properties. A subclass must call set_properties() before the rule is used unless
  /// \p RequiredElements is 0.
  object_rule() : seen_{~0ULL} {}

  /// Constructs a rule which requires the properties [\p first_required, \p last_required) and has no optional
  /// properties. Equivalent to calling set_properties(first_required, last_required).
  ///
  /// \param first_required  The first of the required properties.
  /// \param last_required  The end of the required properties. There must be no more than \p RequiredElements of them.
  template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    requires std::is_same_v<std::iter_value_t<Iterator>, property>
  object_rule(Iterator first_required, Sentinel last_required) {
    this->set_properties(first_required, last_required);
  }
  /// Constructs a rule which requires \p properties and has no optional properties. Equivalent to calling
  /// set_properties(properties).
  ///
  /// \param properties  The required properties. There must be no more than \p RequiredElements of them.
  object_rule(std::initializer_list<property> properties) { this->set_properties(properties); }
  /// Constructs a rule which requires the properties in \p range and has no optional properties. Equivalent to
  /// calling set_properties(range).
  ///
  /// \param range  The required properties. There must be no more than \p RequiredElements of them.
  template <std::ranges::forward_range RequiredRange>
    requires std::is_same_v<std::ranges::range_value_t<RequiredRange>, property>
  explicit object_rule(RequiredRange const& range) {
    this->set_properties(range);
  }
  /// Constructs a rule which requires the properties [\p first_required, \p last_required) and also accepts the
  /// optional properties [\p first_optional, \p last_optional). Equivalent to calling set_properties() with the same
  /// arguments.
  ///
  /// \param first_required  The first of the required properties.
  /// \param last_required  The end of the required properties. There must be no more than \p RequiredElements of them.
  /// \param first_optional  The first of the optional properties.
  /// \param last_optional  The end of the optional properties. There must be no more than \p OptionalElements of them.
  template <std::forward_iterator RequiredIterator, std::sentinel_for<RequiredIterator> RequiredSentinel,
            std::forward_iterator OptionalIterator, std::sentinel_for<OptionalIterator> OptionalSentinel>
    requires std::is_same_v<std::iter_value_t<RequiredIterator>, property> &&
             std::is_same_v<std::iter_value_t<OptionalIterator>, property>
  object_rule(RequiredIterator first_required, RequiredSentinel last_required, OptionalIterator first_optional,
              OptionalSentinel last_optional) {
    this->set_properties(first_required, last_required, first_optional, last_optional);
  }
  /// Constructs a rule which requires \p required_properties and also accepts \p optional_properties. Equivalent to
  /// calling set_properties(required_properties, optional_properties).
  ///
  /// \param required_properties  The required properties. There must be no more than \p RequiredElements of them.
  /// \param optional_properties  The optional properties. There must be no more than \p OptionalElements of them.
  object_rule(std::initializer_list<property> required_properties,
              std::initializer_list<property> optional_properties) {
    this->set_properties(required_properties, optional_properties);
  }
  /// Constructs a rule which requires the properties in \p required_range and also accepts those in
  /// \p optional_range. Equivalent to calling set_properties(required_range, optional_range).
  ///
  /// \param required_range  The required properties. There must be no more than \p RequiredElements of them.
  /// \param optional_range  The optional properties. There must be no more than \p OptionalElements of them.
  template <std::ranges::forward_range RequiredRange, std::ranges::forward_range OptionalRange>
    requires std::is_same_v<std::ranges::range_value_t<RequiredRange>, property> &&
             std::is_same_v<std::ranges::range_value_t<OptionalRange>, property>
  object_rule(RequiredRange const& required_range, OptionalRange const& optional_range) {
    this->set_properties(required_range, optional_range);
  }

  /// Rules are not destroyed through this type: they are discarded without running their destructors.
  ~object_rule() noexcept = default;

  /// Sets the properties which the object must contain and the properties which it may contain, replacing any that
  /// were set before.
  ///
  /// \param first_required  The first of the required properties.
  /// \param last_required  The end of the required properties. There must be no more than \p RequiredElements of them.
  /// \param first_optional  The first of the optional properties.
  /// \param last_optional  The end of the optional properties. There must be no more than \p OptionalElements of them.
  template <std::forward_iterator RequiredIterator, std::sentinel_for<RequiredIterator> RequiredSentinel,
            std::forward_iterator OptionalIterator, std::sentinel_for<OptionalIterator> OptionalSentinel>
    requires std::is_same_v<std::iter_value_t<RequiredIterator>, property> &&
             std::is_same_v<std::iter_value_t<OptionalIterator>, property>
  void set_properties(RequiredIterator first_required, RequiredSentinel last_required, OptionalIterator first_optional,
                      OptionalSentinel last_optional) {
    [[maybe_unused]] auto const n_optional = std::ranges::distance(first_optional, last_optional);
    assert(n_optional >= 0 && n_optional <= std::ssize(optional_) && "overflow of the optional members");

    seen_.reset();
    auto n_required = std::ranges::distance(first_required, last_required);
    assert(n_required >= 0 && n_required <= std::ssize(required_) && "overflow of the required members");
    // Mark the unused entries as seen so that seen_.all() is true once every property has been found.
    auto const last = std::ssize(seen_);
    for (; n_required < last; ++n_required) {
      seen_.set(static_cast<std::size_t>(n_required));
    }
    std::fill(std::ranges::copy(first_required, last_required, required_.begin()).out, required_.end(), property{});
    std::fill(std::ranges::copy(first_optional, last_optional, optional_.begin()).out, optional_.end(), property{});
  }

  /// Sets the properties which the object must contain, replacing any that were set before. The object has no
  /// optional properties.
  ///
  /// \param first_required  The first of the required properties.
  /// \param last_required  The end of the required properties. There must be no more than \p RequiredElements of them.
  template <std::forward_iterator RequiredIterator, std::sentinel_for<RequiredIterator> RequiredSentinel>
    requires std::is_same_v<std::iter_value_t<RequiredIterator>, property>
  void set_properties(RequiredIterator first_required, RequiredSentinel last_required) {
    std::array<property, 0> optional_properties;
    this->set_properties(first_required, last_required, std::begin(optional_properties), std::end(optional_properties));
  }

  /// Sets the properties which the object must contain and the properties which it may contain, replacing any that
  /// were set before.
  ///
  /// \param required_properties  The required properties. There must be no more than \p RequiredElements of them.
  /// \param optional_properties  The optional properties. There must be no more than \p OptionalElements of them.
  void set_properties(std::initializer_list<property> required_properties,
                      std::initializer_list<property> optional_properties) {
    this->set_properties(std::begin(required_properties), std::end(required_properties),
                         std::begin(optional_properties), std::end(optional_properties));
  }
  /// Sets the properties which the object must contain, replacing any that were set before. The object has no
  /// optional properties.
  ///
  /// \param required_properties  The required properties. There must be no more than \p RequiredElements of them.
  void set_properties(std::initializer_list<property> required_properties) {
    std::array<property, 0> optional_properties;
    this->set_properties(std::begin(required_properties), std::end(required_properties),
                         std::begin(optional_properties), std::end(optional_properties));
  }
  /// Sets the properties which the object must contain and the properties which it may contain, replacing any that
  /// were set before.
  ///
  /// \param required_range  The required properties. There must be no more than \p RequiredElements of them.
  /// \param optional_range  The optional properties. There must be no more than \p OptionalElements of them.
  template <std::ranges::forward_range RequiredRange, std::ranges::forward_range OptionalRange>
    requires std::is_same_v<std::ranges::range_value_t<RequiredRange>, property> &&
             std::is_same_v<std::ranges::range_value_t<OptionalRange>, property>
  void set_properties(RequiredRange const& required_range, OptionalRange const& optional_range) {
    this->set_properties(std::begin(required_range), std::end(required_range), std::begin(optional_range),
                         std::end(optional_range));
  }
  /// Sets the properties which the object must contain, replacing any that were set before. The object has no
  /// optional properties.
  ///
  /// \param required_range  The required properties. There must be no more than \p RequiredElements of them.
  template <std::ranges::forward_range RequiredRange>
    requires std::is_same_v<std::ranges::range_value_t<RequiredRange>, property>
  void set_properties(RequiredRange const& required_range) {
    std::array<property, 0> optional_properties;
    this->set_properties(required_range, optional_properties);
  }

private:
  /// The required properties. Unused entries hold std::monostate.
  std::array<property, RequiredElements> required_;
  /// The optional properties. Unused entries hold std::monostate.
  std::array<property, OptionalElements> optional_;
  /// The properties which have been seen. The bits for unused entries are set.
  std::bitset<RequiredElements> seen_;
};

namespace details {

/// Declared but not defined: used only to find the object_rule from which a type is derived.
/// \returns A pointer to the object_rule base class of the argument.
template <policies Policies, std::size_t RequiredElements, std::size_t OptionalElements, typename... UserRules>
object_rule<Policies, RequiredElements, OptionalElements, UserRules...> const* object_rule_base(
    object_rule<Policies, RequiredElements, OptionalElements, UserRules...> const*);

}  // end namespace details

/// The rules pushed by object_rule are included in the stack size of a subclass even if its `children` hides those
/// of object_rule.
/// \tparam Rule  A rule derived from object_rule.
template <typename Rule>
  requires requires(Rule const* r) { details::object_rule_base(r); }
struct base_children<Rule> {
  /// The rules that object_rule may push.
  using type = std::remove_pointer_t<decltype(details::object_rule_base(std::declval<Rule const*>()))>::children;
};

/// \brief This concept is satisfied by a type to which enum_rule can assign a value: an enumeration or one of the
///   integer types accepted by integer_rule.
template <typename T>
concept enum_target = (std::is_enum_v<T> && !std::is_const_v<T> && !std::is_volatile_v<T>) || integer_target<T>;

/// \brief A base class for rules which accept one of a fixed set of strings and store the value associated with it.
///
/// A subclass passes its enumerators to enum_rule's constructor, or to set_enumerators(). Each enumerator pairs a
/// string with the value which is stored when that string is encountered. If the same string appears more than once,
/// the first enumerator is used. A string which is not one of the enumerators produces
/// error::unrecognized_enumerator.
///
/// The rule can be used directly or as one of object_rule's UserRules. For the latter, \p T should be an enumeration
/// because object_rule already handles pointers to the integer types.
///
/// \tparam Policies  The parser policies.
/// \tparam T  The type of the value stored by this rule.
/// \tparam Elements  The maximum number of enumerators.
template <policies Policies, enum_target T, std::size_t Elements> class enum_rule : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = T;
  /// An enumerator: a string and the value which is stored when that string is encountered.
  using enumerator = std::pair<std::u8string_view, value_type>;

  /// \returns The name of this rule.
  [[nodiscard]] constexpr char const* name() const noexcept override { return "enum"; }
  /// Called when a string value has been parsed. If \p str matches one of the enumerators, stores the associated value
  /// and pops this rule.
  /// \param context  The parse context.
  /// \param str  The string value.
  /// \returns error::unrecognized_enumerator if \p str is not one of the enumerators, otherwise success.
  std::error_code string_value(context_base<Policies>* const context, std::u8string_view const str) override {
    auto const first = enumerators_.begin();
    auto const last = first + size_;
    auto const pos = std::find_if(first, last, [str](enumerator const& e) { return e.first == str; });
    if (pos == last) {
      return error::unrecognized_enumerator;
    }
    *value_ = pos->second;
    return this->pop(context);
  }

protected:
  /// \param value  The location to which the value will be written.
  explicit constexpr enum_rule(value_type* const value) noexcept : value_{value} {}
  /// Constructs a rule which accepts the enumerators [\p first, \p last). Equivalent to constructing the rule from
  /// \p value then calling set_enumerators(first, last).
  ///
  /// \param value  The location to which the value will be written.
  /// \param first  The first of the enumerators.
  /// \param last  The end of the enumerators. There must be no more than \p Elements of them.
  template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    requires std::is_same_v<std::iter_value_t<Iterator>, enumerator>
  enum_rule(value_type* const value, Iterator first, Sentinel last) : value_{value} {
    this->set_enumerators(first, last);
  }
  /// Constructs a rule which accepts \p enumerators. Equivalent to constructing the rule from \p value then calling
  /// set_enumerators(enumerators).
  ///
  /// \param value  The location to which the value will be written.
  /// \param enumerators  The enumerators. There must be no more than \p Elements of them.
  enum_rule(value_type* const value, std::initializer_list<enumerator> enumerators) : value_{value} {
    this->set_enumerators(enumerators);
  }
  /// Constructs a rule which accepts the enumerators in \p range. Equivalent to constructing the rule from \p value
  /// then calling set_enumerators(range).
  ///
  /// \param value  The location to which the value will be written.
  /// \param range  The enumerators. There must be no more than \p Elements of them.
  template <std::ranges::forward_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, enumerator>
  enum_rule(value_type* const value, Range const& range) : value_{value} {
    this->set_enumerators(range);
  }
  /// Rules are not destroyed through this type: they are discarded without running their destructors.
  ~enum_rule() noexcept = default;

  /// Sets the enumerators which the rule accepts.
  ///
  /// \param first  The first of the enumerators.
  /// \param last  The end of the enumerators. There must be no more than \p Elements of them.
  template <std::forward_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    requires std::is_same_v<std::iter_value_t<Iterator>, enumerator>
  void set_enumerators(Iterator first, Sentinel last) {
    auto const size = std::ranges::distance(first, last);
    assert(size >= 0 && static_cast<std::size_t>(size) <= Elements);
    std::ranges::copy(first, last, enumerators_.begin());
    size_ = static_cast<std::size_t>(size);
  }
  /// Sets the enumerators which the rule accepts.
  ///
  /// \param enumerators  The enumerators. There must be no more than \p Elements of them.
  void set_enumerators(std::initializer_list<enumerator> enumerators) {
    this->set_enumerators(std::begin(enumerators), std::end(enumerators));
  }
  /// Sets the enumerators which the rule accepts.
  ///
  /// \param range  The enumerators. There must be no more than \p Elements of them.
  template <std::ranges::forward_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, enumerator>
  void set_enumerators(Range const& range) {
    this->set_enumerators(std::begin(range), std::end(range));
  }

private:
  /// The location to which the value will be written.
  value_type* value_;
  /// The number of entries in enumerators_ that are in use.
  std::size_t size_ = 0;
  /// The enumerators. Only the first size_ entries are used.
  std::array<enumerator, Elements> enumerators_{};
};

/// \brief This concept is satisfied by a container to which homogeneous_array_rule can append elements.
///
/// The container must have a member type `value_type`, a member function `emplace_back()` which default-constructs a
/// new element at the end of the container and returns a reference to it, and member functions `clear()` and
/// `pop_back()`. std::vector<>, std::deque<>, std::list<>, and arrayvec<> all satisfy this concept.
template <typename Container>
concept emplace_back_container = requires(Container& c) {
  typename Container::value_type;
  { c.emplace_back() } -> std::same_as<typename Container::value_type&>;
  c.clear();
  c.pop_back();
};

/// \brief This concept is satisfied by a container which reports its size and the largest number of elements that
///   it can hold.
///
/// homogeneous_array_rule uses max_size() to avoid adding an element to a full container, such as an arrayvec<>
/// whose capacity has been reached.
template <typename Container>
concept max_size_container = requires(Container const& c) {
  { c.size() } -> std::convertible_to<std::size_t>;
  { c.max_size() } -> std::convertible_to<std::size_t>;
};

/// \brief A rule which parses a JSON array whose elements are all of the same type.
///
/// The container's existing contents are replaced: it is cleared at the start of the array, then an element is
/// appended for each member of the array and \p Rule is pushed to parse its value. If the parse fails, the container
/// holds the elements which were completed before the failure.
/// An array with more members than the container can hold (as given by its max_size() member function, if it has one)
/// is rejected with error::too_many_array_elements, so a fixed-capacity container such as arrayvec<> is never overrun.
/// The rule can be used directly or as one of object_rule's UserRules.
///
/// \tparam Policies  The parser policies.
/// \tparam Container  The container to which an element is appended for each member of the array.
/// \tparam Rule  The rule used to parse each array member. Defaults to the terminal rule for the container's
///   value_type (see type_to_terminal), so it may be omitted for containers of bool, integers, the policy's
///   float_type, std::u8string, std::string, or arrayvec<char8_t, Size>.
/// \tparam MaxElements  The maximum number of elements allowed in the container or 0 for no limit. If not 0,
///   Container must also be a sized range.
template <policies Policies, emplace_back_container Container,
          typename Rule = typename type_to_terminal<typename Container::value_type, Policies>::type,
          std::size_t MaxElements = 0U>
  requires(MaxElements == 0 || std::ranges::sized_range<Container>)
class homogeneous_array_rule final : public rule<Policies> {
public:
  /// The parser policies.
  using policies = Policies;
  /// The type of the value stored by this rule.
  using value_type = Container;
  /// The rules that this rule may push.
  using children = type_list::type_list<Rule>;

  /// \param container  The container to which the array's elements are appended.
  explicit constexpr homogeneous_array_rule(Container* const container) noexcept : container_{container} {}
  /// \returns The name of this rule.
  [[nodiscard]] constexpr char const* name() const noexcept override { return "homogeneous-array"; }
  /// Called at the start of the array. Removes any existing contents of the container.
  /// \param context  The parse context.
  /// \returns Success.
  std::error_code begin_array(context_base<Policies>* const context) override {
    (void)context;
    container_->clear();
    return {};
  }
  /// Called before each member of the array. Appends an element for that member.
  /// \param context  The parse context.
  /// \returns error::too_many_array_elements if the container already holds MaxElements elements or as many as its
  ///   max_size(), peejay::error::nesting_too_deep if
  ///   there is no space for the rule (in which case the element is removed), otherwise success.
  std::error_code element(context_base<Policies>* const context) override {
    if constexpr (MaxElements != 0U) {
      if (container_->size() >= MaxElements) {
        return error::too_many_array_elements;
      }
    }
    if constexpr (max_size_container<Container>) {
      if (container_->size() >= container_->max_size()) {
        return error::too_many_array_elements;
      }
    }
    auto const erc = this->template push<Rule>(this, context, &container_->emplace_back());
    if (erc) {
      container_->pop_back();
    }
    return erc;
  }
  /// Called if the parse ends before the array is complete. Removes the element whose value was being parsed, if any,
  /// so that the container holds only complete elements.
  /// \param child_active  True if the rule parsing the last element was still on the stack.
  void abandon(bool const child_active) noexcept override {
    if (child_active) {
      container_->pop_back();
    }
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

#endif  // PEEJAY_RULES_NON_TERMINAL_RULE_HPP
