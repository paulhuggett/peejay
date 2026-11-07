//===- unit_tests/test_array.cpp ------------------------------------------===//
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
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
#include "callbacks.hpp"
#include "peejay/json.hpp"
#include "peejay/null.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

using coord = peejay::coord<true>;
using peejay::error;
using peejay::make_parser;
using peejay::parser;

using testing::DoubleEq;
using testing::InSequence;
using testing::StrictMock;

namespace peejay {

void PrintTo(coord<true> const& c, std::ostream* os);
void PrintTo(coord<true> const& c, std::ostream* const os) {
  *os << "{.line=" << c.line << '}';
}

}  // end namespace peejay

// The backend element() member function is optional but, if present, must return a value convertible to
// std::error_code.
static_assert(peejay::backend<peejay::null<>>);
struct bad_element_backend : peejay::null<> {
  static int element() noexcept;  // only used in an unevaluated context
};
static_assert(!peejay::backend<bad_element_backend>);

namespace {

class JsonArray : public testing::Test {
protected:
  mockable_callbacks<peejay::default_policies> mock_;
};

// NOLINTNEXTLINE
TEST_F(JsonArray, EmptyNoWhitespace) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  p.input(u8"[]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, Empty) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  p.input(u8"[\n]\n"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 3U}));
  EXPECT_EQ(p.input_pos(), (coord{.line = 3U}));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, BeginArrayReturnsError) {
  std::error_code const error = make_error_code(std::errc::io_error);
  using testing::Return;
  EXPECT_CALL(mock_.callbacks, begin_array()).WillOnce(Return(error));

  auto p = make_parser(mock_.proxy);
  p.input(u8"[\n]\n"sv);
  EXPECT_EQ(p.last_error(), error) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(p.pos(), (coord{.line = 1U}));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, ArrayNoCloseBracket) {
  auto p = make_parser(json_out_callbacks{});
  input(p, u8"["sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_array_member));

  std::error_code const& err = p.last_error();
  EXPECT_GE(err.category().message(err.value()).length(), 0);
}

// NOLINTNEXTLINE
TEST_F(JsonArray, SingleElement) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, integer_value(1)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  auto const str = u8"[ 1 ]"sv;
  input(p, str).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 1U}));
  EXPECT_EQ(p.pos(), p.input_pos());
}

// NOLINTNEXTLINE
TEST_F(JsonArray, SingleStringElement) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, string_value(u8"a"sv)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  p.input(u8"[\"a\"]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, ZeroExpPlus1) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, float_value(0)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[0e+1]"sv);
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, SimpleFloat) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, float_value(DoubleEq(1.234))).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[1.234]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, MinusZero) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, integer_value(0)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[-0]"sv);
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, TwoElements) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, integer_value(1)).Times(1);
  EXPECT_CALL(mock_.callbacks, string_value(u8"hello"sv)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[ 1 ,\n \"hello\" ]"sv);
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
  EXPECT_EQ(p.input_pos(), (coord{.line = 2U}));
  EXPECT_EQ(p.pos(), (coord{.line = 2U}));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, MisplacedComma1) {
  parser p{json_out_callbacks{}};
  input(p, u8"[,"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));

  std::error_code const& err = p.last_error();
  EXPECT_GE(err.category().message(err.value()).length(), 0);
}
// NOLINTNEXTLINE
TEST_F(JsonArray, MisplacedComma2) {
  parser p{json_out_callbacks{}};
  input(p, u8"[,1"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
}
// NOLINTNEXTLINE
TEST_F(JsonArray, MisplacedComma3) {
  parser p{json_out_callbacks{}};
  input(p, u8"[1,,2]"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
}
// NOLINTNEXTLINE
TEST_F(JsonArray, MisplacedComma4) {
  parser p{json_out_callbacks{}};
  input(p, u8"[1 true]"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_array_member));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, TrailingComma) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, integer_value(1)).Times(1);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[1 , ]"sv).eof();
  EXPECT_EQ(p.last_error(), peejay::error::expected_token);
}

// NOLINTNEXTLINE
TEST_F(JsonArray, TrailingComma1) {
  parser p{json_out_callbacks{}};
  input(p, u8"[,]"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
  EXPECT_EQ(p.pos(), (coord{.line = 1U}));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, TrailingComma2) {
  parser p{json_out_callbacks{}};
  input(p, u8"[1,]"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token));
  EXPECT_EQ(p.pos(), (coord{.line = 1U}));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, NestedError1) {
  parser p{json_out_callbacks{}};
  input(p, u8"[[no"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::unrecognized_token));
}

// NOLINTNEXTLINE
TEST_F(JsonArray, NestedError2) {
  auto p = make_parser(json_out_callbacks{});
  input(p, u8"[[null"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_array_member))
      << "Actual error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, Nested) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(2);
  EXPECT_CALL(mock_.callbacks, null_value()).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(2);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[[null]]"sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(JsonArray, Nested2) {
  InSequence const _;
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(2);
  EXPECT_CALL(mock_.callbacks, null_value()).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, begin_array()).Times(1);
  EXPECT_CALL(mock_.callbacks, integer_value(1)).Times(1);
  EXPECT_CALL(mock_.callbacks, end_array()).Times(2);

  auto p = make_parser(mock_.proxy);
  input(p, u8"[[null], [1]]"sv).eof();
  EXPECT_FALSE(p.has_error());
}

// NOLINTNEXTLINE
TEST_F(JsonArray, TooDeeplyNested) {
  parser p{json_out_callbacks{}};
  p.input(std::u8string(std::string::size_type{200}, '[')).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::nesting_too_deep)) << "Real error was: " << p.last_error().message();

  std::error_code const& err = p.last_error();
  EXPECT_GE(err.category().message(err.value()).length(), 0);
}

// NOLINTNEXTLINE
TEST_F(JsonArray, BeginFails) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::file_exists);
  EXPECT_CALL(mock_.callbacks, begin_array()).WillOnce(Return(erc));

  auto p = make_parser(mock_.proxy);
  p.input(u8"[]"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, BeginFails2) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::file_exists);
  EXPECT_CALL(mock_.callbacks, begin_array()).WillOnce(Return(erc));

  auto p = make_parser(mock_.proxy);
  p.input(u8"[ 1 ]"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArray, EndFails) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::file_exists);
  EXPECT_CALL(mock_.callbacks, begin_array());
  EXPECT_CALL(mock_.callbacks, end_array()).WillOnce(Return(erc));

  auto p = make_parser(mock_.proxy);
  p.input(u8"[]"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

// A mock backend which also provides the optional element() member function.
template <peejay::policy Policies> class mock_element_callbacks : public mock_json_callbacks<Policies> {
public:
  // NOLINTNEXTLINE
  MOCK_METHOD(std::error_code, element, ());
};

template <typename T> class element_callbacks_proxy : public callbacks_proxy<T> {
public:
  explicit element_callbacks_proxy(T& original) : callbacks_proxy<T>(original), original_(original) {}
  std::error_code element() { return original_.element(); }

private:
  T& original_;
};

static_assert(peejay::backend<element_callbacks_proxy<mock_element_callbacks<peejay::default_policies>>>);

class JsonArrayElement : public testing::Test {
protected:
  StrictMock<mock_element_callbacks<peejay::default_policies>> callbacks_;
  element_callbacks_proxy<mock_element_callbacks<peejay::default_policies>> proxy_{callbacks_};
};

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, Empty) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, EmptyWithWhitespace) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[ \n ]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, SingleElement) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[ 1 ]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, ThreeElements) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(2)).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(3)).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[1 , 2,3]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, Nested) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, null_value()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, boolean_value(true)).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, end_array()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[[null,true],1]"sv).eof();
  EXPECT_FALSE(p.last_error()) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, ElementFails) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::file_exists);
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, element()).WillOnce(Return(erc));

  auto p = make_parser(proxy_);
  p.input(u8"[1,2]"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, FirstElementFails) {
  using testing::Return;
  auto const erc = make_error_code(std::errc::file_exists);
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).WillOnce(Return(erc));

  auto p = make_parser(proxy_);
  p.input(u8"[1]"sv).eof();
  EXPECT_EQ(p.last_error(), erc) << "Real error was: " << p.last_error().message();
}

// NOLINTNEXTLINE
TEST_F(JsonArrayElement, TrailingComma) {
  InSequence const _;
  EXPECT_CALL(callbacks_, begin_array()).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);
  EXPECT_CALL(callbacks_, integer_value(1)).Times(1);
  EXPECT_CALL(callbacks_, element()).Times(1);

  auto p = make_parser(proxy_);
  p.input(u8"[1,]"sv).eof();
  EXPECT_EQ(p.last_error(), make_error_code(error::expected_token)) << "Real error was: " << p.last_error().message();
}

}  // end anonymous namespace
