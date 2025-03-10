//===- tools/check/check.cpp ----------------------------------------------===//
//*       _               _     *
//*   ___| |__   ___  ___| | __ *
//*  / __| '_ \ / _ \/ __| |/ / *
//* | (__| | | |  __/ (__|   <  *
//*  \___|_| |_|\___|\___|_|\_\ *
//*                             *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "peejay/json/json.hpp"
#include "peejay/json/null.hpp"
#include "peejay/json/small_vector.hpp"

using peejay::char8;
using peejay::null;
using peejay::parser;
using peejay::u8string;

namespace {

template <typename... Params>
bool report_error(parser<Params...> const& p, std::string_view const& file_name, std::string_view const& line) {
  auto const& pos = p.pos();
  std::cerr << file_name << ':' << pos.get_line() << ':' << pos.get_column() << ':'
            << " error: " << p.last_error().message() << '\n'
            << line << std::string(pos.get_column() - 1U, ' ') << "^\n";
  return false;
}

/// Read an input stream line by line, feeding it to the parser. Any errors
/// encountered are reported to stderr.
bool slurp(std::istream& in, char const* file_name) {
  auto p = make_parser(null{}, peejay::extensions::all);
  std::string line;
  auto const op = [](char const c) {
    static_assert(sizeof(c) == sizeof(std::byte));
    return static_cast<std::byte>(c);
  };

  while (in.rdstate() == std::ios_base::goodbit) {
    std::getline(in, line);
    line += '\n';
    p.input(line | std::views::transform(op));
    if (auto const err = p.last_error()) {
      return report_error(p, file_name, line);
    }
  }
  if (auto const state = in.rdstate(); (state & std::ios_base::eofbit) != 0) {
    p.eof();
    if (auto const err = p.last_error()) {
      return report_error(p, file_name, line);
    }
  } else if ((state & (std::ios_base::badbit | std::ios_base::failbit)) != 0) {
    std::cerr << "cannot read from \"" << file_name << "\"\n";
    return false;
  }
  return true;
}

std::pair<std::istream&, char const*> open(std::span<char const*> argv, std::ifstream& file) {
  if (argv.size() < 2U) {
    return {std::cin, "<stdin>"};
  }
  auto const* path = argv[1];
  file.open(path);
  return {file, path};
}

}  // end anonymous namespace

int main(int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    std::ifstream file;
    if (!std::apply(slurp, open(std::span{argv, argv + argc}, file))) {
      exit_code = EXIT_FAILURE;
    }
  } catch (std::exception const& ex) {
    std::cerr << "error: " << ex.what() << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception.\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
