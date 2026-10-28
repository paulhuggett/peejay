//===- unit_tests/test_parser_copy.cpp ------------------------------------===//
//*                                                          *
//*  _ __   __ _ _ __ ___  ___ _ __    ___ ___  _ __  _   _  *
//* | '_ \ / _` | '__/ __|/ _ \ '__|  / __/ _ \| '_ \| | | | *
//* | |_) | (_| | |  \__ \  __/ |    | (_| (_) | |_) | |_| | *
//* | .__/ \__,_|_|  |___/\___|_|     \___\___/| .__/ \__, | *
//* |_|                                        |_|    |___/  *
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
#include "peejay/dom.hpp"
#include "peejay/json.hpp"

// standard library
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

// 3rd party includes
#include <gtest/gtest.h>

using namespace std::string_view_literals;
using peejay::parser;

namespace {

/// A copyable backend which records the events it receives as text.
class recorder {
public:
  using policies = peejay::default_policies;

  [[maybe_unused, nodiscard]] std::string result() const { return out_; }

  [[maybe_unused]] std::error_code string_value(std::u8string_view const& sv) { return this->append("s:"sv, sv); }
  [[maybe_unused]] std::error_code integer_value(int const v) { return this->append(std::string{"i:"} + std::to_string(v)); }
  [[maybe_unused]] std::error_code float_value(double const v) { return this->append(std::string{"f:"} + std::to_string(v)); }  // LCOV_EXCL_LINE
  [[maybe_unused]] std::error_code boolean_value(bool const v) { return this->append(v ? "b:true"sv : "b:false"sv); }
  [[maybe_unused]] std::error_code null_value() { return this->append("null"sv); }
  [[maybe_unused]] std::error_code begin_array() { return this->append("["sv); }
  [[maybe_unused]] std::error_code end_array() { return this->append("]"sv); }
  [[maybe_unused]] std::error_code begin_object() { return this->append("{"sv); }
  [[maybe_unused]] std::error_code key(std::u8string_view const& sv) { return this->append("k:"sv, sv); }
  [[maybe_unused]] std::error_code end_object() { return this->append("}"sv); }

private:
  std::error_code append(std::string_view const& s) {
    out_ += s;
    out_ += ' ';
    return {};
  }
  std::error_code append(std::string_view const& prefix, std::u8string_view const& sv) {
    return this->append(std::string{prefix} + std::string{sv.begin(), sv.end()});
  }

  std::string out_;
};

/// A recorder whose copy assignment throws if the source's fail flag is set.
class throwing_recorder : public recorder {
public:
  throwing_recorder() = default;
  [[maybe_unused]] throwing_recorder(throwing_recorder const&) = default;
  [[maybe_unused]] throwing_recorder(throwing_recorder&&) noexcept = default;
  ~throwing_recorder() noexcept = default;

  [[maybe_unused]] throwing_recorder& operator=(throwing_recorder&& other) noexcept = default;
  throwing_recorder& operator=(throwing_recorder const& other) {
    if (other.fail) {
      throw std::runtime_error{"backend copy failed"};
    }
    recorder::operator=(other); // LCOV_EXCL_LINE
    fail = other.fail; // LCOV_EXCL_LINE
    return *this; // LCOV_EXCL_LINE
  }
  bool fail = false;
};

static_assert(std::is_copy_constructible_v<parser<recorder>>);
static_assert(std::is_copy_assignable_v<parser<recorder>>);
// A backend held by reference is shared, not copied, so the parser cannot be copied.
static_assert(!std::is_copy_constructible_v<parser<recorder&>>);
static_assert(!std::is_copy_assignable_v<parser<recorder&>>);
// The DOM backend is not copyable so neither is a parser that uses it.
static_assert(!std::is_copy_constructible_v<parser<peejay::dom::dom<>>>);
static_assert(!std::is_copy_assignable_v<parser<peejay::dom::dom<>>>);

// NOLINTNEXTLINE
TEST(ParserCopy, MidString) {
  parser<recorder> p;
  p.input(u8"[\n\"ab"sv);
  parser<recorder> q{p};
  EXPECT_EQ(q.input_pos().line, 2U);
  p.input(u8"c\"]"sv);
  q.input(u8"d\"]"sv);
  EXPECT_EQ(p.eof(), "[ s:abc ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ s:abd ] ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, MidNumber) {
  parser<recorder> p;
  p.input(u8"[12"sv);
  parser<recorder> q{p};
  p.input(u8"3]"sv);
  q.input(u8"]"sv);
  EXPECT_EQ(p.eof(), "[ i:123 ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ i:12 ] ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, MidToken) {
  parser<recorder> p;
  p.input(u8"[tr"sv);
  parser<recorder> q{p};
  p.input(u8"ue]"sv);
  q.input(u8"ip]"sv);
  EXPECT_EQ(p.eof(), "[ b:true ] ");
  EXPECT_FALSE(p.has_error());
  q.eof();
  EXPECT_EQ(q.last_error(), make_error_code(peejay::error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST(ParserCopy, BetweenValues) {
  // The whitespace matcher is on top of the stack: there is no active matcher to copy.
  parser<recorder> p;
  p.input(u8"[1, "sv);
  parser<recorder> q{p};
  p.input(u8"2]"sv);
  q.input(u8"null]"sv);
  EXPECT_EQ(p.eof(), "[ i:1 i:2 ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ i:1 null ] ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, ErrorState) {
  parser<recorder> p;
  p.input(u8"]"sv);
  parser<recorder> const q{p};
  EXPECT_EQ(q.last_error(), make_error_code(peejay::error::expected_token));
}

// NOLINTNEXTLINE
TEST(ParserCopy, AssignReplacesActiveMatcher) {
  // The target is part-way through a number; the source is part-way through a string.
  parser<recorder> p;
  p.input(u8"[12"sv);
  parser<recorder> q;
  q.input(u8"{\"ab"sv);
  p = q;
  p.input(u8"c\":true}"sv);
  q.input(u8"d\":null}"sv);
  EXPECT_EQ(p.eof(), "{ k:abc b:true } ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "{ k:abd null } ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, AssignToSelf) {
  parser<recorder> p;
  p.input(u8"[\"ab"sv);
  auto const& alias = p;
  p = alias;
  p.input(u8"c\"]"sv);
  EXPECT_EQ(p.eof(), "[ s:abc ] ");
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, AssignToMovedFrom) {
  parser<recorder> p;
  p.input(u8"[\"ab"sv);
  parser<recorder> q{std::move(p)};
  p = q;  // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
  p.input(u8"c\"]"sv);
  q.input(u8"d\"]"sv);
  EXPECT_EQ(p.eof(), "[ s:abc ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ s:abd ] ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserCopy, AssignBackendThrows) {
  // The target is part-way through a number; the source is part-way through a string.
  parser<throwing_recorder> p;
  p.input(u8"[12"sv);
  parser<throwing_recorder> q;
  q.input(u8"[\"ab"sv);
  q.backend().fail = true;
  EXPECT_THROW(p = q, std::runtime_error);
  // p must be unchanged: still part-way through its number.
  p.input(u8"3]"sv);
  EXPECT_EQ(p.eof(), "[ i:123 ] ");
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST(ParserMove, MovedFromConstructionInput) {
  parser<recorder> p;
  p.input(u8"[\"ab"sv);
  parser<recorder> q{std::move(p)};
  // A moved-from parser continues from the point at which it was moved. Its backend has been moved from too so
  // give it a fresh one.
  p.backend() = recorder{};  // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
  p.input(u8"c\"]"sv);
  q.input(u8"d\"]"sv);
  EXPECT_EQ(p.eof(), "s:abc ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ s:abd ] ");
  EXPECT_FALSE(q.has_error());
}

// NOLINTNEXTLINE
TEST(ParserMove, MovedFromAssignmentInput) {
  // The target is part-way through a number; the source is part-way through a string.
  parser<recorder> p;
  p.input(u8"[\"ab"sv);
  parser<recorder> q;
  q.input(u8"[12"sv);
  q = std::move(p);
  p.backend() = recorder{};  // NOLINT(bugprone-use-after-move,hicpp-invalid-access-moved)
  p.input(u8"c\"]"sv);
  q.input(u8"d\"]"sv);
  EXPECT_EQ(p.eof(), "s:abc ] ");
  EXPECT_FALSE(p.has_error());
  EXPECT_EQ(q.eof(), "[ s:abd ] ");
  EXPECT_FALSE(q.has_error());
}

}  // end anonymous namespace
