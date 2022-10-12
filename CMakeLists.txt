#===- CMakeLists.txt ------------------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Distributed under the Apache License v2.0 with LLVM Exceptions.
# See https://github.com/paulhuggett/json/blob/master/LICENSE.txt for license
# information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===----------------------------------------------------------------------===//
cmake_minimum_required(VERSION 3.0)

project (json C CXX)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

function (setup_target target)
  set (clang_options
    -Weverything
    -Wno-c++14-extensions
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c99-extensions
    -Wno-exit-time-destructors
    -Wno-padded
    -Wno-undef
  )
  set (gcc_options
    -Wall
    -Wextra
    -pedantic
  )
  set (msvc_options
    -W4
    -wd4324 # 4324: structure was padded due to alignment specifier
  )

  if (WERROR)
    list (APPEND clang_options -Werror)
    list (APPEND gcc_options -Werror)
    list (APPEND msvc_options /WX)
  endif ()
  if (INTEL_DIALECT)
    list (APPEND clang_options -masm=intel)
    list (APPEND gcc_options -masm=intel)
  endif ()

  set_target_properties (${target} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED Yes
  )
  target_compile_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )
endfunction (setup_target)




# Tell gtest to link against the "Multi-threaded Debug DLL runtime
# library" on Windows.
set (gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
add_subdirectory (googletest)
set_target_properties (gtest gmock gmock_main gtest_main PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED Yes
)

add_library (json-parser STATIC
    include/json/dom_types.hpp
    include/json/json.hpp
    include/json/json_error.hpp
    include/json/utf.hpp
    include/json/utility.hpp
    lib/json/json_error.cpp
    lib/json/utf.cpp
    lib/json/utility.cpp
)
target_include_directories (json-parser PUBLIC include)
setup_target (json-parser)



add_subdirectory (unittests)


add_executable (json-main main.cpp)
target_link_libraries (json-main PUBLIC json-parser)
setup_target (json-main)

add_custom_command (TARGET json-main
    PRE_LINK
    COMMAND json-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/output.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running json unit tests"
    VERBATIM
)



add_executable (main-klee main_klee.cpp)
target_link_libraries (main-klee PUBLIC json-parser)
setup_target (main-klee)

