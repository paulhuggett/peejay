//===- include/peejay/details/cbii.hpp --------------------*- mode: C++ -*-===//
//*       _     _ _  *
//*   ___| |__ (_|_) *
//*  / __| '_ \| | | *
//* | (__| |_) | | | *
//*  \___|_.__/|_|_| *
//*                  *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
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
#ifndef PEEJAY_DETAILS_CBII_HPP
#define PEEJAY_DETAILS_CBII_HPP

#include <cstddef>
#include <iterator>

namespace peejay {

/// checked_back_insert_iterator is an output iterator that appends elements
/// to a container for which it was constructed. The container must provide
/// size(), max_size(), and push_back() member functions. If a value is
/// assigned when the container already holds max_size() elements, the
/// \p overflow value is set to true and push_back() is not called.
///
/// The container's push_back() member function is called when the iterator
/// (whether dereferenced or not) is assigned to unless an overflow is
/// detected. Incrementing the checked_back_insert_iterator is a no-op.
///
/// \tparam Container  The type of the container to which elements are appended.
/// \tparam OutputType  The type of the values which may be assigned to the iterator. These are converted to
///   Container::value_type.
template <typename Container, typename OutputType = typename Container::value_type> class checked_back_insert_iterator {
public:
  /// The iterator category.
  using iterator_category = std::output_iterator_tag;
  /// The value type (void for an output iterator).
  using value_type = void;
  /// The difference type.
  using difference_type = std::ptrdiff_t;
  /// The pointer type (void for an output iterator).
  using pointer = void;
  /// The reference type (void for an output iterator).
  using reference = void;
  /// The type of the container.
  using container_type = Container;

  /// \param container  The container to which elements are appended.
  /// \param overflow  Points to a value which is set to true if an overflow is detected. It is not modified
  ///   otherwise.
  constexpr checked_back_insert_iterator(Container *const container, bool *const overflow) noexcept
      : container_{container}, overflow_{overflow} {
    if (container_->size() > container_->max_size()) {
      *overflow_ = true;
    }
  }

  /// Appends a value to the container unless it is full, in which case the overflow flag is set.
  /// \param value  The value to be appended.
  /// \returns *this.
  constexpr checked_back_insert_iterator &operator=(OutputType const &value) {
    if (container_->size() >= container_->max_size()) {
      *overflow_ = true;
    } else {
      container_->push_back(static_cast<typename Container::value_type>(value));
    }
    return *this;
  }
  /// A no-op.
  /// \returns *this.
  constexpr checked_back_insert_iterator &operator*() noexcept { return *this; }
  /// A no-op.
  /// \returns *this.
  constexpr checked_back_insert_iterator &operator++() noexcept { return *this; }
  /// A no-op.
  /// \returns *this.
  constexpr checked_back_insert_iterator operator++(int) noexcept { return *this; }

private:
  Container *container_;
  bool *overflow_;
};

/// Deduction guide for constructing a checked_back_insert_iterator from a container pointer.
template <typename Container, typename OutputType = typename Container::value_type>
checked_back_insert_iterator(Container *, bool *) -> checked_back_insert_iterator<Container, OutputType>;

}  // end namespace peejay

#endif  // PEEJAY_DETAILS_CBII_HPP
