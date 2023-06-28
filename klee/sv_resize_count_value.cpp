#include <cstddef>

#ifdef KLEE_RUN
#include <iostream>
#include <vector>
#endif

#include <klee/klee.h>

#include "av_member.hpp"
#include "peejay/small_vector.hpp"
#include "vcommon.hpp"

int main () {
  try {
    constexpr std::size_t body_elements = 5;
    constexpr std::size_t max_elements = 13;

    MAKE_SYMBOLIC (member::throw_number);

    peejay::small_vector<member, body_elements>::size_type initial_size;
    MAKE_SYMBOLIC (initial_size);
    klee_assume (initial_size <= av_size);

    peejay::small_vector<member, body_elements>::size_type new_size;
    MAKE_SYMBOLIC (new_size);
    klee_assume (new_size <= max_elements);

    peejay::small_vector<member, body_elements> sv;
    populate (sv, initial_size);

    member value{23};
    // Call the function under test.
    sv.resize (new_size, value);

#ifdef KLEE_RUN
    std::vector<member> v;
    populate (v, initial_size);
    // A mirror call to std::vector<>::assign for comparison.
    v.resize (new_size, value);

    if (!std::equal (sv.begin (), sv.end (), v.begin (), v.end ())) {
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