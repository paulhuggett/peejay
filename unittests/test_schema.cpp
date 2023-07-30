//===- unittests/test_schema.cpp ------------------------------------------===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
//
// Distributed under the Apache License v2.0.
// See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
// for license information.
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "peejay/schema.hpp"

using peejay::dom;
using peejay::element;
using peejay::error;
using peejay::error_or;
using peejay::make_parser;
using peejay::schema::check;

using namespace std::literals;
using testing::Test;

namespace {

void parse_error () {
  GTEST_FAIL ();
}

std::optional<element> parse (peejay::u8string_view const& str) {
  auto p = make_parser (dom{});
  std::optional<peejay::element> result =
      p.input (std::begin (str), std::end (str)).eof ();
  if (auto const erc = p.last_error ()) {
    parse_error ();
    return {};
  }
  return result;
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (SchemaConst, NumberPassing) {
  auto schema = parse (u8R"({ "const": 1234 })"sv);
  auto instance = parse (u8"1234"sv);
  ASSERT_TRUE (schema.has_value () && instance.has_value ());
  EXPECT_EQ (check (*schema, *instance), error_or<bool>{true});
}

class SchemaEnum : public Test {
protected:
  element const schema = parse (u8R"({ "enum": [ 123, "foo" ] })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaEnum, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"123"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, IntegerFloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"123.0"sv).value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, StringPassing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv).value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("bar")"sv).value ()),
             error_or<bool>{false});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, ObjectFailing) {
  EXPECT_EQ (check (schema, parse (u8R"({"a":1,"b":2})"sv).value ()),
             error_or<bool>{false});
}

class SchemaTypeNumber : public Test {
protected:
  element const schema = parse (u8R"({ "type": "number" })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"1234"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, FloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"12.0"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, SIntPassing) {
  EXPECT_EQ (check (schema, parse (u8"-1234"sv).value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv).value ()),
             error_or<bool>{false});
}

class SchemaTypeInteger : public Test {
protected:
  element const schema = parse (u8R"({ "type": "integer" })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"1234"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, FloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"12.0"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, SIntPassing) {
  EXPECT_EQ (check (schema, parse (u8"-1234"sv).value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv).value ()),
             error_or<bool>{false});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, RationalFailing) {
  EXPECT_EQ (check (schema, parse (u8"12.01"sv).value ()),
             error_or<bool>{false});
}

class SchemaTypeArray : public Test {
protected:
  element const schema =
      parse (u8R"({ "type": ["boolean", "null"] })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, BoolPassing) {
  EXPECT_EQ (check (schema, parse (u8"true"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, NullPassing) {
  EXPECT_EQ (check (schema, parse (u8"null"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, UIntFailing) {
  EXPECT_EQ (check (schema, parse (u8"0"sv).value ()), error_or<bool>{false});
}

class SchemaMaxLength : public Test {
protected:
  element const schema = parse (u8R"({ "maxLength": 2 })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, ShortStringPassing) {
  EXPECT_EQ (check (schema, parse (u8R"("ab")"sv).value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, NotStringPassing) {
  EXPECT_EQ (check (schema, parse (u8"1"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, LongStringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("abc")"sv).value ()),
             error_or<bool>{false});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, BadSchemaValue) {
  element const bad_schema = parse (u8R"({ "maxLength": "foo" })"sv).value ();
  EXPECT_EQ (check (bad_schema, parse (u8R"("ab")"sv).value ()),
             error_or<bool>{make_error_code (error::schema_maxlength_number)});
}

class SchemaMinLength : public Test {
protected:
  element const schema_ = parse (u8R"({ "minLength": 2 })"sv).value ();
};
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, ShortStringFailing) {
  EXPECT_EQ (check (schema_, parse (u8R"("a")"sv).value ()),
             error_or<bool>{false});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, NotStringPassing) {
  EXPECT_EQ (check (schema_, parse (u8"1"sv).value ()), error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, LongStringPassing) {
  EXPECT_EQ (check (schema_, parse (u8R"("abc")"sv).value ()),
             error_or<bool>{true});
}
TEST_F (SchemaMinLength, BadSchemaValue) {
  element const bad_schema = parse (u8R"({ "minLength": "foo" })"sv).value ();
  EXPECT_EQ (check (bad_schema, parse (u8R"("ab")"sv).value ()),
             error_or<bool>{make_error_code (error::schema_minlength_number)});
}

class SchemaProperties : public Test {
protected:
  element const schema_ = parse (
                              u8R"({
  "properties": {
    "name": {
      "type": ["string"]
    }
  }
})"sv)
                              .value ();
};

// NOLINTNEXTLINE
TEST_F (SchemaProperties, HasPropertyPassing) {
  // valid - instance has name, which is a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "name": "Alice" })").value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, MissingPropertyPassing) {
  // valid - instance has name, which is a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "fullName": "Alice" })").value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, ArrayPassing) {
  // valid - instance is not an object, therefore `properties` isn't applicable.
  EXPECT_EQ (check (schema_, parse (u8R"([ "name", 123 ])").value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, NoApplicablePropertiesPassing) {
  // valid - instance data has no applicable properties.
  EXPECT_EQ (check (schema_, parse (u8R"({ })").value ()),
             error_or<bool>{true});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, PropertyHasWrongTypeFailing) {
  // invalid - The `name` property value must be a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "name": 123 })").value ()),
             error_or<bool>{false});
}
