#include <cassert>
#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/arrayvec.hpp"

namespace {
constexpr std::size_t av_size = 8;
std::array<int, av_size> primes{{2, 3, 5, 7, 11, 13, 17, 19}};
}  // namespace

template <typename Container>
void populate (Container& c, std::size_t n) {
  assert (n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back (primes[ctr]);
  }
}

#define MAKE_SYMBOLIC(x) klee_make_symbolic (&(x), sizeof (x), #x)

int main () {
  try {
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    MAKE_SYMBOLIC (member::throw_number);

    std::size_t size;
    MAKE_SYMBOLIC (size);
    klee_assume (size > 0);
    klee_assume (size <= av_size);

    arrayvec_type::size_type first;
    arrayvec_type::size_type last;
    MAKE_SYMBOLIC (first);
    MAKE_SYMBOLIC (last);
    // (The end iterator is not valid for erase().)
    klee_assume (last < size);
    klee_assume (first <= last);

    arrayvec_type av;
    populate (av, size);

    // Call the function under test.
    av.erase (av.begin () + first, av.begin () + last);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);

    // A mirror call to std::vector<>::insert for comparison.
    v.erase (v.begin () + first, v.begin () + last);

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
