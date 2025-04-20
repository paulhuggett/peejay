//===- include/peejay/details/pointer_based_iterator.hpp --*- mode: C++ -*-===//
//*              _       _              _                        _  *
//*  _ __   ___ (_)_ __ | |_ ___ _ __  | |__   __ _ ___  ___  __| | *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__| | '_ \ / _` / __|/ _ \/ _` | *
//* | |_) | (_) | | | | | ||  __/ |    | |_) | (_| \__ \  __/ (_| | *
//* | .__/ \___/|_|_| |_|\__\___|_|    |_.__/ \__,_|___/\___|\__,_| *
//* |_|                                                             *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===----------------------------------------------------------------------===//
// Copyright © 2025 Paul Bowen-Huggett
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

/// \file pointer_based_iterator.hpp
/// \brief Provides peejay::pointer_based_iterator: an iterator wrapper for
///   pointers.
///
/// Pointers to an array make perfectly good random access iterators. However
/// there are a few of minor niggles with their usage.
///
/// First, pointers sometimes take the value nullptr to indicate the end of a
/// sequence rather than the "one beyond the end" definition used by iterators.
/// Consider the POSIX readdir() API or a traditional singly-linked list where
/// the last element of the list has a 'next' pointer of nullptr.
///
/// Second, there’s no easy way to portably add debug-time checks to raw
/// pointers. Having a class allows us to sanity-check the value of the pointer
/// relative to the container to which it points.
///
/// Third, our style guide (and clang-tidy) promotes the use of explicit '*'
/// when declaring an auto variable of pointer type. This is good, but if we're
/// declarating the result type of a standard library function that returns an
/// iterator we would prefer to avoid hard-wiring the type as a pointer and
/// instead stick with an abstract 'iterator'.
///
/// The pointer_based_iterator<> class is intended to resolve both of these
/// "problems" by providing a random access iterator wrapper around a pointer.

#ifndef PEEJAY_DETAILS_POINTER_BASED_ITERATOR_HPP
#define PEEJAY_DETAILS_POINTER_BASED_ITERATOR_HPP

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

namespace peejay {

template <typename T> class pointer_based_iterator {
public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::contiguous_iterator_tag;
  using iterator_concept = std::contiguous_iterator_tag;

  explicit constexpr pointer_based_iterator() noexcept = default;
  explicit constexpr pointer_based_iterator(std::nullptr_t) noexcept {}

  /// Copy constructor. Allows for implicit conversion from a regular iterator
  /// to a const_iterator
  // NOLINTNEXTLINE(hicpp-explicit-conversions)
  template <typename Other>
    requires(std::is_const_v<T> && !std::is_const_v<Other>)
  constexpr pointer_based_iterator(pointer_based_iterator<Other> rhs) noexcept : pos_{rhs.operator->()} {}

  template <typename Other>
    requires(std::is_const_v<T> && !std::is_const_v<Other>)
  explicit constexpr pointer_based_iterator(Other const *const pos) noexcept : pos_{pos} {}

  template <typename Other>
    requires(std::is_const_v<T> || std::is_const_v<T> == std::is_const_v<Other>)
  explicit constexpr pointer_based_iterator(Other *const pos) noexcept : pos_{pos} {}

  template <typename Other>
  friend constexpr bool operator==(pointer_based_iterator lhs, pointer_based_iterator<Other> rhs) noexcept {
    return lhs.pos_ == std::to_address(rhs);
  }
  template <typename Other> constexpr pointer_based_iterator &operator=(pointer_based_iterator<Other> rhs) noexcept {
    pos_ = std::to_address(rhs);
    return *this;
  }

  constexpr pointer operator->() const noexcept { return pos_; }
  constexpr reference operator*() const noexcept { return *pos_; }
  constexpr reference operator[](std::size_t const n) const noexcept { return *(pos_ + n); }

  constexpr pointer_based_iterator &operator++() noexcept {
    ++pos_;
    return *this;
  }
  constexpr pointer_based_iterator operator++(int) noexcept {
    auto const prev = *this;
    ++*this;
    return prev;
  }
  constexpr pointer_based_iterator &operator--() noexcept {
    --pos_;
    return *this;
  }
  constexpr pointer_based_iterator operator--(int) noexcept {
    auto const prev = *this;
    --*this;
    return prev;
  }

  template <std::integral U> pointer_based_iterator &operator+=(U const n) noexcept {
    pos_ += n;
    return *this;
  }
  template <std::integral U> pointer_based_iterator &operator-=(U const n) noexcept {
    pos_ -= n;
    return *this;
  }

  template <typename Other>
  friend constexpr auto operator<=>(pointer_based_iterator lhs, pointer_based_iterator<Other> rhs) noexcept {
    return lhs.pos_ <=> &*rhs;
  }

  /// Returns the distance between two iterators \p b - \p a.
  ///
  /// \param b  The first iterator.
  /// \param a  The second iterator.
  /// \returns  distance between two iterators \p b - \p a.
  template <typename Other>
    requires(std::is_same_v<std::remove_const_t<T>, std::remove_const_t<Other>>)
  friend constexpr difference_type operator-(pointer_based_iterator<T> b, pointer_based_iterator<Other> a) noexcept {
    return std::to_address(b) - std::to_address(a);
  }

  /// @{
  /// Move an iterator \p it forwards by distance \p n. \p n can be both positive
  /// or negative.

  /// \param it  The iterator to be moved.
  /// \param n  The distance by which iterator \p it should be moved.
  /// \returns  The new iterator.
  template <std::integral U>
  friend constexpr pointer_based_iterator<T> operator+(pointer_based_iterator<T> const it, U const n) noexcept {
    auto temp = it;
    return temp += n;
  }
  template <std::integral U>
  friend constexpr pointer_based_iterator<T> operator+(U const n, pointer_based_iterator<T> const it) noexcept {
    return it + n;
  }
  /// @}

  /// @{
  /// Move an iterator \p it backwards by distance \p n. \p n can be both positive
  /// or negative.
  /// \param it  The iterator to be moved.
  /// \param n  The distance by which iterator \p it should be moved.
  /// \returns  The new iterator.
  template <std::integral U>
  friend constexpr pointer_based_iterator<T> operator-(pointer_based_iterator<T> const it, U const n) noexcept {
    auto temp = it;
    return temp -= n;
  }
  /// Move an iterator \p it backwards by distance \p n. \p n can be both positive
  /// or negative.
  /// \param it  The iterator to be moved.
  /// \param n  The distance by which iterator \p it should be moved.
  /// \returns  The new iterator.
  template <std::integral U>
  friend constexpr pointer_based_iterator<T> operator-(U const n, pointer_based_iterator<T> const it) noexcept {
    return it - n;
  }
  /// @}
private:
  pointer pos_ = nullptr;
};

template <typename T> pointer_based_iterator(T *) -> pointer_based_iterator<T>;

}  // end namespace peejay

#endif  // PEEJAY_DETAILS_POINTER_BASED_ITERATOR_HPP
