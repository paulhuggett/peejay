#include "json.hpp"
#include "dom_types.hpp"
#include "gtest/gtest.h"

TEST (Number, Zero) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("0");
    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), 0L);
}
TEST (Number, MinusOne) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("-1");
    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), -1L);
}
TEST (Number, MinusMinus) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("--");
    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
TEST (Number, OneTwoThree) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("123");
    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), 123L);
}
TEST (Number, Pi) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("3.1415");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 3.1415);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("-3.1415");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), -3.1415);
    }
}
TEST (Number, Point45) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("0.45");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.45);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("-0.45");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), -0.45);
    }
}
TEST (Number, ZeroExp2) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("0e2");
    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_double (), nullptr);
    ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.0);
}
TEST (Number, OneExp2) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1e2");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 100.0);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1e+2");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 100.0);
    }
}
TEST (Number, OneExpMinus2) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("0.01");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1e-2");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1E-2");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1E-02");
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
}

TEST (Number, IntegerMaxAndMin) {
    {
        auto const long_max = std::numeric_limits<long>::max ();
        auto const str_max = std::to_string (long_max);
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse (str_max);
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_long (), nullptr);
        EXPECT_EQ (v->as_long ()->get (), long_max);
    }
    {
        auto const long_min = std::numeric_limits<long>::min ();
        auto const str_min = std::to_string (long_min);
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse (str_min);
        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_long (), nullptr);
        EXPECT_EQ (v->as_long ()->get (), long_min);
    }
}

TEST (Number, IntegerPositiveOverflow) {
    auto const str =
        std::to_string (static_cast<unsigned long> (std::numeric_limits<long>::max ()) + 1L);

    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse (str);
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST (Number, IntegerNegativeOverflow) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("-123123123123123123123123123123");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
    {
        constexpr auto min = std::numeric_limits<long>::min ();
        auto const str = std::to_string (static_cast<unsigned long long> (min) + 1L);

        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse (str);
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
}

TEST (Number, RealPositiveOverflow) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("123123e100000");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("9999E999");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
}

TEST (Number, BadExponentDigit) {
    json::parser<json::yaml_output> p;
    std::shared_ptr<json::value::dom_element> v = p.parse ("1Ex");
    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
TEST (Number, BadFractionDigit) {
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1..");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
    }
    {
        json::parser<json::yaml_output> p;
        std::shared_ptr<json::value::dom_element> v = p.parse ("1.E");
        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
    }
}
