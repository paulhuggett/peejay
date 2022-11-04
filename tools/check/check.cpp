//===- tools/check/check.cpp ----------------------------------------------===//
//*       _               _     *
//*   ___| |__   ___  ___| | __ *
//*  / __| '_ \ / _ \/ __| |/ / *
//* | (__| | | |  __/ (__|   <  *
//*  \___|_| |_|\___|\___|_|\_\ *
//*                             *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <fstream>
#include <iostream>
#include <string>

#include "json/json.hpp"
#include "json/null.hpp"

template <typename N>
void report_error (peejay::parser<N>& p, std::string_view const& file_name,
                   std::string_view const& line) {
  auto const& pos = p.pos ();
  std::cout << file_name << ':' << pos.line << ':' << pos.column << ':'
            << " error: " << p.last_error ().message () << '\n'
            << line << '\n'
            << std::string (pos.column - 1U, ' ') << "^\n";
}

template <typename IStream>
bool slurp (IStream&& in, char const* file_name) {
  auto p = make_parser (peejay::null{});
  std::string line;
  while ((in.rdstate () & (std::ios_base::badbit | std::ios_base::failbit |
                           std::ios_base::eofbit)) == 0) {
    std::getline (in, line);
    p.input (line);
    if (auto const err = p.last_error ()) {
      report_error (p, file_name, line);
      return false;
    }
  }
  p.eof ();
  if (auto const err = p.last_error ()) {
    report_error (p, file_name, line);
    return false;
  }
  return true;
}

int main (int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    if (!(argc < 2 ? slurp (std::cin, "<stdin>")
                   : slurp (std::ifstream{argv[1]}, argv[1]))) {
      exit_code = EXIT_FAILURE;
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