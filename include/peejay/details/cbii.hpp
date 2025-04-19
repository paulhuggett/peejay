//===- include/peejay/details/cbii.hpp --------------------*- mode: C++ -*-===//
//*       _     _ _  *
//*   ___| |__ (_|_) *
//*  / __| '_ \| | | *
//* | (__| |_) | | | *
//*  \___|_.__/|_|_| *
//*                  *
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
#ifndef PEEJAY_DETAILS_CBII_HPP
#define PEEJAY_DETAILS_CBII_HPP

#include <cstddef>
#include <iterator>

namespace peejay {

/// checked_back_insert_iterator is an output iterator that appends elements
/// to a container for which it was constructed. The container must provide
/// \p size(), \p max_size(), and \p push_back() member functions. Once the
/// number of elements in the container exceeds max_size(), the \p overflow
/// value is set to true and \p push_back() is no longer called.
///
/// The container's push_back() member function is called when the iterator
/// (whether dereferenced or not) is assigned to unless an overflow is
/// detected. Incrementing the checked_back_insert_iterator is a no-op.
template <typename Container> class checked_back_insert_iterator {
public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr checked_back_insert_iterator(Container *const container, bool *const overflow) noexcept
      : container_{container}, overflow_{overflow} {
    if (container_->size() > container_->max_size()) {
      *overflow_ = true;
    }
  }

  constexpr checked_back_insert_iterator &operator=(typename Container::value_type const &value) {
    if (container_->size() >= container_->max_size()) {
      *overflow_ = true;
    } else {
      container_->push_back(value);
    }
    return *this;
  }
  constexpr checked_back_insert_iterator &operator=(typename Container::value_type &&value) {
    if (container_->size() >= container_->max_size()) {
      *overflow_ = true;
    } else {
      container_->push_back(std::move(value));
    }
    return *this;
  }
  constexpr checked_back_insert_iterator &operator*() { return *this; }
  constexpr checked_back_insert_iterator &operator++() { return *this; }
  constexpr checked_back_insert_iterator operator++(int) { return *this; }

private:
  Container *container_;
  bool *overflow_;
};

template <typename Container>
checked_back_insert_iterator(Container *, bool *) -> checked_back_insert_iterator<Container>;

}  // end namespace peejay

#endif  // PEEJAY_DETAILS_CBII_HPP
