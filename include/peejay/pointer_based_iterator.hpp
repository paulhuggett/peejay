//===- include/peejay/pointer_based_iterator.hpp ----------*- mode: C++ -*-===//
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
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file pointer_based_iterator.hpp
/// \brief Provides peejay::pointer_based_iterator: an iterator wrapper for
///   pointers.
///
/// Pointers to an array make perfectly good random access iterators. However
/// there are a few of minor niggles with their usage.
///
/// First, pointers sometimes take the value nullptr to indicate the end of a
/// sequence. Consider the POSIX readdir() API or a traditional singly-linked
/// list where the last element of the list has a 'next' pointer of nullptr.
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

#ifndef PEEJAY_POINTER_BASED_ITERATOR_HPP
#define PEEJAY_POINTER_BASED_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <version>

#include "peejay/portab.hpp"

namespace peejay {

template <typename T>
class pointer_based_iterator {
public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::random_access_iterator_tag;
#if defined(__cpp_lib_concepts) && __cpp_lib_concepts >= 202207L
  using iterator_concept = std::contiguous_iterator_tag;
#endif  // __cpp_lib_concepts

  explicit constexpr pointer_based_iterator () noexcept = default;
  explicit constexpr pointer_based_iterator (std::nullptr_t) noexcept {}

  /// Copy constructor. Allows for implicit conversion from a regular iterator
  /// to a const_iterator
  // NOLINTNEXTLINE(hicpp-explicit-conversions)
  template <typename OtherType,
            typename = typename std::enable_if_t<std::is_const_v<T> &&
                                                 !std::is_const_v<OtherType>>>
  constexpr pointer_based_iterator (
      pointer_based_iterator<OtherType> rhs) noexcept
      : pos_{rhs.operator->()} {}

  template <typename Other, typename = std::enable_if_t<
                                std::is_const_v<T> && !std::is_const_v<Other>>>
  explicit constexpr pointer_based_iterator (Other const *const pos) noexcept
      : pos_{pos} {}

  template <typename Other, typename = std::enable_if_t<
                                std::is_const_v<T> ||
                                std::is_const_v<T> == std::is_const_v<Other>>>
  explicit constexpr pointer_based_iterator (Other *const pos) noexcept
      : pos_{pos} {}

  template <typename Other>
  constexpr bool operator== (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ == to_address (other);
  }
  template <typename Other>
  constexpr bool operator!= (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ != to_address (other);
  }

  template <typename Other>
  constexpr pointer_based_iterator &operator= (
      pointer_based_iterator<Other> const &other) noexcept {
    pos_ = to_address (other);
    return *this;
  }

  constexpr pointer operator->() const noexcept { return pos_; }
  constexpr reference operator* () const noexcept { return *pos_; }

  constexpr value_type &operator[] (std::size_t const n) noexcept {
    return *(pos_ + n);
  }
  constexpr value_type const &operator[] (std::size_t const n) const noexcept {
    return *(pos_ + n);
  }

  pointer_based_iterator &operator++ () noexcept {
    ++pos_;
    return *this;
  }
  pointer_based_iterator operator++ (int) noexcept {
    auto const prev = *this;
    ++*this;
    return prev;
  }
  pointer_based_iterator &operator-- () noexcept {
    --pos_;
    return *this;
  }
  pointer_based_iterator operator-- (int) noexcept {
    auto const prev = *this;
    --*this;
    return prev;
  }

  template <typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
  pointer_based_iterator &operator+= (U const n) noexcept {
    pos_ += n;
    return *this;
  }
  template <typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
  pointer_based_iterator &operator-= (U const n) noexcept {
    pos_ -= n;
    return *this;
  }

  template <typename Other>
  constexpr bool operator< (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ < &*other;
  }
  template <typename Other>
  constexpr bool operator> (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ > &*other;
  }
  template <typename Other>
  constexpr bool operator<= (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ <= &*other;
  }
  template <typename Other>
  constexpr bool operator>= (
      pointer_based_iterator<Other> const &other) const noexcept {
    return pos_ >= &*other;
  }

private:
  pointer pos_ = nullptr;
};

/// Move an iterator \p i forwards by distance \p n. \p n can be both positive
/// or negative.
///
/// \param i  The iterator to be moved.
/// \param n  The distance by which iterator \p i should be moved.
/// \returns  The new iterator.
template <typename T, typename U,
          typename = std::enable_if_t<std::is_integral_v<U>>>
inline pointer_based_iterator<T> operator+ (pointer_based_iterator<T> const i,
                                            U const n) noexcept {
  auto temp = i;
  return temp += n;
}

/// Move an iterator \p i forwards by distance \p n. \p n can be both positive
/// or negative.
///
/// \param i  The iterator to be moved.
/// \param n  The distance by which iterator \p i should be moved.
/// \returns  The new iterator.
template <typename T, typename U,
          typename = std::enable_if_t<std::is_integral_v<U>>>
inline pointer_based_iterator<T> operator+ (
    U const n, pointer_based_iterator<T> const i) noexcept {
  auto temp = i;
  return temp += n;
}

/// Move an iterator \p i backwards by distance \p n. \p n can be both positive
/// or negative.
///
/// \param i  The iterator to be moved.
/// \param n  The distance by which iterator \p i should be moved.
/// \returns  The new iterator.
template <typename T, typename U,
          typename = std::enable_if_t<std::is_integral_v<U>>>
inline pointer_based_iterator<T> operator- (pointer_based_iterator<T> const i,
                                            U const n) noexcept {
  auto temp = i;
  return temp -= n;
}

/// Move an iterator \p i backwards by distance \p n. \p n can be both positive
/// or negative.
///
/// \param i  The iterator to be moved.
/// \param n  The distance by which iterator \p i should be moved.
/// \returns  The new iterator.
template <typename T, typename U,
          typename = std::enable_if_t<std::is_integral_v<U>>>
inline pointer_based_iterator<T> operator- (
    U const n, pointer_based_iterator<T> const i) noexcept {
  auto temp = i;
  return temp -= n;
}

/// Returns the distance between two iterators \p b - \p a.
///
/// \param b  The first iterator.
/// \param a  The second iterator.
/// \returns  distance between two iterators \p b - \p a.
template <typename LhsT, typename RhsT,
          typename = std::enable_if_t<std::is_same_v<
              std::remove_const_t<LhsT>, std::remove_const_t<RhsT>>>>
constexpr typename pointer_based_iterator<LhsT>::difference_type operator- (
    pointer_based_iterator<LhsT> b, pointer_based_iterator<RhsT> a) noexcept {
  return to_address (b) - to_address (a);
}

template <typename T>
pointer_based_iterator (T *) -> pointer_based_iterator<T>;

}  // end namespace peejay

#endif  // PEEJAY_POINTER_BASED_ITERATOR_HPP
