//===- main.cpp -----------------------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/paulhuggett/json/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <array>
#include <fstream>
#include <iostream>

#include "json/json.hpp"
#include "json/utf.hpp"

namespace {

class json_writer {
public:
  explicit json_writer (std::ostream& os) : os_{os} {}

  using result_type = void;
  result_type result () {}

  std::error_code string_value (std::string_view const& s) {
    os_ << '"';
    std::copy (std::begin (s), std::end (s),
               std::ostream_iterator<char>{os_, ""});
    os_ << '"';
    return {};
  }

  std::error_code int64_value (std::int64_t v) { return write (v); }
  std::error_code uint64_value (std::uint64_t v) { return write (v); }
  std::error_code double_value (double v) { return write (v); }
  std::error_code boolean_value (bool v) {
    return write (v ? "true" : "false");
  }
  std::error_code null_value () { return write ("null"); }

  std::error_code begin_array () { return write ('['); }
  std::error_code end_array () { return write (']'); }

  std::error_code begin_object () { return write ('{'); }
  std::error_code key (std::string_view const& s) {
    this->string_value (s);
    return write (": ");
  }
  std::error_code end_object () { return write ('}'); }

private:
  template <typename T>
  std::error_code write (T t) {
    os_ << t;
    return {};
  }
  std::ostream& os_;
};

template <typename IStream>
std::error_code slurp (IStream&& in) {
  auto p = json::make_parser (json_writer{std::cout});

  using ustreamsize = std::make_unsigned_t<std::streamsize>;
  std::array<char, 256> buffer{{0}};

  while ((in.rdstate () & (std::ios_base::badbit | std::ios_base::failbit |
                           std::ios_base::eofbit)) == 0) {
    auto* const data = buffer.data ();
    in.read (data, buffer.size ());
    auto const available =
        static_cast<ustreamsize> (std::max (in.gcount (), std::streamsize{0}));
    p.input (std::span<char>{data, available});
    if (auto const err = p.last_error ()) {
      return err;
    }
  }

  p.eof ();
  return p.last_error ();
}

}  // end anonymous namespace

int main (int argc, char const* argv[]) {
  int exit_code = EXIT_SUCCESS;
  try {
    auto const err =
        argc < 2 ? slurp (std::cin) : slurp (std::ifstream{argv[1]});
    if (err) {
      std::cerr << "Error: " << err.message () << '\n';
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
