#include <cstddef>
#include <iostream>

#ifdef KLEE
#include "klee/klee.h"
#endif

#include "json/json.hpp"
int main () {
#ifdef KLEE
  static constexpr std::size_t const size = 10;
  char input[size];

  // Make the input symbolic.
  klee_make_symbolic (input, sizeof input, "input");
  input[size - 1] = '\0';

  json::parser p;
  p.match_number (input);
#endif
}
