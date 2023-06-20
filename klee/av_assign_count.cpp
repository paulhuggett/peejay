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

template <typename Container>
void populate (Container& c, std::size_t n) {
  assert (n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back (primes[ctr]);
  }
}

}  // namespace

int main () {
  try {
    MAKE_SYMBOLIC (member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC (size);
    klee_assume (size <= av_size);

    peejay::arrayvec<member, av_size>::size_type count;
    MAKE_SYMBOLIC (count);
    klee_assume (count <= av_size);

    peejay::arrayvec<member, av_size> av;
    populate (av, size);

    // Call the function under test.
    av.assign (count, member{23});

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);
    // A mirror call to std::vector<>::assign for comparison.
    v.assign (count, member{23});

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
