//===- include/peejay/uri/rule.hpp ------------------------*- mode: C++ -*-===//
//*             _       *
//*  _ __ _   _| | ___  *
//* | '__| | | | |/ _ \ *
//* | |  | |_| | |  __/ *
//* |_|   \__,_|_|\___| *
//*                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file rule.hpp
/// \brief A class to aid in the implementation of ABNF grammars.
///
/// The rule class is intended to enable the (reasonably) straightforward
/// conversion of ABNF grammars to C++ code. Translation is usually
/// straightforward.
///
/// ## Concatenation
///
/// A definition such as: `C = A B` would be translated as:
///
/// ~~~cpp
/// auto C (rule const & r) {
///   return r.concat(A).concat(B).matched("C", r);
/// }
/// ~~~
///
/// ## Alternative
///
/// A definition such as: `C = A / B` would be translated as:
///
/// ~~~cpp
/// auto C (rule const & r) {
///   return r.alternative(A, B).matched("C", r);
/// }
/// ~~~
///
/// ## Optional Sequence
///
/// An optional sequence such as `B = [A]` can implemented as:
///
/// ~~~cpp
/// auto B (rule const & r) {
///     return r.optional(A).matched("B", r);
/// }
/// ~~~
///
/// ## Repetition
///
/// To indicate repetition of an element, the form `<a>*<b>Rule` is used. The
/// optional `<a>` gives the minimum number of elements to be matched (with a
/// default of 0). Similarly, the optional `<b>` gives the maximum number of
/// elements to be matched with a default of the largest integer. The `nRule`
/// form is equivalent to `<n>*<n>Rule`. The can all be implemented using
/// `star()`:
///
/// For example: `h16 = 1*4HEXDIG` can be implemented as:
///
/// ~~~cpp
/// auto h16 (rule const& r) {
///   return r.star(hexdig, 1, 4).matched("h16", r);
/// }
/// ~~~
///
/// # Gotchas
///
/// There are a couple of gotchas which are important to be aware of:
///
/// 1. `star()` is greedy. It will match as many rules as it can (up to the
/// specified maximum). This greedy matching could cause later rules to fail in
/// cases where matching fewer items might enable them to succeed.
/// 2. When using `alternative()`, the order of evaluation is significant. The
/// function evaluates each of the alternative rules from left to right and
/// stops as soon as one is matched. Care needs to be taken where there is
/// potential ambiguity between alternative rules.

#ifndef PEEJAY_URI_RULE_HPP
#define PEEJAY_URI_RULE_HPP

#include <cctype>
#include <functional>
#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace uri {

class rule {
public:
  using acceptor_container = std::vector<std::tuple<std::function<void(std::string_view)>, std::string_view>>;
  using matched_result = std::optional<std::tuple<std::string_view, acceptor_container>>;

  explicit rule(std::string_view string) : tail_{string} {}
  rule(rule const& rhs) = default;
  rule(rule&& rhs) noexcept = default;
  ~rule() noexcept = default;

  rule& operator=(rule const& rhs) = default;
  rule& operator=(rule&& rhs) noexcept = default;

  [[nodiscard]] bool done() const;

  template <typename MatchFunction, typename AcceptFunction>
    requires std::is_invocable_v<MatchFunction, rule&&> && std::is_invocable_v<AcceptFunction, std::string_view>
  [[nodiscard]] rule concat(MatchFunction match, AcceptFunction accept) const {
    return concat_impl(match, accept, false);
  }

  template <typename MatchFunction>
    requires std::is_invocable_v<MatchFunction, rule&&>
  [[nodiscard]] rule concat(MatchFunction match) const {
    return concat_impl(match, &rule::accept_nop, false);
  }

  template <typename MatchFunction, typename AcceptFunction>
    requires std::is_invocable_v<MatchFunction, rule&&> && std::is_invocable_v<AcceptFunction, std::string_view>
  [[nodiscard]] rule optional(MatchFunction match, AcceptFunction accept) const;

  template <typename MatchFunction>
    requires std::is_invocable_v<MatchFunction, rule&&>
  [[nodiscard]] rule optional(MatchFunction match) const;

  // Variable Repetition:  *Rule
  //
  // The operator "*" preceding an element indicates repetition.  The full form
  // is:
  //
  //   <a>*<b>element
  //
  // where <a> and <b> are optional decimal values, indicating at least <a> and
  // at most <b> occurrences of the element.
  //
  // Default values are 0 and infinity so that *<element> allows any number,
  // including zero; 1*<element> requires at least one; 3*3<element> allows
  // exactly 3 and 1*2<element> allows one or two.
  template <typename MatchFunction>
    requires std::is_invocable_v<MatchFunction, rule&&>
  [[nodiscard]] rule star(MatchFunction match, unsigned min = 0,
                          unsigned max = std::numeric_limits<unsigned>::max()) const;

  [[nodiscard]] static rule alternative() { return {}; }

  template <typename MatchFunction, typename... Rest>
    requires std::is_invocable_v<MatchFunction, rule&&>
  [[nodiscard]] rule alternative(MatchFunction match, Rest&&... rest) const;

  [[nodiscard]] constexpr std::optional<std::string_view> tail() const { return tail_; }

  [[nodiscard]] matched_result matched(char const* name, rule const& in) const;

  template <typename Predicate> [[nodiscard]] constexpr matched_result single_char(Predicate pred) const {
    if (auto const sv = this->tail(); sv && !sv->empty() && pred(sv->front())) {
      return std::make_tuple(sv->substr(0, 1), acceptor_container{});
    }
    return {};
  }
  [[nodiscard]] constexpr matched_result single_char(char const c) const {
    return single_char(
        [c2 = std::tolower(static_cast<int>(c))](char d) { return c2 == std::tolower(static_cast<int>(d)); });
  }

private:
  rule(std::optional<std::string_view> tail, acceptor_container acceptors)
      : tail_{tail}, acceptors_{std::move(acceptors)} {}
  rule() noexcept = default;

  template <typename MatchFunction, typename AcceptFunction>
  rule concat_impl(MatchFunction match, AcceptFunction accept, bool optional) const;

  static acceptor_container join(acceptor_container const& a, acceptor_container const& b) {
    acceptor_container result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
  }

  [[nodiscard]] rule join_rule(matched_result::value_type const& m) const {
    auto const& [head, acc] = m;
    return {tail_->substr(head.length()), join(acceptors_, acc)};
  }

  [[nodiscard]] rule join_rule(rule const& other) const { return {other.tail_, join(acceptors_, other.acceptors_)}; }

  static void accept_nop(std::string_view str) {
    (void)str;
    // do nothing.
  }
  template <typename Function> constexpr bool is_nop(Function f) const noexcept;

  std::optional<std::string_view> tail_;
  acceptor_container acceptors_;
};

// star
// ~~~~
template <typename MatchFunction>
  requires std::is_invocable_v<MatchFunction, rule&&>
rule rule::star(MatchFunction const match, unsigned const min, unsigned const max) const {
  if (!tail_) {
    return *this;
  }
  auto length = std::string_view::size_type{0};
  std::string_view str = *tail_;
  auto acc = acceptors_;
  auto count = 0U;
  for (;;) {
    matched_result const m = match(rule{str});
    if (!m) {
      break;  // No match so no more repetitions.
    }
    ++count;
    if (count > max) {
      break;  // Stop after max repeats.
    }
    // Strip the matched text from the string.
    auto const l = std::get<std::string_view>(*m).length();
    str.remove_prefix(l);
    length += l;
    // Remember the corresponding acceptor functions.
    auto const& a = std::get<acceptor_container>(*m);
    acc.insert(acc.end(), a.begin(), a.end());
  }
  if (count < min) {
    return {};
  }

  return {tail_->substr(length), std::move(acc)};
}

// alternative
// ~~~~~~~~~~~
template <typename MatchFunction, typename... Rest>
  requires std::is_invocable_v<MatchFunction, rule&&>
rule rule::alternative(MatchFunction match, Rest&&... rest) const {
  if (!tail_) {
    // If matching has already failed, then pass that condition down the chain.
    return *this;
  }
  if (matched_result const m = match(rule{*tail_})) {
    return join_rule(*m);
  }
  // This didn't match, so try the next one.
  return this->alternative(std::forward<Rest>(rest)...);
}

// is nop
// ~~~~~~
template <typename Function> constexpr bool rule::is_nop(Function f) const noexcept {
  if constexpr (std::is_pointer_v<Function>) {
    if (f == &rule::accept_nop) {
      return true;
    }
  }
  return false;
}

// optional
// ~~~~~~~~
template <typename MatchFunction, typename AcceptFunction>
  requires std::is_invocable_v<MatchFunction, rule&&> && std::is_invocable_v<AcceptFunction, std::string_view>
rule rule::optional(MatchFunction match, AcceptFunction accept) const {
  if (!tail_) {
    return *this;  // If matching previously failed, yield failure.
  }
  rule res = rule{*tail_}.concat_impl(match, accept, true);
  if (!res.tail_) {
    return *this;  // The rule failed, so carry on as if nothing happened.
  }
  return join_rule(res);
}

template <typename MatchFunction>
  requires std::is_invocable_v<MatchFunction, rule&&>
rule rule::optional(MatchFunction match) const {
  return this->optional(match, &rule::accept_nop);
}

// concat impl
// ~~~~~~~~~~~
template <typename MatchFunction, typename AcceptFunction>
rule rule::concat_impl(MatchFunction match, AcceptFunction accept, bool optional) const {
  if (!tail_) {
    // If matching has already failed, then pass that condition down the chain.
    return *this;
  }
  if (matched_result m = match(rule{*tail_})) {
    if (!is_nop(accept)) {
      std::get<acceptor_container>(*m).emplace_back(accept, std::get<std::string_view>(*m));
    }
    return join_rule(*m);
  }
  if (optional) {
    return *this;
  }
  return {};  // Matching failed: yield nothing or failure.
}

constexpr auto single_char(char const first) {
  return [=](rule const& r) { return r.single_char(first); };
}
inline auto char_range(char const first, char const last) {
  return [f = std::tolower(static_cast<int>(first)), l = std::tolower(static_cast<int>(last))](rule const& r) {
    return r.single_char([=](char const c) {
      auto const cl = std::tolower(static_cast<int>(c));
      return cl >= f && cl <= l;
    });
  };
}

constexpr auto alpha(rule const& r) {
  return r.single_char([](char const c) { return std::isalpha(static_cast<int>(c)); });
}
constexpr auto digit(rule const& r) {
  return r.single_char([](char const c) { return std::isdigit(static_cast<int>(c)); });
}
constexpr auto hexdig(rule const& r) {
  return r.single_char([](char const c) { return std::isxdigit(static_cast<int>(c)); });
}

}  // end namespace uri

#endif  // PEEJAY_URI_RULE_HPP
