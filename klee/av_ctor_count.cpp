#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/arrayvec.hpp"

#define MAKE_SYMBOLIC(x) klee_make_symbolic (&(x), sizeof (x), #x)

namespace {

constexpr std::size_t av_size = 8;

}  // namespace

int main () {
  try {
    MAKE_SYMBOLIC (member::throw_number);

    peejay::arrayvec<member, av_size>::size_type count;
    MAKE_SYMBOLIC (count);
    klee_assume (count <= av_size);

    peejay::arrayvec<member, av_size> av{count};

#ifdef KLEE_RUN
    std::vector<member> v{count};

    if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }
#endif  // KLEE_RUN
  } catch (memberex const&) {
  }
#ifdef KLEE_RUN
  if (auto const inst = member::instances (); inst != 0) {
    std::cerr << "** Fail: instances = " << inst << '\n';
    return EXIT_FAILURE;
  }
  std::cerr << "Pass!\n";
#endif  // KLEE_RUN
  return EXIT_SUCCESS;
}
