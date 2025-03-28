//===- tools/tree/tree.cpp ------------------------------------------------===//
//*  _                  *
//* | |_ _ __ ___  ___  *
//* | __| '__/ _ \/ _ \ *
//* | |_| | |  __/  __/ *
//*  \__|_|  \___|\___| *
//*                     *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
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

#include "peejay/json/dom.hpp"
#include "peejay/json/emit.hpp"
#include "peejay/json/portab.hpp"

using pjparser = peejay::parser<peejay::dom<1024>>;

namespace {

template <typename T>
  requires(std::is_integral_v<T>)
constexpr auto as_unsigned(T v) {
  return static_cast<std::make_unsigned_t<T>>(std::max(T{0}, v));
}

std::variant<std::error_code, std::optional<peejay::element>> slurp(pjparser& p, std::istream& in) {
  std::array<char, 256> buffer{};

  while (in.rdstate() == std::ios_base::goodbit) {
    auto* const data = buffer.data();
    in.read(data, buffer.size());
    auto const available = as_unsigned(in.gcount());
    // TODO(paul) I just assume that the IStream yields UTF-8.
    p.input(std::span{peejay::pointer_cast<std::byte const>(data), available});
    if (auto const err = p.last_error()) {
      return err;
    }
  }
  if ((in.rdstate() & std::ios_base::badbit) != 0) {
    return make_error_code(std::errc::io_error);
  }
  std::optional<peejay::element> result = p.eof();
  if (auto const erc = p.last_error()) {
    return erc;
  }
  return result;
}

#ifdef _WIN32

std::variant<std::error_code, std::optional<peejay::element>> slurp(pjparser& p, std::istream&& in) {
  return slurp(p, std::ref(in));
}

std::variant<std::error_code, std::optional<peejay::element>> slurp_file(pjparser& p, char const* file) {
  return slurp(p, std::ifstream{file});
}

#else

class closer {
public:
  explicit constexpr closer(int const fd) noexcept : fd_{fd} {}
  closer(closer const&) = delete;
  closer(closer&&) noexcept = delete;

  ~closer() noexcept {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  closer& operator=(closer const&) = delete;
  closer& operator=(closer&&) noexcept = delete;

  [[nodiscard]] constexpr int get() const noexcept { return fd_; }

private:
  int const fd_;
};

template <typename T> class unmapper {
public:
  explicit constexpr unmapper(void* ptr, size_t size) noexcept : span_{ptr, size} {}
  unmapper(unmapper const&) = delete;
  unmapper(unmapper&&) noexcept = delete;

  ~unmapper() noexcept { ::munmap(span_.first, span_.second); }

  unmapper& operator=(unmapper const&) = delete;
  unmapper& operator=(unmapper&&) noexcept = delete;

  [[nodiscard]] constexpr T const* begin() const noexcept { return peejay::pointer_cast<T const>(span_.first); }
  [[nodiscard]] constexpr T const* end() const noexcept { return begin() + span_.second / sizeof(T); }

private:
  // TODO(paul) use std::span<> when we can drop C++17 support!
  std::pair<void*, size_t> span_;
};

std::variant<std::error_code, std::optional<peejay::element>> slurp_file(pjparser& p, char const* file) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
  closer fd{::open(file, O_RDONLY | O_CLOEXEC)};
  if (fd.get() == -1) {
    return std::error_code{errno, std::generic_category()};
  }
  struct stat sb{};
  if (::fstat(fd.get(), &sb) == -1) {
    return std::error_code{errno, std::generic_category()};
  }
  void* const mapped = ::mmap(nullptr, as_unsigned(sb.st_size), PROT_READ, MAP_SHARED, fd.get(), off_t{0});
  if (mapped == MAP_FAILED) {
    return std::error_code{errno, std::generic_category()};
  }
  unmapper<std::byte const> ptr{mapped, as_unsigned(sb.st_size)};
  std::optional<peejay::element> result = p.input(std::begin(ptr), std::end(ptr)).eof();
  if (std::error_code const erc = p.last_error()) {
    return {erc};
  }
  return {std::move(result)};
}

#endif  // _WIN32

void report_error(pjparser const& p, std::string_view const& file_name) {
  auto const& pos = p.pos();
  std::cerr << file_name << ':' << pos.get_line() << ':' << pos.get_column() << ':'
            << " error: " << p.last_error().message() << '\n';
}

}  // end anonymous namespace

int main(int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    pjparser p = make_parser(peejay::dom{});
    auto const res = argc < 2 ? slurp(p, std::cin) : slurp_file(p, argv[1]);
    if (std::holds_alternative<std::error_code>(res)) {
      report_error(p, argc < 2 ? "<stdin>" : argv[1]);
      exit_code = EXIT_FAILURE;
    } else {
      emit(std::cout, std::get<1>(res));
    }
  } catch (std::exception const& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception.\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
