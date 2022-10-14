// Self include
#include "json/json.hpp"

// 3rd party includes
#include <gtest/gtest.h>

TEST (Coord, DefaultCtor) {
  using namespace peejay;
  coord c;
  EXPECT_EQ (c.row, 1U);
  EXPECT_EQ (c.column, 1U);
}

TEST (Coord, RowThenColumnInit) {
  using namespace peejay;
  coord c{row{2U}, column{3U}};
  EXPECT_EQ (c.row, 2U);
  EXPECT_EQ (c.column, 3U);
}

TEST (Coord, ColumnThenRowInit) {
  using namespace peejay;
  coord c{column{2U}, row{3U}};
  EXPECT_EQ (c.row, 3U);
  EXPECT_EQ (c.column, 2U);
}

TEST (Coord, Eq) {
  using namespace peejay;
  coord lhs{column{2U}, row{3U}};
  coord rhs{row{3U}, column{2U}};
  EXPECT_TRUE (lhs == rhs);
  EXPECT_FALSE (lhs != rhs);
}

TEST (Coord, Neq) {
  using namespace peejay;
  coord lhs{column{2U}, row{3U}};
  coord rhs{row{5U}, column{7U}};
  EXPECT_TRUE (lhs != rhs);
  EXPECT_FALSE (lhs == rhs);
}
