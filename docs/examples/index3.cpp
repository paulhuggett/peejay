//===- docs/examples/index3.cpp -------------------------------------------===//
//*  _           _           _____  *
//* (_)_ __   __| | _____  _|___ /  *
//* | | '_ \ / _` |/ _ \ \/ / |_ \  *
//* | | | | | (_| |  __/>  < ___) | *
//* |_|_| |_|\__,_|\___/_/\_\____/  *
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
#include "peejay/rules/helpers.hpp"

using namespace std::string_view_literals;

namespace {

struct multiport_address {
  std::string name;
  std::vector<unsigned> ports;
};

template <peejay::policy Policies>
using multiport_rule = peejay::rules::homogeneous_array_helper<Policies, std::vector<unsigned>>;

template <peejay::policy Policies>
class multiport_address_rule final : public peejay::rules::object_helper<Policies, 2, multiport_rule<Policies>> {
public:
  using policies = Policies;
  using value_type = multiport_address;

  explicit multiport_address_rule(multiport_address* const addr) {
    this->set_properties({{u8"name"sv, &addr->name}, {u8"ports"sv, &addr->ports}});
  }
  [[nodiscard]] constexpr char const* name() const noexcept override { return "multiport-address"; }
};

}  // end anonymous namespace

int main() {
  multiport_address addr;
  using policies = peejay::default_policies;
  peejay::rules::context<policies, multiport_address_rule<policies>> context{&addr};
  auto parser = context.make_parser();
  parser.input(u8R"({"name":"http","ports":[80,8080]})"sv).eof();
  if (parser.has_error()) {
    std::cerr << "error: " << parser.last_error().message() << '\n';
    return EXIT_FAILURE;
  }
  std::cout << "name:" << addr.name << ", ports:[";
  std::ranges::copy(addr.ports, std::ostream_iterator<unsigned>(std::cout, ","));
  std::cout << "]\n";
}
