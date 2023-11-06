//===- unittests/test_schema.cpp ------------------------------------------===//
//*           _                           *
//*  ___  ___| |__   ___ _ __ ___   __ _  *
//* / __|/ __| '_ \ / _ \ '_ ` _ \ / _` | *
//* \__ \ (__| | | |  __/ | | | | | (_| | *
//* |___/\___|_| |_|\___|_| |_| |_|\__,_| *
//*                                       *
//===----------------------------------------------------------------------===//
// Distributed under the Apache License v2.0.
// See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
// SPDX-License-Identifier: Apache-2.0
//===----------------------------------------------------------------------===//
#include <gtest/gtest.h>

#include "peejay/schema.hpp"

using peejay::dom;
using peejay::element;
using peejay::error;
using peejay::make_parser;
using peejay::schema::check;

using namespace std::literals;
using testing::Test;

namespace {

void parse_error () {
  GTEST_FAIL ();
}

element parse (peejay::u8string_view const& str) {
  auto p = make_parser (dom{});
  std::optional<peejay::element> result =
      p.input (std::begin (str), std::end (str)).eof ();
  if (auto const erc = p.last_error ()) {
    parse_error ();
    return {};
  }
  if (!result) {
    parse_error ();
    return {};
  }
  return std::move (result.value ());
}

}  // end anonymous namespace

// NOLINTNEXTLINE
TEST (SchemaConst, NumberPassing) {
  EXPECT_EQ (check (parse (u8R"({ "const": 1234 })"sv), parse (u8"1234"sv)),
             std::error_code{});
}

class SchemaEnum : public Test {
protected:
  element const schema = parse (u8R"({ "enum": [ 123, "foo" ] })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaEnum, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"123"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, IntegerFloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"123.0"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, StringPassing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("bar")"sv)),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaEnum, ObjectFailing) {
  EXPECT_EQ (check (schema, parse (u8R"({"a":1,"b":2})"sv)),
             make_error_code (error::schema_validation));
}

class SchemaTypeNumber : public Test {
protected:
  element const schema = parse (u8R"({ "type": "number" })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"1234"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, FloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"12.0"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, SIntPassing) {
  EXPECT_EQ (check (schema, parse (u8"-1234"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeNumber, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv)),
             make_error_code (error::schema_validation));
}

class SchemaTypeInteger : public Test {
protected:
  element const schema = parse (u8R"({ "type": "integer" })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, UintPassing) {
  EXPECT_EQ (check (schema, parse (u8"1234"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, FloatPassing) {
  EXPECT_EQ (check (schema, parse (u8"12.0"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, SIntPassing) {
  EXPECT_EQ (check (schema, parse (u8"-1234"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, StringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("foo")"sv)),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeInteger, RationalFailing) {
  EXPECT_EQ (check (schema, parse (u8"12.01"sv)),
             make_error_code (error::schema_validation));
}

class SchemaTypeArray : public Test {
protected:
  element const schema = parse (u8R"({ "type": ["boolean", "null"] })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, BoolPassing) {
  EXPECT_EQ (check (schema, parse (u8"true"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, NullPassing) {
  EXPECT_EQ (check (schema, parse (u8"null"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaTypeArray, UIntFailing) {
  EXPECT_EQ (check (schema, parse (u8"0"sv)),
             make_error_code (error::schema_validation));
}

class SchemaMaxLength : public Test {
protected:
  element const schema = parse (u8R"({ "maxLength": 2 })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, ShortStringPassing) {
  EXPECT_EQ (check (schema, parse (u8R"("ab")"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, NotStringPassing) {
  EXPECT_EQ (check (schema, parse (u8"1"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, LongStringFailing) {
  EXPECT_EQ (check (schema, parse (u8R"("abc")"sv)),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxLength, BadSchemaValue) {
  element const bad_schema = parse (u8R"({ "maxLength": "foo" })"sv);
  EXPECT_EQ (check (bad_schema, parse (u8R"("ab")"sv)),
             make_error_code (error::schema_expected_non_negative_integer));
}

class SchemaMinLength : public Test {
protected:
  element const schema_ = parse (u8R"({ "minLength": 2 })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, ShortStringFailing) {
  EXPECT_EQ (check (schema_, parse (u8R"("a")"sv)),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, NotStringPassing) {
  EXPECT_EQ (check (schema_, parse (u8"1"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, LongStringPassing) {
  EXPECT_EQ (check (schema_, parse (u8R"("abc")"sv)), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinLength, BadSchemaValue) {
  EXPECT_EQ (
      check (parse (u8R"({ "minLength": "foo" })"sv), parse (u8R"("ab")"sv)),
      make_error_code (error::schema_expected_non_negative_integer));
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
})"sv);
};

// NOLINTNEXTLINE
TEST_F (SchemaProperties, HasPropertyPassing) {
  // valid - instance has name, which is a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "name": "Alice" })")),
             std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, MissingPropertyPassing) {
  // valid - instance has name, which is a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "fullName": "Alice" })")),
             std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, ArrayPassing) {
  // valid - instance is not an object, therefore `properties` isn't applicable.
  EXPECT_EQ (check (schema_, parse (u8R"([ "name", 123 ])")),
             std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, NoApplicablePropertiesPassing) {
  // valid - instance data has no applicable properties.
  EXPECT_EQ (check (schema_, parse (u8R"({ })")), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaProperties, PropertyHasWrongTypeFailing) {
  // invalid - The `name` property value must be a string.
  EXPECT_EQ (check (schema_, parse (u8R"({ "name": 123 })")),
             make_error_code (error::schema_validation));
}

class SchemaMaxProperties : public Test {
protected:
  element const schema_ = parse (u8R"({ "maxProperties": 2 })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaMaxProperties, ObjectPassing) {
  EXPECT_EQ (check (schema_, parse (u8R"({ "a": 1, "b": 2 })")),
             std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxProperties, ObjectFailing) {
  EXPECT_EQ (check (schema_, parse (u8R"({ "a": 1, "b": 2, "c": 3 })")),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxProperties, NonObjectPassing) {
  EXPECT_EQ (check (schema_, parse (u8"1")), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxProperties, MaxPropertiesValueIsNegative) {
  EXPECT_EQ (check (parse (u8R"({ "maxProperties": -2 })"sv), parse (u8"{}")),
             make_error_code (error::schema_expected_non_negative_integer));
}
// NOLINTNEXTLINE
TEST_F (SchemaMaxProperties, MaxPropertiesValueIsWrongType) {
  EXPECT_EQ (
      check (parse (u8R"({ "maxProperties": "one" })"sv), parse (u8"{}")),
      make_error_code (error::schema_expected_non_negative_integer));
}

class SchemaMinProperties : public Test {
protected:
  element const schema_ = parse (u8R"({ "minProperties": 2 })"sv);
};
// NOLINTNEXTLINE
TEST_F (SchemaMinProperties, ObjectPassing) {
  EXPECT_EQ (check (schema_, parse (u8R"({ "a": 1, "b": 2 })")),
             std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinProperties, ObjectFailing) {
  EXPECT_EQ (check (schema_, parse (u8R"({ "a": 1 })")),
             make_error_code (error::schema_validation));
}
// NOLINTNEXTLINE
TEST_F (SchemaMinProperties, NonObjectPassing) {
  EXPECT_EQ (check (schema_, parse (u8"1")), std::error_code{});
}
// NOLINTNEXTLINE
TEST_F (SchemaMinProperties, MaxPropertiesValueIsNegative) {
  EXPECT_EQ (check (parse (u8R"({ "minProperties": -2 })"sv), parse (u8"{}")),
             make_error_code (error::schema_expected_non_negative_integer));
}
// NOLINTNEXTLINE
TEST_F (SchemaMinProperties, MaxPropertiesValueIsWrongType) {
  EXPECT_EQ (
      check (parse (u8R"({ "minProperties": "one" })"sv), parse (u8"{}")),
      make_error_code (error::schema_expected_non_negative_integer));
}

// NOLINTNEXTLINE
TEST (SchemaNumberInstanceChecks, MultipleOf) {
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2 })"sv), parse (u8"2"sv)),
             std::error_code{});
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2 })"sv), parse (u8"3"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2.5 })"sv), parse (u8"5"sv)),
             std::error_code{});
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2.5 })"sv), parse (u8"4"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2.4 })"sv), parse (u8"4.8"sv)),
             std::error_code{});
  EXPECT_EQ (check (parse (u8R"({ "multipleOf": 2.4 })"sv), parse (u8"4.9"sv)),
             make_error_code (error::schema_validation));
}
TEST (SchemaNumberInstanceChecks, MaximumInteger) {
  auto two = parse (u8R"({ "maximum": 2 })"sv);
  EXPECT_EQ (check (two, parse (u8"2"sv)), std::error_code{});
  EXPECT_EQ (check (two, parse (u8"-1"sv)), std::error_code{});
  EXPECT_EQ (check (two, parse (u8"2.1"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (two, parse (u8"3"sv)),
             make_error_code (error::schema_validation));
}
TEST (SchemaNumberInstanceChecks, MaximumFp) {
  auto pi = parse (u8R"({ "maximum": 3.14 })"sv);
  EXPECT_EQ (check (pi, parse (u8"2"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"-1"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"3.14"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"3.15"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (pi, parse (u8"4"sv)),
             make_error_code (error::schema_validation));
}
TEST (SchemaNumberInstanceChecks, ExclusiveMaximumInteger) {
  auto three = parse (u8R"({ "exclusiveMaximum": 3 })"sv);
  EXPECT_EQ (check (three, parse (u8"2"sv)), std::error_code{});
  EXPECT_EQ (check (three, parse (u8"-1"sv)), std::error_code{});
  EXPECT_EQ (check (three, parse (u8"2.1"sv)), std::error_code{});
  EXPECT_EQ (check (three, parse (u8"2.9999"sv)), std::error_code{});
  EXPECT_EQ (check (three, parse (u8"3"sv)),
             make_error_code (error::schema_validation));
}
TEST (SchemaNumberInstanceChecks, ExclusiveMaximumFp) {
  auto pi = parse (u8R"({ "exclusiveMaximum": 3.14 })"sv);
  EXPECT_EQ (check (pi, parse (u8"2"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"-1"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"3.13999"sv)), std::error_code{});
  EXPECT_EQ (check (pi, parse (u8"3.14"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (pi, parse (u8"3.15"sv)),
             make_error_code (error::schema_validation));
  EXPECT_EQ (check (pi, parse (u8"4"sv)),
             make_error_code (error::schema_validation));
}
