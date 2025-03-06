#===- unittests/uri/CMakeLists.txt ----------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
# Distributed under the Apache License v2.0.
# See <https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT>.
# SPDX-License-Identifier: Apache-2.0
#===----------------------------------------------------------------------===//
add_executable (pj-uri-unittests
  test_find_last.cpp
  test_parts.cpp
  test_pctdecode.cpp
  test_pctencode.cpp
  test_punycode.cpp
  test_starts_with.cpp
  test_rule.cpp
  test_uri.cpp
)
target_compile_options (
  pj-uri-unittests
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
      -Wno-global-constructors
      -Wno-used-but-marked-unused>
    $<$<CXX_COMPILER_ID:GNU>:>
    # -wd4702 : unreachable code.
    $<$<CXX_COMPILER_ID:MSVC>:
      -wd4702>
)
target_link_libraries (pj-uri-unittests PUBLIC uri)
setup_target (pj-uri-unittests PEDANTIC $<NOT:$<BOOL:${URI_FUZZTEST}>>)
add_test(NAME pj-uri-unittests COMMAND pj-uri-unittests)

if (URI_FUZZTEST)
  target_link_libraries (pj-uri-unittests PUBLIC gtest gmock)
  link_fuzztest (pj-uri-unittests)
  gtest_discover_tests (pj-uri-unittests)
else ()
  target_link_libraries (pj-uri-unittests PUBLIC gmock_main)
endif (URI_FUZZTEST)
