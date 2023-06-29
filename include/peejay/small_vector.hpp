//===- include/peejay/small_vector.hpp --------------------*- mode: C++ -*-===//
//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
/// \file small_vector.hpp
/// \brief Provides a small, normally stack allocated, buffer but which can be
///   resized dynamically when necessary.

#ifndef PEEJAY_SMALL_VECTOR_HPP
#define PEEJAY_SMALL_VECTOR_HPP

#include <array>
#include <cstddef>
#include <initializer_list>
#include <new>
#include <variant>
#include <vector>

#include "peejay/arrayvec.hpp"

namespace peejay {

/// A class which provides a vector-like interface to a small, normally stack
/// allocated, buffer which may, if necessary, be resized. It is normally used
/// to contain string buffers where they are typically small enough to be
/// stack-allocated, but where the code must gracefully suport arbitrary
/// lengths.
template <typename ElementType,
          std::size_t BodyElements = 256 / sizeof (ElementType)>
class small_vector {
public:
  using value_type = ElementType;

  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using iterator = pointer_based_iterator<value_type>;
  using const_iterator = pointer_based_iterator<value_type const>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// Constructs the buffer with an initial size of 0.
  small_vector () noexcept = default;
  /// Constructs the buffer with the given initial number of elements.
  explicit small_vector (std::size_t required_elements);
  /// Constructs the buffer with a given initial collection of values.
  small_vector (std::initializer_list<ElementType> init);
  small_vector (small_vector const &rhs) = default;
  small_vector (small_vector &&other) noexcept = default;

  ~small_vector () noexcept = default;

  small_vector &operator= (small_vector const &other) = default;
  small_vector &operator= (small_vector &&other) noexcept = default;

  /// \name Element access
  ///@{
  const_pointer data () const noexcept {
    assert (!arr_.valueless_by_exception ());
    return std::visit ([] (auto const &a) { return std::data (a); }, arr_);
  }
  pointer data () noexcept {
    assert (!arr_.valueless_by_exception ());
    return std::visit ([] (auto &a) { return std::data (a); }, arr_);
  }

  const_reference operator[] (std::size_t n) const noexcept {
    assert (!arr_.valueless_by_exception ());
    return std::visit (
        [n] (auto const &a) -> const_reference {
          using asize = typename std::decay_t<decltype (a)>::size_type;
          return a[static_cast<asize> (n)];
        },
        arr_);
  }
  reference operator[] (std::size_t n) noexcept {
    assert (!arr_.valueless_by_exception ());
    return std::visit (
        [n] (auto &a) -> reference {
          using asize = typename std::decay_t<decltype (a)>::size_type;
          return a[static_cast<asize> (n)];
        },
        arr_);
  }

  const_reference back () const {
    assert (!arr_.valueless_by_exception ());
    return std::visit (
        [] (auto const &a) -> const_reference { return a.back (); }, arr_);
  }
  reference back () {
    assert (!arr_.valueless_by_exception ());
    return std::visit ([] (auto &a) -> reference { return a.back (); }, arr_);
  }

  ///@}

  /// \name Capacity
  ///@{
  /// Returns the number of elements.
  std::size_t size () const noexcept {
    assert (!arr_.valueless_by_exception ());
    return std::visit (
        [] (auto const &a) { return static_cast<std::size_t> (std::size (a)); },
        arr_);
  }
  std::size_t size_bytes () const noexcept {
    return this->size () * sizeof (ElementType);
  }

  /// Checks whether the container is empty.
  bool empty () const noexcept { return size () == 0U; }

  /// Returns the number of elements that can be held in currently allocated
  /// storage.
  std::size_t capacity () const noexcept {
    size_t large_cap = 0;
    if (auto const *const large_arr = std::get_if<large_type> (&arr_)) {
      large_cap = large_arr->capacity ();
    }
    return std::max (BodyElements, large_cap);
  }

  /// The number of elements stored within the body of the object.
  static constexpr std::size_t body_elements () noexcept {
    return BodyElements;
  }

  /// Increase the capacity of the vector to a value that's greater or equal to
  /// new_cap. If new_cap is greater than the current capacity(), new storage is
  /// allocated, otherwise the method does nothing. reserve() does not change
  /// the size of the vector.
  ///
  /// \note If new_cap is greater than capacity(), all iterators, including the past-the-end
  /// iterator, and all references to the elements are invalidated. Otherwise,
  /// no iterators or references are invalidated.
  ///
  /// \param new_cap The new capacity of the vector
  void reserve (std::size_t new_cap);

  /// Resizes the container so that it is large enough for accommodate the
  /// given number of elements.
  ///
  /// \note Calling this function invalidates the contents of the buffer and
  ///   any iterators.
  ///
  /// \param count  The number of elements that the buffer is to accommodate.
  void resize (std::size_t count);
  /// Resizes the container so that it is large enough for accommodate the
  /// given number of elements.
  ///
  /// \note Calling this function invalidates the contents of the buffer and
  ///   any iterators.
  ///
  /// \param count  The number of elements that the buffer is to accommodate.
  /// \param value  The value with which to initialize the new elements.
  void resize (size_type count, const_reference value);
  ///@}

  /// \name Iterators
  ///@{
  /// Returns an iterator to the beginning of the container.
  constexpr const_iterator begin () const noexcept {
    return const_iterator{data ()};
  }
  constexpr iterator begin () noexcept { return iterator{data ()}; }
  constexpr const_iterator cbegin () noexcept {
    return const_iterator{data ()};
  }
  /// Returns a reverse iterator to the first element of the reversed
  /// container. It corresponds to the last element of the non-reversed
  /// container.
  constexpr reverse_iterator rbegin () noexcept {
    return reverse_iterator{this->end ()};
  }
  constexpr const_reverse_iterator rbegin () const noexcept {
    return const_reverse_iterator{this->end ()};
  }
  constexpr const_reverse_iterator rcbegin () noexcept {
    return const_reverse_iterator{this->cend ()};
  }

  /// Returns an iterator to the end of the container.
  constexpr const_iterator end () const noexcept {
    return const_iterator{data () + size ()};
  }
  constexpr iterator end () noexcept { return iterator{data () + size ()}; }
  constexpr const_iterator cend () noexcept {
    return const_iterator{data () + size ()};
  }
  constexpr reverse_iterator rend () noexcept {
    return reverse_iterator{this->begin ()};
  }
  constexpr const_reverse_iterator rend () const noexcept {
    return const_reverse_iterator{this->begin ()};
  }
  constexpr const_reverse_iterator rcend () noexcept {
    return const_reverse_iterator{this->cbegin ()};
  }
  ///@}

  /// \name Modifiers
  ///@{

  /// Removes all elements from the container.
  /// Invalidates any references, pointers, or iterators referring to contained
  /// elements. Any past-the-end iterators are also invalidated.
  void clear () noexcept {
    assert (!arr_.valueless_by_exception ());
    std::visit ([] (auto &a) { a.clear (); }, arr_);
  }

  /// Erases the specified element from the container. Invalidates iterators
  /// and references at or after the point of the erase, including the end()
  /// iterator.
  ///
  /// \p pos Iterator to the element to remove.
  /// \returns Iterator following the last removed element. If \p pos refers
  ///   to the last element, then the end() iterator is returned.
  iterator erase (const_iterator pos);
  /// Erases the elements in the range [\p first, \p last). Invalidates
  /// iterators and references at or after the point of the erase, including
  /// the end() iterator.
  ///
  /// \p first  The first of the range of elements to remove.
  /// \p last  The last of the range of elements to remove.
  /// \returns Iterator following the last removed element. If last == end()
  ///   prior to removal, then the updated end() iterator is returned. If
  ///   [\p first, \p last) is an empty range, then last is returned.
  iterator erase (const_iterator first, const_iterator last);

  /// Adds an element to the end.
  void push_back (ElementType const &v);
  template <typename... Args>
  void emplace_back (Args &&...args);

  template <typename InputIt>
  void assign (InputIt first, InputIt last);

  void assign (std::initializer_list<ElementType> ilist) {
    this->assign (std::begin (ilist), std::end (ilist));
  }

  /// Add the specified range to the end of the small vector.
  template <typename InputIt>
  void append (InputIt first, InputIt last);
  void append (std::initializer_list<ElementType> ilist) {
    this->append (std::begin (ilist), std::end (ilist));
  }

  void pop_back () {
    assert (!arr_.valueless_by_exception ());
    std::visit ([] (auto &v) { v.pop_back (); }, arr_);
  }
  ///@}

private:
  /// A "small" in-object buffer that is used for relatively small
  /// allocations.
  using small_type = arrayvec<ElementType, BodyElements>;
  /// A (potentially) large buffer that is used to satify requests for
  /// buffer element counts that are too large for type 'small'.
  using large_type = std::vector<ElementType>;
  std::variant<small_type, large_type> arr_;

  large_type &to_large ();

  template <typename... Args>
  void emplace_back_small_to_large (small_type const &s, Args &&...args);
};

// (ctor)
// ~~~~~~
template <typename ElementType, std::size_t BodyElements>
small_vector<ElementType, BodyElements>::small_vector (
    std::size_t const required_elements) {
  if (required_elements <= BodyElements) {
    arr_.template emplace<small_type> (
        static_cast<typename small_type::size_type> (required_elements));
  } else {
    arr_.template emplace<large_type> (required_elements);
  }
}

template <typename ElementType, std::size_t BodyElements>
small_vector<ElementType, BodyElements>::small_vector (
    std::initializer_list<ElementType> init)
    : small_vector () {
  if (init.size () <= BodyElements) {
    arr_.template emplace<small_type> (init);
  } else {
    arr_.template emplace<large_type> (init);
  }
}

template <typename ElementType, std::size_t BodyElements>
auto small_vector<ElementType, BodyElements>::to_large () -> large_type & {
  assert (std::holds_alternative<small_type> (arr_));
  if (auto const *const sm = std::get_if<small_type> (&arr_)) {
    // Switch from small to large.
    std::vector<ElementType> vec{std::begin (*sm), std::end (*sm)};
    arr_.template emplace<large_type> (std::move (vec));
  }
  return std::get<large_type> (arr_);
}

// reserve
// ~~~~~~~
template <typename ElementType, std::size_t BodyElements>
void small_vector<ElementType, BodyElements>::reserve (
    std::size_t const new_cap) {
  if (auto const *const sm = std::get_if<small_type> (&arr_)) {
    if (sm->capacity () < new_cap) {
      to_large ();
    }
  }
  if (auto *const vec = std::get_if<large_type> (&arr_)) {
    std::get<large_type> (arr_).reserve (new_cap);
  }
}

// resize
// ~~~~~~
template <typename ElementType, std::size_t BodyElements>
void small_vector<ElementType, BodyElements>::resize (size_type count) {
  this->resize (count, ElementType{});
}

template <typename ElementType, std::size_t BodyElements>
void small_vector<ElementType, BodyElements>::resize (size_type count,
                                                      const_reference value) {
  assert (!arr_.valueless_by_exception ());
  if (auto *const vec = std::get_if<large_type> (&arr_)) {
    vec->resize (count, value);
    return;
  }
  if (count <= BodyElements) {
    // Resize entirely within the small buffer.
    std::get<small_type> (arr_).resize (
        static_cast<typename small_type::size_type> (count), value);
    return;
  }
  to_large ().resize (count, value);
}

// push back
// ~~~~~~~~~
template <typename ElementType, std::size_t BodyElements>
inline void small_vector<ElementType, BodyElements>::push_back (
    ElementType const &v) {
  if (auto *const vec = std::get_if<large_type> (&arr_)) {
    return vec->push_back (v);
  }

  auto &arr = std::get<small_type> (arr_);
  auto const new_elements = arr.size () + 1U;
  if (new_elements <= BodyElements) {
    arr.push_back (v);
  } else {
    // Switch from small to large.
    std::vector<ElementType> vec{std::begin (arr), std::end (arr)};
    vec.push_back (v);
    arr_.template emplace<large_type> (std::move (vec));
  }
}

// emplace back
// ~~~~~~~~~~~~
template <typename ElementType, std::size_t BodyElements>
template <typename... Args>
inline void small_vector<ElementType, BodyElements>::emplace_back (
    Args &&...args) {
  if (auto *const vec = std::get_if<large_type> (&arr_)) {
    vec->emplace_back (std::forward<Args> (args)...);
    return;
  }

  auto &arr = std::get<small_type> (arr_);
  if (arr.size () < BodyElements) {
    arr.emplace_back (std::forward<Args> (args)...);
  } else {
    emplace_back_small_to_large (arr, std::forward<Args> (args)...);
  }
}

// emplace back small to large
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The "slow" path for emplace_back which inserts into the "large" vector.
template <typename ElementType, std::size_t BodyElements>
template <typename... Args>
void small_vector<ElementType, BodyElements>::emplace_back_small_to_large (
    small_type const &s, Args &&...args) {
  std::vector<ElementType> vec{std::begin (s), std::end (s)};
  vec.emplace_back (std::forward<Args> (args)...);
  arr_.template emplace<large_type> (std::move (vec));
}

// assign
// ~~~~~~
template <typename ElementType, std::size_t BodyElements>
template <typename InputIt>
void small_vector<ElementType, BodyElements>::assign (InputIt first,
                                                      InputIt last) {
  this->clear ();
  this->append (first, last);
}

// append
// ~~~~~~
template <typename ElementType, std::size_t BodyElements>
template <typename Iterator>
void small_vector<ElementType, BodyElements>::append (Iterator first,
                                                      Iterator last) {
  for (; first != last; ++first) {
    this->push_back (*first);
  }
}

// erase
// ~~~~~
template <typename ElementType, std::size_t BodyElements>
auto small_vector<ElementType, BodyElements>::erase (const_iterator pos)
    -> iterator {
  assert (!arr_.valueless_by_exception ());
  return std::visit (
      [this, pos] (auto &v) {
        // Convert 'pos' to an iterator in v.
        auto const vpos = v.begin () + (to_address (pos) - data ());
        // Do the erase itself.
        auto const it = v.erase (vpos);
        // convert the result into an iterator in this.
        return iterator{data () + (it - v.begin ())};
      },
      arr_);
}

template <typename ElementType, std::size_t BodyElements>
auto small_vector<ElementType, BodyElements>::erase (const_iterator first,
                                                     const_iterator last)
    -> iterator {
  assert (!arr_.valueless_by_exception ());
  return std::visit (
      [this, first, last] (auto &v) {
        auto b = v.begin ();
        auto *const d = data ();
        auto const vfirst = b + (to_address (first) - d);
        auto const vlast = b + (to_address (last) - d);

        auto const it = v.erase (vfirst, vlast);
        // convert the result into an iterator in this.
        return iterator{data () + (it - v.begin ())};
      },
      arr_);
}

template <typename ElementType, std::size_t LhsBodyElements,
          std::size_t RhsBodyElements>
bool operator== (small_vector<ElementType, LhsBodyElements> const &lhs,
                 small_vector<ElementType, RhsBodyElements> const &rhs) {
  return std::equal (std::begin (lhs), std::end (lhs), std::begin (rhs),
                     std::end (rhs));
}
template <typename ElementType, std::size_t LhsBodyElements,
          std::size_t RhsBodyElements>
bool operator!= (small_vector<ElementType, LhsBodyElements> const &lhs,
                 small_vector<ElementType, RhsBodyElements> const &rhs) {
  return !operator== (lhs, rhs);
}

}  // end namespace peejay

#endif  // PEEJAY_SMALL_VECTOR_HPP
