// Self include
#include "json/json.hpp"

// 3rd party includes
#include <gtest/gtest.h>

using namespace peejay;

TEST (Coord, DefaultCtor) {
  coord c;
  EXPECT_EQ (c.row, 1U);
  EXPECT_EQ (c.column, 1U);
}

TEST (Coord, RowThenColumnInit) {
  coord c{row{2U}, column{3U}};
  EXPECT_EQ (c.row, 2U);
  EXPECT_EQ (c.column, 3U);
}

TEST (Coord, ColumnThenRowInit) {
  coord c{column{2U}, row{3U}};
  EXPECT_EQ (c.row, 3U);
  EXPECT_EQ (c.column, 2U);
}

TEST (Coord, Eq) {
  coord lhs{column{2U}, row{3U}};
  coord rhs{row{3U}, column{2U}};
  EXPECT_TRUE (lhs == rhs);
  EXPECT_FALSE (lhs != rhs);
}

TEST (Coord, Neq) {
  coord lhs{column{2U}, row{3U}};
  coord rhs{row{5U}, column{7U}};
  EXPECT_TRUE (lhs != rhs);
  EXPECT_FALSE (lhs == rhs);
}

TEST (Coord, Lt) {
  EXPECT_TRUE ((coord{row{3U}, column{1U}} < coord{row{4U}, column{1U}}));
  EXPECT_TRUE ((coord{row{3U}, column{2U}} < coord{row{4U}, column{1U}}));
  EXPECT_TRUE ((coord{row{3U}, column{1U}} < coord{row{3U}, column{2U}}));
  EXPECT_FALSE ((coord{row{4U}, column{1U}} < coord{row{3U}, column{2U}}));
  EXPECT_FALSE (coord{} < coord{});
}

TEST (Coord, LtEq) {
  EXPECT_TRUE ((coord{row{3U}, column{1U}} <= coord{row{4U}, column{1U}}));
  EXPECT_TRUE ((coord{row{3U}, column{2U}} <= coord{row{4U}, column{1U}}));
  EXPECT_TRUE ((coord{row{3U}, column{1U}} <= coord{row{3U}, column{2U}}));
  EXPECT_FALSE ((coord{row{4U}, column{1U}} <= coord{row{3U}, column{2U}}));
  EXPECT_TRUE (coord{} <= coord{});
}
