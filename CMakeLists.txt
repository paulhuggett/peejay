#===- CMakeLists.txt ------------------------------------------------------===//
#*   ____ __  __       _        _     _     _        *
#*  / ___|  \/  | __ _| | _____| |   (_)___| |_ ___  *
#* | |   | |\/| |/ _` | |/ / _ \ |   | / __| __/ __| *
#* | |___| |  | | (_| |   <  __/ |___| \__ \ |_\__ \ *
#*  \____|_|  |_|\__,_|_|\_\___|_____|_|___/\__|___/ *
#*                                                   *
#===----------------------------------------------------------------------===//
#
# Distributed under the Apache License v2.0.
# See https://github.com/paulhuggett/peejay/blob/main/LICENSE.TXT
# for license information.
# SPDX-License-Identifier: Apache-2.0
#
#===----------------------------------------------------------------------===//
cmake_minimum_required(VERSION 3.0)

project (peejay C CXX)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

option (WERROR "Compiler warnings are errors")
option (CXX17 "Use C++17 (rather than the default C++20")
option (LIBCXX "Use libc++ rather than libstdc++ (clang only)")
option (COVERAGE "Generate LLVM Source-based Coverage")

if (CXX17)
  set (STANDARD 17)
else ()
  set (STANDARD 20)
endif ()

function (setup_target target)
  set (clang_options
    -Weverything
    -Wno-c++14-extensions
    -Wno-c++20-compat
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
    -wd4068 # 4068: unknown pragma
    -wd4324 # 4324: structure was padded due to alignment specifier
  )

  if (WERROR)
    list (APPEND clang_options -Werror)
    list (APPEND gcc_options -Werror)
    list (APPEND msvc_options /WX)
  endif ()

  if (COVERAGE)
    list (APPEND clang_options -fprofile-instr-generate -fcoverage-mapping)
  endif ()
  if (LIBCXX)
    list (APPEND clang_options -stdlib=libc++)
  endif ()

  set_target_properties (${target} PROPERTIES
    CXX_STANDARD ${STANDARD}
    CXX_STANDARD_REQUIRED Yes
    CXX_EXTENSIONS No
  )

  target_compile_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )
  target_link_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:>
  )
endfunction (setup_target)


set (googletest_dir "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
if (EXISTS "${googletest_dir}" AND IS_DIRECTORY "${googletest_dir}")
  # Tell gtest to link against the "Multi-threaded Debug DLL runtime
  # library" on Windows.
  set (gtest_force_shared_crt On CACHE BOOL "Always use msvcrt.dll")
  # We don't want to install either gtest or gmock.
  set (INSTALL_GTEST Off CACHE BOOL "Disable gtest install")
  set (INSTALL_GMOCK Off CACHE BOOL "Disable gmock install")
  add_subdirectory (googletest)
  foreach (target gtest gmock gmock_main gtest_main)
    set_target_properties (${target} PROPERTIES
      CXX_STANDARD ${STANDARD}
      CXX_STANDARD_REQUIRED Yes
      CXX_EXTENSIONS No
    )

    set (gclang_options -Wno-implicit-int-float-conversion)
    if (LIBCXX)
      list (APPEND gclang_options -stdlib=libc++)
    endif ()

    target_compile_definitions (${target} PUBLIC GTEST_REMOVE_LEGACY_TEST_CASEAPI_=1)
    target_compile_options (${target} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
    )
    target_link_options (${target} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
    )
  endforeach()
endif ()


add_subdirectory (lib)
if (TARGET gtest)
  add_subdirectory (unittests)
endif ()

add_executable (json-main main.cpp)
target_link_libraries (json-main PUBLIC peejay)
setup_target (json-main)

add_custom_command (TARGET json-main
    PRE_LINK
    COMMAND pj-unittests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/output.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running PJ unit tests"
    VERBATIM
)



add_executable (main-klee main_klee.cpp)
target_link_libraries (main-klee PUBLIC peejay)
setup_target (main-klee)

add_subdirectory (tools)
