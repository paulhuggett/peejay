#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "peejay/arrayvec.hpp"

int main () {
  static constexpr std::size_t av_size = 8;
  peejay::arrayvec<int, av_size> av{3, 5, 7};

  decltype (av)::size_type count;
  klee_make_symbolic (&count, sizeof count, "count");
  klee_assume (count <= av_size);
  int value = 11;

  av.assign (count, value);

#ifdef KLEE_RUN
  std::vector<int> v{3, 5, 7};
  v.assign (count, value);

  if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
    std::cerr << "** Fail!\n";
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
  return EXIT_SUCCESS;
#endif
}
