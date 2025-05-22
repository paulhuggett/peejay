#===- unit_tests/CMakeLists.txt -------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
# Copyright © 2025 Paul Bowen-Huggett
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# “Software”), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT
#===----------------------------------------------------------------------===//

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("int main() { __int128 v; }" PEEJAY_HAVE_INT128)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

add_executable(pj2-unittests
  "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
  backtrace.cpp
  callbacks.hpp
  test_array.cpp
  test_arrayvec.cpp
  test_boolean.cpp
  test_coord.cpp
  test_dom.cpp
  test_json.cpp
  test_object.cpp
  test_number.cpp
  test_pointer_based_iterator.cpp
  test_string.cpp
  test_variant.cpp
)
setup_target(pj2-unittests)

set(clang_options -Wno-global-constructors -Wno-used-but-marked-unused)
set(gcc_options -Wno-pedantic)
set(msvc_options -wd4702) # unreachable code.

target_compile_options(pj2-unittests
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
)
target_include_directories(pj2-unittests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(pj2-unittests PUBLIC peejay)

if(PEEJAY_FUZZTEST)
  target_link_libraries(pj2-unittests PUBLIC gtest gmock)
  link_fuzztest(pj2-unittests)
  gtest_discover_tests(pj2-unittests)
else()
  target_sources(pj2-unittests PUBLIC harness.cpp)
  target_link_libraries(pj2-unittests PUBLIC gmock_main)
endif(PEEJAY_FUZZTEST)
