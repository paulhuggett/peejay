//===- klee/av_member.hpp ---------------------------------*- mode: C++ -*-===//
//*                                         _                *
//*   __ ___   __  _ __ ___   ___ _ __ ___ | |__   ___ _ __  *
//*  / _` \ \ / / | '_ ` _ \ / _ \ '_ ` _ \| '_ \ / _ \ '__| *
//* | (_| |\ V /  | | | | | |  __/ | | | | | |_) |  __/ |    *
//*  \__,_| \_/   |_| |_| |_|\___|_| |_| |_|_.__/ \___|_|    *
//*                                                          *
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
#ifndef AV_MEMBER_HPP
#define AV_MEMBER_HPP

#include <cassert>
#include <cstddef>
#include <stdexcept>

class memberex : public std::runtime_error {
public:
  memberex() : std::runtime_error("memberex") {}
};

class member {
public:
  static inline std::size_t throw_number;

  member() {
    throw_check();
    ++instances_;
  }
  member(int v) : v_{v} {
    // The memory underlying arrayvec<> is initialized to 0xFF therefore we can
    // check that we're not using uninitialized stored by checking that the
    // values stored by instances of 'member' are not less than 0.
    assert(v >= 0);
    throw_check();
    ++instances_;
  }
  member(member const& rhs) : v_{rhs.v_} {
    assert(rhs.v_ >= 0);
    throw_check();
    ++instances_;
  }
  member(member&& rhs) noexcept : v_{rhs.v_} {
    assert(rhs.v_ >= 0);
    ++instances_;
    rhs.v_ = 0;
  }

  ~member() noexcept {
    assert(v_ >= 0);
    --instances_;
  }

  member& operator=(member const& rhs) {
    assert(v_ >= 0 && rhs.v_ >= 0);
    if (&rhs != this) {
      v_ = rhs.v_;
      throw_check();
    }
    return *this;
  }
  member& operator=(member&& rhs) noexcept {
    assert(v_ >= 0 && rhs.v_ >= 0);
    if (&rhs != this) {
      v_ = rhs.v_;
      rhs.v_ = 0;
    }
    return *this;
  }
  bool operator==(member const& rhs) const noexcept { return v_ == rhs.v_; }
  bool operator!=(member const& rhs) const noexcept { return v_ != rhs.v_; }

  static std::size_t instances() noexcept { return instances_; }

private:
  static inline std::size_t instances_ = 0;
  static inline std::size_t operations_ = 0;

  int v_ = 0;

  static void throw_check() {
    if (operations_ >= throw_number) {
      throw memberex{};
    }
    ++operations_;
  }
};

#endif  // AV_MEMBER_HPP
