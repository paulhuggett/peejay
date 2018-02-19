#include "json_error.hpp"

// ******************
// * error category *
// ******************
json::error_category::error_category () noexcept = default;

char const * json::error_category::name () const noexcept {
    return "json parser category";
}

std::string json::error_category::message (int error) const {
    switch (static_cast<error_code> (error)) {
    case error_code::none: return "none";
    case error_code::expected_array_member: return "expected array member";
    case error_code::expected_close_quote: return "expected close quote";
    case error_code::expected_colon: return "expected colon";
    case error_code::expected_digits: return "expected digits";
    case error_code::expected_object_member: return "expected object member";
    case error_code::expected_token: return "expected token";
    case error_code::invalid_escape_char: return "invalid escape character";
    case error_code::expected_string: return "expected string";
    case error_code::unrecognized_token: return "unrecognized token";
    case error_code::unexpected_extra_input: return "unexpected extra input";
    case error_code::bad_unicode_code_point: return "bad UNICODE code point";
    case error_code::number_out_of_range: return "number out of range";
    }
    return "unknown json::error_category error";
}

std::error_category const & json::get_error_category () noexcept {
    static json::error_category const cat;
    return cat;
}

// eof:json.cpp
