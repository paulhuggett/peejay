#ifndef VCOMMON_HPP
#define VCOMMON_HPP

#include <array>
#include <cassert>

#define MAKE_SYMBOLIC(x) klee_make_symbolic (&(x), sizeof (x), #x)

constexpr std::size_t av_size = 8;
inline std::array<int, av_size> const primes{{2, 3, 5, 7, 11, 13, 17, 19}};

template <typename Container>
Container& populate (Container& c, std::size_t n) {
  assert (n <= av_size);
  for (auto ctr = std::size_t{0}; ctr < n; ++ctr) {
    c.emplace_back (primes[ctr]);
  }
  return c;
}

#endif  // VCOMMON_HPP
