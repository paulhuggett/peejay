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

#include "peejay/schema.hpp"

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
PEEJAY_CXX20REQUIRES (std::is_integral_v<T>)
constexpr auto as_unsigned (T v) {
  return static_cast<std::make_unsigned_t<T>> (std::max (T{0}, v));
}

void parse_error (pjparser const& p) {
  auto const& pos = p.pos ();
  std::cerr << static_cast<unsigned> (peejay::line (pos)) << ':'
            << static_cast<unsigned> (peejay::column (pos)) << ':'
            << " error: " << p.last_error ().message () << '\n';
}

void parse_error (pjparser& p, std::filesystem::path const& file_path) {
  std::cerr << file_path << ':';
  parse_error (p);
}

std::optional<element> parse (peejay::u8string_view const& str) {
  pjparser p;
#if PEEJAY_HAVE_SPAN
  p.input (std::span{str.data (), str.size ()});
#else
  p.input (std::begin (str), std::end (str));
#endif  // PEEJAY_HAVE_SPAN
  std::optional<peejay::element> result = p.eof ();
  if (auto const erc = p.last_error ()) {
    parse_error (p);
    std::exit (EXIT_FAILURE);
  }
  return result;
}
std::optional<element> parse (std::filesystem::path const& file_path) {
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
    std::cerr << "Could not read " << std::quoted (file_path.native ()) << '\n';
    std::exit (EXIT_FAILURE);
  }

  std::optional<peejay::element> result = p.eof ();
  if (auto const erc = p.last_error ()) {
    parse_error (p, file_path);
    std::exit (EXIT_FAILURE);
  }
  return result;
}

void show_schema (object const& root) {
  if (auto schema_it = root->find (u8"$schema"); schema_it != root->end ()) {
    if (auto const* const str = std::get_if<u8string> (&schema_it->second)) {
      std::cout << std::string (str->begin (), str->end ()) << '\n';
    }
  }
}

void schema_const () {
  auto schema = parse (u8R"({ "const": 1234 })"sv);
  auto instance = parse (u8"1234"sv);
  assert (schema.has_value () && instance.has_value ());
  assert (check_schema (*schema, *instance));
}
void schema_type () {
  auto schema = parse (u8R"({ "type": "number" })"sv);
  assert (schema.has_value ());
  {
    auto instance1 = parse (u8"1234"sv);
    assert (instance1.has_value ());
    assert (check_schema (*schema, *instance1));
  }
  {
    auto instance2 = parse (u8"12.0"sv);
    assert (instance2.has_value ());
    assert (check_schema (*schema, *instance2));
  }
  {
    auto instance3 = parse (u8"-1234"sv);
    assert (instance3.has_value ());
    assert (check_schema (*schema, *instance3));
  }
  {
    auto instance4 = parse (u8R"("1234")"sv);
    assert (instance4.has_value ());
    assert (!check_schema (*schema, *instance4));
  }
}

}  // end anonymous namespace

int main (int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: " << argv[0] << " <schema> <input>\n";
      return EXIT_FAILURE;
    }

    schema_const ();
    schema_type ();
    auto schema = parse (argv[1]);
    auto instance = parse (argv[2]);

    //  "$id": "https://example.com/product.schema.json",
    //  "title": "Product",
    //  "description": "A product in the catalog",
    //  "type": "object"

    if (schema && instance) {
      bool const ok = check_schema (*schema, *instance);
      std::cout << std::boolalpha << ok << '\n';
    }

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
