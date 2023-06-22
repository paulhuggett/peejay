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
inline std::array<int, av_size> const primes{{2, 3, 5, 7, 11, 13, 17, 19}};

}  // namespace

int main () {
  try {
    MAKE_SYMBOLIC (member::throw_number);

    std::size_t first;
    std::size_t last;
    MAKE_SYMBOLIC (first);
    MAKE_SYMBOLIC (last);
    klee_assume (last < av_size);
    klee_assume (first <= last);

    auto const b = primes.begin ();
    peejay::arrayvec<member, av_size> av{b + first, b + last};

#ifdef KLEE_RUN
    std::vector<member> v{b + first, b + last};

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
