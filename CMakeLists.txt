#===- CMakeLists.txt ------------------------------------------------------===//
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
cmake_minimum_required(VERSION 3.28)
project (peejay VERSION 2.0 LANGUAGES CXX)

include (CheckCXXCompilerFlag)
include (CheckLinkerFlag)
include (GNUInstallDirs)

cmake_policy (SET CMP0077 NEW)
option (WERROR "Compiler warnings are errors")
option (PEEJAY_COVERAGE "Generate Source-based Coverage")
option (PEEJAY_FUZZTEST "Enable fuzztest")

# A custom target from which the installs will hang.
add_custom_target (
  install-peejay
  COMMAND "${CMAKE_COMMAND}" -D CMAKE_INSTALL_COMPONENT=peejay -P
          "${CMAKE_BINARY_DIR}/cmake_install.cmake"
  USES_TERMINAL
)


# setup target
# ~~~~~~~~~~~~
function (setup_target target)
  set (
    clang_options
    -Weverything
    -Wno-c++14-extensions
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c99-extensions
    -Wno-documentation-unknown-command
    -Wno-exit-time-destructors
    -Wno-padded
    -Wno-undef
    -Wno-weak-vtables
    -Wno-covered-switch-default
  )
  set (gcc_options -Wall -Wextra -pedantic -Wno-maybe-uninitialized)
  set (
    msvc_options
    -W4 # enable lots of warnings
    -wd4068 # unknown pragma
    -wd4324 # structure was padded due to alignment specifier
  )
  target_compile_features(${target} PUBLIC cxx_std_20)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    check_cxx_compiler_flag (
      -Wno-return-std-move-in-c++11 CLANG_RETURN_STD_MOVE_IN_CXX11
    )
    if (${CLANG_RETURN_STD_MOVE_IN_CXX11})
      list (APPEND clang_options -Wno-return-std-move-in-c++11)
    endif ()
    check_cxx_compiler_flag (-Wno-c++20-compat CLANG_W_NO_CXX20_COMPAT)
    if (${CLANG_W_NO_CXX20_COMPAT})
      list (APPEND clang_options -Wno-c++20-compat)
    endif ()
    check_cxx_compiler_flag (-Wno-c++2a-compat CLANG_W_NO_CXX2A_COMPAT)
    if (${CLANG_W_NO_CXX2A_COMPAT})
      list (APPEND clang_options -Wno-c++2a-compat)
    endif ()
    check_cxx_compiler_flag (
      -Wno-unsafe-buffer-usage CLANG_W_UNSAFE_BUFFER_USAGE
    )
    if (${CLANG_W_UNSAFE_BUFFER_USAGE})
      list (APPEND clang_options -Wno-unsafe-buffer-usage)
    endif ()
  endif ()

  if (WERROR)
    list (APPEND clang_options -Werror)
    list (APPEND gcc_options -Werror)
    list (APPEND msvc_options /WX)
  endif ()

  if (PEEJAY_COVERAGE)
    list (APPEND gcc_options -fprofile-arcs -ftest-coverage)
    list (APPEND clang_options -fprofile-instr-generate -fcoverage-mapping)
  endif ()

  if (PEEJAY_FUZZTEST)
    target_compile_definitions (${target} PUBLIC PEEJAY_FUZZTEST=1)
  endif ()

  target_compile_features (${target} PUBLIC $<IF:$<BOOL:${PEEJAY_CXX17}>,cxx_std_17,cxx_std_20>)
  # Produce PIC so that PJ static libraries can be used in a shared library.
  set_target_properties (${target} PROPERTIES POSITION_INDEPENDENT_CODE Yes)

  target_compile_options (
    ${target}
    PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
      $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
      $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )
  target_link_options (
    ${target}
    PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
      $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
      $<$<CXX_COMPILER_ID:MSVC>:>
  )

  # If the link step supports -Wl,-Map generate a map file from the link.
  check_linker_flag (CXX "-Wl,-Map=map.txt" map_supported)
  if (map_supported)
    target_link_options (${target} PRIVATE "-Wl,-Map=$<TARGET_FILE_DIR:${target}>/map.txt")
  endif ()
endfunction (setup_target)



set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(peejay_matchers
  "${include_dir}/peejay/matchers/array.hpp"
  "${include_dir}/peejay/matchers/eof.hpp"
  "${include_dir}/peejay/matchers/number.hpp"
  "${include_dir}/peejay/matchers/object.hpp"
  "${include_dir}/peejay/matchers/root.hpp"
  "${include_dir}/peejay/matchers/string.hpp"
  "${include_dir}/peejay/matchers/token.hpp"
  "${include_dir}/peejay/matchers/whitespace.hpp"
)
set(peejay_details
  "${include_dir}/peejay/details/arrayvec.hpp"
  "${include_dir}/peejay/details/cbii.hpp"
  "${include_dir}/peejay/details/parser_impl.hpp"
  "${include_dir}/peejay/details/pointer_based_iterator.hpp"
  "${include_dir}/peejay/details/portab.hpp"
  "${include_dir}/peejay/details/states.hpp"
  "${include_dir}/peejay/details/uinteger.hpp"
  "${include_dir}/peejay/details/type_list.hpp"
)
set(peejay_public
  "${include_dir}/peejay/concepts.hpp"
  "${include_dir}/peejay/error.hpp"
  "${include_dir}/peejay/icubaby.hpp"
  "${include_dir}/peejay/json.hpp"
  "${include_dir}/peejay/null.hpp"
  "${include_dir}/peejay/dom.hpp"
  "${include_dir}/peejay/parser.hpp"
)

add_library(peejay STATIC
  ${peejay_public}
  ${peejay_matchers}
  ${peejay_details}
  lib/peejay/dummy.cpp
)
source_group(public FILES ${peejay_public})
source_group(matchers FILES ${peejay_matchers})
source_group(details FILES ${peejay_details})
target_include_directories(peejay PUBLIC
  $<BUILD_INTERFACE:${include_dir}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>
)
setup_target(peejay)
add_executable(dump main.cpp)
target_link_libraries(dump PUBLIC peejay)


install(
  TARGETS peejay
  EXPORT peejay
  PUBLIC_HEADER
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/peejay"
    COMPONENT peejay
  LIBRARY COMPONENT peejay
  ARCHIVE COMPONENT peejay
  INCLUDES
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
add_dependencies (install-peejay peejay)


# setup gtest
# ~~~~~~~~~~~
function(setup_gtest)
  # Check that we've got the googletest submodule checked out and that a parent project
  # didn't already create the gtest/gmock targets
  if(TARGET gtest)
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
    # Tell gtest to link against the "Multi-threaded Debug DLL runtime library" on
    # Windows.
    set(gtest_force_shared_crt On CACHE BOOL "Always use msvcrt.dll")
    # We don't want to install either gtest or gmock.
    set(INSTALL_GTEST Off CACHE BOOL "Disable gtest install")
    set(INSTALL_GMOCK Off CACHE BOOL "Disable gmock install")

    add_subdirectory(googletest)

    foreach(target gtest gmock gmock_main gtest_main)
      target_compile_features(${target} PUBLIC cxx_std_20)

      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
        check_cxx_compiler_flag (
          -Wno-implicit-int-float-conversion
          CLANG_W_NO_IMPLICIT_INT_FLOAT_CONVERSION
        )
        if(${CLANG_W_NO_IMPLICIT_INT_FLOAT_CONVERSION})
          list(APPEND gclang_options -Wno-implicit-int-float-conversion)
        endif()
      endif()

      if(LIBCXX)
        list (APPEND gclang_options -stdlib=libc++)
      endif()

      target_compile_definitions(
        ${target} PUBLIC GTEST_REMOVE_LEGACY_TEST_CASEAPI_=1
      )
      target_compile_options(
        ${target}
        PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
      )
      target_link_options(
        ${target}
        PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
      )
    endforeach()
  endif()
endfunction(setup_gtest)



if (PEEJAY_FUZZTEST)
  set (FUZZTEST_FUZZING_MODE On)
  include (FetchContent)
  set (FUZZTEST_REPO_BRANCH "1635d42" CACHE STRING "FuzzTest repository branch.")
  message ("Building fuzztest at branch " ${FUZZTEST_REPO_BRANCH})
  FetchContent_Declare (
    fuzztest
    GIT_REPOSITORY https://github.com/google/fuzztest.git
    GIT_TAG ${FUZZTEST_REPO_BRANCH}
  )
  FetchContent_MakeAvailable (fuzztest)
  enable_testing ()
  include (GoogleTest)
  fuzztest_setup_fuzzing_flags ()

  foreach (target gtest gmock gmock_main gtest_main)
    target_compile_features (${target} PUBLIC $<IF:$<BOOL:${PEEJAY_CXX17}>,cxx_std_17,cxx_std_20>)
  endforeach()
else ()
  setup_gtest ()
endif (PEEJAY_FUZZTEST)


if (TARGET gtest)
  add_subdirectory (unit_tests)
endif ()
#add_subdirectory (tools)
#add_subdirectory (klee)

include (GNUInstallDirs)
install (
  EXPORT peejay
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/peejay"
  NAMESPACE peejay::
)
