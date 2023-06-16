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
    constexpr std::size_t size = 3;
    using arrayvec_type = peejay::arrayvec<member, av_size>;

    arrayvec_type::size_type pos;
    klee_make_symbolic (&pos, sizeof pos, "pos");
    klee_assume (pos <= size);
    arrayvec_type::size_type count;
    klee_make_symbolic (&count, sizeof count, "count");
    klee_assume (count <= av_size - size);

    klee_make_symbolic (&member::throw_number, sizeof (member::throw_number),
                        "throw_number");

    arrayvec_type av;
    populate (av);
    assert (av.size () == size);

    member value{11};

    // Call the function under test.
    av.insert (av.begin () + pos, count, value);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v);
    // A mirror call to std::vector<>::insert for comparison.
    v.insert (v.begin () + pos, count, value);

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
