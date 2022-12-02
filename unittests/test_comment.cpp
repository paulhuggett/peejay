//===- unittests/test_comment.cpp -----------------------------------------===//
//*                                           _    *
//*   ___ ___  _ __ ___  _ __ ___   ___ _ __ | |_  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \ '_ \| __| *
//* | (_| (_) | | | | | | | | | | |  __/ | | | |_  *
//*  \___\___/|_| |_| |_|_| |_| |_|\___|_| |_|\__| *
//*                                                *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "callbacks.hpp"
#include "peejay/json.hpp"

using namespace std::string_view_literals;

using namespace peejay;

namespace {

class Comment : public testing::Test {
protected:
  testing::StrictMock<mock_json_callbacks> callbacks_;
  callbacks_proxy<mock_json_callbacks> proxy_{callbacks_};
};

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST_F (Comment, BashDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"# comment\nnull"sv).eof ();
  EXPECT_TRUE (p.has_error ());
}

// NOLINTNEXTLINE
TEST_F (Comment, BashSingleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::bash_comments);
  p.input (u8"# comment\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, BashMultipleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::bash_comments);
  p.input (u8"# comment\n\n    # remark\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, BashTrailing) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::bash_comments);
  p.input (u8"null # comment"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, BashInsideArray) {
  using testing::_;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::bash_comments);
  p.input (
       u8R"([#comment
1,     # comment containing #
2 # comment
]
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"// comment\nnull"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineSingleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::single_line_comments);
  p.input (u8"// comment\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineMultipleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::single_line_comments);
  p.input (u8"// comment\n\n    // remark\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineTrailing) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::single_line_comments);
  p.input (u8"null // comment"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineInsideArray) {
  using testing::_;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::single_line_comments);
  p.input (
       u8R"([//comment
1,    // comment containing //
2 // comment
]
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, SingleLineRowCounting) {
  using testing::_;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::single_line_comments);
  p.input (
       u8R"([ //comment
1, // comment
2 // comment
] // comment
// comment
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{line{4U}, column{1U}}))
      << "Comments count as whitespace so the last token start was line 4";
  EXPECT_EQ (p.input_pos (), (coord{line{6U}, column{1U}}));
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineDisabled) {
  auto p = make_parser (proxy_);
  p.input (u8"// comment\nnull"sv).eof ();
  EXPECT_TRUE (p.has_error ());
  EXPECT_EQ (p.last_error (), make_error_code (error::expected_token));
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineSingleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (u8"/* comment */\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineMultipleLeading) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (u8"/* comment\ncomment */\nnull"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineTrailing) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (u8"null\n/* comment */\n"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineInsideArray) {
  using testing::_;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (
       u8R"([ /* comment */
1,    /* comment containing / * */
2 /* comment */
]
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}

// NOLINTNEXTLINE
TEST_F (Comment, MultiLineRowCounting) {
  using testing::_;
  EXPECT_CALL (callbacks_, begin_array ()).Times (1);
  EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
  EXPECT_CALL (callbacks_, end_array ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (
       u8R"([ /*comment */
1, /* comment
comment
*/
2 /* comment */
]
/* comment
comment */
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{line{6U}, column{1U}}));
  EXPECT_EQ (p.input_pos (), (coord{line{9U}, column{1U}}));
}

// A missing multi-line comment close is currently ignored. It could reasonably
// raise an error, but at this point I've chosen not to do so.
// NOLINTNEXTLINE
TEST_F (Comment, MultiLineUnclosed) {
  using testing::_;
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::multi_line_comments);
  p.input (u8"null /*comment"sv).eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
  EXPECT_EQ (p.pos (), (coord{line{1U}, column{5U}}));
  EXPECT_EQ (p.input_pos (), (coord{line{1U}, column{15U}}));
}

// NOLINTNEXTLINE
TEST_F (Comment, Mixed) {
  EXPECT_CALL (callbacks_, null_value ()).Times (1);

  auto p = make_parser (proxy_, extensions::bash_comments |
                                    extensions::single_line_comments |
                                    extensions::multi_line_comments);
  p.input (
       u8R"(# comment 1
// comment 2
/* comment 3 */
null
)"sv)
      .eof ();
  EXPECT_FALSE (p.has_error ())
      << "JSON error was: " << p.last_error ().message ();
}
