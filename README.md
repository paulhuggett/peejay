# peejay

[![CI Build & Test](https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml/badge.svg)](https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=paulhuggett_peejay&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=paulhuggett_peejay)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/a37157bbd85c440daadd8039cda137b2)](https://www.codacy.com/gh/paulhuggett/peejay/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=paulhuggett/peejay&amp;utm_campaign=Badge_Grade)

A state-machine based JSON parser for C++17 or later. (The silly name comes from the English pronunciation of P.J. which is short for **P**arse **J**SON.)

## JSON5 extension support

Please refer to the [JSON5 specification](https://json5.org) for further details.

Feature | Support
------- | ---------
Object keys may be an ECMAScript 5.1 IdentifierName | `extensions::identifier_object_key`
Objects may have a single trailing comma | `extensions::object_trailing_comma`
Arrays may have a single trailing comma | `extensions::array_trailing_comma`
Strings may be single quoted | `extensions::single_quote_string`
Strings may span multiple lines by escaping new line characters | `extensions::string_escapes`
Strings may include character escapes | `extensions::string_escapes`
Numbers may be hexadecimal | `extensions::numbers`
Numbers may have a leading or trailing decimal point | 
Numbers may be IEEE 754 positive infinity, negative infinity, and NaN |
Numbers may begin with an explicit plus sign | `extensions::leading_plus`
Single and multi-line comments are allowed | `extensions::bash_comments` (enables use of single line comments beginning with a hash “#” character), `extensions::single_line_comments` (enables use of single line comments beginning with two slash (“//”) characters), `extensions::multi_line_comments` (enables use of multi-line comments enclosed by /\* … \*/ characters)
Additional white space characters are allowed | `extensions::extra_whitespace`
