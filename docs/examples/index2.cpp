//===- docs/examples/index2.cpp -------------------------------------------===//
//*  _           _           ____   *
//* (_)_ __   __| | _____  _|___ \  *
//* | | '_ \ / _` |/ _ \ \/ / __) | *
//* | | | | | (_| |  __/>  < / __/  *
//* |_|_| |_|\__,_|\___/_/\_\_____| *
//*                                 *
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
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "peejay/parser.hpp"
#include "peejay/rules/non_terminal_rule.hpp"

using namespace std::string_view_literals;

namespace {

struct address {
  std::string name;
  unsigned port = 0;
};

template <peejay::policies Policies> class address_rule final : public peejay::rules::object_rule<Policies, 3> {
public:
  using policies = Policies;
  using value_type = address;

  explicit address_rule(address* const addr) {
    this->set_properties({{u8"name"sv, &addr->name}, {u8"port"sv, &addr->port}});
  }
  [[nodiscard]] constexpr char const* name() const noexcept override { return "address"; }
};

template <peejay::policies Policies>
using address_array_rule =
    peejay::rules::homogeneous_array_rule<Policies, std::vector<address>, address_rule<Policies>>;

}  // end anonymous namespace

int main() {
  std::vector<address> addresses;
  using policies = peejay::default_policies;
  peejay::rules::context<policies, address_array_rule<policies>> context{&addresses};
  auto parser = context.make_parser();
  parser.input(u8R"([{"name":"http","port":80},{"name":"ssh","port":22}])"sv).eof();
  if (parser.has_error()) {
    std::cerr << "error: " << parser.last_error().message() << '\n';
    return EXIT_FAILURE;
  }
  for (auto const& addr : addresses) {
    std::cout << "name:" << addr.name << ", port:" << addr.port << '\n';
  }
}
