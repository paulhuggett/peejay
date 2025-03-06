#===- unittests/schema/CMakeLists.txt -------------------------------------===//
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
add_executable (
  pj-schema-unittests
  callbacks.hpp
  harness.cpp
  test_schema.cpp
)
setup_target (pj-schema-unittests)

set (clang_options -Wno-global-constructors -Wno-used-but-marked-unused)
set (gcc_options)
set (msvc_options -wd4702) # unreachable code.

target_compile_options (
  pj-schema-unittests
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
)
target_link_libraries (pj-schema-unittests PUBLIC schema)

if (PEEJAY_FUZZTEST)
  target_link_libraries (pj-schema-unittests PUBLIC gtest gmock)
  link_fuzztest (pj-schema-unittests)
  gtest_discover_tests (pj-schema-unittests)
else ()
  target_sources (pj-schema-unittests PUBLIC harness.cpp)
  target_link_libraries (pj-schema-unittests PUBLIC gmock_main)
endif (PEEJAY_FUZZTEST)
