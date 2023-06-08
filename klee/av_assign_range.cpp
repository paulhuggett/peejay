#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "peejay/arrayvec.hpp"

int main () {
  constexpr std::size_t av_size = 8;
  std::array<int, av_size> src{{11, 13, 17, 19, 23, 29, 31, 37}};
  peejay::arrayvec<int, av_size> av{3, 5, 7};

  std::size_t first;
  std::size_t last;
  klee_make_symbolic (&first, sizeof first, "first");
  klee_make_symbolic (&last, sizeof last, "last");
  klee_assume (first < av_size);
  klee_assume (last < av_size);
  klee_assume (first <= last);

  auto b = std::begin (src);
  av.assign (b + first, b + last);

#ifdef KLEE_RUN
  std::vector<int> v{3, 5, 7};
  v.assign (b + first, b + last);
  if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
    std::cerr << "** Fail!\n";
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
  return EXIT_SUCCESS;
#endif
}
