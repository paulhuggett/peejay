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
    std::array<int, av_size> primes {{2, 3, 5, 7, 11, 13, 17, 19}};
}

template <typename Container>
void populate (Container& c, std::size_t n) {
    for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
        c.emplace_back (primes[ctr]);
    }
}

int main () {
  try {
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    std::size_t size;
    klee_make_symbolic (&size, sizeof size, "size");
    klee_assume (size > 0);
    klee_assume (size <= av_size);

    arrayvec_type::size_type pos;
    klee_make_symbolic (&pos, sizeof pos, "pos");
    // (The end iterator is not valid for erase().)
    klee_assume (pos < size);

    klee_make_symbolic (&member::throw_number, sizeof (member::throw_number),
                        "throw_number");

    arrayvec_type av;
    populate (av, size);

    // Call the function under test.
    av.erase (av.begin () + pos);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, size);
    // A mirror call to std::vector<>::insert for comparison.
    v.erase (v.begin () + pos);

    if (!std::equal (av.begin (), av.end (), v.begin (), v.end ())) {
      std::cerr << "** Fail!\n";
      return EXIT_FAILURE;
    }
#endif  // KLEE_RUN
  } catch (...) {
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
