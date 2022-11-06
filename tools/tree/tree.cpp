//===- tools/tree/tree.cpp ------------------------------------------------===//
//*  _                  *
//* | |_ _ __ ___  ___  *
//* | __| '__/ _ \/ _ \ *
//* | |_| | |  __/  __/ *
//*  \__|_|  \___|\___| *
//*                     *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <cassert>
#include <iostream>

// Platform includes
#ifdef _WIN32
#include <fstream>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // !_WIN32

#include "json/emit.hpp"

namespace {

template <typename T>
PEEJAY_CXX20REQUIRES (std::is_integral_v<T>)
constexpr auto as_unsigned (T v) {
  return static_cast<std::make_unsigned_t<T>> (std::max (T{0}, v));
}

template <typename Notifications, typename IStream>
std::variant<std::error_code, std::optional<peejay::element>> slurp (
    peejay::parser<Notifications>& p, IStream&& in) {
  std::array<char, 256> buffer{{0}};

  while ((in.rdstate () & (std::ios_base::badbit | std::ios_base::failbit |
                           std::ios_base::eofbit)) == 0) {
    auto* const data = buffer.data ();
    in.read (data, buffer.size ());
    auto const available = as_unsigned (in.gcount ());
#if PEEJAY_CXX20
    p.input (std::span<char>{data, available});
#else
    p.input (data, data + available);
#endif  // PEEJAY_CXX20
    if (auto const err = p.last_error ()) {
      return {err};
    }
  }
  std::optional<peejay::element> result = p.eof ();
  if (std::error_code const erc = p.last_error ()) {
    return {erc};
  }
  return {std::move (result)};
}

#ifdef _WIN32

template <typename Notifications>
std::variant<std::error_code, std::optional<peejay::element>> slurp_file (
    peejay::parser<Notifications>& p, char const* file) {
  return slurp (p, std::ifstream{file});
}

#else
class closer {
public:
  explicit constexpr closer (int const fd) noexcept : fd_{fd} {}
  closer (closer const&) = delete;
  closer& operator= (closer const&) = delete;
  ~closer () noexcept {
    if (fd_ != -1) {
      close (fd_);
    }
  }
  constexpr int get () const noexcept { return fd_; }

private:
  int const fd_;
};

template <typename T>
class unmapper {
public:
  explicit constexpr unmapper (void* ptr, size_t size) noexcept
      : span_{ptr, size} {}
  unmapper (unmapper const&) = delete;
  unmapper& operator= (unmapper const&) = delete;
  ~unmapper () noexcept { ::munmap (span_.first, span_.second); }
  constexpr T const* begin () const noexcept {
    return reinterpret_cast<T const*> (span_.first);
  }
  constexpr T const* end () const noexcept {
    return begin () + span_.second / sizeof (T);
  }

private:
  // TODO: use std::span<> when we can drop C++17 support!
  std::pair<void*, size_t> span_;
};

template <typename Notifications>
std::variant<std::error_code, std::optional<peejay::element>> slurp_file (
    peejay::parser<Notifications>& p, char const* file) {
  closer fd{::open (file, O_RDONLY)};
  if (fd.get () == -1) {
    return std::error_code{errno, std::generic_category ()};
  }
  struct stat sb;
  if (::fstat (fd.get (), &sb) == -1) {
    return std::error_code{errno, std::generic_category ()};
  }
  void* const mapped = ::mmap (nullptr, as_unsigned (sb.st_size), PROT_READ,
                               MAP_SHARED, fd.get (), off_t{0});
  if (mapped == MAP_FAILED) {
    return std::error_code{errno, std::generic_category ()};
  }
  unmapper<char const> ptr{mapped, as_unsigned (sb.st_size)};
  std::optional<peejay::element> result =
      p.input (std::begin (ptr), std::end (ptr)).eof ();
  if (std::error_code const erc = p.last_error ()) {
    return {erc};
  }
  return {std::move (result)};
}
#endif  // _WIN32

template <typename N>
void report_error (peejay::parser<N>& p, std::string_view const& file_name) {
  auto const& pos = p.pos ();
  std::cout << file_name << ':' << pos.line << ':' << pos.column << ':'
            << " error: " << p.last_error ().message () << '\n';
}

}  // end anonymous namespace

int main (int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    auto p = peejay::make_parser (peejay::dom{});
    auto const res = argc < 2 ? slurp (p, std::cin) : slurp_file (p, argv[1]);
    if (std::holds_alternative<std::error_code> (res)) {
      report_error(p, argc < 2 ? "<stdin" : argv[1]);
      exit_code = EXIT_FAILURE;
    } else {
      emit (std::cout, std::get<1> (res));
    }
  } catch (std::exception const& ex) {
    std::cerr << "Error: " << ex.what () << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception.\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
