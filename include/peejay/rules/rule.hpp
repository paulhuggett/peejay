//===- include/peejay/rules/rule.hpp ----------------------*- mode: C++ -*-===//
//*             _       *
//*  _ __ _   _| | ___  *
//* | '__| | | | |/ _ \ *
//* | |  | |_| | |  __/ *
//* |_|   \__,_|_|\___| *
//*                     *
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
/// \file rule.hpp
/// \brief Declares peejay::rules::rule, the base class for rules which model the productions of a JSON grammar,
///   peejay::rules::context which holds the stack of active rules, and peejay::rules::backend which connects them to
///   the parser.
///
/// A parse using rules is driven by a stack of rule instances held by a peejay::rules::context. The
/// peejay::rules::backend object passes each event from the parser to the rule at the top of that stack. A rule
/// handles the events that it expects by overriding the corresponding virtual member function; it may push another
/// rule to handle a nested value, and it pops itself once its production is complete. Any event that a rule does not
/// expect produces one of the peejay::rules::error values, which stops the parse.
///
/// A parse does not allocate. The stack is a buffer inside the context, and each rule occupies only as many bytes as
/// it needs. Each rule declares the rules that it may push in its member type `children`, and the context uses these
/// to compute the size of the buffer at compile time:
///
/// - If no rule can be reached from itself, the buffer holds exactly the most bytes that can be in use at once, so the
///   stack cannot overflow.
/// - Otherwise the grammar is recursive and the buffer holds `max_stack_depth` of the largest rule. The parser limits
///   the nesting of its input to the same depth.
///
/// A rule's `children` are checked at compile time when it pushes, so the size calculation cannot be silently
/// invalidated by a rule pushing something that it did not declare.

#ifndef PEEJAY_RULES_RULE_HPP
#define PEEJAY_RULES_RULE_HPP

// standard library
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// local includes
#include "peejay/concepts.hpp"
#include "peejay/json.hpp"
#include "peejay/rules/errors.hpp"

namespace peejay::rules {

/// \brief The concept integer_target<T> is satisfied if T is a type to which integer_rule can assign a JSON integer.
///
/// That is any signed or unsigned integer type other than bool and the character types (char, wchar_t, char8_t,
/// char16_t, and char32_t). signed char and unsigned char (and so std::int8_t and std::uint8_t) are accepted.
template <typename T>
concept integer_target = std::integral<T> && !std::is_const_v<T> && !std::is_volatile_v<T> && !std::same_as<T, bool> &&
                         !std::same_as<T, char> && !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> &&
                         !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;

template <policy Policies> class rule;
template <policy Policies> class backend;
template <policy Policies> class bool_rule;
template <policy Policies, integer_target T = typename Policies::integer_type> class integer_rule;
template <policy Policies> class float_rule;
template <policy Policies> class string_rule;

/// A rule which accepts a non-negative integer value no larger than the unsigned equivalent of the policy's
/// integer_type can hold.
///
/// \tparam Policies  The parser policies.
template <policy Policies = default_policies>
using uinteger_rule = integer_rule<Policies, std::make_unsigned_t<typename Policies::integer_type>>;

namespace details {

/// Appends \p T to \p List unless it is already present.
template <type_list::sequence List, typename T>
using add_unique =
    std::conditional_t<type_list::has_type_v<List, T>, List, type_list::concat<List, type_list::type_list<T>>>;

/// Provides an alias template which maps an integer type to the integer_rule which assigns it.
/// \tparam Policies  The parser policies.
template <policy Policies> struct integer_rule_for {
  /// The integer_rule which assigns values of type \p T.
  /// \tparam T  The integer type.
  template <typename T> using type = integer_rule<Policies, T>;
};

}  // end namespace details

/// The integer types to which integer_rule instances are made available to object_rule: the standard signed and
/// unsigned integer types plus the policy's integer_type and its unsigned equivalent if those are not already present.
///
/// \tparam Policies  The parser policies.
template <policy Policies>
using builtin_integers = details::add_unique<
    details::add_unique<type_list::type_list<signed char, short, int, long, long long, unsigned char, unsigned short,
                                             unsigned, unsigned long, unsigned long long>,
                        typename Policies::integer_type>,
    std::make_unsigned_t<typename Policies::integer_type>>;

/// The rules which handle the terminals of the grammar.
///
/// \tparam Policies  The parser policies.
template <policy Policies>
using builtin_rules = type_list::concat<
    type_list::type_list<bool_rule<Policies>, float_rule<Policies>, string_rule<Policies>>,
    type_list::transform<builtin_integers<Policies>, details::integer_rule_for<Policies>::template type>>;

/// \brief The part of a context which does not depend on the rules that it holds: the stack of active rules. The rule
///   at the top of the stack receives the events generated by the parser.
///
/// A rule's event handlers are passed a pointer to this type, so they do not depend on the context in which they are
/// used. Instances are created by deriving from this class: see peejay::rules::context.
///
/// \tparam Policies  The parser policies.
template <policy Policies> class context_base {
  friend class rule<Policies>;
  friend class backend<Policies>;

public:
  /// The parser policies.
  using policies = Policies;

  /// The stack refers to itself, so a context cannot be copied.
  context_base(context_base const&) = delete;
  /// The stack refers to itself, so a context cannot be moved.
  context_base(context_base&&) noexcept = delete;
  /// The stack refers to itself, so a context cannot be assigned.
  /// \returns *this.
  context_base& operator=(context_base const&) = delete;
  /// The stack refers to itself, so a context cannot be moved.
  context_base& operator=(context_base&&) noexcept = delete;

  /// Creates a parser whose events are passed to the rule at the top of this context's stack. The context must
  /// outlive the parser.
  /// \returns The new parser.
  [[nodiscard]] auto make_parser() { return parser<backend<policies>>{backend<policies>{this}}; }

protected:
  /// \param first  The start of the buffer which holds the stack.
  /// \param last  The end of the buffer which holds the stack.
  constexpr context_base(std::byte* const first, std::byte* const last) noexcept : sp_{first}, end_{last} {}
  /// A context is destroyed only by the class which derives from this one.
  ~context_base() noexcept = default;

  /// Creates an instance of type T at the top of the stack.
  ///
  /// \tparam T  The type of the new rule. Rules are discarded without running their destructors, so it must be
  ///   trivially destructible.
  /// \tparam Args  The types of the arguments for the constructor of type T.
  /// \param args  The arguments to be passed to the constructor of the new instance of type T.
  /// \returns peejay::error::nesting_too_deep if there is insufficient space, otherwise success.
  template <typename T, typename... Args>
    requires std::derived_from<T, rule<Policies>> && std::is_trivially_destructible_v<T>
  std::error_code emplace(Args&&... args);

private:
  /// \returns The rule at the top of the stack. The stack must not be empty.
  [[nodiscard]] constexpr rule<policies>* top() const noexcept {
    assert(top_ != nullptr);
    return top_;
  }

  /// The rule at the top of the stack, or nullptr if the stack is empty.
  rule<policies>* top_ = nullptr;
  /// The first unused byte of the stack's buffer.
  std::byte* sp_;
  /// The end of the stack's buffer.
  std::byte* end_;
};

/// Maps a value type to the terminal rule which assigns it. The member `type` is absent if there is no such rule.
/// \tparam T  The type of the value.
/// \tparam Policies  The parser policies.
template <typename T, policy Policies> struct type_to_terminal {};
/// Integers are assigned by integer_rule.
/// \tparam T  The type of the value.
/// \tparam Policies  The parser policies.
template <integer_target T, policy Policies> struct type_to_terminal<T, Policies> {
  /// The terminal rule.
  using type = integer_rule<Policies, T>;
};
/// Floating-point values are assigned by float_rule.
/// \tparam Policies  The parser policies.
template <policy Policies> struct type_to_terminal<typename Policies::float_type, Policies> {
  /// The terminal rule.
  using type = float_rule<Policies>;
};
/// Booleans are assigned by bool_rule.
/// \tparam Policies  The parser policies.
template <policy Policies> struct type_to_terminal<bool, Policies> {
  /// The terminal rule.
  using type = bool_rule<Policies>;
};
/// Strings are assigned by string_rule.
/// \tparam Policies  The parser policies.
template <policy Policies> struct type_to_terminal<std::u8string, Policies> {
  /// The terminal rule.
  using type = string_rule<Policies>;
};

/// \brief The base class for rules which model the productions of a JSON grammar.
///
/// Each member function which handles a parser event returns an error by default. A subclass overrides the functions
/// for the events that it accepts.
///
/// A subclass which pushes other rules must list them in a public member type `children`. The rules listed by a
/// subclass replace those of its base class.
///
/// \tparam Policies  The parser policies.
template <policy Policies> class rule {
  friend class context_base<Policies>;

public:
  /// The parser policies.
  using policies = Policies;
  /// The rules that this rule may push: none, unless a subclass says otherwise.
  using children = type_list::type_list<>;

  /// A rule is held in place on the parse stack, so it cannot be copied.
  rule(rule const&) = delete;
  /// A rule is held in place on the parse stack, so it cannot be assigned.
  /// \returns *this.
  rule& operator=(rule const&) = delete;

  /// \returns The name of this rule, used for logging.
  virtual char const* name() const noexcept = 0;

  /// Called when an integer value has been parsed.
  /// \param context  The parse context.
  /// \param value  The integer value.
  /// \returns error::unexpected_number unless overridden.
  virtual std::error_code integer_value(context_base<policies>* const context, policies::integer_type const value) {
    (void)context;
    (void)value;
    return error::unexpected_number;
  }
  /// Called when a floating-point value has been parsed.
  /// \param context  The parse context.
  /// \param value  The floating-point value.
  /// \returns error::unexpected_number unless overridden.
  virtual std::error_code float_value(context_base<policies>* const context, policies::float_type const value) {
    (void)context;
    (void)value;
    return error::unexpected_number;
  }
  /// Called when a string value has been parsed.
  /// \param context  The parse context.
  /// \param value  The string value.
  /// \returns error::unexpected_string unless overridden.
  virtual std::error_code string_value(context_base<policies>* const context, std::u8string_view const value) {
    (void)context;
    (void)value;
    return error::unexpected_string;
  }
  /// Called when a boolean value has been parsed.
  /// \param context  The parse context.
  /// \param value  The boolean value.
  /// \returns error::unexpected_boolean unless overridden.
  virtual std::error_code boolean_value(context_base<policies>* const context, bool const value) {
    (void)context;
    (void)value;
    return error::unexpected_boolean;
  }
  /// Called when a null value has been parsed.
  /// \param context  The parse context.
  /// \returns error::unexpected_null unless overridden.
  virtual std::error_code null_value(context_base<policies>* const context) {
    (void)context;
    return error::unexpected_null;
  }

  /// Called at the start of an array.
  /// \param context  The parse context.
  /// \returns error::unexpected_array unless overridden.
  virtual std::error_code begin_array(context_base<policies>* const context) {
    (void)context;
    return error::unexpected_array;
  }
  /// Called before each element of an array.
  /// \param context  The parse context.
  /// \returns Success unless overridden.
  virtual std::error_code element(context_base<policies>* const context) {
    (void)context;
    return {};
  }
  /// Called at the end of an array.
  /// \param context  The parse context.
  /// \returns error::unexpected_end_array unless overridden.
  virtual std::error_code end_array(context_base<policies>* const context) {
    (void)context;
    return error::unexpected_end_array;
  }

  /// Called at the start of an object.
  /// \param context  The parse context.
  /// \returns error::unexpected_object unless overridden.
  virtual std::error_code begin_object(context_base<policies>* const context) {
    (void)context;
    return error::unexpected_object;
  }
  /// Called when an object key has been parsed.
  /// \param context  The parse context.
  /// \param key  The key.
  /// \returns error::unexpected_object_key unless overridden.
  virtual std::error_code key(context_base<policies>* const context, std::u8string_view const key) {
    (void)context;
    (void)key;
    return error::unexpected_object_key;
  }
  /// Called at the end of an object.
  /// \param context  The parse context.
  /// \returns error::unexpected_end_object unless overridden.
  virtual std::error_code end_object(context_base<policies>* const context) {
    (void)context;
    return error::unexpected_end_object;
  }

protected:
  /// Rules are created by subclasses.
  rule() noexcept = default;
  /// Rules are not destroyed through this type: they are discarded without running their destructors.
  ~rule() noexcept = default;

  /// Creates an instance of type T and pushes it onto the parse stack. The new instance's constructor is passed
  /// \p args.
  ///
  /// \tparam T  The type of the new rule to instantiate. It must be listed in `Self::children`.
  /// \tparam Self  The type of the rule which is performing the push. It is deduced from \p self.
  /// \tparam Args  The types of the arguments for the constructor of type T.
  /// \param self  The rule which is performing the push (that is, `this`).
  /// \param context  The parse context.
  /// \param args  The parameters to be passed to the constructor of the new instance of type T.
  /// \returns peejay::error::nesting_too_deep if there is insufficient space, otherwise success.
  template <typename T, typename Self, typename... Args>
  static std::error_code push(Self const* const self, context_base<policies>* const context, Args&&... args) {
    static_assert(type_list::has_type_v<typename Self::children, T>, "T must be listed in the children of Self");
    auto const erc = context->template emplace<T>(std::forward<Args>(args)...);
    if (!erc) {
      static_cast<rule const*>(self)->log_top(true);
    }
    return erc;
  }
  /// Pushes a terminal rule which stores the next value in \p value.
  /// \tparam Self  The type of the rule which is performing the push. It is deduced from \p self.
  /// \param self  The rule which is performing the push (that is, `this`).
  /// \param context  The parse context.
  /// \param value  The location to which the value will be written.
  /// \returns peejay::error::nesting_too_deep if there is insufficient space, otherwise success.
  template <typename Self, typename T>
  static std::error_code push_terminal(Self const* const self, context_base<policies>* const context, T* const value) {
    return push<typename type_to_terminal<T, policies>::type>(self, context, value);
  }
  /// Pushes a string_rule which stores the next value in \p value.
  /// \tparam Self  The type of the rule which is performing the push. It is deduced from \p self.
  /// \param self  The rule which is performing the push (that is, `this`).
  /// \param context  The parse context.
  /// \param value  The location to which the value will be written.
  /// \returns peejay::error::nesting_too_deep if there is insufficient space, otherwise success.
  template <typename Self>
  static std::error_code push_terminal(Self const* const self, context_base<policies>* const context,
                                       std::string* const value) {
    return push_terminal(self, context, reinterpret_cast<std::u8string*>(value));
  }

  /// Removes this rule, which must be at the top of the parse stack, and returns "no-error". This member function is
  /// usually called to signal the end of the current grammar rule. The rule must not be used after this call because
  /// its storage may be reused by the next push.
  ///
  /// \param context  The parse context.
  /// \returns Success.
  std::error_code pop(context_base<policies>* const context) noexcept {
    assert(context->top() == this);
    this->log_top(false);
    context->sp_ = frame_;
    context->top_ = previous_;
    return {};
  }

private:
  /// Logs the rule at the top of the parse stack. Logging is currently disabled.
  /// \param is_push  True if the rule has just been pushed, false if it is about to be popped.
  void log_top(bool const is_push) const noexcept { (void)is_push; }

  /// The rule below this one on the stack, or nullptr if this is the bottom of the stack.
  rule* previous_ = nullptr;
  /// The value to which the context's stack pointer is restored when this rule is popped.
  std::byte* frame_ = nullptr;
};

template <policy Policies>
template <typename T, typename... Args>
  requires std::derived_from<T, rule<Policies>> && std::is_trivially_destructible_v<T>
std::error_code context_base<Policies>::emplace(Args&&... args) {
  void* where = sp_;
  if (auto space = static_cast<std::size_t>(end_ - sp_); std::align(alignof(T), sizeof(T), where, space) == nullptr) {
    return peejay::error::nesting_too_deep;
  }
  auto* const instance = ::new (where) T(std::forward<Args>(args)...);
  rule<policies>* const base = instance;
  base->previous_ = top_;
  base->frame_ = sp_;
  top_ = base;
  sp_ = reinterpret_cast<std::byte*>(instance) + sizeof(T);
  return {};
}

/// Implements the backend interface required by the JSON parser. Each member function
/// forwards to the top-most element on the parse stack which will be an instance of a subclass of
/// rule.
///
/// \tparam Policies  The parser policies.
template <policy Policies = default_policies> class backend {
public:
  /// The parser policies.
  using policies = Policies;
  /// \param context  The context whose parse stack receives the parser's events. The stack must not be empty.
  explicit constexpr backend(context_base<policies>* const context) noexcept : context_{context} {}

  /// Called at the end of the parse. The rules write their results to the objects that they were given, so there is
  /// no result.
  static constexpr void result() { /* There is no result! */ }
  /// \param v  The integer value.
  /// \returns The result of the top rule's integer_value().
  std::error_code integer_value(policies::integer_type const v) { return context_->top()->integer_value(context_, v); }
  /// \param v  The floating-point value.
  /// \returns The result of the top rule's float_value().
  std::error_code float_value(policies::float_type const v) { return context_->top()->float_value(context_, v); }
  /// \param v  The string value.
  /// \returns The result of the top rule's string_value().
  std::error_code string_value(std::u8string_view const v) { return context_->top()->string_value(context_, v); }
  /// \param v  The boolean value.
  /// \returns The result of the top rule's boolean_value().
  std::error_code boolean_value(bool const v) { return context_->top()->boolean_value(context_, v); }
  /// \returns The result of the top rule's null_value().
  std::error_code null_value() { return context_->top()->null_value(context_); }

  /// \returns The result of the top rule's begin_array().
  std::error_code begin_array() { return context_->top()->begin_array(context_); }
  /// \returns The result of the top rule's element().
  std::error_code element() { return context_->top()->element(context_); }
  /// \returns The result of the top rule's end_array().
  std::error_code end_array() { return context_->top()->end_array(context_); }

  /// \returns The result of the top rule's begin_object().
  std::error_code begin_object() { return context_->top()->begin_object(context_); }
  /// \param k  The object key.
  /// \returns The result of the top rule's key().
  std::error_code key(std::u8string_view const k) { return context_->top()->key(context_, k); }
  /// \returns The result of the top rule's end_object().
  std::error_code end_object() { return context_->top()->end_object(context_); }

private:
  /// The context whose parse stack receives the parser's events.
  context_base<policies>* context_;
};

/// \brief This concept is satisfied by a user-defined rule which object_rule can push to parse the value of a
///   property.
///
/// The rule must derive from rule<Policies> and have a member type `value_type`.
template <typename Rule, typename Policies>
concept is_rule = std::derived_from<Rule, rule<Policies>> && requires { typename Rule::value_type; };

namespace details {

/// The stack space needed by a rule and the rules that can be reached from it.
struct stack_bound {
  /// True if a rule can be reached from itself.
  bool recursive = false;
  /// The most bytes used by any non-repeating path through the rules.
  std::size_t longest_path = 0;
  /// The most bytes used by a single rule.
  std::size_t largest_frame = 0;

  /// \param lhs  The first of the two bounds to combine.
  /// \param rhs  The second of the two bounds to combine.
  /// \returns A bound which is sufficient for either of two alternatives.
  friend consteval stack_bound operator|(stack_bound const& lhs, stack_bound const& rhs) noexcept {
    return {.recursive = lhs.recursive || rhs.recursive,
            .longest_path = std::max(lhs.longest_path, rhs.longest_path),
            .largest_frame = std::max(lhs.largest_frame, rhs.largest_frame)};
  }
};

/// Computes the stack space needed by a rule by walking the graph of rules formed by their `children` types.
///
/// \tparam Rule  The rule to be examined.
/// \tparam Base  The base class of all of the rules. The stack is always aligned for this type.
/// \tparam Path  The rules that were followed to reach \p Rule.
/// \returns The stack space needed by \p Rule.
template <typename Rule, typename Base, typename... Path> consteval stack_bound analyze_rule() {
  if constexpr ((std::is_same_v<Rule, Path> || ...)) {
    return {.recursive = true};
  } else {
    return []<typename... Children>(std::type_identity<type_list::type_list<Children...>>) {
      // A rule may be preceded by padding if it is more strictly aligned than the base class.
      auto const frame = sizeof(Rule) + alignof(Rule) - alignof(Base);
      auto const children = (analyze_rule<Children, Base, Rule, Path...>() | ... | stack_bound{});
      return stack_bound{.recursive = children.recursive,
                         .longest_path = frame + children.longest_path,
                         .largest_frame = std::max(frame, children.largest_frame)};
    }(std::type_identity<typename Rule::children>{});
  }
}

/// The number of bytes needed for the stack of a context. If no rule can be reached from itself, this is the most
/// bytes that can be in use at once. Otherwise it is the space for max_stack_depth instances of the largest rule.
///
/// \tparam Policies  The parser policies.
/// \tparam Root  The rule which parses the top-level value.
template <policy Policies, typename Root>
inline constexpr std::size_t stack_bytes_v = [] {
  constexpr stack_bound bound = analyze_rule<Root, rule<Policies>>();
  return bound.recursive ? Policies::max_stack_depth * bound.largest_frame : bound.longest_path;
}();

/// The storage for a context's stack. It is a base class of context so that it is constructed before context_base,
/// which is given pointers into it.
///
/// \tparam Size  The number of bytes in the buffer.
/// \tparam Alignment  The alignment of the buffer.
template <std::size_t Size, std::size_t Alignment> struct stack_storage {
  /// The buffer holding the stack.
  alignas(Alignment) std::array<std::byte, Size> buffer;
};

}  // end namespace details

/// The state shared by the rules taking part in a parse: the stack of active rules and the storage that holds it. The
/// stack initially holds an instance of \p Root.
///
/// \tparam Policies  The parser policies.
/// \tparam Root  The rule which parses the top-level value. It and the rules that can be reached through their
///   `children` member types determine the size of the context.
template <policy Policies, is_rule<Policies> Root>
class context final : private details::stack_storage<details::stack_bytes_v<Policies, Root>, alignof(rule<Policies>)>,
                      public context_base<Policies> {
public:
  /// The number of bytes in the buffer which holds the stack.
  static constexpr std::size_t stack_bytes = details::stack_bytes_v<Policies, Root>;

  /// Constructs a context whose stack holds a single instance of \p Root.
  ///
  /// A context cannot be copied or moved, so this constructor does not accept a single argument of type context
  /// (however qualified): such an argument selects the deleted copy constructor instead.
  ///
  /// \tparam Args  The types of the arguments for the constructor of \p Root.
  /// \param args  The arguments to be passed to the constructor of \p Root.
  template <typename... Args>
    requires(!(sizeof...(Args) == 1 && (std::same_as<std::remove_cvref_t<Args>, context> && ...)) &&
             std::constructible_from<Root, Args...>)
  explicit context(Args&&... args) : context_base<Policies>{this->buffer.data(), this->buffer.data() + stack_bytes} {
    [[maybe_unused]] auto const erc = this->template emplace<Root>(std::forward<Args>(args)...);
    assert(!erc && "the stack always has space for the root rule");
  }
};

}  // end namespace peejay::rules

#endif  // PEEJAY_RULES_RULE_HPP
