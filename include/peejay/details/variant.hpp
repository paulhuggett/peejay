#ifndef PEEJAY_DETAILS_VARIANT_HPP
#define PEEJAY_DETAILS_VARIANT_HPP

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <type_traits>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "peejay/details/type_list.hpp"

#ifndef NDEBUG

namespace peejay::details {
// #define PEEJAY_HAVE_GETPAGESIZE 1

/// \returns True if the input value is a power of 2.
constexpr bool is_power_of_two(std::size_t const n) noexcept {
  return n > 0U && !(n & (n - 1U));
}

inline unsigned system_page_size() noexcept {
#ifdef _WIN32
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  auto const result = system_info.dwPageSize;
  assert(result > 0 && result <= std::numeric_limits<unsigned>::max());
  return static_cast<unsigned>(result);
#elif defined(PEEJAY_HAVE_GETPAGESIZE)
  int const result = getpagesize();
  return result >= 0 && is_power_of_two(result) ? static_cast<unsigned>(result) : 0;
#else
  long const result = sysconf(_SC_PAGESIZE);
  return result >= 0 && static_cast<unsigned long>(result) <= std::numeric_limits<unsigned>::max() &&
                 is_power_of_two(result)
             ? static_cast<unsigned>(result)
             : 0;
#endif
}

/// \param alignment  The alignment required for \p v.
/// \param v  The value to be aligned.
/// \returns  The value closest to but greater than or equal to \p v for which
/// \p v modulo \p align is zero.
constexpr std::size_t aligned(std::size_t const alignment, std::size_t const v) noexcept {
  assert(is_power_of_two(alignment) && "alignment must be a power of 2");
  return (v + alignment - 1U) & ~(alignment - 1U);
}

template <typename T> using unique_ptr_aligned = std::unique_ptr<T, decltype(&std::free)>;

template <typename T> unique_ptr_aligned<T> aligned_unique_ptr(std::size_t alignment, std::size_t size) {
  return unique_ptr_aligned<T>(static_cast<T *>(std::aligned_alloc(alignment, size)), &std::free);
}

#endif

template <type_list::sequence Members> class variant {
public:
  //  using members = type_list::type_list<string_matcher<Backend>, number_matcher<Backend>, token_matcher<Backend>>;

  template <typename T> struct type_trivially_copyable : std::bool_constant<std::is_trivially_copyable_v<T>> {};
  static_assert(type_list::all_v<type_list::transform<Members, type_trivially_copyable>>);

  variant() noexcept = default;
  variant(variant const &) noexcept = default;
#ifndef NDEBUG
  variant(variant &&other) noexcept {
    std::memcpy(&v_[0], &other.v_[0], sizeof(v_));
    holds_ = other.holds_;
    other.holds_ = type_list::npos;
  }
  variant &operator=(variant &&other) noexcept {
    std::memcpy(&v_[0], &other.v_[0], sizeof(v_));
    holds_ = other.holds_;
    other.holds_ = type_list::npos;
    return *this;
  }
#else
  variant(variant &&other) noexcept = default;
  variant &operator=(variant &&other) noexcept = default;
#endif

  ~variant() noexcept {
    assert(holds_ == type_list::npos && "Must not destruct a variant that is holding a value");
    // TODO: validate the pattern...
  }
  variant &operator=(variant const &) noexcept = default;

  template <typename T>
    requires type_list::has_type_v<Members, T>
  void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
    assert(holds_ == (type_list::index_of_v<Members, T>) && "The variant does not hold the expected type");
    std::destroy_at(std::bit_cast<T *>(&v_[0]));
#ifndef NDEBUG
    std::memset(&v_[0], 0, sizeof(v_));
    holds_ = type_list::npos;
#endif
  }
  template <typename T, typename... Args>
    requires type_list::has_type_v<Members, T>
  void emplace(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    assert(holds_ == type_list::npos && "The variant is already holding a value");
    // TODO: validate the pattern...
    std::construct_at(std::bit_cast<T *>(&v_[0]), std::forward<Args>(args)...);
#ifndef NDEBUG
    holds_ = type_list::index_of_v<Members, T>;
#endif
  }

  template <typename T>
    requires type_list::has_type_v<Members, T>
  T &get() noexcept {
    assert(holds_ == (type_list::index_of_v<Members, T>));
    return *std::bit_cast<T *>(&v_[0]);
  }
  template <typename T>
    requires type_list::has_type_v<Members, T>
  T const &get() const noexcept {
    assert(holds_ == (type_list::index_of_v<Members, T>));
    return *std::bit_cast<T const *>(&v_[0]);
  }
#ifndef NDEBUG
  constexpr std::size_t holds() const noexcept { return holds_; }
#endif

private:
#ifndef NDEBUG
  std::size_t holds_ = type_list::npos;
#endif
  using member_sizeof = type_list::transform<Members, type_list::type_sizeof>;
  using member_alignof = type_list::transform<Members, type_list::type_alignof>;
  static constexpr auto max_size = type_list::max_v<member_sizeof>;
  static constexpr auto max_align = type_list::max_v<member_alignof>;

  alignas(max_align) std::byte v_[max_size]{};
};

}  // end namespace peejay::details

#endif  // PEEJAY_DETAILS_VARIANT_HPP
