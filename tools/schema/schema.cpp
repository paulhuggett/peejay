//===- tools/schema/schema.cpp --------------------------------------------===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//

#include <fstream>
#include <iomanip>
#include <iostream>

#include "peejay/dom.hpp"

using namespace peejay;
using pjparser = parser<dom<1024>>;

namespace {

template <typename T>
PEEJAY_CXX20REQUIRES (std::is_integral_v<T>)
constexpr auto as_unsigned (T v) {
  return static_cast<std::make_unsigned_t<T>> (std::max (T{0}, v));
}

void parse_error (pjparser& p, char const* file_path) {
  auto const& pos = p.pos ();
  std::cerr << file_path << ':' << static_cast<unsigned> (peejay::line (pos))
            << ':' << static_cast<unsigned> (peejay::column (pos)) << ':'
            << " error: " << p.last_error ().message () << '\n';
}

std::optional<element> parse (char const* file_path) {
  pjparser p;
  std::ifstream in{file_path};
  std::array<char, 256> buffer{};

  while (in.rdstate () == std::ios_base::goodbit) {
    auto* const data = buffer.data ();
    in.read (data, buffer.size ());
    auto const available = as_unsigned (in.gcount ());
    // TODO(paul) I just assume that the IStream yields UTF-8.
#if PEEJAY_HAVE_SPAN
    p.input (std::span{pointer_cast<peejay::char8 const> (data), available});
#else
    p.input (data, data + available);
#endif  // PEEJAY_HAVE_SPAN
    if (auto const err = p.last_error ()) {
      parse_error (p, file_path);
      std::exit (EXIT_FAILURE);
    }
  }
  if ((in.rdstate () & std::ios_base::badbit) != 0) {
    std::cerr << "Could not read " << std::quoted (file_path) << '\n';
    std::exit (EXIT_FAILURE);
  }

  std::optional<peejay::element> result = p.eof ();
  if (auto const erc = p.last_error ()) {
    parse_error (p, file_path);
    std::exit (EXIT_FAILURE);
  }
  return result;
}

}  // end anonymous namespace

int main (int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: " << argv[0] << " <schema> <input>\n";
      return EXIT_FAILURE;
    }

    auto schema = parse (argv[1]);
    auto input = parse (argv[2]);

    // put useful stuff here...

  } catch (std::exception const& ex) {
    std::cerr << "Error: " << ex.what () << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
