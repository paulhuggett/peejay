//===- main.cpp -----------------------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
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
#include <array>
#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <string>

#include "icubaby/icubaby.hpp"
#include "peejay/json.hpp"

using namespace std::literals::string_view_literals;

namespace {

struct policy : public peejay::default_policies {
  static constexpr std::size_t max_length = 64;
  static constexpr std::size_t max_stack_depth = 8;
  static constexpr bool pos_tracking = true;

  using float_type = double;
  using integer_type = std::int64_t;
};

/// A simple backend which prints tokens as they arrive. The resulting output should
/// look somewhat like the input, but is not legal JSON!
class print_tokens {
public:
  using policies = policy;
  static constexpr void result() noexcept {
    // This backend produces no result at all.
  }

  static std::error_code boolean_value(bool const b) { return show(b ? "true"sv : "false"sv); }
  static std::error_code float_value(policy::float_type const v) { return show(v); }
  static std::error_code integer_value(policy::integer_type const v) { return show(v); }
  static std::error_code null_value() { return show("null"sv); }
  static std::error_code string_value(std::u8string_view const& sv) { return show(sv, /*is_key=*/false); }
  static std::error_code begin_array() { return show('['); }
  static std::error_code end_array() { return show(']'); }
  static std::error_code begin_object() { return show('{'); }
  static std::error_code key(std::u8string_view const& sv) { return show(sv, /*is_key=*/true); }
  static std::error_code end_object() { return show('}'); }

private:
  template <typename T> static std::error_code show(T const& value) {
    std::cout << value << '\n';
    return {};
  }
  static std::error_code show(std::u8string_view const& sv, bool const is_key) {
    std::cout << '"';
    std::ranges::copy(std::ranges::subrange{sv} | std::ranges::views::transform([] (char8_t c) { return static_cast<char>(c); }), std::ostream_iterator<char>{std::cout});
    std::cout << '"' << (is_key ? ":" : "") << '\n';
    return {};
  }
};

class deleter {
public:
  constexpr explicit deleter(bool const enabled) : enabled_{enabled} {}
  void operator()(std::istream* ptr) const noexcept {
    if (enabled_) {
      delete ptr;
    }
  }

private:
  bool enabled_ = false;
};

}  // end anonymous namespace

int main(int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;

  using uptr = std::unique_ptr<std::istream, deleter>;
  uptr in{nullptr, deleter{false}};
  std::string_view file_name = "<stdin>";
  if (argc < 2) {
    in = uptr{&std::cin, deleter{false}};
  } else {
    file_name = std::string_view{argv[1]};
    in = uptr{new std::ifstream{argv[1], std::ios::binary}, deleter{true}};
    if (!*in) {
      std::cerr << "Could not open \"" << file_name << "\"\n";
      return EXIT_FAILURE;
    }
  }

  peejay::parser<print_tokens> p;
  std::array<char8_t, 256> buffer{};
  std::array<char8_t, icubaby::longest_sequence_v<char8_t>> trans_out{};
  // The UTF-8 to UTF-8 transcoder is used purely to validate the incoming byte stream:
  // for well-formed input its output is identical to its input, so we discard it and
  // pass the original bytes to the parser.
  icubaby::t8_8 utf8;
  std::size_t offset = 0;
  while (in && utf8.well_formed()) {
    // char may alias any object type, so filling the char8_t array through a char* is OK.
    in->read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    auto const n = static_cast<std::size_t>(in->gcount());
    if (n == 0) {
      break;
    }
    // Count the bytes that are well formed and give just those to the parser.
    std::size_t good = 0;
    for (; good < n; ++good) {
      utf8(buffer[good], trans_out.begin());
      if (!utf8.well_formed()) {
        break;
      }
    }
    offset += good;
    p.input(std::span{buffer.data(), good});
  }

  p.input(std::span{trans_out.begin(), utf8.end_cp(trans_out.begin())});

  if (in->bad()) {
    std::cerr << file_name << ": error: I/O error\n";
    exit_code = EXIT_FAILURE;
  } else if (!utf8.well_formed()) {
    std::cerr << file_name << ": error: ill-formed UTF-8 at byte offset " << offset << '\n';
    exit_code = EXIT_FAILURE;
  } else if (utf8.partial()) {
    std::cerr << file_name << ": error: input ends with an incomplete UTF-8 sequence\n";
    exit_code = EXIT_FAILURE;
  } else {
    // Tell the parser that the input has ended.
    p.eof();
    if (auto const err = p.last_error()) {
      std::cerr << file_name << ':' << p.pos().line << ": error: " << err.message() << '\n';
      exit_code = EXIT_FAILURE;
    }
  }
  return exit_code;
}
