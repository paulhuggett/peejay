#===- unittests/CMakeLists.txt --------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Distributed under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
# for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
add_executable (json-tests
    "${PROJECT_SOURCE_DIR}/googletest/googlemock/src/gmock_main.cc"
    callbacks.cpp
    callbacks.hpp
    test_array.cpp
    test_boolean.cpp
    test_comment.cpp
    test_coord.cpp
    test_json.cpp
    test_object.cpp
    test_utf.cpp
    test_number.cpp
    test_string.cpp
)
setup_target (json-tests)
target_link_libraries (json-tests PRIVATE json-parser gtest gmock)

set (clang_options
  -Wno-global-constructors
  -Wno-used-but-marked-unused
)
set (gcc_options
)
set (msvc_options
)
target_compile_options (json-tests PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
  $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
  $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
)
