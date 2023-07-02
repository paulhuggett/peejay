#include <cctype>
#include <cstddef>

#include "klee/klee.h"
#include "peejay/null.hpp"

int main () {
  static constexpr std::size_t const size = 5;
  peejay::char8 input[size];

  klee_make_symbolic (input, sizeof input, "input");
  klee_assume (std::isalpha (static_cast<char> (input[0])) != 0);
  klee_assume (input[size - 1] == '\0');

  make_parser (peejay::null{}).input (peejay::u8string{input}).eof ();
}
