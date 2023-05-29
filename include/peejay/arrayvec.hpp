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
#include <iterator>
#include <memory>
#include <type_traits>

#include "peejay/portab.hpp"
#include "peejay/to_address.hpp"
#include "peejay/uinteger.hpp"

#if PEEJAY_CXX20 && defined(__has_include) && __has_include(<bit>)
#include <bit>
#endif

namespace peejay {

// pointer cast
// ~~~~~~~~~~~~
#if PEEJAY_HAVE_CONCEPTS
template <typename To, typename From>
  requires (std::is_trivial_v<To> && std::is_trivial_v<From>)
#else
template <typename To, typename From,
          typename = typename std::enable_if_t<std::is_trivial_v<To> &&
                                               std::is_trivial_v<From>>>
#endif
constexpr To pointer_cast (From p) noexcept {
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
  return std::bit_cast<To> (p);
#else
  return reinterpret_cast<To> (p);
#endif
}

// construct at
// ~~~~~~~~~~~~
#if PEEJAY_CXX20
using std::construct_at;
#else
/// Creates a T object initialized with arguments args... at given address p.
template <typename T, typename... Args>
constexpr T *construct_at (T *p, Args &&...args) {
  return ::new (p) T (std::forward<Args> (args)...);
}
#endif  // PEEJAY_CXX20

// forward iterator
// ~~~~~~~~~~~~~~~~
#ifdef PEEJAY_HAVE_CONCEPTS
template <typename Iterator, typename T>
concept forward_iterator = std::forward_iterator<Iterator>;
#else
template <typename Iterator, typename T>
constexpr bool forward_iterator = std::is_convertible_v<
    typename std::iterator_traits<Iterator>::iterator_category,
    std::forward_iterator_tag>;
#endif  // PEEJAY_HAVE_CONCEPTS

//*                                     *
//*  __ _ _ _ _ _ __ _ _  ___ _____ __  *
//* / _` | '_| '_/ _` | || \ V / -_) _| *
//* \__,_|_| |_| \__,_|\_, |\_/\___\__| *
//*                    |__/             *
template <typename T, std::size_t Size = 256 / sizeof (T)>
class arrayvec {
public:
  static_assert (!std::is_move_constructible_v<T> ||
                     std::is_nothrow_move_constructible_v<T>,
                 "If the member type can be move-constructed, the operation "
                 "must be nothrow");
  static_assert (
      !std::is_move_assignable_v<T> || std::is_nothrow_move_assignable_v<T>,
      "If the member type can be move-assigned, the operation must be nothrow");

  using value_type = T;

  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

  using size_type = uinteger_t<bits_required (Size)>;
  using difference_type = std::ptrdiff_t;

  using iterator = value_type *;
  using const_iterator = value_type const *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// Constructs the container with an initial size of 0.
  arrayvec () noexcept = default;
  arrayvec (std::initializer_list<T> init)
      : arrayvec (std::begin (init), std::end (init)) {}
  template <typename ForwardIterator>
  PEEJAY_CXX20REQUIRES ((std::forward_iterator<ForwardIterator>))
  arrayvec (ForwardIterator first, ForwardIterator last);
  arrayvec (size_type count, const_reference value = T ());

  arrayvec (arrayvec const &other)
      : arrayvec (std::begin (other), std::end (other)) {}
  arrayvec (arrayvec &&other) noexcept;

  ~arrayvec () noexcept { clear (); }

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

  /// \name Element access
  ///@{
  constexpr T const *data () const noexcept {
    return pointer_cast<T const *> (data_.data ());
  }
  constexpr T *data () noexcept { return pointer_cast<T *> (data_.data ()); }

  constexpr const_reference operator[] (size_type n) const noexcept {
    assert (n < max_size ());
    return *(data () + n);
  }
  constexpr reference operator[] (size_type n) noexcept {
    assert (n < max_size ());
    return *(data () + n);
  }

  constexpr reference front () noexcept {
    assert (size_ > 0);
    return *data ();
  }
  constexpr const_reference front () const noexcept {
    assert (size_ > 0);
    return *data ();
  }
  constexpr reference back () noexcept {
    assert (size_ > 0);
    return *(data () + size_ - 1U);
  }
  constexpr const_reference back () const noexcept {
    assert (size_ > 0);
    return *(data () + size_ - 1U);
  }

  ///@}

  /// \name Capacity
  ///@{
  /// Returns the number of elements.
  [[nodiscard]] constexpr size_type size () const noexcept { return size_; }

  /// Checks whether the container is empty.
  [[nodiscard]] constexpr bool empty () const noexcept { return size_ == 0U; }

  /// Returns the number of elements that can be held.
  [[nodiscard]] constexpr size_type capacity () const noexcept {
    return max_size ();
  }

  /// Returns the maximum number of elements the container is able to hold.
  [[nodiscard]] constexpr size_type max_size () const noexcept {
    assert (data_.size () == Size);
    return Size;
  }

  /// Resizes the container to contain \p count elements. If the current size is
  /// less than \p count, the container is reduced to its first \p count
  /// elements. If the current size is less than \p count, additional
  /// default-inserted elements are appended.
  ///
  /// \param count  The new number of elements in the container. Must be less
  ///               than or equal to Size.
  void resize (size_type count);

  /// Resizes the container to contain count elements. If the current size is
  /// greater than \p count, the container is reduced to its first \p count
  /// elements. If the current size is less than \p count, additional
  /// copies of \p value are appended.
  ///
  /// \param count  The new number of elements in the container. Must be less
  ///               than or equal to Size.
  /// \param value  The value with which to initialize the new elements.
  void resize (size_type count, const_reference value);

  ///@}

  /// \name Iterators
  ///@{
  /// Returns an iterator to the beginning of the container.
  [[nodiscard]] constexpr const_iterator begin () const noexcept {
    return data ();
  }
  [[nodiscard]] constexpr iterator begin () noexcept { return data (); }
  const_iterator cbegin () const noexcept { return data (); }
  /// Returns a reverse iterator to the first element of the reversed
  /// container. It corresponds to the last element of the non-reversed
  /// container.
  reverse_iterator rbegin () noexcept { return reverse_iterator{this->end ()}; }
  const_reverse_iterator rbegin () const noexcept { return rcbegin (); }
  const_reverse_iterator rcbegin () const noexcept {
    return const_reverse_iterator{this->end ()};
  }

  /// Returns an iterator to the end of the container.
  iterator end () noexcept {
    // (Curious parentheses to avoid a clash with MSVC min macro.)
    return begin () + (std::min) (size (), max_size ());
  }
  const_iterator end () const noexcept { return cend (); }
  const_iterator cend () const noexcept {
    // (Curious parentheses to avoid a clash with MSVC min macro.)
    return begin () + (std::min) (size (), max_size ());
  }
  reverse_iterator rend () noexcept { return reverse_iterator{this->begin ()}; }
  const_reverse_iterator rend () const noexcept { return rcend (); }
  const_reverse_iterator rcend () const noexcept {
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
  /// \param value  The value of the element to append.
  void push_back (const_reference value);
  /// Adds an element to the end of the container.
  /// \param value  The value of the element to append.
  void push_back (T &&value);
  /// Appends a new element to the end of the container.
  template <typename... Args>
  void emplace_back (Args &&...args);

  /// Replaces the contents with \p count copies of value \p value.
  ///
  /// \param count The new size of the container.
  /// \param value The value with which to initialize elements of the container.
  void assign (size_type count, const_reference value);
  /// Replaces the contents with copies of those in the range [first, last). The
  /// behavior is undefined if either argument is an iterator into *this.
  ///
  /// \tparam Iterator An iterator which will produce the elements to be
  ///   inserted. Must meet the requirements of LegacyInputIterator.
  /// \param first The first of the range from which elements are to be copied.
  /// \param last The last of the range from which elements are to be copied.
  template <typename Iterator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<Iterator>))
  void assign (Iterator first, Iterator last);
  /// Replaces the contents with the elements from the initializer list \p ilist
  ///
  /// \param ilist Initializer list from which elements are to be copied.
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
    std::destroy_at (data () + size_);
  }

  /// Inserts \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  /// \returns An iterator pointing to the new element.
  iterator insert (const_iterator pos, const_reference value);
  /// Inserts \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the new element.
  iterator insert (const_iterator pos, T &&value);
  /// Inserts \p count copies of \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new elements will be constructed.
  /// \param count  The number of copies to be inserted.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the first of the new elements or \p pos
  ///   if \p count == 0.
  iterator insert (const_iterator pos, size_type count, const_reference value);
  /// Inserts a new element into the container directly before pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param args Arguments to be forwarded to the element's constructor.
  /// \returns  An iterator pointing to the emplaced element.
  template <typename... Args>
  iterator emplace (const_iterator pos, Args &&...args);
  /// Inserts elements from range [\p first, \p last) before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param first  The beginning of the range of elements to be inserted.
  ///   Cannot be an iterator into the container for which insert() is called.
  /// \param last  The end of the range of elements to be inserted. Cannot
  ///   be an iterator into the container for which insert() is called.
  /// \returns  Iterator pointing to the first element inserted, or \p pos
  ///   if \p first == \p last.
  template <typename InputIterator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
  iterator insert (const_iterator pos, InputIterator first, InputIterator last);
  /// Inserts elements from the initializer list \p ilist before \p pos.
  ///
  /// \param pos  The Iterator before which the new elements will be constructed.
  /// \param ilist  An initializer list from which to to insert the values.
  /// \returns  An iterator pointing to the first element inserted, or \p pos
  ///   if \p ilist is empty.
  iterator insert (const_iterator pos, std::initializer_list<T> ilist) {
    return insert (pos, std::begin (ilist), std::end (ilist));
  }

  /// Erases the specified element from the container. Invalidates iterators
  /// and references at or after the point of the erase, including the end()
  /// iterator.
  ///
  /// \param pos  Iterator to the element to remove.
  /// \returns  Iterator following the last removed element. If \p pos refers
  ///   to the last element, then the end() iterator is returned.
  iterator erase (const_iterator pos);
  /// Erases the elements in the range [\p first, \p last). Invalidates
  /// iterators and references at or after the point of the erase, including
  /// the end() iterator.
  ///
  /// \param first  The first of the range of elements to remove.
  /// \param last  The last of the range of elements to remove.
  /// \returns Iterator following the last removed element. If last == end()
  ///   prior to removal, then the updated end() iterator is returned. If
  ///   [\p first, \p last) is an empty range, then last is returned.
  iterator erase (const_iterator first, const_iterator last);

  ///@}

private:
  template <bool IsMove, typename OtherVec>
  void operator_assign (OtherVec &other) noexcept (IsMove);

  template <typename... Args>
  void resize_impl (size_type count, Args &&...args);

  void move_range (iterator from, iterator to) noexcept;

  /// Converts the const-iterator \p pos, which must be an iterator reference
  /// to this container, to a non-const iterator referencing the same element.
  ///
  /// \param pos A const_iterator instance to be converted.
  /// \returns  A non-const iterator referencing the same element as \p pos.
  constexpr iterator to_non_const_iterator (const_iterator pos) noexcept;

  /// The actual number of elements for which this buffer is sized.
  /// Note that this may be less than Size.
  size_type size_ = 0;
  static_assert (std::numeric_limits<size_type>::max () >= Size);

  std::array<typename std::aligned_storage_t<sizeof (T), alignof (T)>, Size>
      data_;
};

template <typename T, std::size_t Size>
bool operator== (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return lhs.size () == rhs.size () &&
         std::equal (std::begin (lhs), std::end (lhs), std::begin (rhs));
}

template <typename T, std::size_t Size>
bool operator!= (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return !operator== (lhs, rhs);
}

template <typename T, std::size_t Size>
bool operator< (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return std::lexicographical_compare (lhs.begin (), lhs.end (), rhs.begin (),
                                       rhs.end ());
}

template <typename T, std::size_t Size>
bool operator<= (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return !(rhs < lhs);
}

template <typename T, std::size_t Size>
bool operator> (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return rhs < lhs;
}

template <typename T, std::size_t Size>
bool operator>= (arrayvec<T, Size> const &lhs, arrayvec<T, Size> const &rhs) {
  return !(lhs < rhs);
}

// (ctor)
// ~~~~~~
template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (arrayvec &&other) noexcept {
  auto dest = begin ();
  for (auto src = other.begin (), src_end = other.end (); src != src_end;
       ++src) {
    construct_at (&*dest, std::move (*src));
    ++dest;
  }
  size_ = other.size_;
  other.size_ = 0;
}

template <typename T, std::size_t Size>
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

template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (size_type count, const_reference value) {
  assert (count <= Size);
  for (; count > 0; --count) {
    this->emplace_back (value);
  }
}

// operator assign
// ~~~~~~~~~~~~~~~
template <typename T, std::size_t Size>
template <bool IsMove, typename OtherVec>
void arrayvec<T, Size>::operator_assign (OtherVec &other) noexcept (IsMove) {
  assert (&other != this);
  auto const p = this->begin ();
  auto src = other.begin ();
  auto dest = p;
  // Step 1: where both *this and other have constructed members we can just use
  // assignment.
  // [ Wierd () to avoid std::min() clashing with the MSVC min macros. ]
  auto end = dest + (std::min) (size_, other.size_);
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

// to non const iterator
// ~~~~~~~~~~~~~~~~~~~~~
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::to_non_const_iterator (
    const_iterator pos) noexcept -> iterator {
  assert (pos >= begin () && pos <= end () &&
          "Iterator argument is out of range");
  auto *const d = data ();
  return iterator{d + (pos - d)};
}

// clear
// ~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::clear () noexcept {
  std::for_each (begin (), end (), [] (reference t) { std::destroy_at (&t); });
  size_ = 0;
}

// resize
// ~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
void arrayvec<T, Size>::resize_impl (size_type count, Args &&...args) {
  assert (count <= Size);
  // Wierd () to avoid std::min() clash with the MSVC min macros.
  count = (std::min) (count, max_size ());
  if (count < size_) {
    std::for_each (begin () + count, end (),
                   [] (reference t) { std::destroy_at (&t); });
  } else {
    for (auto it = end (), e = begin () + count; it != e; ++it) {
      construct_at (to_address (it), std::forward<Args> (args)...);
    }
  }
  size_ = count;
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::resize (size_type count) {
  return resize_impl (count);
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::resize (size_type count, const_reference value) {
  return resize_impl (count, value);
}

// push back
// ~~~~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::push_back (const_reference value) {
  assert (size () < max_size ());
  construct_at (data () + size_, value);
  ++size_;
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::push_back (T &&value) {
  assert (size () < max_size ());
  construct_at (data () + size_, std::move (value));
  ++size_;
}

// emplace back
// ~~~~~~~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
void arrayvec<T, Size>::emplace_back (Args &&...args) {
  assert (size () < max_size ());
  construct_at (data () + size_, std::forward<Args> (args)...);
  ++size_;
}

// assign
// ~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::assign (size_type count, const_reference value) {
  this->clear ();
  for (; count > 0; --count) {
    this->push_back (value);
  }
}

template <typename T, std::size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
void arrayvec<T, Size>::assign (InputIterator first, InputIterator last) {
  this->clear ();
  this->append (first, last);
}

// append
// ~~~~~~
template <typename T, std::size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
void arrayvec<T, Size>::append (InputIterator first, InputIterator last) {
  for (; first != last; ++first) {
    this->emplace_back (*first);
  }
}

// erase
// ~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator pos) -> iterator {
  auto const r = this->to_non_const_iterator (pos);
  std::move (r + 1, this->end (), r);
  std::destroy_at (&this->back ());
  --size_;
  return r;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator first, const_iterator last)
    -> iterator {
  assert (first >= begin () && first <= last && last <= cend () &&
          "Iterator range to erase() is invalid");
  auto *const p = data () + (first - begin ());
  auto new_end = std::move (iterator{p + (last - first)}, this->end (), p);
  std::for_each (new_end, end (), [] (reference v) { std::destroy_at (&v); });
  size_ -= static_cast<size_type> (last - first);
  return iterator{p};
}

// move range
// ~~~~~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::move_range (iterator const from,
                                    iterator const to) noexcept {
  iterator e = this->end ();
  assert (e >= from && to >= from);
  difference_type const dist = to - from;  // how far to move.
  iterator const new_end = e + dist;
  difference_type const num_to_move = e - from;
  difference_type const num_uninit =
      (std::min) (num_to_move, new_end - e);  // the number past the end.

  iterator dest = new_end - num_uninit;
  for (iterator src = e - num_uninit; src < e; ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
  }
  std::move_backward (from, from + num_to_move - num_uninit,
                      new_end - num_uninit);
}

// insert
// ~~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator const pos, size_type const count,
                                const_reference value) -> iterator {
  assert (size () + count < max_size () && "Insert will overflow");

  iterator const from = this->to_non_const_iterator (pos);
  auto const to = from + count;
  auto const current_end = this->end ();
  assert (current_end >= from && to >= from);

  iterator const new_end = current_end + (to - from);
  difference_type const num_to_move = current_end - from;
  difference_type const num_uninit =
      (std::min) (num_to_move, new_end - current_end);

  // Copy-construct into uninitialized elements.
  for (auto it = current_end; it < to; ++it) {
    construct_at (to_address (it), value);
    ++size_;
  }
  // Move existing elements into uninitialized space.
  iterator dest = new_end - num_uninit;
  for (iterator src = current_end - num_uninit; src < current_end; ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
    ++size_;
  }
  // Move elements between the initialized elements.
  std::move_backward (from, from + num_to_move - num_uninit,
                      new_end - num_uninit);
  // Copy into initialized elements.
  std::fill (from, to, value);
  return from;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator pos, const_reference value)
    -> iterator {
  if (auto const e = this->end (); pos == e) {
    // Fast path for appending an element.
    push_back (value);
    assert (this->to_non_const_iterator (pos) == e);
    return e;
  }

  return insert (pos, size_type{1}, value);
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator pos, T &&value) -> iterator {
  assert (size () < max_size () && "Insert will cause overflow");
  iterator const r = this->to_non_const_iterator (pos);
  if (r == this->end ()) {
    emplace_back (std::move (value));
    return r;
  }
  move_range (r, r + 1);
  size_ += 1;
  *r = std::move (value);
  return r;
}

template <typename T, std::size_t Size>
template <typename Iterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<Iterator>))
auto arrayvec<T, Size>::insert (const_iterator pos, Iterator first,
                                Iterator last) -> iterator {
  iterator const r = this->to_non_const_iterator (pos);
  iterator const e = this->end ();

  if (pos == e) {
    // Insert at the end can be efficiently mapped to a series of push_back()
    // calls.
    std::for_each (first, last,
                   [this] (value_type const &v) { this->push_back (v); });
    return r;
  }

  if constexpr (forward_iterator<Iterator, T>) {
    // A forward iterator can be used with multi-pass algorithms such as this...
    auto const n = std::distance (first, last);
    // If all the new objects land on existing, initialized, elements we don't
    // need to worry about leaving the container in an invalid state if a ctor
    // throws.
    if (pos + n <= e) {
      assert (n <= max_size () - size ());
      move_range (r, r + n);
      size_ += static_cast<size_type> (n);
      std::copy (first, last, r);
      return r;
    }

    // TODO: Add an additional optimization path for random-access iterators.
    // 1. Construct any objects in uninitialized space.
    // 2. Move objects from initialized to uninitialize space after the objects
    //    created in step 1.
    // 3. Construct objects in initialized space.
  }

  // A single-pass fallback algorith for input iterators.
  while (first != last) {
    this->insert (pos, *first);
    ++first;
    ++pos;
  }
  return r;
}

// emplace
// ~~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
auto arrayvec<T, Size>::emplace (const_iterator pos, Args &&...args)
    -> iterator {
  assert (size () < max_size () && pos >= begin () && pos <= end ());

  auto *const d = data ();
  auto const e = end ();
  auto r = iterator{d + (pos - d)};
  if (pos == e) {
    emplace_back (std::forward<Args> (args)...);
    return r;
  }

  move_range (r, r + 1);
  if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
    // After destroying the object at 'r', we can construct in-place.
    auto *const p = to_address (r);
    std::destroy_at (p);
    construct_at (p, std::forward<Args> (args)...);
  } else {
    // Constructing the object might throw and the location is already occupied
    // by an element so we first make a temporary instance then move-assign it
    // to the required location.
    T t{std::forward<Args> (args)...};
    *r = std::move (t);
  }
  size_ += 1;
  return r;
}

}  // end namespace peejay

#endif  // PEEJAY_ARRAYVEC_HPP
