//===- unit_tests/test_writer_backend.cpp ---------------------------------===//
//*                _ _              _                _                  _  *
//* __      ___ __(_) |_ ___ _ __  | |__   __ _  ___| | _____ _ __   __| | *
//* \ \ /\ / / '__| | __/ _ \ '__| | '_ \ / _` |/ __| |/ / _ \ '_ \ / _` | *
//*  \ V  V /| |  | | ||  __/ |    | |_) | (_| | (__|   <  __/ | | | (_| | *
//*   \_/\_/ |_|  |_|\__\___|_|    |_.__/ \__,_|\___|_|\_\___|_| |_|\__,_| *
//*                                                                        *
//===----------------------------------------------------------------------===//
// Copyright © 2026 Paul Bowen-Huggett
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// “Software”), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>

#include "peejay/json.hpp"
#include "peejay/writer.hpp"

using namespace std::string_view_literals;

using peejay::error;

namespace {

struct int64_policies : peejay::default_policies {
  using integer_type = std::int64_t;
};
struct no_float_policies : peejay::default_policies {
  using float_type = peejay::no_float_type;
};
struct shallow_policies : peejay::default_policies {
  static constexpr std::size_t max_stack_depth = 2;
};

/// Parses \p in and returns the output of a writer backend with the given indent.
template <peejay::policies Policies = int64_policies>
std::string write(std::u8string_view const& in, std::size_t const indent = 0) {
  std::string out;
  auto p = peejay::make_parser(peejay::make_writer<Policies>(std::back_inserter(out), indent));
  p.input(in).eof();
  EXPECT_FALSE(p.last_error()) << p.last_error().message();
  return out;
}

TEST(WriterBackend, Keywords) {
  EXPECT_EQ(write(u8" true "), "true");
  EXPECT_EQ(write(u8"false"), "false");
  EXPECT_EQ(write(u8"null"), "null");
}

TEST(WriterBackend, Integers) {
  EXPECT_EQ(write(u8"0"), "0");
  EXPECT_EQ(write(u8"-0"), "0");
  EXPECT_EQ(write(u8"42"), "42");
  EXPECT_EQ(write(u8"-9223372036854775808"), "-9223372036854775808");
  EXPECT_EQ(write(u8"9223372036854775807"), "9223372036854775807");
}

TEST(WriterBackend, Floats) {
  EXPECT_EQ(write(u8"3.14"), "3.14");
  EXPECT_EQ(write(u8"-0.5"), "-0.5");
  EXPECT_EQ(write(u8"1e300"), "1e+300");
  EXPECT_EQ(write(u8"1.5e-7"), "1.5e-07");
}

TEST(WriterBackend, WholeFloats) {
  // These are passed to float_value() but are written without a fraction.
  EXPECT_EQ(write(u8"1.0"), "1");
  EXPECT_EQ(write(u8"-0.0"), "-0");
  EXPECT_EQ(write(u8"2E2"), "200");
  // One more than the largest int64_t, so it is passed to float_value().
  EXPECT_EQ(write(u8"9223372036854775808"), "9223372036854775808");
}

TEST(WriterBackend, NonFiniteFloats) {
  std::string out;
  peejay::writer w{std::back_inserter(out)};
  EXPECT_EQ(w.float_value(std::numeric_limits<double>::infinity()), make_error_code(error::number_out_of_range));
  EXPECT_EQ(w.float_value(-std::numeric_limits<double>::infinity()), make_error_code(error::number_out_of_range));
  EXPECT_EQ(w.float_value(std::numeric_limits<double>::quiet_NaN()), make_error_code(error::number_out_of_range));
  EXPECT_EQ(out, "");
}

TEST(WriterBackend, NoFloat) {
  EXPECT_EQ(write<no_float_policies>(u8"[1,-1]"), "[1,-1]");
}

TEST(WriterBackend, StringAscii) {
  EXPECT_EQ(write(u8R"("")"), R"("")");
  EXPECT_EQ(write(u8R"("hello world")"), R"("hello world")");
  // DEL (U+007F) is the last ASCII character and need not be escaped.
  EXPECT_EQ(write(u8"\"~\\u007F\""), "\"~\x7F\"");
}

TEST(WriterBackend, StringShortEscapes) {
  // The solidus need not be escaped.
  EXPECT_EQ(write(u8"\"\\\"\\\\\\/\\b\\f\\n\\r\\t\""), "\"\\\"\\\\/\\b\\f\\n\\r\\t\"");
}

TEST(WriterBackend, StringControlCharacters) {
  EXPECT_EQ(write(u8"\"\\u0000\""), "\"\\u0000\"");
  EXPECT_EQ(write(u8"\"\\u001F\""), "\"\\u001f\"");
  EXPECT_EQ(write(u8"\"\\u0020\""), "\" \"");
}

TEST(WriterBackend, StringNonAscii) {
  // The input strings hold UTF-8 encoded characters rather than JSON escapes.
  EXPECT_EQ(write(u8"\"\u0080\""), "\"\\u0080\"");
  EXPECT_EQ(write(u8"\"caf\u00E9\""), "\"caf\\u00e9\"");
  EXPECT_EQ(write(u8"\"\u20AC1\""), "\"\\u20ac1\"");
  EXPECT_EQ(write(u8"\"\uFFFF\""), "\"\\uffff\"");
}

TEST(WriterBackend, StringSurrogatePairs) {
  EXPECT_EQ(write(u8"\"\U00010000\""), "\"\\ud800\\udc00\"");
  EXPECT_EQ(write(u8"\"\U0001F600\""), "\"\\ud83d\\ude00\"");
  EXPECT_EQ(write(u8"\"\U0010FFFF\""), "\"\\udbff\\udfff\"");
  // A surrogate pair written as JSON escapes.
  EXPECT_EQ(write(u8"\"\\uD83D\\uDE00\""), "\"\\ud83d\\ude00\"");
}

TEST(WriterBackend, StringIllFormedUtf8) {
  // 0xFF never appears in UTF-8.
  constexpr std::array<char8_t, 2> ill_formed{u8'a', char8_t{0xFF}};
  std::string out;
  peejay::writer w{std::back_inserter(out)};
  EXPECT_EQ(w.string_value(std::u8string_view{ill_formed.data(), ill_formed.size()}),
            make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(out, "\"a\\ufffd\"");
}

TEST(WriterBackend, Key) {
  EXPECT_EQ(write(u8"{\"k\u00E9\\\"y\":1}"), "{\"k\\u00e9\\\"y\":1}");
}

TEST(WriterBackend, KeyTruncatedUtf8) {
  // 0xC3 starts a two-byte sequence.
  constexpr std::array<char8_t, 1> truncated{char8_t{0xC3}};
  std::string out;
  peejay::writer w{std::back_inserter(out)};
  EXPECT_FALSE(w.begin_object());
  EXPECT_EQ(w.key(std::u8string_view{truncated.data(), truncated.size()}),
            make_error_code(error::bad_unicode_code_point));
  EXPECT_EQ(out, "{\"\\ufffd\":");
}

TEST(WriterBackend, CompactEmptyContainers) {
  EXPECT_EQ(write(u8"[ ]"), "[]");
  EXPECT_EQ(write(u8"{ }"), "{}");
}

TEST(WriterBackend, CompactNested) {
  EXPECT_EQ(write(u8R"( [ 1 , [ ] , { } , { "a" : [ true , null ] , "b" : "c" } ] )"),
            R"([1,[],{},{"a":[true,null],"b":"c"}])");
}

TEST(WriterBackend, PrettyScalar) {
  EXPECT_EQ(write(u8"1", 2), "1");
}

TEST(WriterBackend, PrettyEmptyContainers) {
  EXPECT_EQ(write(u8"[]", 2), "[]");
  EXPECT_EQ(write(u8"{}", 2), "{}");
}

TEST(WriterBackend, PrettyNested) {
  EXPECT_EQ(write(u8R"([1,[],{},{"a":[true,null],"b":"c"}])", 2),
            "[\n"
            "  1,\n"
            "  [],\n"
            "  {},\n"
            "  {\n"
            "    \"a\": [\n"
            "      true,\n"
            "      null\n"
            "    ],\n"
            "    \"b\": \"c\"\n"
            "  }\n"
            "]");
}

TEST(WriterBackend, PrettyIndentFour) {
  EXPECT_EQ(write(u8R"({"a":{"b":1}})", 4),
            "{\n"
            "    \"a\": {\n"
            "        \"b\": 1\n"
            "    }\n"
            "}");
}

TEST(WriterBackend, NestingTooDeep) {
  std::string out;
  auto w = peejay::make_writer<shallow_policies>(std::back_inserter(out));
  EXPECT_FALSE(w.begin_array());
  EXPECT_FALSE(w.begin_object());
  EXPECT_EQ(w.begin_array(), make_error_code(error::nesting_too_deep));
  EXPECT_EQ(w.begin_object(), make_error_code(error::nesting_too_deep));
  EXPECT_EQ(out, "[{");
}

TEST(WriterBackend, ResultIsEndOfOutput) {
  std::array<char, 8> buffer{};
  auto p = peejay::make_parser(peejay::writer{buffer.data()});
  char* const end = p.input(u8"[1, 2]"sv).eof();
  EXPECT_EQ(std::string_view(buffer.data(), end), "[1,2]");
}

}  // end anonymous namespace
