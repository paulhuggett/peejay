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
#include <stack>
#include <type_traits>

#include "peejay/portab.hpp"

namespace peejay {

/// The stack class is a clone of std::stack but adds the capability of using
/// iterators to examine the contents of the underlying container, in addition
/// to the strictly stack-based members.
template <typename T, typename Container = std::deque<T>>
class stack : public std::stack<T, Container> {
public:
  using const_iterator = typename Container::const_iterator;
  using iterator = typename Container::iterator;
  using reverse_iterator = typename Container::reverse_iterator;
  using const_reverse_iterator = typename Container::const_reverse_iterator;

  using std::stack<T, Container>::stack;

  /// \name Iterators
  ///@{

  /// Returns an stack::iterator to the beginning of the container.
  constexpr const_iterator begin () const noexcept { return this->c.begin (); }
  /// Returns an stack::iterator to the beginning of the container.
  constexpr iterator begin () noexcept { return this->c.begin (); }
  /// Returns a stack::const_iterator to the beginning of the container.
  const_iterator cbegin () noexcept { return this->c.cbegin (); }
  /// Returns a reverse iterator to the first element of the reversed
  /// container. It corresponds to the last element of the non-reversed
  /// container.
  reverse_iterator rbegin () noexcept { return this->c.rbegin (); }
  const_reverse_iterator rbegin () const noexcept { return this->c.rbegin (); }
  const_reverse_iterator rcbegin () noexcept { return this->c.rcbegin (); }

  /// Returns an iterator to the end of the container.
  const_iterator end () const noexcept { return this->c.end (); }
  iterator end () noexcept { return this->c.end (); }
  const_iterator cend () noexcept { return this->c.end (); }
  reverse_iterator rend () noexcept { return this->c.rend (); }
  const_reverse_iterator rend () const noexcept { return this->c.rend (); }
  const_reverse_iterator rcend () noexcept { return this->c.rcend (); }
  ///@}
};

template <typename Container>
stack (Container) -> stack<typename Container::value_type, Container>;

}  // end namespace peejay

template <typename T, typename Container, typename Allocator>
struct std::uses_allocator<peejay::stack<T, Container>, Allocator>
    : public std::uses_allocator<Container, Allocator> {};

#endif  // PEEJAY_STACK_HPP
