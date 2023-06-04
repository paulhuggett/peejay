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
#include <limits>
#include <memory>
#include <type_traits>

#include "peejay/portab.hpp"
#include "peejay/uinteger.hpp"

#if PEEJAY_CXX20 && defined(__has_include) && __has_include(<bit>)
#include <bit>
#endif

namespace peejay {

//*                                      _                   *
//*  __ _ _ _ _ _ __ _ _  ___ _____ __  | |__  __ _ ___ ___  *
//* / _` | '_| '_/ _` | || \ V / -_) _| | '_ \/ _` (_-</ -_) *
//* \__,_|_| |_| \__,_|\_, |\_/\___\__| |_.__/\__,_/__/\___| *
//*                    |__/                                  *
class arrayvec_base {
protected:
  template <bool IsMove, typename SrcType, typename DestType>
  static std::size_t operator_assign (
      std::pair<SrcType *, std::size_t> const &dest,
      std::pair<DestType *, std::size_t> const &src) noexcept (IsMove);

  template <typename Iterator>
  static void clear (Iterator first, Iterator last) noexcept;

  template <typename T, typename... Args>
  static std::size_t resize (T *data, std::size_t size, std::size_t new_size,
                             Args &&...args);

  template <typename Iterator>
  static void move_range (Iterator from, Iterator end, Iterator to) noexcept;

  template <typename Iterator>
  static std::size_t erase (Iterator pos, Iterator end, std::size_t size);
};

// operator assign
// ~~~~~~~~~~~~~~~
template <bool IsMove, typename SrcType, typename DestType>
std::size_t arrayvec_base::operator_assign (
    std::pair<SrcType *, std::size_t> const &dest,
    std::pair<DestType *, std::size_t> const &src) noexcept (IsMove) {
  auto *src_ptr = src.first;
  auto *dest_ptr = dest.first;
  // Step 1: where both source and destination arrays have constructed members
  // we can just use assignment.
  // [ Wierd () to avoid std::min() clashing with the MSVC min macros. ]
  auto *const uninit_end = dest_ptr + (std::min) (dest.second, src.second);
  for (; dest_ptr < uninit_end; ++src_ptr, ++dest_ptr) {
    if constexpr (IsMove) {
      *dest_ptr = std::move (*src_ptr);
    } else {
      *dest_ptr = *src_ptr;
    }
  }
  // Step 2: target memory does not contain constructed members.
  for (; dest_ptr < dest.first + src.second; ++src_ptr, ++dest_ptr) {
    if constexpr (IsMove) {
      construct_at (dest_ptr, std::move (*src_ptr));
    } else {
      construct_at (dest_ptr, *src_ptr);
    }
  }
  // Step 3: The 'other' array is shorter than this object so release any extra
  // members.
  for (; dest_ptr < dest.first + dest.second; ++dest_ptr) {
    std::destroy_at (dest_ptr);
  }
  return src.second;
}

// clear
// ~~~~~
template <typename Iterator>
void arrayvec_base::clear (Iterator first, Iterator last) noexcept {
  using T = typename std::iterator_traits<Iterator>::value_type;
  std::for_each (first, last, [] (T &value) { std::destroy_at (&value); });
}

// resize
// ~~~~~~
template <typename T, typename... Args>
std::size_t arrayvec_base::resize (T *const data, std::size_t const size,
                                   std::size_t const new_size, Args &&...args) {
  auto *const end = data + size;
  auto *const new_end = data + new_size;
  if (new_size < size) {
    arrayvec_base::clear (new_end, end);
  } else {
    for (auto *it = end; it != new_end; ++it) {
      construct_at (to_address (it), std::forward<Args> (args)...);
    }
  }
  return new_size;
}

// move range
// ~~~~~~~~~~
template <typename Iterator>
void arrayvec_base::move_range (Iterator const from, Iterator const end,
                                Iterator const to) noexcept {
  assert (end >= from && to >= from);
  auto const dist = to - from;  // how far to move.
  Iterator const new_end = end + dist;
  auto const num_to_move = end - from;
  auto const num_uninit =
      (std::min) (num_to_move, new_end - end);  // the number past the end.

  Iterator dest = new_end - num_uninit;
  for (Iterator src = end - num_uninit; src < end; ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
  }
  std::move_backward (from, from + num_to_move - num_uninit,
                      new_end - num_uninit);
}

// erase
// ~~~~~
template <typename Iterator>
std::size_t arrayvec_base::erase (Iterator pos, Iterator end,
                                  std::size_t size) {
  std::move (pos + 1, end, pos);
  std::destroy_at (&*(end - 1));
  return size - 1;
}

//*                                     *
//*  __ _ _ _ _ _ __ _ _  ___ _____ __  *
//* / _` | '_| '_/ _` | || \ V / -_) _| *
//* \__,_|_| |_| \__,_|\_, |\_/\___\__| *
//*                    |__/             *
template <typename T, std::size_t Size = 256 / sizeof (T)>
class arrayvec : public arrayvec_base {
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
  template <typename InputIterator>
  PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
  arrayvec (InputIterator first, InputIterator last);
  arrayvec (size_type count, const_reference value = T ());

  arrayvec (arrayvec const &other)
      : arrayvec (std::begin (other), std::end (other)) {}
  arrayvec (arrayvec &&other) noexcept;

  ~arrayvec () noexcept { this->clear (); }

  arrayvec &operator= (arrayvec const &other) {
    if (&other != this) {
      std::size_t const new_size = arrayvec_base::operator_assign<false> (
          std::make_pair (this->data (),
                          static_cast<std::size_t> (this->size ())),
          std::make_pair (other.data (),
                          static_cast<std::size_t> (other.size ())));
      assert (new_size <= this->max_size ());
      size_ = static_cast<size_type> (new_size);
    }
    return *this;
  }
  arrayvec &operator= (arrayvec &&other) noexcept {
    if (&other != this) {
      std::size_t const new_size = arrayvec_base::operator_assign<true> (
          std::make_pair (this->data (),
                          static_cast<std::size_t> (this->size ())),
          std::make_pair (other.data (),
                          static_cast<std::size_t> (other.size ())));
      assert (new_size <= this->max_size ());
      size_ = static_cast<size_type> (new_size);
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
    assert (n < this->size ());
    return *(this->data () + n);
  }
  constexpr reference operator[] (size_type n) noexcept {
    assert (n < this->size ());
    return *(this->data () + n);
  }

  constexpr reference front () noexcept;
  constexpr const_reference front () const noexcept;
  constexpr reference back () noexcept;
  constexpr const_reference back () const noexcept;

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
  iterator end () noexcept { return this->begin () + this->size (); }
  const_iterator end () const noexcept { return cend (); }
  const_iterator cend () const noexcept {
    return this->begin () + this->size ();
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

  void pop_back () {
    assert (size_ > 0U && "Attempt to use pop_back() with an empty container");
    --size_;
    std::destroy_at (to_address (this->end ()));
  }

  /// Inserts \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the new element.
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
  template <typename... Args>
  void resize_impl (size_type count, Args &&...args);

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

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator== (arrayvec<T, LhsSize> const &lhs,
                 arrayvec<T, RhsSize> const &rhs) {
  return lhs.size () == rhs.size () &&
         std::equal (std::begin (lhs), std::end (lhs), std::begin (rhs));
}

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator!= (arrayvec<T, LhsSize> const &lhs,
                 arrayvec<T, RhsSize> const &rhs) {
  return !operator== (lhs, rhs);
}

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator< (arrayvec<T, LhsSize> const &lhs,
                arrayvec<T, RhsSize> const &rhs) {
  return std::lexicographical_compare (lhs.begin (), lhs.end (), rhs.begin (),
                                       rhs.end ());
}

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator<= (arrayvec<T, LhsSize> const &lhs,
                 arrayvec<T, RhsSize> const &rhs) {
  return !(rhs < lhs);
}

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator> (arrayvec<T, LhsSize> const &lhs,
                arrayvec<T, RhsSize> const &rhs) {
  return rhs < lhs;
}

template <typename T, std::size_t LhsSize, std::size_t RhsSize>
bool operator>= (arrayvec<T, LhsSize> const &lhs,
                 arrayvec<T, RhsSize> const &rhs) {
  return !(lhs < rhs);
}

// (ctor)
// ~~~~~~
template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (arrayvec &&other) noexcept {
  auto *dest = this->data ();
  for (auto src = other.begin (), src_end = other.end (); src != src_end;
       ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
    ++size_;
  }
}

template <typename T, std::size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
arrayvec<T, Size>::arrayvec (InputIterator first, InputIterator last) {
  auto out = this->begin ();
  for (; first != last; ++first) {
    if (size_ == Size) {
      assert (false && "arrayvec container overflow");
      return;
    }
    construct_at (to_address (out), *first);
    ++size_;
    ++out;
  }
}

template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (size_type count, const_reference value) {
  assert (count <= Size);
  for (; count > 0; --count) {
    this->emplace_back (value);
  }
}

// front
// ~~~~~
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::front () noexcept -> reference {
  assert (size_ > 0);
  return *this->data ();
}
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::front () const noexcept -> const_reference {
  assert (size_ > 0);
  return *this->data ();
}

// back
// ~~~~
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::back () noexcept -> reference {
  assert (this->size () > 0U);
  return *(this->end () - 1);
}
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::back () const noexcept -> const_reference {
  assert (this->size () > 0U);
  return *(this->end () - 1);
}

// to non const iterator
// ~~~~~~~~~~~~~~~~~~~~~
template <typename T, std::size_t Size>
constexpr auto arrayvec<T, Size>::to_non_const_iterator (
    const_iterator pos) noexcept -> iterator {
  assert (pos >= this->begin () && pos <= this->end () &&
          "Iterator argument is out of range");
  auto *const d = this->data ();
  return iterator{d + (pos - d)};
}

// clear
// ~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::clear () noexcept {
  arrayvec_base::clear (this->begin (), this->end ());
  size_ = 0;
}

// resize
// ~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
void arrayvec<T, Size>::resize_impl (size_type count, Args &&...args) {
  assert (count <= this->max_size ());
  // Wierd () around the call to std::min() to avoid clashing with the MSVC
  // 'min' macro.
  std::size_t const new_size = arrayvec_base::resize (
      this->data (), this->size (), (std::min) (count, this->max_size ()),
      std::forward<Args> (args)...);
  assert (new_size <= this->max_size ());
  size_ = static_cast<size_type> (new_size);
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::resize (size_type count) {
  return this->resize_impl (count);
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::resize (size_type count, const_reference value) {
  return this->resize_impl (count, value);
}

// push back
// ~~~~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::push_back (const_reference value) {
  assert (this->size () < this->max_size ());
  construct_at (to_address (this->end ()), value);
  ++size_;
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::push_back (T &&value) {
  assert (this->size () < this->max_size ());
  construct_at (to_address (this->end ()), std::move (value));
  ++size_;
}

// emplace back
// ~~~~~~~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
void arrayvec<T, Size>::emplace_back (Args &&...args) {
  assert (this->size () < this->max_size ());
  construct_at (to_address (this->end ()), std::forward<Args> (args)...);
  ++size_;
}

// assign
// ~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::assign (size_type count, const_reference value) {
  // TODO(paul): this would be better done in a single pass.
  this->clear ();
  for (; count > 0; --count) {
    this->push_back (value);
  }
}

template <typename T, std::size_t Size>
template <typename InputIterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<InputIterator>))
void arrayvec<T, Size>::assign (InputIterator first, InputIterator last) {
  // TODO(paul): this would be better done in a single pass.
  this->clear ();
  this->insert (this->end (), first, last);
}

// erase
// ~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator pos) -> iterator {
  iterator const p = this->to_non_const_iterator (pos);
  size_ =
      static_cast<size_type> (arrayvec_base::erase (p, this->end (), size_));
  return p;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator first, const_iterator last)
    -> iterator {
  assert (first >= this->begin () && first <= last && last <= this->cend () &&
          "Iterator range to erase() is invalid");
  auto *const p = this->data () + std::distance (this->cbegin (), first);
  auto new_end = std::move (iterator{p + (last - first)}, this->end (), p);
  std::for_each (new_end, end (), [] (reference v) { std::destroy_at (&v); });
  size_ -= static_cast<size_type> (last - first);
  return iterator{p};
}

// insert
// ~~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator const pos, size_type const count,
                                const_reference value) -> iterator {
  assert (this->size () + count < this->max_size () && "Insert will overflow");

  // NOLINTNEXTLINE(misc-misplaced-const)
  iterator const from = this->to_non_const_iterator (pos);
  auto const to = from + count;
  auto const current_end = this->end ();
  assert (current_end >= from && to >= from);

  // NOLINTNEXTLINE(misc-misplaced-const)
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
    this->push_back (value);  // Fast path for appending an element.
    return e;
  }
  return insert (pos, size_type{1}, value);
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator pos, T &&value) -> iterator {
  assert (this->size () < this->max_size () && "Insert will cause overflow");
  // NOLINTNEXTLINE(misc-misplaced-const)
  iterator const e = this->end ();
  // NOLINTNEXTLINE(misc-misplaced-const)
  iterator const r = this->to_non_const_iterator (pos);
  if (r == e) {
    this->emplace_back (std::move (value));
    return r;
  }
  arrayvec_base::move_range (r, e, r + 1);
  size_ += 1;
  *r = std::move (value);  // FIXME: into uninitialized space?
  return r;
}

template <typename T, std::size_t Size>
template <typename Iterator>
PEEJAY_CXX20REQUIRES ((std::input_iterator<Iterator>))
auto arrayvec<T, Size>::insert (const_iterator pos, Iterator first,
                                Iterator last) -> iterator {
  // NOLINTNEXTLINE(misc-misplaced-const)
  iterator const r = this->to_non_const_iterator (pos);
  // NOLINTNEXTLINE(misc-misplaced-const)
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
      assert (n <= this->max_size () - this->size () &&
              "Container size overflow");
      arrayvec_base::move_range (r, e, r + n);
      size_ += static_cast<size_type> (n);
      std::copy (first, last, r);
      return r;
    }

    // TODO(paul): Add an additional optimization path for random-access
    // iterators.
    // 1. Construct any objects in uninitialized space.
    // 2. Move objects from initialized to uninitialized space following the
    // objects
    //    created in step 1.
    // 3. Construct objects in initialized space.
  }

  // A single-pass fallback algorithm for input iterators.
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
  assert (this->size () < this->max_size () &&
          "Cannot emplace(), container is full");
  assert (pos >= this->begin () && pos <= this->end () &&
          "Insert position is invalid");

  iterator const e = this->end ();
  iterator const r = this->to_non_const_iterator (pos);
  if (r == e) {
    this->emplace_back (std::forward<Args> (args)...);
    return r;
  }

  arrayvec_base::move_range (r, e, r + 1);
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
