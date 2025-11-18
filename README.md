# peejay
JSON Parser for C++

## Status

| Category | Badges |
| --- | --- |
| Continuous Integration | [![CI Build & Test](https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml/badge.svg)](https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml) |
| Static Analysis | [![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=paulhuggett_peejay&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=paulhuggett_peejay) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/a37157bbd85c440daadd8039cda137b2)](https://app.codacy.com/gh/paulhuggett/peejay/dashboard) [![Coverity Scan Build Status](https://img.shields.io/coverity/scan/28476.svg)](https://scan.coverity.com/projects/paulhuggett-peejay) [![Microsoft C++ Code Analysis](https://github.com/paulhuggett/peejay/actions/workflows/msvc.yaml/badge.svg)](https://github.com/paulhuggett/peejay/actions/workflows/msvc.yaml)
| Dynamic Analysis | [![KLEE Tests](https://github.com/paulhuggett/peejay/actions/workflows/klee.yaml/badge.svg)](https://github.com/paulhuggett/peejay/actions/workflows/klee.yaml) [![CodeCov](https://codecov.io/github/paulhuggett/peejay/graph/badge.svg?token=BSNN6OFIJU)](https://codecov.io/github/paulhuggett/peejay) [![Fuzz Tests](https://github.com/paulhuggett/peejay/actions/workflows/fuzztest.yaml/badge.svg)](https://github.com/paulhuggett/peejay/actions/workflows/fuzztest.yaml) |
| [OpenSSF](https://openssf.org) | [![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8006/badge)](https://www.bestpractices.dev/projects/8006) [![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/paulhuggett/peejay/badge)](https://securityscorecards.dev/viewer/?uri=github.com/paulhuggett/peejay) |

## Introduction

Peejay (PJ, from the English pronunciation of **P**arse **J**SON) is a header-only C++ library that provides a state-machine-driven JSON parser with the following characteristics:

- C++20 or later language standard requirement
- Single-character parsing: feed content to the parser a byte or entire document at a time
- The parser front-end has fixed and predictable overhead: it does not allocate memory and does not throw or catch exceptions
- Template-based backend system allowing DOM construction, validation-only parsing, or custom output formats
- Policy-driven configuration for customisation

## Example

A configuration policy for the parser:

~~~cpp
#include "peejay/json.hpp"

struct policy : public peejay::default_policies {
  static constexpr std::size_t max_length = 64;
  static constexpr std::size_t max_stack_depth = 8;
  static constexpr bool pos_tracking = true;

  using float_type = double;
  using integer_type = std::int64_t;
};
~~~

A class which implements a simple backend which prints the tokens as they arrive from the front-end parser:

~~~cpp
/// A backend which prints tokens as they arrive.
class print_tokens {
public:
  using policies = policy;
  static constexpr void result() noexcept {
    // The "print_tokens" backend produces no result at all.
  }

  static std::error_code boolean_value(bool const b) noexcept { std::cout << (b ? "true" : "false") << ' '; return {}; }
  static std::error_code float_value(policy::float_type const v) { std::cout << v << ' '; return {}; }
  static std::error_code integer_value(policy::integer_type const v) { std::cout << v << ' '; return {}; }
  static std::error_code null_value() { std::cout << "null"; return {}; }
  static std::error_code string_value(std::u8string_view const &sv) { show_string(sv); std::cout << ' '; return {}; }

  static std::error_code begin_array() { std::cout << "[ "; return {}; }
  static std::error_code end_array() { std::cout << "] "; return {}; }

  static std::error_code begin_object() { std::cout << "{ "; return {}; }
  static std::error_code key(std::u8string_view const &sv) { show_string(sv); std::cout << ": "; return {}; }
  static std::error_code end_object() { std::cout << "} "; return {}; }

private:
  static void show_string(std::u8string_view const &sv) {
    std::cout << '"';
    std::ranges::copy(std::ranges::subrange{sv} | std::ranges::views::transform([] (char8_t c) { return static_cast<char>(c); }), std::ostream_iterator<char>{std::cout});
    std::cout << '"';
  }
};
~~~

A top-level driver function. Here the entire JSON document is presented to `parser<>::input()` at once. We could also supply it in smaller pieces down to a single byte at a time.

~~~cpp
int main() {
  using namespace std::string_view_literals;

  int exit_code = EXIT_SUCCESS;
  peejay::parser<print_tokens> p;
  p.input(u8R"(  { "a":123, "b" : [false,"c"], "c":true }  )"sv).eof();
  if (auto const err = p.last_error()) {
    std::cerr << "Error: " << err.message() << '\n';
    exit_code = EXIT_FAILURE;
  }
  return exit_code;
}

~~~
