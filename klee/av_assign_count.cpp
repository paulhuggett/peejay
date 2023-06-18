#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/arrayvec.hpp"

template <typename Container>
void populate (Container& c) {
  c.emplace_back (1);
  c.emplace_back (3);
  c.emplace_back (5);
}

int main () {
  try {
    constexpr std::size_t av_size = 8;

    peejay::arrayvec<member, av_size>::size_type count;
    klee_make_symbolic (&count, sizeof (count), "count");
    klee_assume (count <= av_size);
    klee_make_symbolic (&member::throw_number, sizeof (member::throw_number),
                        "throw_number");

    peejay::arrayvec<member, av_size> av;
    populate (av);

    // Call the function under test.
    av.assign (count, member{7});

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v);
    // A mirror call to std::vector<>::assign for comparison.
    v.assign (count, member{7});

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
