//===- include/peejay/details/variant.hpp -----------------*- mode: C++ -*-===//
//*                  _             _    *
//* __   ____ _ _ __(_) __ _ _ __ | |_  *
//* \ \ / / _` | '__| |/ _` | '_ \| __| *
//*  \ V / (_| | |  | | (_| | | | | |_  *
//*   \_/ \__,_|_|  |_|\__,_|_| |_|\__| *
//*                                     *
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
/// \file variant.hpp
/// \brief Provides peejay::details::variant, a minimal untagged variant used to hold the active parser matcher.
///
/// If PEEJAY_MPROTECT_VARIANT is defined to a non-zero value (and NDEBUG is not defined), the variant's storage is
/// allocated in its own memory pages which are protected whenever the variant does not hold a value, so that any
/// access to a stale value faults.
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
/// If non-zero, the storage of peejay::details::variant is protected whenever it does not hold a value.
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

/// \returns The size of a virtual memory page, or zero if it cannot be determined or is not a power of 2.
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
/// \returns  The value closest to but greater than or equal to \p v which is a
///   multiple of \p alignment.
constexpr std::size_t aligned(std::size_t const alignment, std::size_t const v) noexcept {
  assert(is_power_of_two(alignment) && "alignment must be a power of 2");
  return (v + alignment - 1U) & ~(alignment - 1U);
}

#ifdef _MSC_VER
/// A unique pointer to memory allocated with _aligned_malloc().
template <typename T> using unique_ptr_aligned = std::unique_ptr<T, decltype(&_aligned_free)>;
#else
/// A unique pointer to memory allocated with std::aligned_alloc().
template <typename T> using unique_ptr_aligned = std::unique_ptr<T, decltype(&std::free)>;
#endif

#endif  // PEEJAY_MPROTECT_VARIANT

/// A unary type function which yields true if \p T is trivially copyable.
/// \tparam T  The type to be checked.
template <typename T> struct type_is_trivially_copyable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

/// \brief A minimal variant which holds at most one value from a list of types.
///
/// Unlike std::variant, this class does not record which type it holds (except in debug builds, where the type
/// is checked by assertions). The owner is responsible for knowing the type of the value held and for destroying
/// it (by calling destroy()) before the variant is destroyed or another value is emplaced.
///
/// \tparam Members  A type list of the types that the variant may hold. All must be trivially copyable.
template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
class variant {
public:
#if PEEJAY_MPROTECT_VARIANT
  /// True if the variant allocates its storage on the heap.
  static constexpr bool allocates = true;
#else
  /// True if the variant allocates its storage on the heap.
  static constexpr bool allocates = false;
#endif  // PEEJAY_MPROTECT_VARIANT

  /// Constructs a variant which holds no value.
  variant() noexcept(allocates) { this->protect(/*usable=*/false); }
  // The contained value is copied byte-for-byte: this is safe because the member types are all trivially copyable.
#if PEEJAY_MPROTECT_VARIANT
  /// Copy constructor.
  /// \param other  The variant to be copied.
  variant(variant const& other) noexcept;
  /// Copy assignment.
  /// \param other  The variant to be copied.
  /// \returns *this.
  variant& operator=(variant const& other) noexcept;
#else
  /// Copy constructor.
  /// \param other  The variant to be copied.
  variant(variant const& other) noexcept = default;
  /// Copy assignment.
  /// \param other  The variant to be copied.
  /// \returns *this.
  variant& operator=(variant const& other) noexcept = default;
#endif
#if !defined(NDEBUG) || PEEJAY_MPROTECT_VARIANT
  /// Move constructor.
  /// \param other  The variant to be moved.
  variant(variant &&other) noexcept;
  /// Move assignment.
  /// \param other  The variant to be moved.
  /// \returns *this.
  variant &operator=(variant &&other) noexcept;
#else
  /// Move constructor.
  /// \param other  The variant to be moved.
  variant(variant &&other) noexcept = default;
  /// Move assignment.
  /// \param other  The variant to be moved.
  /// \returns *this.
  variant &operator=(variant &&other) noexcept = default;
#endif

  /// Destructor. The variant must not be holding a value.
  ~variant() noexcept {
    this->protect(/*usable=*/true);
    assert((holds_ == type_list::npos) && "Must not destruct a variant that is holding a value");
  }
  /// Destroys the value held by the variant.
  /// \tparam T  The type of the value held by the variant.
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
  /// \tparam T  The type of the value to be constructed.
  /// \tparam Args  The types of the constructor arguments.
  /// \param args  Constructor arguments to use when constructing the new value.
  /// \returns A reference to the new contained value.
  template <typename T, typename... Args>
    requires type_list::has_type_v<Members, T>
  T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    this->protect(/*usable=*/true);
    assert((holds_ == type_list::npos) && "The variant is already holding a value");
    // TODO: validate the pattern...
    auto *const result = std::construct_at(std::bit_cast<T *>(&contents_[0]), std::forward<Args>(args)...);
#ifndef NDEBUG
    holds_ = type_list::index_of_v<Members, T>;
#endif  // NDEBUG
    return *result;
  }

  /// \tparam T  The type of the value held by the variant.
  /// \returns A reference to the value held by the variant.
  template <typename T>
    requires type_list::has_type_v<Members, T>
  T& get() noexcept {
    assert(holds_ == static_cast<std::size_t>(type_list::index_of_v<Members, T>));
    return *std::bit_cast<T *>(&contents_[0]);
  }
  /// \tparam T  The type of the value held by the variant.
  /// \returns A reference to the value held by the variant.
  template <typename T>
    requires type_list::has_type_v<Members, T>
  T const& get() const noexcept {
    assert(holds_ == static_cast<std::size_t>(type_list::index_of_v<Members, T>));
    return *std::bit_cast<T const *>(&contents_[0]);
  }
#ifndef NDEBUG
  /// \returns The index within Members of the type of the value held by the variant, or type_list::npos if the
  ///   variant does not hold a value. Available only in debug builds.
  constexpr std::size_t holds() const noexcept { return holds_; }
#endif  // NDEBUG

private:
#ifndef NDEBUG
  /// The index within Members of the type of the value held, or type_list::npos.
  std::size_t holds_ = type_list::npos;
#endif  // NDEBUG
  static constexpr auto max_size_ =
      type_list::max_v<std::size_t, type_list::transform<Members, type_list::type_sizeof>>;
  static constexpr auto max_align_ =
      type_list::max_v<std::size_t, type_list::transform<Members, type_list::type_alignof>>;

#if PEEJAY_MPROTECT_VARIANT
  static std::size_t const page_size_;
  unique_ptr_aligned<std::byte[]> contents_ = aligned_unique_ptr<std::byte[]>();

  /// \returns The size of the storage, rounded up to a whole number of pages.
  static constexpr std::size_t adjusted_size();
  /// Allocates page-aligned storage for the variant's value.
  /// \returns A pointer to the new storage.
  template <typename T> static unique_ptr_aligned<T> aligned_unique_ptr();
  /// Changes the memory protection of the variant's storage.
  /// \param usable  If true, the storage is made readable and writable. If false, all access is prohibited.
  void protect(bool usable) noexcept;
#else
  /// The storage for the variant's value.
  alignas(max_align_) std::byte contents_[max_size_]{};
  /// A no-op: memory protection is disabled.
  /// \param usable  Ignored.
  constexpr void protect(bool const usable) const noexcept { (void)usable; }
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
variant<Members>::variant(variant const& other) noexcept : holds_{other.holds_} {
  // other's storage is readable whenever it holds a value.
  if (holds_ != type_list::npos) {
    std::memcpy(contents_.get(), other.contents_.get(), max_size_);
  }
  this->protect(/*usable=*/holds_ != type_list::npos);
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
auto variant<Members>::operator=(variant const& other) noexcept -> variant& {
  if (this == &other) {
    return *this;
  }
  this->protect(/*usable=*/true);
  holds_ = other.holds_;
  if (holds_ != type_list::npos) {
    std::memcpy(contents_.get(), other.contents_.get(), max_size_);
  }
  this->protect(/*usable=*/holds_ != type_list::npos);
  return *this;
}

// Moving copies the value into this variant's own storage rather than taking other's. This leaves other with storage
// and a copy of its value, exactly as it is left by a move in the other build modes.
template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
variant<Members>::variant(variant&& other) noexcept : variant(static_cast<variant const&>(other)) {
}

template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
auto variant<Members>::operator=(variant&& other) noexcept -> variant& {
  return *this = static_cast<variant const&>(other);
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
variant<Members>::variant(variant&& other) noexcept : holds_{other.holds_} {
  // memcpy() is safe here because we assert that the contained member types are trivially copyable.
  std::memcpy(&contents_[0], &other.contents_[0], sizeof(contents_));
}
template <type_list::sequence Members>
  requires type_list::all_of_v<type_list::transform<Members, type_is_trivially_copyable>>
auto variant<Members>::operator=(variant&& other) noexcept -> variant& {
  holds_ = other.holds_;
  std::memcpy(&contents_[0], &other.contents_[0], sizeof(contents_));
  return *this;
}

#endif  // PEEJAY_MPROTECT_VARIANT

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_VARIANT_HPP
