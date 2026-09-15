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
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>

#include "peejay/json.hpp"

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

  static std::error_code boolean_value(bool const b) { return show(b ? "true" : "false"); }
  static std::error_code float_value(policy::float_type const v) { return show(v); }
  static std::error_code integer_value(policy::integer_type const v) { return show(v); }
  static std::error_code null_value() { return show("null"); }
  static std::error_code string_value(std::u8string_view const& sv) { return show(sv); }
  static std::error_code begin_array() { return show('['); }
  static std::error_code end_array() { return show(']'); }
  static std::error_code begin_object() { return show('{'); }
  static std::error_code key(std::u8string_view const& sv) { return show(sv); }
  static std::error_code end_object() { return show('}'); }

private:
  template <typename T> static std::error_code show(T const& value) {
    std::cout << value << '\n';
    return {};
  }
  static std::error_code show(std::u8string_view const& sv) {
    std::cout << '"';
    std::ranges::copy(std::ranges::subrange{sv} | std::ranges::views::transform([] (char8_t c) { return static_cast<char>(c); }), std::ostream_iterator<char>{std::cout});
    std::cout << "\"\n";
    return {};
  }
};

}  // end anonymous namespace

int main() {
  using namespace std::string_view_literals;

  int exit_code = EXIT_SUCCESS;
  peejay::parser<print_tokens> p;
  p.input(u8R"(  { "a":123, "b" : [false,"c"], "c":true }  )"sv).eof();
  if (auto const err = p.last_error()) {
    std::cout << "Error: " << err.message() << '\n';
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
