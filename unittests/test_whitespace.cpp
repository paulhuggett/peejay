#include <gtest/gtest.h>

#include "callbacks.hpp"
#include "peejay/json.hpp"

using namespace std::string_view_literals;
using peejay::char_set;
using peejay::error;
using peejay::extensions;
using peejay::make_parser;

namespace {

class Whitespace : public testing::Test {
protected:
  testing::StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end of anonymous namespace

// NOLINTNEXTLINE
TEST_F (Whitespace, Empty) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  auto p = make_parser (proxy_);
  p.input (u8"0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Whitespace, MultipleLeadingSpaces) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  auto p = make_parser (proxy_);
  p.input (u8"    0"sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Whitespace, MultipleTrailingSpaces) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  auto p = make_parser (proxy_);
  p.input (u8"0    "sv).eof ();
  EXPECT_FALSE (p.has_error ());
}

namespace {

std::u32string const extra_ws_chars{char_set::character_tabulation,
                                    char_set::vertical_tabulation,
                                    char_set::space,
                                    char_set::no_break_space,
                                    char_set::en_quad,
                                    '0'};

} // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (Whitespace, ExtendedWhitespaceCharactersEnabled) {
  EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
  auto p = make_parser (proxy_, extensions::extra_whitespace);
  p.input (extra_ws_chars).eof ();
  EXPECT_FALSE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Whitespace, ExtendedWhitespaceCharactersDisabled) {
  auto p = make_parser (proxy_);
  p.input (extra_ws_chars).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}
