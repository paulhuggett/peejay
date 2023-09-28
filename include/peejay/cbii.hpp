//===- include/peejay/cbii.hpp ----------------------------*- mode: C++ -*-===//
//*       _     _ _  *
//*   ___| |__ (_|_) *
//*  / __| '_ \| | | *
//* | (__| |_) | | | *
//*  \___|_.__/|_|_| *
//*                  *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
/// \file cbii.hpp
/// \brief Provides peejay::checked_back_insert_iterator is an output
///   iterator that appends elements to a container.
#ifndef PEEJAY_CBII_HPP
#define PEEJAY_CBII_HPP

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
template <typename Container>
class checked_back_insert_iterator {
public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr checked_back_insert_iterator (Container *const container,
                                          bool *const overflow) noexcept
      : container_{container}, overflow_{overflow} {
    if (container_->size () > container_->max_size ()) {
      *overflow_ = true;
    }
  }

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

#endif  // PEEJAY_CBII_HPP
