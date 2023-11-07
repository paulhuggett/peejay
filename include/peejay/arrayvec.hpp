//===- include/peejay/arrayvec.hpp ------------------------*- mode: C++ -*-===//
//*                                             *
//*   __ _ _ __ _ __ __ _ _   ___   _____  ___  *
//*  / _` | '__| '__/ _` | | | \ \ / / _ \/ __| *
//* | (_| | |  | | | (_| | |_| |\ V /  __/ (__  *
//*  \__,_|_|  |_|  \__,_|\__, | \_/ \___|\___| *
//*                       |___/                 *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file arrayvec.hpp
/// \brief Provides a sequence container, arrayvec<>. that encapsulates
///   dynamically sized arrays within a fixed size container.
#ifndef PEEJAY_ARRAYVEC_HPP
#define PEEJAY_ARRAYVEC_HPP

#include <array>
#include <initializer_list>

#include "peejay/avbase.hpp"
#include "peejay/uinteger.hpp"

#if PEEJAY_CXX20 && defined(__has_include) && __has_include(<bit>)
#include <bit>
#endif

namespace peejay {

//*                                     *
//*  __ _ _ _ _ _ __ _ _  ___ _____ __  *
//* / _` | '_| '_/ _` | || \ V / -_) _| *
//* \__,_|_| |_| \__,_|\_, |\_/\___\__| *
//*                    |__/             *
/// \brief A sequence container that encapsulates dynamic size arrays
///   within a fixed size container.
///
/// The elements of an arrayvec are stored within the body of the object
/// itself, in constrast to std::vector<> which maintains a heap-allocated
/// buffer to store its contents.
///
/// The elements are stored contiguously, which means that elements can be
/// accessed not only through iterators, but also using offsets to regular
/// pointers to elements. This means that a pointer to an element of an
/// arrayvec may be passed to any function that expects a pointer to an element
/// of an array.
///
/// The storage of the arrayvec is a fixed-size array contained within the body
/// of the object.
///
/// Most member functions offer the basic exception guarantee. Some functions
/// have support the strong exception guarantee to leave the object in its
/// original state in the event of an exception being thrown.
template <typename T, std::size_t Size = 256 / sizeof (T)>
class arrayvec : public details::avbase<T> {
public:
  /// The type of an element in this container.
  using value_type = T;

  /// A reference to arrayvec::value_type.
  using reference = value_type &;
  /// A constant reference to arrayvec::value_type.
  using const_reference = value_type const &;
  /// A pointer to arrayvec::value_type.
  using pointer = value_type *;
  /// A pointer to constant arrayvec::value_type.
  using const_pointer = value_type const *;

  /// \brief An unsigned integer type which is sufficiently wide to hold a maximum
  /// value of \p Size.
  using size_type = uinteger_t<bits_required (Size)>;
  /// \brief A signed integer typed which can represent the difference between the
  /// addresses of any two elements in the container.
  using difference_type = std::ptrdiff_t;

  /// A random-access and contiguous iterator to value_type.
  using iterator = pointer_based_iterator<value_type>;
  /// A random-access and contiguous iterator to const value_type.
  using const_iterator = pointer_based_iterator<value_type const>;
  /// std::reverse_iterator<iterator>
  using reverse_iterator = std::reverse_iterator<iterator>;
  /// std::reverse_iterator<const_iterator>
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  /// Constructs the container with an initial size of 0.
  arrayvec () noexcept;
  /// \brief Constructs the container with the contents of the initializer list
  ///   \p init
  ///
  /// \param init  An initializer list with which to initialize the elements of
  ///   the container.
  arrayvec (std::initializer_list<T> init)
      : arrayvec (std::begin (init), std::end (init)) {}
  /// \brief Constructs the container with the contents of the range [\p first,
  ///   \p last).
  ///
  /// \tparam InputIterator  A type which satisfies std::input_iterator<>.
  /// \param first  The start of the range from which to copy the elements.
  /// \param last  The end of the range from which to copy the elements.
  template <typename InputIterator,
            typename = std::enable_if_t<input_iterator<InputIterator>>>
  arrayvec (InputIterator first, InputIterator last);

  /// \brief Constructs the container with \p count default-inserted instances
  ///   of T.
  ///
  /// No copies are made.
  ///
  /// \param count  The number of elements to be initialized. This must be less
  ///   than or equal to max_size().
  explicit arrayvec (size_type count);

  /// \brief Constructs the container with \p count copies of elements with
  ///   value \p value.
  ///
  /// \param count  The number of elements to be initialized. This must be less
  ///   than or equal to max_size().
  /// \param value  The value with which to initialize elements of the
  ///   container.
  arrayvec (size_type count, const_reference value);

  arrayvec (arrayvec const &other)
      : arrayvec (std::begin (other), std::end (other)) {}
  arrayvec (arrayvec &&other) noexcept (
      std::is_nothrow_move_constructible_v<T>);

  ~arrayvec () noexcept { this->clear (); }

  template <std::size_t OtherSize>
  arrayvec &operator= (arrayvec<T, OtherSize> const &other);
  template <std::size_t OtherSize>
  arrayvec &operator= (arrayvec<T, OtherSize> &&other) noexcept (
      std::is_nothrow_move_constructible_v<T>
          &&std::is_nothrow_move_assignable_v<T>);

  arrayvec &operator= (arrayvec const &other);
  arrayvec &operator= (arrayvec &&other) noexcept (
      std::is_nothrow_move_constructible_v<T>
          &&std::is_nothrow_move_assignable_v<T>);

  /// \name Element access
  ///@{

  /// Direct access to the underlying array.
  constexpr T const *data () const noexcept {
    return pointer_cast<T const> (data_.data ());
  }
  /// Direct access to the underlying array.
  constexpr T *data () noexcept { return pointer_cast<T> (data_.data ()); }

  /// \brief Access the specified element.
  ///
  /// Returns a reference to the element at specified location \p n. No bounds
  /// checking is performed.
  ///
  /// \param n  Position of the element to return.
  /// \returns  A reference to the requested element.
  constexpr const_reference operator[] (size_type n) const noexcept {
    assert (n < this->size ());
    return *(this->data () + n);
  }
  /// \brief Access the specified element.
  ///
  /// Returns a reference to the element at specified location \p n. No bounds
  /// checking is performed.
  ///
  /// \param n  Position of the element to return.
  /// \returns  A reference to the requested element.
  constexpr reference operator[] (size_type n) noexcept {
    assert (n < this->size ());
    return *(this->data () + n);
  }

  /// \brief Returns a reference to the element at specified location \p pos,
  ///   with bounds checking.
  ///
  /// If pos is not within the range of the container, an exception of type
  /// std::out_of_range is thrown.
  ///
  /// \param pos  Position of the element to return.
  /// \returns  A reference to the requested element.
  reference at (size_type pos);
  /// \brief Returns a reference to the element at specified location \p pos,
  ///   with bounds checking.
  ///
  /// If pos is not within the range of the container, an exception of type
  /// std::out_of_range is thrown.
  ///
  /// \param pos  Position of the element to return.
  /// \returns  A reference to the requested element.
  const_reference at (size_type pos) const;

  /// \brief Access the first element.
  ///
  /// The effect of calling front() on a zero-sized arrayvec is undefined.
  constexpr reference front () noexcept;
  /// \brief Access the first element.
  ///
  /// The effect of calling front() on a zero-sized arrayvec is undefined.
  constexpr const_reference front () const noexcept;
  /// \brief Access the last element.
  ///
  /// The effect of calling back() on a zero-sized arrayvec is undefined.
  constexpr reference back () noexcept;
  /// \brief Access the last element.
  ///
  /// The effect of calling back() on a zero-sized arrayvec is undefined.
  constexpr const_reference back () const noexcept;

  ///@}

  /// \name Capacity
  ///@{

  /// Returns the number of elements held by the container.
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

  /// \brief Resizes the container to contain \p count elements.
  ///
  /// If the current size is less than \p count, the container is reduced to
  /// its first \p count elements. If the current size is less than \p count,
  /// additional default-inserted elements are appended.
  ///
  /// If an exception is thrown, this function has no effect (strong exception
  /// guarantee).
  ///
  /// \param count  The new number of elements in the container. Must be less
  ///               than or equal to Size.
  void resize (size_type count);

  /// \brief Resizes the container to contain count elements.
  ///
  /// If the current size is greater than \p count, the container is reduced to
  /// its first \p count elements. If the current size is less than \p count,
  /// additional copies of \p value are appended.
  ///
  /// If an exception is thrown, this function has no effect (strong exception
  /// guarantee).
  ///
  /// \param count  The new number of elements in the container. Must be less
  ///               than or equal to Size.
  /// \param value  The value with which to initialize the new elements.
  void resize (size_type count, const_reference value);

  ///@}

  /// \name Iterators
  ///@{

  /// \brief Returns an iterator to the beginning of the container.
  ///
  /// If the arrayvec is empty, the returned iterator will be equal to end().
  [[nodiscard]] constexpr iterator begin () noexcept {
    return iterator{this->data ()};
  }
  /// \brief Returns an iterator to the beginning of the container.
  ///
  /// If the arrayvec is empty, the returned iterator will be equal to end().
  [[nodiscard]] constexpr const_iterator begin () const noexcept {
    return const_iterator{this->data ()};
  }
  /// \brief Returns an iterator to the beginning of the container.
  ///
  /// If the arrayvec is empty, the returned iterator will be equal to end().
  [[nodiscard]] constexpr const_iterator cbegin () const noexcept {
    return const_iterator{this->data ()};
  }
  /// Returns a reverse iterator to the beginning of the container.
  [[nodiscard]] constexpr reverse_iterator rbegin () noexcept {
    return reverse_iterator{iterator{this->end ()}};
  }
  /// Returns a reverse iterator to the beginning of the container.
  [[nodiscard]] constexpr const_reverse_iterator rbegin () const noexcept {
    return rcbegin ();
  }
  /// Returns a reverse iterator to the beginning of the container.
  [[nodiscard]] constexpr const_reverse_iterator rcbegin () const noexcept {
    return const_reverse_iterator{this->end ()};
  }

  /// Returns an iterator to the end of the container.
  [[nodiscard]] constexpr iterator end () noexcept {
    return iterator{this->begin () + size_};
  }
  /// Returns an iterator to the end of the container.
  [[nodiscard]] constexpr const_iterator end () const noexcept {
    return cend ();
  }
  /// Returns an iterator to the end of the container.
  [[nodiscard]] constexpr const_iterator cend () const noexcept {
    return this->begin () + this->size ();
  }
  /// Returns a reverse iterator to the end of the container.
  [[nodiscard]] constexpr reverse_iterator rend () noexcept {
    return reverse_iterator{this->begin ()};
  }
  /// Returns a reverse iterator to the end of the container.
  [[nodiscard]] constexpr const_reverse_iterator rend () const noexcept {
    return rcend ();
  }
  /// Returns a reverse iterator to the end of the container.
  [[nodiscard]] constexpr const_reverse_iterator rcend () const noexcept {
    return const_reverse_iterator{this->begin ()};
  }
  ///@}

  /// \name Modifiers
  ///@{

  /// \brief Removes all elements from the container.
  ///
  /// Invalidates any references, pointers, or iterators referring to contained
  /// elements. Any past-the-end iterators are also invalidated.
  void clear () noexcept;

  /// \brief Adds an element to the end of the container.
  ///
  /// The new element is initialized as a copy of \p value.
  ///
  /// If an exception is thrown, this function has no effect (strong exception
  /// guarantee).
  ///
  /// \param value  The value of the element to append.
  void push_back (const_reference value) noexcept (
      std::is_nothrow_copy_constructible_v<T>);
  /// \brief Adds an element to the end of the container.
  ///
  /// \p value is moved into the new element.
  ///
  /// If an exception is thrown, this function has no effect (strong exception
  /// guarantee).
  ///
  /// \param value  The value of the element to append.
  void push_back (T &&value) noexcept (std::is_nothrow_move_constructible_v<T>);
  /// \brief Appends a new element to the end of the container.
  ///
  /// The arguments \p args... are forwarded to the constructor as
  /// `std::forward<Args>(args)....`.
  ///
  /// If an exception is thrown, this function has no effect (strong exception
  /// guarantee).
  ///
  /// \param args	 Arguments to forward to the constructor of the element.
  template <typename... Args>
  reference emplace_back (Args &&...args);

  /// \brief Replaces the contents with \p count copies of value \p value.
  ///
  /// \param count The new size of the container.
  /// \param value The value with which to initialize elements of the container.
  void assign (size_type count, const_reference value);
  /// \brief Replaces the contents with copies of those in the range [first,
  ///   last).
  ///
  /// The behavior is undefined if either argument is an iterator into *this.
  ///
  /// \tparam InputIterator  An iterator which will produce the elements to be
  ///   inserted. Must meet the requirements of LegacyInputIterator.
  /// \param first  The first of the range from which elements are to be copied.
  /// \param last  The last of the range from which elements are to be copied.
  template <typename InputIterator,
            typename = std::enable_if_t<input_iterator<InputIterator>>>
  void assign (InputIterator first, InputIterator last);
  /// \brief Replaces the contents with the elements from the initializer
  ///   list \p ilist
  ///
  /// \param ilist Initializer list from which elements are to be copied.
  void assign (std::initializer_list<T> ilist) {
    this->assign (std::begin (ilist), std::end (ilist));
  }

  /// \brief Removes the last element of the container.
  ///
  /// Calling pop_back() on an empty container results in undefined behavior.
  /// Iterators and references to the last element, as well as the end()
  /// iterator, are invalidated.
  void pop_back () {
    assert (size_ > 0U && "Attempt to use pop_back() with an empty container");
    --size_;
    std::destroy_at (to_address (this->end ()));
  }

  /// \brief Inserts \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the new element.
  iterator insert (const_iterator pos, const_reference value);
  /// \brief Inserts \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the new element.
  iterator insert (const_iterator pos, T &&value) noexcept (
      std::is_nothrow_move_constructible_v<T>
          &&std::is_nothrow_move_assignable_v<T>);

  /// \brief Inserts \p count copies of \p value before \p pos.
  ///
  /// \param pos  Iterator before which the new elements will be constructed.
  /// \param count  The number of copies to be inserted.
  /// \param value  Element value to insert.
  /// \returns  An iterator pointing to the first of the new elements or \p pos
  ///   if \p count == 0.
  iterator insert (const_iterator pos, size_type count, const_reference value);
  /// \brief Inserts elements from range [\p first, \p last) before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param first  The beginning of the range of elements to be inserted.
  ///   Cannot be an iterator into the container for which insert() is called.
  /// \param last  The end of the range of elements to be inserted. Cannot
  ///   be an iterator into the container for which insert() is called.
  /// \returns  Iterator pointing to the first element inserted, or \p pos
  ///   if \p first == \p last.
  template <typename InputIterator,
            typename = std::enable_if_t<input_iterator<InputIterator>>>
  iterator insert (const_iterator pos, InputIterator first, InputIterator last);
  /// \brief Inserts elements from the initializer list \p ilist before \p pos.
  ///
  /// \param pos  The Iterator before which the new elements will be constructed.
  /// \param ilist  An initializer list from which to to insert the values.
  /// \returns  An iterator pointing to the first element inserted, or \p pos
  ///   if \p ilist is empty.
  iterator insert (const_iterator pos, std::initializer_list<T> ilist) {
    return insert (pos, std::begin (ilist), std::end (ilist));
  }

  /// \brief Inserts a new element into the container directly before \p pos.
  ///
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param args Arguments to be forwarded to the element's constructor.
  /// \returns  An iterator pointing to the emplaced element.
  template <typename... Args>
  iterator emplace (const_iterator pos, Args &&...args);

  /// \brief Erases the specified element from the container.
  ///
  /// Invalidates iterators and references at or after the point of the erase,
  /// including the end() iterator.
  ///
  /// \param pos  Iterator to the element to remove.
  /// \returns  Iterator following the last removed element. If \p pos refers
  ///   to the last element, then the end() iterator is returned.
  iterator erase (const_iterator pos);
  /// \brief Erases the elements in the range [\p first, \p last).
  ///
  /// Invalidates iterators and references at or after the point of the erase,
  /// including the end() iterator.
  ///
  /// \param first  The first of the range of elements to remove.
  /// \param last  The last of the range of elements to remove.
  /// \returns Iterator following the last removed element. If last == end()
  ///   prior to removal, then the updated end() iterator is returned. If
  ///   [\p first, \p last) is an empty range, then last is returned.
  iterator erase (const_iterator first, const_iterator last);

  ///@}

private:
  void flood () noexcept {
#ifndef NDEBUG
    T *const p = this->data ();
    std::fill (pointer_cast<std::byte> (p + this->size ()),
               pointer_cast<std::byte> (p + this->max_size ()),
               std::byte{0xFF});
#endif
  }

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
arrayvec<T, Size>::arrayvec () noexcept {
  this->flood ();
}

template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (arrayvec &&other) noexcept (
    std::is_nothrow_move_constructible_v<T>) {
  this->flood ();
  auto *dest = this->data ();
  for (auto src = other.begin (), src_end = other.end (); src != src_end;
       ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
    ++size_;
  }
}

template <typename T, std::size_t Size>
template <typename InputIterator, typename>
arrayvec<T, Size>::arrayvec (InputIterator first, InputIterator last) {
  this->flood ();
  details::avbase<T>::init (this->begin (), &size_, first, last);
}

template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (size_type count) {
  this->flood ();
  assert (count <= Size);
  details::avbase<T>::init (this->begin (), &size_, count);
}

template <typename T, std::size_t Size>
arrayvec<T, Size>::arrayvec (size_type count, const_reference value) {
  this->flood ();
  assert (count <= Size);
  details::avbase<T>::init (this->begin (), &size_, count, value);
}

// operator=
// ~~~~~~~~~
template <typename T, std::size_t Size>
template <std::size_t OtherSize>
auto arrayvec<T, Size>::operator= (arrayvec<T, OtherSize> const &other)
    -> arrayvec & {
  details::avbase<T>::template operator_assign<false> (
      this->data (), &size_,
      std::make_pair (other.data (), static_cast<std::size_t> (other.size ())));
  assert (size_ <= this->max_size ());
  return *this;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::operator= (arrayvec const &other) -> arrayvec & {
  if (&other != this) {
    this->operator=<Size> (other);
  }
  return *this;
}

template <typename T, std::size_t Size>
template <std::size_t OtherSize>
auto arrayvec<T, Size>::operator= (arrayvec<T, OtherSize> &&other) noexcept (
    std::is_nothrow_move_constructible_v<T>
        &&std::is_nothrow_move_assignable_v<T>) -> arrayvec & {
  details::avbase<T>::template operator_assign<true> (
      this->data (), &size_,
      std::make_pair (other.data (), static_cast<std::size_t> (other.size ())));
  assert (size_ <= this->max_size ());
  return *this;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::operator= (arrayvec &&other) noexcept (
    std::is_nothrow_move_constructible_v<T>
        &&std::is_nothrow_move_assignable_v<T>) -> arrayvec & {
  if (&other != this) {
    details::avbase<T>::template operator_assign<true> (
        this->data (), &size_,
        std::make_pair (other.data (),
                        static_cast<std::size_t> (other.size ())));
    assert (size_ <= this->max_size ());
  }
  return *this;
}

// at
// ~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::at (size_type pos) -> reference {
  if (pos >= this->size ()) {
    details::avbase<T>::raise_out_of_range ();
  }
  return (*this)[pos];
}
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::at (size_type pos) const -> const_reference {
  if (pos >= this->size ()) {
    details::avbase<T>::raise_out_of_range ();
  }
  return (*this)[pos];
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
  auto const b = this->begin ();
  return iterator{b + (pos - b)};
}

// clear
// ~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::clear () noexcept {
  details::avbase<T>::clear (&size_, this->begin (), this->end ());
}

// resize
// ~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
void arrayvec<T, Size>::resize_impl (size_type count, Args &&...args) {
  assert (count <= this->max_size ());
  // Wierd () around the call to std::min() to avoid clashing with the MSVC
  // 'min' macro.
  details::avbase<T>::resize (this->begin (), &size_,
                              std::min (count, this->max_size ()),
                              std::forward<Args> (args)...);
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
void arrayvec<T, Size>::push_back (const_reference value) noexcept (
    std::is_nothrow_copy_constructible_v<T>) {
  assert (this->size () < this->max_size ());
  construct_at (to_address (this->end ()), value);
  ++size_;
}

template <typename T, std::size_t Size>
void arrayvec<T, Size>::push_back (T &&value) noexcept (
    std::is_nothrow_move_constructible_v<T>) {
  assert (this->size () < this->max_size ());
  construct_at (to_address (this->end ()), std::move (value));
  ++size_;
}

// emplace back
// ~~~~~~~~~~~~
template <typename T, std::size_t Size>
template <typename... Args>
auto arrayvec<T, Size>::emplace_back (Args &&...args) -> reference {
  assert (this->size () < this->max_size ());
  auto pos = this->end ();
  construct_at (to_address (pos), std::forward<Args> (args)...);
  ++size_;
  return *pos;
}

// assign
// ~~~~~~
template <typename T, std::size_t Size>
void arrayvec<T, Size>::assign (size_type count, const_reference value) {
  assert (count <= this->max_size ());
  details::avbase<T>::assign (this->begin (), &size_, count, value);
}

template <typename T, std::size_t Size>
template <typename InputIterator, typename>
void arrayvec<T, Size>::assign (InputIterator first, InputIterator last) {
  // TODO(paul): this would be better done in a single pass.
  this->clear ();
  this->insert (this->end (), first, last);
}

// erase
// ~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator pos) -> iterator {
  assert (pos >= this->begin () && pos < this->cend () &&
          "erase() iterator pos is invalid");
  auto const p = this->to_non_const_iterator (pos);
  size_ = static_cast<size_type> (
      details::avbase<T>::erase (p, this->end (), size_));
  this->flood ();
  return p;
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::erase (const_iterator first, const_iterator last)
    -> iterator {
  assert (first >= this->begin () && first <= last && last <= this->cend () &&
          "Iterator range to erase() is invalid");
  auto const f = this->to_non_const_iterator (first);
  size_ = static_cast<size_type> (details::avbase<T>::erase (
      f, this->to_non_const_iterator (last), this->end (), size_));
  this->flood ();
  return f;
}

// insert
// ~~~~~~
template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator const pos, size_type const count,
                                const_reference value) -> iterator {
  assert (this->size () + count <= this->max_size () && "Insert will overflow");
  return details::avbase<T>::insert (
      this->begin (), &size_, this->to_non_const_iterator (pos), count, value);
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator pos, const_reference value)
    -> iterator {
  assert (this->size () < this->max_size () && "Insert will overflow");
  assert (pos >= this->begin () && pos <= this->end () &&
          "Insert position is invalid");
  if (auto const e = this->end (); pos == e) {
    this->push_back (value);  // Fast path for appending an element.
    return e;
  }
  return this->insert (pos, size_type{1}, value);
}

template <typename T, std::size_t Size>
auto arrayvec<T, Size>::insert (const_iterator pos, T &&value) noexcept (
    std::is_nothrow_move_constructible_v<T>
        &&std::is_nothrow_move_assignable_v<T>) -> iterator {
  assert (this->size () < this->max_size () && "Insert will cause overflow");
  assert (pos >= this->begin () && pos <= this->end () &&
          "Insert position is invalid");
  static_assert (std::is_move_constructible_v<T>,
                 "T must be move-constructible");
  return details::avbase<T>::insert (this->begin (), &size_,
                                     this->to_non_const_iterator (pos),
                                     std::move (value));
}

template <typename T, std::size_t Size>
template <typename InputIterator, typename>
auto arrayvec<T, Size>::insert (const_iterator pos, InputIterator first,
                                InputIterator last) -> iterator {
  assert (pos >= this->begin () && pos <= this->end () &&
          "Insert position is invalid");
  return details::avbase<T>::insert (
      this->begin (), &size_, this->to_non_const_iterator (pos), first, last);
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
  return details::avbase<T>::emplace (this->end (), &size_,
                                      this->to_non_const_iterator (pos),
                                      std::forward<Args> (args)...);
}

}  // end namespace peejay

#endif  // PEEJAY_ARRAYVEC_HPP
