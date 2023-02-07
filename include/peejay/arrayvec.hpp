//===- include/peejay/arrayvec.hpp ------------------------*- mode: C++ -*-===//
//*                                             *
//*   __ _ _ __ _ __ __ _ _   ___   _____  ___  *
//*  / _` | '__| '__/ _` | | | \ \ / / _ \/ __| *
//* | (_| | |  | | | (_| | |_| |\ V /  __/ (__  *
//*  \__,_|_|  |_|  \__,_|\__, | \_/ \___|\___| *
//*                       |___/                 *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_ARRAYVEC_HPP
#define PEEJAY_ARRAYVEC_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include "peejay/portab.hpp"

#ifdef __has_include
#if __has_include(<bit>)
#include <bit>
#endif
#endif

namespace peejay {

#if PEEJAY_HAVE_CONCEPTS
template <typename To, typename From>
  requires (std::is_trivial_v<To> && std::is_trivial_v<From>)
#else
template <typename To, typename From,
          typename = typename std::enable_if_t<std::is_trivial_v<To> &&
                                               std::is_trivial_v<From>>>
#endif
constexpr To pointer_cast (From p) noexcept {
#if __cpp_lib_bit_cast
  return std::bit_cast<To> (p);
#else
  return reinterpret_cast<To> (p);
#endif
}

#if PEEJAY_CXX20
using std::construct_at;
#else
/// Creates a T object initialized with arguments args... at given address p.
template <typename T, typename... Args>
constexpr T *construct_at (T *p, Args &&...args) {
  return ::new (p) T (std::forward<Args> (args)...);
}
#endif

/// Provides a member typedef inherit_const::type, which is defined as \p R
/// const if \p T is a const type and \p R if \p T is non-const.
///
/// \tparam T  A type whose constness will determine the constness of
///   inherit_const::type.
/// \tparam R  The result type.
/// \tparam RC  The const-result type.
template <typename T, typename R, typename RC = R const>
struct inherit_const {
  /// If \p T is const, \p R const otherwise \p R.
  using type =
      typename std::conditional_t<std::is_const_v<std::remove_reference_t<T>>,
                                  RC, R>;
};

template <typename T, typename R, typename RC = R const>
using inherit_const_t = typename inherit_const<T, R, RC>::type;

static_assert (std::is_same_v<inherit_const_t<int, bool>, bool>, "int -> bool");
static_assert (std::is_same_v<inherit_const_t<int const, bool>, bool const>,
               "int const -> bool const");
static_assert (std::is_same_v<inherit_const_t<int &, bool>, bool>,
               "int& -> bool");
static_assert (std::is_same_v<inherit_const_t<int const &, bool>, bool const>,
               "int const & -> bool const");
static_assert (std::is_same_v<inherit_const_t<int &&, bool>, bool>,
               "int && -> bool");
static_assert (std::is_same_v<inherit_const_t<int const &&, bool>, bool const>,
               "int const && -> bool const");

template <typename T, size_t Size = 256>
class arrayvec {
public:
  using value_type = T;

  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

  using iterator = value_type *;
  using const_iterator = value_type const *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// Constructs the buffer with an initial size of 0.
  arrayvec () noexcept = default;
  arrayvec (std::initializer_list<T> init)
      : arrayvec (std::begin (init), std::end (init)) {}
  template <typename ForwardIterator>
  PEEJAY_CXX20REQUIRES ((std::forward_iterator<ForwardIterator>))
  arrayvec (ForwardIterator first, ForwardIterator last);
  arrayvec (size_type count, T const &value);

  arrayvec (arrayvec const &other)
      : arrayvec (std::begin (other), std::end (other)) {}
  arrayvec (arrayvec &&other) noexcept (
      std::is_nothrow_move_constructible_v<T>);

  arrayvec &operator= (arrayvec const &other) {
    if (&other != this) {
      operator_assign<false> (other);
    }
    return *this;
  }
  arrayvec &operator= (arrayvec &&other) noexcept {
    if (&other != this) {
      operator_assign<true> (other);
    }
    return *this;
  }

  ~arrayvec () noexcept { clear (); }

  /// \name Element access
  ///@{
  constexpr T const *data () const noexcept { return &element (*this, 0); }
  constexpr T *data () noexcept { return &element (*this, 0); }

  constexpr T const &operator[] (size_t n) const noexcept {
    return element (*this, n);
  }
  constexpr T &operator[] (size_t n) noexcept { return element (*this, n); }

  constexpr T &front () noexcept { return element (*this, 0); }
  constexpr T const &front () const noexcept { return element (*this, 0); }
  constexpr T &back () noexcept { return element (*this, size_ - 1U); }
  constexpr T const &back () const noexcept {
    return element (*this, size_ - 1U);
  }

  ///@}

  /// \name Capacity
  ///@{
  /// Returns the number of elements.
  [[nodiscard]] constexpr size_t size () const noexcept { return size_; }

  /// Checks whether the container is empty.
  [[nodiscard]] constexpr bool empty () const noexcept { return size_ == 0U; }

  /// Returns the number of elements that can be held.
  [[nodiscard]] constexpr size_t capacity () const noexcept { return Size; }

  /// Returns the maximum number of elements the container is able to hold.
  [[nodiscard]] constexpr size_type max_size () const noexcept { return Size; }

  /// Resizes the container to contain \p count elements. If the current size is
  /// less than \p count, the container is reduced to its first \p count
  /// elements. If the current size is less than \p count, additional
  /// default-inserted elements are appended.
  ///
  /// \param count  The new number of elements in the container. Must be less
  ///               than or equal to Size.
  void resize (size_type count);

  ///@}

  /// \name Iterators
  ///@{
  /// Returns an iterator to the beginning of the container.
  [[nodiscard]] constexpr const_iterator begin () const noexcept {
    return &element (*this, 0);
  }
  [[nodiscard]] constexpr iterator begin () noexcept {
    return &element (*this, 0);
  }
  const_iterator cbegin () noexcept { return &element (*this, 0); }
  /// Returns a reverse iterator to the first element of the reversed
  /// container. It corresponds to the last element of the non-reversed
  /// container.
  reverse_iterator rbegin () noexcept { return reverse_iterator{this->end ()}; }
  const_reverse_iterator rbegin () const noexcept {
    return const_reverse_iterator{this->end ()};
  }
  const_reverse_iterator rcbegin () noexcept {
    return const_reverse_iterator{this->end ()};
  }

  /// Returns an iterator to the end of the container.
  const_iterator end () const noexcept { return begin () + size_; }
  iterator end () noexcept { return begin () + size_; }
  const_iterator cend () noexcept { return begin () + size_; }
  reverse_iterator rend () noexcept { return reverse_iterator{this->begin ()}; }
  const_reverse_iterator rend () const noexcept {
    return const_reverse_iterator{this->begin ()};
  }
  const_reverse_iterator rcend () noexcept {
    return const_reverse_iterator{this->begin ()};
  }
  ///@}

  /// \name Modifiers
  ///@{

  /// Removes all elements from the container.
  /// Invalidates any references, pointers, or iterators referring to contained
  /// elements. Any past-the-end iterators are also invalidated.
  void clear () noexcept;

  /// Adds an element to the end of the container.
  template <typename OtherType>
  void push_back (OtherType const &v) {
    assert (size_ < Size);
    construct_at (&element (*this, size_), v);
    ++size_;
  }
  /// Appends a new element to the end of the container.
  template <typename... Args>
  void emplace_back (Args &&...args) {
    assert (size_ < Size);
    construct_at (&element (*this, size_), std::forward<Args> (args)...);
    ++size_;
  }

  /// Replaces the contents with \p count copies of value \p value.
  ///
  /// \p count The new size of the container.
  /// \p value The value with which to initialize elements of the container.
  void assign (size_type count, T const &value);
  /// Replaces the contents with copies of those in the range [first, last). The
  /// behavior is undefined if either argument is an iterator into *this.
  ///
  /// \p first The first of the range from which elements are to be copied.
  /// \p last The last of the range from which elements are to be copied.
  template <typename InputIterator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
  void assign (InputIterator first, InputIterator last);

  /// Replaces the contents with the elements from the initializer list \p ilist
  ///
  /// \p ilist Initializer list from which elements are to be copied.
  void assign (std::initializer_list<T> ilist) {
    this->assign (std::begin (ilist), std::end (ilist));
  }

  /// Add the specified range to the end of the vector.
  template <typename InputIterator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
  void append (InputIterator first, InputIterator last);
  void append (std::initializer_list<T> ilist) {
    this->append (std::begin (ilist), std::end (ilist));
  }

  void pop_back () {
    assert (size_ > 0U && "Attempt to use pop_back() with an empty container");
    --size_;
    std::destroy_at (&this->element (*this, size_));
  }

  ///@}

private:
  template <typename ArrayVec,
            typename ResultType = inherit_const_t<ArrayVec, T>>
  static ResultType &element (ArrayVec &&v, size_t n) noexcept {
    assert (n < v.max_size ());
    return *pointer_cast<ResultType *> (v.data_.data () + n);
  }

  template <bool IsMove, typename OtherVec>
  void operator_assign (OtherVec &other) noexcept;

  /// The actual number of elements for which this buffer is sized.
  /// Note that this may be less than Size.
  size_type size_ = 0;

  std::array<typename std::aligned_storage_t<sizeof (T), alignof (T)>, Size>
      data_;
};

// (ctor)
// ~~~~~~
template <typename T, size_t Size>
arrayvec<T, Size>::arrayvec (arrayvec &&other) noexcept (
    std::is_nothrow_move_constructible_v<T>) {
  auto dest = begin();
  for (auto src = other.begin(), src_end = other.end(); src != src_end; ++src) {
    construct_at (&*dest, std::move (*src));
    ++dest;
  }
  size_ = other.size_;
  other.size_ = 0;
}

template <typename T, size_t Size>
template <typename ForwardIterator>
PEEJAY_CXX20REQUIRES ((std::forward_iterator<ForwardIterator>))
arrayvec<T, Size>::arrayvec (ForwardIterator first, ForwardIterator last) {
  auto out = begin ();
  for (; first != last; ++size_, ++first, ++out) {
    if (size_ == Size) {
      assert (false && "arrayvec container overflow");
      return;
    }
    construct_at (&*out, *first);
  }
}

template <typename T, size_t Size>
arrayvec<T, Size>::arrayvec (size_type count, T const &value) {
  assert (count <= Size);
  while (count-- > 0) {
    this->emplace_back (value);
  }
}

// operator assign
// ~~~~~~~~~~~~~~~
template <typename T, size_t Size>
template <bool IsMove, typename OtherVec>
void arrayvec<T, Size>::operator_assign (OtherVec &other) noexcept {
  assert (&other != this);
  auto const p = this->begin ();
  auto src = other.begin();
  auto dest = p;
  // Step 1: where both *this and other have constructed members we can just use
  // assignment.
  auto end = dest + std::min (size_, other.size_);
  for (; dest < end; ++src, ++dest) {
    if constexpr (IsMove) {
      *dest = std::move (*src);
    } else {
      *dest = *src;
    }
  }
  // Step 2: target memory does not contain constructed members.
  end = p + other.size_;
  for (; dest < end; ++src, ++dest) {
    if constexpr (IsMove) {
      construct_at (&*dest, std::move (*src));
    } else {
      construct_at (&*dest, *src);
    }
  }
  // Step 3: The 'other' array is shorter than this object so release any extra
  // members.
  end = p + size_;
  for (; dest < end; ++dest) {
    std::destroy_at (&*dest);
  }
  size_ = other.size_;
}

// clear
// ~~~~~
template <typename T, size_t Size>
void arrayvec<T, Size>::clear () noexcept {
  std::for_each (begin (), end (), [] (T &t) { std::destroy_at (&t); });
  size_ = 0;
}

// resize
// ~~~~~~
template <typename T, size_t Size>
void arrayvec<T, Size>::resize (size_type count) {
  assert (count <= Size);
  if (count < size_) {
    std::for_each (begin () + count, end (),
                   [] (T &t) { std::destroy_at (&t); });
    size_ = count;
  } else if (count > size_) {
    for (auto it = end(), e = begin() + count; it != e; ++it) {
      construct_at (&*it);
    }
    size_ = count;
  }
}

// assign
// ~~~~~~
template <typename T, size_t Size>
void arrayvec<T, Size>::assign (size_type count, T const &value) {
  this->clear ();
  for (; count > 0; --count) {
    this->emplace_back (value);
  }
}

template <typename T, size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
void arrayvec<T, Size>::assign (InputIterator first, InputIterator last) {
  this->clear ();
  this->append (first, last);
}

// append
// ~~~~~~
template <typename T, size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
void arrayvec<T, Size>::append (InputIterator first, InputIterator last) {
  for (; first != last; ++first) {
    this->emplace_back (*first);
  }
}

template <typename Container>
class checked_back_insert_iterator {
public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr checked_back_insert_iterator (Container *const container,
                                          bool *const overflow) noexcept
      : container_{container}, overflow_{overflow} {}

  constexpr checked_back_insert_iterator &operator= (
      typename Container::value_type const &value) {
    if (container_->size () >= container_->max_size ()) {
      *overflow_ = true;
    } else {
      container_->push_back (value);
    }
    return *this;
  }
  constexpr checked_back_insert_iterator &operator= (
      typename Container::value_type &&value) {
    if (container_->size () >= container_->max_size ()) {
      *overflow_ = true;
    } else {
      container_->push_back (std::move (value));
    }
    return *this;
  }

  constexpr checked_back_insert_iterator &operator* () { return *this; }
  constexpr checked_back_insert_iterator &operator++ () { return *this; }
  constexpr checked_back_insert_iterator operator++ (int) { return *this; }

private:
  Container *container_;
  bool *overflow_;
};

template <typename Container>
checked_back_insert_iterator (Container *, bool *)
    -> checked_back_insert_iterator<Container>;

}  // end namespace peejay

#endif  // PEEJAY_ARRAYVEC_HPP
