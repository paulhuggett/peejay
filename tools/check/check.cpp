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

namespace {

template <typename Backend, size_t MaxLength>
bool report_error (parser<Backend, MaxLength> const& p,
                   std::string_view const& file_name,
                   std::string_view const& line) {
  auto const& pos = p.pos ();
  std::cerr << file_name << ':' << pos.line << ':' << pos.column << ':'
            << " error: " << p.last_error ().message () << '\n'
            << line << std::string (pos.column - 1U, ' ') << "^\n";
  return false;
}

/// Convert from a string of char to a string of char8.
u8string& line_to_u8 (std::string const& line, u8string& u8line) {
  // TODO(paul) need to convert encoding from host to UTF-8 here?
  u8line.clear ();
  u8line.reserve (line.size ());
  auto const op = [] (char c) { return static_cast<char8> (c); };
  auto const out = std::back_inserter (u8line);
#if __cpp_lib_ranges
  std::ranges::transform (line, out, op);
#else
  std::transform (std::begin (line), std::end (line), out, op);
#endif
  return u8line;
}

/// Read an input stream line by line, feeding it to the parser. Any errors
/// encountered are reported to stderr.
bool slurp (std::istream& in, char const* file_name) {
  auto p = make_parser (null{}, peejay::extensions::all);
  std::string line;
  u8string u8line;
  while (in.rdstate () == std::ios_base::goodbit) {
    std::getline (in, line);
    line += '\n';
    p.input (line_to_u8 (line, u8line));
    if (auto const err = p.last_error ()) {
      return report_error (p, file_name, line);
    }
  }
  auto const state = in.rdstate ();
  if ((state & std::ios_base::eofbit) != 0) {
    p.eof ();
    if (auto const err = p.last_error ()) {
      return report_error (p, file_name, line);
    }
  } else if ((state & (std::ios_base::badbit | std::ios_base::failbit)) != 0) {
    std::cerr << "cannot read from \"" << file_name << "\"\n";
    return false;
  }
  return true;
}

std::pair<std::istream&, char const*> open (int argc, char const* argv[],
                                            std::ifstream& file) {
  if (argc < 2) {
    return {std::cin, "<stdin>"};
  }
  auto const* path = argv[1];
  file.open (path);
  return {file, path};
}

}  // end anonymous namespace

int main (int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    std::ifstream file;
    if (!std::apply (slurp, open (argc, argv, file))) {
      exit_code = EXIT_FAILURE;
    }
  } catch (std::exception const& ex) {
    std::cerr << "error: " << ex.what () << '\n';
    exit_code = EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception.\n";
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}
