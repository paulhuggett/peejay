#===- unittests/CMakeLists.txt --------------------------------------------===//
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
  pj-unittests
  backtrace.cpp
  callbacks.hpp
  test_almost_equal.cpp
  test_array.cpp
  test_arrayvec.cpp
  test_boolean.cpp
  test_comment.cpp
  test_coord.cpp
  test_cprun.cpp
  test_dom.cpp
  test_emit.cpp
  test_json.cpp
  test_object.cpp
  test_number.cpp
  test_pointer_based_iterator.cpp
  test_small_vector.cpp
  test_stack.cpp
  test_string.cpp
  test_utf.cpp
  test_whitespace.cpp
)
setup_target (pj-unittests)

set (clang_options -Wno-global-constructors -Wno-used-but-marked-unused)
set (gcc_options)
set (msvc_options -wd4702) # unreachable code.

target_compile_options (
  pj-unittests
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
)
target_link_libraries (pj-unittests PUBLIC schema)

if (PEEJAY_FUZZTEST)
  target_link_libraries (pj-unittests PUBLIC gtest gmock)
  link_fuzztest (pj-unittests)
  gtest_discover_tests (pj-unittests)
else ()
  target_sources (pj-unittests PUBLIC harness.cpp)
  target_link_libraries (pj-unittests PUBLIC gmock_main)
endif (PEEJAY_FUZZTEST)
