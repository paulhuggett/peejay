# peejay

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=paulhuggett_peejay&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=paulhuggett_peejay)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/a37157bbd85c440daadd8039cda137b2)](https://www.codacy.com/gh/paulhuggett/peejay/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=paulhuggett/peejay&amp;utm_campaign=Badge_Grade)

A state-machine based JSON parser for C++17 or later. (The silly name comes from the English pronunciation of P.J. which is short for **P**arse **J**SON.)


## [JSON5](https://json5.org) Extension Support

### Objects

- Object keys may be an ECMAScript 5.1 IdentifierName.
- ✔︎ Objects may have a single trailing comma.

### Arrays

- ✔︎ Arrays may have a single trailing comma.

### Strings

- ✔︎ Strings may be single quoted.
- Strings may span multiple lines by escaping new line characters.
- Strings may include character escapes.

### Numbers

- Numbers may be hexadecimal.
- Numbers may have a leading or trailing decimal point.
- Numbers may be IEEE 754 positive infinity, negative infinity, and NaN.
- ✔︎ Numbers may begin with an explicit plus sign.

### Comments

- ✔︎ Single and multi-line comments are allowed.

### White Space

- Additional white space characters are allowed.
