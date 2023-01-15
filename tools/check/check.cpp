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
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include "peejay/json.hpp"
#include "peejay/null.hpp"

using peejay::char8;
using peejay::null;
using peejay::parser;
using peejay::u8string;

using null_parser = parser<null>;

static void report_error (null_parser const& p,
                          std::string_view const& file_name,
                          std::string_view const& line) {
  auto const& pos = p.pos ();
  std::cerr << file_name << ':' << pos.line << ':' << pos.column << ':'
            << " error: " << p.last_error ().message () << '\n'
            << line << '\n'
            << std::string (pos.column - 1U, ' ') << "^\n";
}

template <typename IStream>
static bool slurp (IStream&& in, char const* file_name) {
  null_parser p = make_parser (null{}, peejay::extensions::all);
  std::string line;
  u8string u8line;
  while ((in.rdstate () & (std::ios_base::badbit | std::ios_base::failbit |
                           std::ios_base::eofbit)) == 0) {
    std::getline (in, line);
    // TODO: need to convert encoding from host to UTF-8 here?
    u8line.clear();
    u8line.reserve (line.size ());
    std::transform (std::begin (line), std::end (line),
                    std::back_inserter (u8line),
                    [] (char c) { return static_cast<char8> (c); });
    p.input (u8line);
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
