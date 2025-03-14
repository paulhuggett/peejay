//===- tools/schema/schema.cpp --------------------------------------------===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//

#include "peejay/schema/schema.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <variant>

using namespace peejay;
using namespace std::string_view_literals;
using pjparser = parser<dom<1024>>;

namespace {

template <typename T>
  requires(std::is_integral_v<T>)
constexpr auto as_unsigned(T v) {
  return static_cast<std::make_unsigned_t<T>>(std::max(T{0}, v));
}

void parse_error(pjparser const& p) {
  auto const& pos = p.pos();
  std::cerr << pos.get_line() << ':' << pos.get_column() << ':' << " error: " << p.last_error().message() << '\n';
}

void parse_error(pjparser& p, std::filesystem::path const& file_path) {
  std::cerr << file_path << ':';
  parse_error(p);
}

std::optional<element> parse(std::filesystem::path const& file_path) {
  pjparser p;
  std::ifstream in{file_path};
  std::array<char, 256> buffer{};

  while (in.rdstate() == std::ios_base::goodbit) {
    auto* const data = buffer.data();
    in.read(data, buffer.size());
    auto const available = as_unsigned(in.gcount());
    // TODO(paul) I just assume that the IStream yields UTF-8.
    p.input(std::span{pointer_cast<std::byte const>(data), available});
    if (auto const err = p.last_error()) {
      parse_error(p, file_path);
      std::exit(EXIT_FAILURE);
    }
  }
  if ((in.rdstate() & std::ios_base::badbit) != 0) {
    std::cerr << "Could not read " << file_path << '\n';
    std::exit(EXIT_FAILURE);
  }

  std::optional<peejay::element> result = p.eof();
  if (auto const erc = p.last_error()) {
    parse_error(p, file_path);
    std::exit(EXIT_FAILURE);
  }
  return result;
}

}  // end anonymous namespace

int main(int argc, char* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    if (argc != 3) {
      std::cerr << "Usage: " << argv[0] << " <schema> <input>\n";
      return EXIT_FAILURE;
    }

    auto schema = parse(argv[1]);
    auto instance = parse(argv[2]);
    if (schema && instance) {
      if (std::error_code const erc = peejay::schema::check(*schema, *instance)) {
        std::cerr << "Error: " << erc.message() << '\n';
        exit_code = EXIT_FAILURE;
      }
    }
  } catch (std::exception const& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
