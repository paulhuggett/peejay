//===- unittests/backtrace.cpp --------------------------------------------===//
//*  _                _    _                       *
//* | |__   __ _  ___| | _| |_ _ __ __ _  ___ ___  *
//* | '_ \ / _` |/ __| |/ / __| '__/ _` |/ __/ _ \ *
//* | |_) | (_| | (__|   <| |_| | | (_| | (_|  __/ *
//* |_.__/ \__,_|\___|_|\_\\__|_|  \__,_|\___\___| *
//*                                                *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include "peejay/json/portab.hpp"

#if (__linux__ || __APPLE__) && PEEJAY_HAVE_RANGES && PEEJAY_HAVE_CONCEPTS

#include <execinfo.h>
#include <unistd.h>

#include <array>
#include <concepts>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, hicpp-avoid-c-arrays, cppcoreguidelines-pro-bounds-array-to-pointer-decay)

namespace {

// NOLINTBEGIN(misc-no-recursion)
template <std::unsigned_integral Unsigned>
constexpr std::size_t base10digits (
    Unsigned const value = std::numeric_limits<Unsigned>::max ()) noexcept {
  return value < 10U
             ? std::size_t{1}
             : std::size_t{1} + base10digits<Unsigned> (value / Unsigned{10});
}
// NOLINTEND(misc-no-recursion)

template <std::unsigned_integral Unsigned>
class unsigned_to_characters {
public:
  using base10storage = std::array<char, base10digits<Unsigned> ()>;

  /// Converts an unsigned numeric value to an array of characters.
  ///
  /// \param value  The unsigned number value to be converted.
  /// \result  A range denoting the range of valid characters in the internal buffer.
  std::ranges::subrange<typename base10storage::iterator> operator() (
      Unsigned value) noexcept {
    auto const end = buffer_.end ();
    auto pos = std::prev (end);
    if (value == 0U) {
      *pos = '0';
      return {pos, end};
    }

    for (; value > 0; value /= 10U) {
      *(pos--) = (value % 10U) + '0';
    }
    return {std::next (pos), end};
  }

private:
  base10storage buffer_;
};

template <std::size_t Size>
constexpr std::size_t strlength (char const (&str)[Size]) noexcept {
  (void)str;
  return Size - 1U;
}

ssize_t write_char (int const file_descriptor, char const chr) noexcept {
  return ::write (file_descriptor, &chr, sizeof (chr));
}

void say_signal_number (int const file_descriptor, int sig) {
  if (static char const message[] = "Signal: ";
      ::write (file_descriptor, &message[0], strlength (message))) {
    /* do nothing */
  }
  if (sig < 0) {
    (void)write_char (file_descriptor, '-');
    sig = -sig;
  }
  static unsigned_to_characters<unsigned> str;
  if (auto const range = str (static_cast<unsigned> (sig));
      ::write (file_descriptor, std::to_address (std::begin (range)),
               range.size ())) {
    /* do nothing */
  }
  (void)write_char (file_descriptor, '\n');
}

}  // end anonymous namespace

extern "C" {

[[noreturn]] static void handler (int const sig) {
  say_signal_number (STDERR_FILENO, sig);

  static std::array<void *, 20> arr;
  // get void*'s for all entries on the stack
  auto const size = ::backtrace (arr.data (), arr.size ());
  ::backtrace_symbols_fd (arr.data (), size, STDERR_FILENO);
  std::_Exit (EXIT_FAILURE);
}

}  // extern "C"

class sigsegv_backtrace {
public:
  sigsegv_backtrace () {
    if (static char const message[] = "Installing SIGSEGV handler\n";
        ::write (STDERR_FILENO, &message[0], strlength (message))) {
      /* do nothing */
    }
    (void)signal (SIGSEGV, handler);
  }
};

// NOLINTEND(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, hicpp-avoid-c-arrays, cppcoreguidelines-pro-bounds-array-to-pointer-decay)

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern sigsegv_backtrace backtracer;
// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables)
sigsegv_backtrace backtracer;

#endif  // (__linux__ || __APPLE__) && ICUBABY_HAVE_RANGES &&
        // ICUBABY_HAVE_CONCEPTS
