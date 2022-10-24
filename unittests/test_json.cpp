//===- unittests/test_json.cpp --------------------------------------------===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include "json/json.hpp"
// standard library
#include <stack>

#include "callbacks.hpp"
#include "json/dom_types.hpp"

using namespace std::string_literals;
using testing::DoubleEq;
using testing::StrictMock;
using namespace peejay;

namespace {

class Json : public ::testing::Test {
protected:
  static void check_error (std::string const& src, error_code err) {
    ASSERT_NE (err, error_code::none);
    parser p{json_out_callbacks{}};
    std::string const res = p.input (src).eof ();
    EXPECT_EQ (res, "");
    EXPECT_NE (p.last_error (), make_error_code (error_code::none));
  }

  static inline auto const cr = "\r"s;
  static inline auto const lf = "\n"s;
  static inline auto const crlf = cr + lf;
  static inline auto const keyword = "null"s;
  static inline auto const xord =
      static_cast<unsigned> (keyword.length ()) + 1U;
};

}  // end anonymous namespace

TEST_F (Json, Empty) {
  parser p{json_out_callbacks{}};
  p.input (std::string{}).eof ();
  EXPECT_EQ (p.last_error (), make_error_code (error_code::expected_token));
  EXPECT_EQ (p.coordinate (), (coord{column{1U}, row{1U}}));
}

TEST_F (Json, StringInput) {
  parser p{json_out_callbacks{}};
  std::string const res = p.input (keyword).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{5U}, row{1U}}));
}

TEST_F (Json, IteratorInput) {
  parser p{json_out_callbacks{}};
  std::string const res =
      p.input (std::begin (keyword), std::end (keyword)).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{5U}, row{1U}}));
}

TEST_F (Json, LeadingWhitespace) {
  parser p1{json_out_callbacks{}};
  std::string const res = p1.input ("   \t    null"s).eof ();
  EXPECT_FALSE (p1.has_error ());
  EXPECT_EQ (res, "null");
  EXPECT_EQ (p1.coordinate (), (coord{column{13U}, row{1U}}));
}

TEST_F (Json, POSIXLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  p.input (lf + lf + keyword);
  std::string const res = p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{xord}, row{3U}}));
}

TEST_F (Json, ClassicMacLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  p.input (cr + cr + keyword);  // MacOS Classic line endings
  std::string const res = p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{xord}, row{3U}}));
}

TEST_F (Json, CrLfLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  p.input (crlf + crlf + keyword);  // Windows-style CRLF
  std::string const res = p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{xord}, row{3U}}));
}

TEST_F (Json, BadLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // Nobody's line-endings. Each counts as a new line. Note that the middle
  // cr+lf pair will match a single Windows crlf.
  std::string const res = p.input (lf + cr + lf + cr + keyword).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{xord}, row{4U}}));
}

TEST_F (Json, MixedLeadingLineEndings) {
  parser p{json_out_callbacks{}};
  // A groovy mixture of line-ending characters.
  p.input (lf + lf + crlf + cr + keyword);
  std::string const res = p.eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (res, keyword);
  EXPECT_EQ (p.coordinate (), (coord{column{xord}, row{5U}}));
}

TEST_F (Json, Null) {
  StrictMock<mock_json_callbacks> callbacks;
  callbacks_proxy proxy{callbacks};
  EXPECT_CALL (callbacks, null_value ()).Times (1);

  parser p{proxy};
  p.input (" null "s).eof ();
  EXPECT_FALSE (p.has_error ());
  EXPECT_EQ (p.coordinate (), (coord{column{7U}, row{1U}}));
}

TEST_F (Json, Move) {
  // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is
  // usable.
  auto p1 = parser<null_output>{};
  auto p2 = std::move (p1);
  p2.input ("null"s).eof ();
  EXPECT_FALSE (p2.has_error ());
  EXPECT_EQ (p2.coordinate (), (coord{column{5U}, row{1U}}));
}

TEST_F (Json, TwoKeywords) {
  parser p{json_out_callbacks{}};
  p.input (" true false "s);
  EXPECT_EQ (p.last_error (),
             make_error_code (error_code::unexpected_extra_input));
  EXPECT_EQ (p.coordinate (), (coord{column{7U}, row{1U}}));
}

TEST_F (Json, BadKeyword) {
  check_error ("nu", error_code::expected_token);
  check_error ("bad", error_code::expected_token);
  check_error ("fal", error_code::expected_token);
  check_error ("falsehood", error_code::unexpected_extra_input);
}
