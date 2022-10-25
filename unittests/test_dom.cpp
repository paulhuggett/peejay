#include <gmock/gmock.h>

#include "json/dom.hpp"
#include "json/json.hpp"

using namespace peejay;
using namespace std::string_literals;
using namespace std::string_view_literals;

bool operator== (std::nullopt_t const &, std::nullopt_t const &) noexcept {
  return true;
}
bool operator== (dom::mark, dom::mark) noexcept {
  return true;
}

TEST (Dom, One) {
  dom::element const root = make_parser (dom{}).input ("1"sv).eof ();
  EXPECT_EQ (std::get<uint64_t> (root), 1U);
}

TEST (Dom, NegativeOne) {
  dom::element const root = make_parser (dom{}).input ("-1"sv).eof ();
  EXPECT_EQ (std::get<int64_t> (root), -1);
}

TEST (Dom, String) {
  dom::element const root = make_parser (dom{}).input (R"("string")"sv).eof ();
  EXPECT_EQ (std::get<std::string> (root), "string");
}

TEST (Dom, Double) {
  dom::element const root = make_parser (dom{}).input ("3.14"sv).eof ();
  EXPECT_DOUBLE_EQ (std::get<double> (root), 3.14);
}

TEST (Dom, BooleanTrue) {
  dom::element const root = make_parser (dom{}).input ("true"sv).eof ();
  EXPECT_TRUE (std::get<bool> (root));
}

TEST (Dom, BooleanFalse) {
  dom::element const root = make_parser (dom{}).input ("false"sv).eof ();
  EXPECT_FALSE (std::get<bool> (root));
}

TEST (Dom, Array) {
  using testing::ElementsAre;
  dom::element const root = make_parser (dom{}).input ("[1,2]"sv).eof ();
  EXPECT_THAT (
      std::get<dom::array> (root),
      ElementsAre (dom::element{uint64_t{1}}, dom::element{uint64_t{2}}));
}

TEST (Dom, Object) {
  using testing::UnorderedElementsAre;
  dom::element const root =
      make_parser (dom{}).input (R"({"a":1,"b":2})"sv).eof ();
  EXPECT_THAT (
      std::get<dom::object> (root),
      UnorderedElementsAre (std::make_pair ("a"s, dom::element{uint64_t{1}}),
                            std::make_pair ("b"s, dom::element{uint64_t{2}})));
}
