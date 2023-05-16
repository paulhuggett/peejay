#ifndef PEEJAY_CBII_HPP
#define PEEJAY_CBII_HPP

#include <cstddef>
#include <iterator>

namespace peejay {

template <typename Container>
class checked_back_insert_iterator {
public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

  constexpr checked_back_insert_iterator (Container *const container,
                                          bool *const overflow) noexcept
      : container_{container}, overflow_{overflow} {}

  constexpr checked_back_insert_iterator &operator= (
      typename Container::value_type const &value) {
    if (container_->size () >= container_->max_size ()) {
      *overflow_ = true;
    } else {
      container_->push_back (value);
    }
    return *this;
  }
  constexpr checked_back_insert_iterator &operator= (
      typename Container::value_type &&value) {
    if (container_->size () >= container_->max_size ()) {
      *overflow_ = true;
    } else {
      container_->push_back (std::move (value));
    }
    return *this;
  }

  constexpr checked_back_insert_iterator &operator* () { return *this; }
  constexpr checked_back_insert_iterator &operator++ () { return *this; }
  constexpr checked_back_insert_iterator operator++ (int) { return *this; }

private:
  Container *container_;
  bool *overflow_;
};

template <typename Container>
checked_back_insert_iterator (Container *, bool *)
    -> checked_back_insert_iterator<Container>;

}  // end namespace peejay

#endif  // PEEJAY_CBII_HPP
