//===- include/peejay/details/avbase.hpp ------------------*- mode: C++ -*-===//
//*              _                     *
//*   __ ___   _| |__   __ _ ___  ___  *
//*  / _` \ \ / / '_ \ / _` / __|/ _ \ *
//* | (_| |\ V /| |_) | (_| \__ \  __/ *
//*  \__,_| \_/ |_.__/ \__,_|___/\___| *
//*                                    *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#ifndef PEEJAY_DETAILS_AVBASE_HPP
#define PEEJAY_DETAILS_AVBASE_HPP

#include <algorithm>
#include <cassert>
#include <limits>

#include "peejay/pointer_based_iterator.hpp"
#include "peejay/portab.hpp"

namespace peejay::details {

//*           _                   *
//*  __ ___ _| |__  __ _ ___ ___  *
//* / _` \ V / '_ \/ _` (_-</ -_) *
//* \__,_|\_/|_.__/\__,_/__/\___| *
//*                               *
template <typename T>
class avbase {
protected:
  template <typename SizeType, typename InputIterator,
            typename = std::enable_if_t<input_iterator<InputIterator>>>
  static void init (pointer_based_iterator<T> begin, SizeType *size,
                    InputIterator first, InputIterator last);

  template <typename SizeType>
  static void init (pointer_based_iterator<T> begin, SizeType *size,
                    SizeType count);
  template <typename SizeType>
  static void init (pointer_based_iterator<T> begin, SizeType *size,
                    SizeType count, T const &value);

  template <bool IsMove, typename SizeType, typename SrcType>
  static void operator_assign (
      T *destp, SizeType *destsize,
      std::pair<SrcType *, std::size_t> const &src) noexcept (IsMove);

  template <typename SizeType>
  static void clear (SizeType *size, pointer_based_iterator<T> first,
                     pointer_based_iterator<T> last) noexcept;

  template <typename SizeType, typename... Args>
  static void resize (pointer_based_iterator<T> begin, SizeType *size,
                      std::size_t new_size, Args &&...args);

  template <typename SizeType>
  static void assign (pointer_based_iterator<T> begin, SizeType *size,
                      std::size_t count, T const &value);

  static void move_range (pointer_based_iterator<T> from,
                          pointer_based_iterator<T> end,
                          pointer_based_iterator<T> to) noexcept;

  static std::size_t erase (pointer_based_iterator<T> pos,
                            pointer_based_iterator<T> end, std::size_t size);

  static std::size_t erase (pointer_based_iterator<T> first,
                            pointer_based_iterator<T> last,
                            pointer_based_iterator<T> end, std::size_t size);

  /// \param begin  The start of the array data.
  /// \param size  On entry, the original size of the container. On exit, the new container size.
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param value  Element value to insert.
  template <typename SizeType>
  static pointer_based_iterator<T> insert (pointer_based_iterator<T> begin,
                                           SizeType *size,
                                           pointer_based_iterator<T> pos,
                                           T &&value);

  /// \tparam SizeType  The type of the array-vector size value.
  /// \tparam InputIterator  A type which satisfies std::input_iterator<>.
  /// \param begin  The start of the array data.
  /// \param size  On entry, the original size of the container. On exit, the new container size.
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param first  The start of the range from which to copy the elements.
  /// \param last  The end of the range from which to copy the elements.
  template <typename SizeType, typename InputIterator,
            typename = std::enable_if_t<input_iterator<InputIterator>>>
  static pointer_based_iterator<T> insert (pointer_based_iterator<T> begin,
                                           SizeType *size,
                                           pointer_based_iterator<T> pos,
                                           InputIterator first,
                                           InputIterator last);

  /// \param data  The start of the array data.
  /// \param size  On entry, the original size of the container. On exit, the new container size.
  /// \param pos  Iterator before which the new element will be constructed.
  /// \param count The number of copies to insert.
  /// \param value  Element value to insert.
  template <typename SizeType>
  static pointer_based_iterator<T> insert (pointer_based_iterator<T> data,
                                           SizeType *size,
                                           pointer_based_iterator<T> pos,
                                           SizeType count, T const &value);

  template <typename SizeType, typename... Args>
  static pointer_based_iterator<T> emplace (pointer_based_iterator<T> end,
                                            SizeType *size,
                                            pointer_based_iterator<T> pos,
                                            Args &&...args) {
    if (pos == end) {
      construct_at (to_address (pos), std::forward<Args> (args)...);
      ++(*size);
      return pos;
    }

    if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
      avbase::move_range (pos, end, pos + 1);
      // After destroying the object at 'pos', we can construct in-place.
      auto *const p = to_address (pos);
      std::destroy_at (p);
      construct_at (p, std::forward<Args> (args)...);
    } else {
      // Constructing the object might throw and the location is already
      // occupied by an element so we first make a temporary instance then
      // move-assign it to the required location.
      T new_element{std::forward<Args> (args)...};
      // Make space for the new element.
      avbase::move_range (pos, end, pos + 1);
      *pos = std::move (new_element);
    }
    *size += 1;
    return pos;
  }
};

// init
// ~~~~
template <typename T>
template <typename SizeType, typename InputIterator, typename>
void avbase<T>::init (pointer_based_iterator<T> begin, SizeType *const size,
                      InputIterator first, InputIterator last) {
  auto out = begin;
  try {
    for (; first != last; ++first) {
      construct_at (to_address (out), *first);
      ++(*size);
      ++out;
    }
  } catch (...) {
    avbase<T>::clear (size, begin, begin + *size);
    throw;
  }
}

template <typename T>
template <typename SizeType>
void avbase<T>::init (pointer_based_iterator<T> begin, SizeType *const size,
                      SizeType count, T const &value) {
  *size = 0;
  try {
    for (auto it = begin, end = begin + count; it != end; ++it) {
      construct_at (to_address (it), value);
      ++(*size);
    }
  } catch (...) {
    avbase<T>::clear (size, begin, begin + *size);
    assert (*size == 0);
    throw;
  }
}

template <typename T>
template <typename SizeType>
void avbase<T>::init (pointer_based_iterator<T> begin, SizeType *const size,
                      SizeType count) {
  avbase<T>::init (begin, size, count, T{});
}

// operator assign
// ~~~~~~~~~~~~~~~
template <typename T>
template <bool IsMove, typename SizeType, typename SrcType>
void avbase<T>::operator_assign (
    T *const destp, SizeType *const destsize,
    std::pair<SrcType *, std::size_t> const &src) noexcept (IsMove) {
  auto *src_ptr = src.first;
  auto *dest_ptr = destp;
  auto *const old_dest_end = destp + *destsize;

  // Step 1: where both source and destination arrays have constructed members
  // we can just use assignment.
  // [ Wierd () to avoid std::min() clashing with the MSVC min macros. ]
  auto *const uninit_end =
      dest_ptr + std::min (static_cast<std::size_t> (*destsize), src.second);
  for (; dest_ptr < uninit_end; ++src_ptr, ++dest_ptr) {
    if constexpr (IsMove) {
      *dest_ptr = std::move (*src_ptr);
    } else {
      *dest_ptr = *src_ptr;
    }
  }
  // Step 2: target memory does not contain constructed members.
  for (; dest_ptr < destp + src.second; ++src_ptr, ++dest_ptr) {
    if constexpr (IsMove) {
      construct_at (dest_ptr, std::move (*src_ptr));
    } else {
      construct_at (dest_ptr, *src_ptr);
    }
    ++(*destsize);
  }
  // Step 3: The 'other' array is shorter than this object so release any extra
  // members.
  for (; dest_ptr < old_dest_end; ++dest_ptr) {
    std::destroy_at (dest_ptr);
    --(*destsize);
  }
}

// clear
// ~~~~~
template <typename T>
template <typename SizeType>
void avbase<T>::clear (SizeType *const size,
                       pointer_based_iterator<T> const first,
                       pointer_based_iterator<T> const last) noexcept {
  auto n = SizeType{0};
  std::for_each (first, last, [&] (auto &value) {
    std::destroy_at (&value);
    ++n;
  });
  *size -= n;
}

// resize
// ~~~~~~
template <typename T>
template <typename SizeType, typename... Args>
void avbase<T>::resize (pointer_based_iterator<T> const begin,
                        SizeType *const size, std::size_t const new_size,
                        Args &&...args) {
  auto const end = begin + *size;
  auto const new_end = begin + new_size;
  if (new_size < *size) {
    avbase::clear (size, new_end, end);
    return;
  }
  auto it = end;
  try {
    for (; it != new_end; ++it) {
      construct_at (to_address (it), std::forward<Args> (args)...);
      (*size)++;
    }
  } catch (...) {
    // strong exception guarantee means removing any objects that we
    // constructed.
    avbase::clear (size, end, it);
    throw;
  }
}

// assign
// ~~~~~~
template <typename T>
template <typename SizeType>
void avbase<T>::assign (pointer_based_iterator<T> begin, SizeType *const size,
                        std::size_t count, T const &value) {
  auto const end_actual = begin + *size;
  auto const end_desired = begin + count;
  auto const end_inited = std::min (end_desired, end_actual);

  std::fill (begin, end_inited, value);
  if (end_desired < end_actual) {
    *size = static_cast<SizeType> (
        avbase::erase (end_desired, end_actual, end_actual, *size));
    return;
  }
  for (auto pos = end_actual; pos < end_desired; ++pos) {
    construct_at (to_address (pos), value);
    ++(*size);
  }
}

// move range
// ~~~~~~~~~~
template <typename T>
void avbase<T>::move_range (pointer_based_iterator<T> const from,
                            pointer_based_iterator<T> const end,
                            pointer_based_iterator<T> const to) noexcept {
  assert (end >= from && to >= from);
  auto const dist = to - from;  // how far to move.
  auto const new_end = end + dist;
  auto const num_to_move = end - from;
  auto const num_uninit =
      std::min (num_to_move, new_end - end);  // the number past the end.

  auto dest = new_end - num_uninit;
  for (auto src = end - num_uninit; src < end; ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
  }
  std::move_backward (from, from + num_to_move - num_uninit,
                      new_end - num_uninit);
}

// erase
// ~~~~~
template <typename T>
std::size_t avbase<T>::erase (pointer_based_iterator<T> const pos,
                              pointer_based_iterator<T> const end,
                              std::size_t const size) {
  std::move (pos + 1, end, pos);
  std::destroy_at (&*(end - 1));
  return size - 1;
}

template <typename T>
std::size_t avbase<T>::erase (pointer_based_iterator<T> const first,
                              pointer_based_iterator<T> const last,
                              pointer_based_iterator<T> const end,
                              std::size_t const size) {
  assert (first <= last && last <= end);
  auto const delta = static_cast<std::size_t> (last - first);
  auto const new_end = std::move (first + delta, end, first);
  std::for_each (new_end, end, [] (T &v) { std::destroy_at (&v); });
  return size - delta;
}

// insert
// ~~~~~~
template <typename T>
template <typename SizeType>
pointer_based_iterator<T> avbase<T>::insert (
    pointer_based_iterator<T> const data, SizeType *const size,
    pointer_based_iterator<T> const pos, SizeType const count, T const &value) {
  assert (pos >= data && pos <= data + *size &&
          "pos must lie within the allocated array");
  auto const to = pos + count;
  auto const current_end = data + *size;
  assert (current_end >= pos);
  assert (to >= pos);

  auto const new_end = current_end + (to - pos);
  auto const num_to_move = current_end - pos;
  auto const num_uninit = std::min (num_to_move, new_end - current_end);

  // Copy-construct into uninitialized elements.
  for (auto it = current_end; it < to; ++it) {
    construct_at (to_address (it), value);
    ++(*size);
  }
  // Move existing elements into uninitialized space.
  auto dest = new_end - num_uninit;
  for (auto src = current_end - num_uninit; src < current_end; ++src) {
    construct_at (to_address (dest), std::move (*src));
    ++dest;
    ++(*size);
  }
  // Move elements between the initialized elements.
  // T * const p = data + (pos - data);
  std::move_backward (pos, pos + num_to_move - num_uninit,
                      new_end - num_uninit);
  // Copy into initialized elements.
  std::fill (pos, pos + count, value);
  return pos;
}

template <typename T>
template <typename SizeType>
pointer_based_iterator<T> avbase<T>::insert (pointer_based_iterator<T> begin,
                                             SizeType *const size,
                                             pointer_based_iterator<T> pos,
                                             T &&value) {
  if (auto const end = begin + *size; pos == end) {
    construct_at (to_address (pos), std::move (value));
  } else {
    avbase<T>::move_range (pos, end, pos + 1);
    *pos = std::move (value);
  }
  ++(*size);
  return pos;
}

template <typename T>
template <typename SizeType, typename InputIterator, typename>
pointer_based_iterator<T> avbase<T>::insert (
    pointer_based_iterator<T> const begin, SizeType *const size,
    pointer_based_iterator<T> pos, InputIterator first, InputIterator last) {
  assert (pos >= begin && pos <= begin + *size &&
          "pos must lie within the allocated array");
  pointer_based_iterator<T> end = begin + *size;
  if (pos == end) {
    // Insert at the end can be efficiently mapped to a series of emplace_back()
    // calls.
    std::for_each (first, last, [&end, size] (auto const &v) {
      construct_at (to_address (end), v);
      ++(*size);
      ++end;
    });
    return pos;
  }

  if constexpr (forward_iterator<InputIterator>) {
    // A forward iterator can be used with multi-pass algorithms such as this...
    auto const n = std::distance (first, last);
    // If all the new objects land on existing, initialized, elements we don't
    // need to worry about leaving the container in an invalid state if a ctor
    // throws.
    if (pos + n <= end) {
      details::avbase<T>::move_range (pos, end, pos + n);
      *size += static_cast<SizeType> (n);
      std::copy (first, last, pos);
      return pos;
    }

    // TODO(paul): Add an additional optimization path for random-access
    // iterators.
    // 1. Construct any objects in uninitialized space.
    // 2. Move objects from initialized to uninitialized space following the
    //    objects created in step 1.
    // 3. Construct objects in initialized space.
  }

  // A single-pass fallback algorithm for input iterators.
  auto r = pos;
  while (first != last) {
    avbase::insert (begin, size, pos, SizeType{1}, *first);
    ++first;
    ++pos;
  }
  return r;
}

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_AVBASE_HPP
