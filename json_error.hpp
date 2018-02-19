#ifndef JSON_ERROR_HPP
#define JSON_ERROR_HPP

#include <string>
#include <system_error>

namespace json {
    enum class error_code {
        none,
        expected_array_member,
        expected_close_quote,
        expected_colon,
        expected_digits,
        expected_string,
        number_out_of_range,
        expected_object_member,
        expected_token,
        invalid_escape_char,
        unrecognized_token,
        unexpected_extra_input,
        bad_unicode_code_point,
    };

    // ******************
    // * error category *
    // ******************
    class error_category : public std::error_category {
    public:
        error_category () noexcept;
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    std::error_category const & get_error_category () noexcept;
} // namespace json

namespace std {
    template <>
    struct is_error_code_enum<::json::error_code> : public std::true_type {};

    inline std::error_code make_error_code (::json::error_code e) noexcept {
        return {static_cast<int> (e), ::json::get_error_category ()};
    }
} // namespace std

#endif // JSON_ERROR_HPP
