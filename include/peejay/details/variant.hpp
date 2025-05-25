//===- include/peejay/details/variant.hpp -----------------*- mode: C++ -*-===//
//*                  _             _    *
//* __   ____ _ _ __(_) __ _ _ __ | |_  *
//* \ \ / / _` | '__| |/ _` | '_ \| __| *
//*  \ V / (_| | |  | | (_| | | | | |_  *
//*   \_/ \__,_|_|  |_|\__,_|_| |_|\__| *
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
#ifndef PEEJAY_DETAILS_VARIANT_HPP
#define PEEJAY_DETAILS_VARIANT_HPP

#include <bit>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <type_traits>

#include "peejay/details/portab.hpp"
#include "peejay/details/type_list.hpp"

// mprotect-mode is supported on Linux, macOS, and Windows if NDEBUG is not defined.
#ifndef PEEJAY_MPROTECT_VARIANT
#  define PEEJAY_MPROTECT_VARIANT 0
#elif defined(PEEJAY_MPROTECT_VARIANT) && \
    (defined(NDEBUG) || !(defined(__linux__) || defined(__APPLE__) || defined(_WIN32)))
#  undef PEEJAY_MPROTECT_VARIANT
#  define PEEJAY_MPROTECT_VARIANT 0
#endif

#if PEEJAY_MPROTECT_VARIANT
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !_WIN32
#endif  // PEEJAY_MPROTECT_VARIANT

namespace peejay::details {

#if PEEJAY_MPROTECT_VARIANT

/// \returns True if the input value is a power of 2.
constexpr bool is_power_of_two(std::size_t const n) noexcept {
  return n > 0U && !(n & (n - 1U));
}

inline std::size_t system_page_size() noexcept {
#ifdef _WIN32
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  auto result = system_info.dwPageSize;
#else
  auto result = sysconf(_SC_PAGESIZE);
  if (result == -1) {
    return 0;
  }
#endif  // !_WIN32
  using result_type = decltype(result);
  static_assert(sizeof(result_type) <= sizeof(std::size_t));
  if constexpr (std::is_signed_v<result_type>) {
    result = std::max(result_type{0}, result);
  }
  return is_power_of_two(static_cast<std::size_t>(result)) ? static_cast<std::size_t>(result) : 0;
}

/// \param alignment  The alignment required for \p v.
/// \param v  The value to be aligned.
/// \returns  The value closest to but greater than or equal to \p v for which
///   \p v modulo \p align is zero.
constexpr std::size_t aligned(std::size_t const alignment, std::size_t const v) noexcept {
  assert(is_power_of_two(alignment) && "alignment must be a power of 2");
  return (v + alignment - 1U) & ~(alignment - 1U);
}

#ifdef _MSC_VER
template <typename T> using unique_ptr_aligned = std::unique_ptr<T, decltype(&_aligned_free)>;
#else
template <typename T> using unique_ptr_aligned = std::unique_ptr<T, decltype(&std::free)>;
#endif

#endif  // PEEJAY_MPROTECT_VARIANT

template <typename T> struct type_is_trivially_copyable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
class variant {
public:
#if PEEJAY_MPROTECT_VARIANT
  static constexpr bool allocates = true;
#else
  static constexpr bool allocates = false;
#endif  // PEEJAY_MPROTECT_VARIANT

  variant() noexcept(allocates) { this->protect(/*usable=*/false); }
  variant(variant const &) noexcept = delete;
  variant &operator=(variant const &other) noexcept = delete;
#if !defined(NDEBUG) || PEEJAY_MPROTECT_VARIANT
  variant(variant &&other) noexcept;
  variant &operator=(variant &&other) noexcept;
#else
  variant(variant &&other) noexcept = default;
  variant &operator=(variant &&other) noexcept = default;
#endif

  ~variant() noexcept {
    this->protect(/*usable=*/true);
    assert((holds_ == type_list::npos) && "Must not destruct a variant that is holding a value");
    // TODO: validate the pattern...
  }
  template <typename T>
    requires type_list::has_type_v<Members, T>
  void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
    assert((holds_ == static_cast<std::size_t>(type_list::index_of_v<Members, T>)) &&
           "The variant does not hold the expected type");
    std::destroy_at(std::bit_cast<T *>(&contents_[0]));
#ifndef NDEBUG
    std::memset(&contents_[0], 0, sizeof(contents_));
    holds_ = type_list::npos;
#endif  // NDEBUG
    this->protect(/*usable=*/false);
  }

  /// Creates a new value in-place, in an existing variant object.
  /// The object must not be holding any currently contained value when this function is called.
  /// \p args Constructor arguments to use when constructing the new value
  /// \returns A reference to the new contained value.
  template <typename T, typename... Args>
    requires type_list::has_type_v<Members, T>
  T &emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    this->protect(/*usable=*/true);
    assert((holds_ == type_list::npos) && "The variant is already holding a value");
    // TODO: validate the pattern...
    auto *const result = std::construct_at(std::bit_cast<T *>(&contents_[0]), std::forward<Args>(args)...);
#ifndef NDEBUG
    holds_ = type_list::index_of_v<Members, T>;
#endif  // NDEBUG
    return *result;
  }

  template <typename T>
    requires type_list::has_type_v<Members, T>
  T &get() noexcept {
    assert(holds_ == static_cast<std::size_t>(type_list::index_of_v<Members, T>));
    return *std::bit_cast<T *>(&contents_[0]);
  }
  template <typename T>
    requires type_list::has_type_v<Members, T>
  T const &get() const noexcept {
    assert(holds_ == static_cast<std::size_t>(type_list::index_of_v<Members, T>));
    return *std::bit_cast<T const *>(&contents_[0]);
  }
#ifndef NDEBUG
  constexpr std::size_t holds() const noexcept { return holds_; }
#endif  // NDEBUG

private:
#ifndef NDEBUG
  std::size_t holds_ = type_list::npos;
#endif  // NDEBUG
  static constexpr auto max_size_ =
      type_list::max_v<std::size_t, type_list::transform<Members, type_list::type_sizeof>>;
  static constexpr auto max_align_ =
      type_list::max_v<std::size_t, type_list::transform<Members, type_list::type_alignof>>;

#if PEEJAY_MPROTECT_VARIANT
  static std::size_t const page_size_;
  unique_ptr_aligned<std::byte[]> contents_ = aligned_unique_ptr<std::byte[]>();

  static constexpr std::size_t adjusted_size();
  template <typename T> static unique_ptr_aligned<T> aligned_unique_ptr();
  void protect(bool usable) noexcept;
#else
  alignas(max_align_) std::byte contents_[max_size_]{};
  constexpr void protect(bool const usable) noexcept { (void)usable; }
#endif  // !PEEJAY_MPROTECT_VARIANT
};

#if PEEJAY_MPROTECT_VARIANT

PEEJAY_CLANG_DIAG_PUSH
PEEJAY_CLANG_DIAG_NO_GLOBAL_CONSTRUCTORS
template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
inline std::size_t const variant<Members>::page_size_ = system_page_size();
PEEJAY_CLANG_DIAG_POP

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
variant<Members>::variant(variant &&other) noexcept : contents_{std::move(other.contents_)} {
  holds_ = other.holds_;
  other.holds_ = type_list::npos;
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
auto variant<Members>::operator=(variant &&other) noexcept -> variant & {
  this->protect(/*usable=*/true);
  other.protect(/*usable=*/true);
  contents_ = std::move(other.contents_);
  holds_ = other.holds_;
  other.holds_ = type_list::npos;
  other.protect(/*usable=*/other.holds_ != type_list::npos);
  this->protect(/*usable=*/holds_ != type_list::npos);
  return *this;
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
constexpr std::size_t variant<Members>::adjusted_size() {
  return aligned(std::max(page_size_, max_align_), max_size_);
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
template <typename T>
auto variant<Members>::aligned_unique_ptr() -> unique_ptr_aligned<T> {
  auto const alignment = std::max(page_size_, max_align_);
  auto const size = variant::adjusted_size();
#ifdef _MSC_VER
#ifndef NDEBUG
  auto *const ptr = _aligned_malloc_dbg(size, alignment, __FILE__, __LINE__);
#else
  auto *const ptr = _aligned_malloc(size, alignment);
#endif
  auto const free = &_aligned_free;
#else
  auto *const ptr = std::aligned_alloc(alignment, size);
  auto const free = &std::free;
#endif
  assert((ptr != nullptr) && "aligned_malloc() failed");
  return unique_ptr_aligned<T>(std::bit_cast<std::byte *>(ptr), free);
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
void variant<Members>::protect(bool usable) noexcept {
  if (page_size_ == 0 || !contents_) {
    return;
  }
  auto *const ptr = contents_.get();
  auto const size = variant::adjusted_size();
#ifdef _WIN32
  auto old_protect = DWORD{0};
  VirtualProtect(ptr, size, usable ? PAGE_READWRITE : PAGE_NOACCESS, &old_protect);
#else
  mprotect(ptr, size, usable ? PROT_READ | PROT_WRITE : PROT_NONE);
#endif  // !_WIN32
}

#elif !defined(NDEBUG)

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
variant<Members>::variant(variant &&other) noexcept {
  // memcpy() is safe here because we assert that the contained member types are trivially copyable.
  std::memcpy(&contents_[0], &other.contents_[0], sizeof(contents_));
  holds_ = other.holds_;
  other.holds_ = type_list::npos;
}
template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
auto variant<Members>::operator=(variant &&other) noexcept -> variant & {
  std::memcpy(&contents_[0], &other.contents_[0], sizeof(contents_));
  holds_ = other.holds_;
  other.holds_ = type_list::npos;
  return *this;
}

#endif  // PEEJAY_MPROTECT_VARIANT

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_VARIANT_HPP
