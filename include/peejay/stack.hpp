//===- include/peejay/stack.hpp ---------------------------*- mode: C++ -*-===//
//*      _             _     *
//*  ___| |_ __ _  ___| | __ *
//* / __| __/ _` |/ __| |/ / *
//* \__ \ || (_| | (__|   <  *
//* |___/\__\__,_|\___|_|\_\ *
//*                          *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_STACK_HPP
#define PEEJAY_STACK_HPP

#include <deque>
#include <iterator>
#include <memory>
#include <type_traits>

#include "peejay/portab.hpp"

namespace peejay {

template <typename T, typename Container = std::deque<T>>
class stack;

template <typename T, typename Container>
bool operator== (stack<T, Container> const& lhs,
                 stack<T, Container> const& rhs);

template <typename T, typename Container>
bool operator< (stack<T, Container> const& lhs, stack<T, Container> const& rhs);

template <typename T, typename Container /*= deque<T>*/>
class stack {
  template <typename T1, typename C1>
  friend bool operator== (stack<T1, C1> const& x, stack<T1, C1> const& y);

  template <typename T1, typename C1>
  friend bool operator< (stack<T1, C1> const& x, stack<T1, C1> const& y);

public:
  using container_type = Container;
  using value_type = typename container_type::value_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using size_type = typename container_type::size_type;

  using const_iterator = typename container_type::const_iterator;
  using iterator = typename container_type::iterator;
  using reverse_iterator = typename container_type::reverse_iterator;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;

  static_assert (std::is_same_v<T, value_type>);

  stack () noexcept (
      std::is_nothrow_default_constructible_v<container_type>) = default;
  stack (stack const& other) = default;
  stack (stack&& other) noexcept (
      std::is_nothrow_move_constructible_v<container_type>) = default;
  explicit stack (container_type const& c) : c_{c} {}
  explicit stack (container_type&& c) : c_{std::move (c)} {}

  template <typename Allocator>
  explicit stack (
      Allocator const& a,
      std::enable_if_t<std::uses_allocator_v<container_type, Allocator>>* =
          nullptr)
      : c_ (a) {}

  template <typename Allocator>
  stack (
      stack const& s, Allocator const& a,
      std::enable_if_t<std::uses_allocator_v<container_type, Allocator>>* =
          nullptr)
      : c_ (s.c_, a) {}
  template <typename Allocator>
  stack (
      stack&& s, Allocator const& a,
      std::enable_if_t<std::uses_allocator_v<container_type, Allocator>>* =
          nullptr)
      : c_ (std::move (s.c_), a) {}

  template <typename Allocator>
  stack (
      container_type const& c, Allocator const& a,
      std::enable_if_t<std::uses_allocator_v<container_type, Allocator>>* =
          nullptr)
      : c_ (c, a) {}
  template <typename Allocator>
  stack (
      container_type&& c, Allocator const& a,
      std::enable_if_t<std::uses_allocator_v<container_type, Allocator>>* =
          nullptr)
      : c_ (std::move (c), a) {}

#if PEEJAY_CXX20
  template <typename InputIterator>
  PEEJAY_CXX20REQUIRES (std::input_iterator<InputIterator>)
  stack (InputIterator first, InputIterator last) : c_ (first, last) {}

  template <typename InputIterator, typename Allocator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator> &&
                         std::uses_allocator_v<container_type, Allocator>))
  stack (InputIterator first, InputIterator last, Allocator const& alloc)
      : c_ (first, last, alloc) {}
#endif  // PEEJAY_CXX20

  ~stack () noexcept (std::is_nothrow_destructible_v<container_type>) = default;

  stack& operator= (stack const& other) {
    if (this != &other) {
      c_ = other.c_;
    }
    return *this;
  }
  stack& operator= (stack&& other) noexcept (
      std::is_nothrow_move_assignable_v<container_type>) {
    c_ = std::move (other.c_);
    return *this;
  }

  [[nodiscard]] bool empty () const { return c_.empty (); }
  size_type size () const { return c_.size (); }
  reference top () { return c_.back (); }
  const_reference top () const { return c_.back (); }

  void push (value_type const& v) { c_.push_back (v); }
  void push (value_type&& v) { c_.push_back (std::move (v)); }

  template <typename... Args>
  decltype (auto) emplace (Args&&... args) {
    return c_.emplace_back (std::forward<Args> (args)...);
  }

  void pop () { c_.pop_back (); }

  /// \name Iterators
  ///@{
  /// Returns an iterator to the beginning of the container.
  constexpr const_iterator begin () const noexcept { return c_.begin (); }
  constexpr iterator begin () noexcept { return c_.begin (); }
  const_iterator cbegin () noexcept { return c_.cbegin (); }
  /// Returns a reverse iterator to the first element of the reversed
  /// container. It corresponds to the last element of the non-reversed
  /// container.
  reverse_iterator rbegin () noexcept { return c_.rbegin (); }
  const_reverse_iterator rbegin () const noexcept { return c_.rbegin (); }
  const_reverse_iterator rcbegin () noexcept { return c_.rcbegin (); }

  /// Returns an iterator to the end of the container.
  const_iterator end () const noexcept { return c_.end (); }
  iterator end () noexcept { return c_.end (); }
  const_iterator cend () noexcept { return c_.end (); }
  reverse_iterator rend () noexcept { return c_.rend (); }
  const_reverse_iterator rend () const noexcept { return c_.rend (); }
  const_reverse_iterator rcend () noexcept { return c_.rcend (); }
  ///@}

  void swap (stack& s) noexcept (std::is_nothrow_swappable_v<container_type>) {
    std::swap (c_, s.c_);
  }

private:
  container_type c_;
};

template <typename Container>
stack (Container) -> stack<typename Container::value_type, Container>;

template <typename Container, typename Allocator,
          typename = std::enable_if_t<
              std::uses_allocator_v<Container, Allocator>>>
stack (Container, Allocator)
    -> stack<typename Container::value_type, Container>;

template <typename InputIterator>
PEEJAY_CXX20REQUIRES (std::input_iterator<InputIterator>)
stack (InputIterator, InputIterator)
    -> stack<typename std::iterator_traits<InputIterator>::value_type>;

template <typename InputIterator, typename Allocator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
stack (InputIterator, InputIterator, Allocator) -> stack<
    typename std::iterator_traits<InputIterator>::value_type,
    std::deque<typename std::iterator_traits<InputIterator>::value_type,
               Allocator>>;

template <typename T, typename Container>
inline bool operator== (stack<T, Container> const& lhs,
                        stack<T, Container> const& rhs) {
  return lhs.c_ == rhs.c_;
}
template <typename T, typename Container>
inline bool operator< (stack<T, Container> const& lhs,
                       stack<T, Container> const& rhs) {
  return lhs.c_ < rhs.c_;
}
template <typename T, typename Container>
inline bool operator!= (stack<T, Container> const& lhs,
                        stack<T, Container> const& rhs) {
  return !(lhs == rhs);
}
template <typename T, typename Container>
inline bool operator> (stack<T, Container> const& lhs,
                       stack<T, Container> const& rhs) {
  return rhs < lhs;
}
template <typename T, typename Container>
inline bool operator>= (stack<T, Container> const& lhs,
                        stack<T, Container> const& rhs) {
  return !(lhs < rhs);
}
template <typename T, typename Container>
inline bool operator<= (stack<T, Container> const& lhs,
                        stack<T, Container> const& rhs) {
  return !(rhs < lhs);
}

template <typename T, typename Container>
inline std::enable_if_t<std::is_swappable_v<Container>, void>
swap (stack<T, Container>& x, stack<T, Container>& y) noexcept (
    std::is_nothrow_swappable_v<stack<T, Container>>) {
  x.swap (y);
}

}  // end namespace peejay

template <typename T, typename Container, typename Allocator>
struct std::uses_allocator<peejay::stack<T, Container>, Allocator>
    : public std::uses_allocator<Container, Allocator> {};

#endif  // PEEJAY_STACK_HPP
