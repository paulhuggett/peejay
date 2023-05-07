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
cmake_minimum_required(VERSION 3.13)

project (peejay C CXX)

include (CheckCXXCompilerFlag)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

cmake_policy(SET CMP0077 NEW)
option (WERROR "Compiler warnings are errors")
option (PEEJAY_CXX17 "Use C++17 (rather than the default C++20")
option (LIBCXX "Use libc++ rather than libstdc++ (clang only)")
option (COVERAGE "Generate LLVM Source-based Coverage")

if (PEEJAY_CXX17)
  set (STANDARD 17)
else ()
  set (STANDARD 20)
endif ()

# A custom target from which the installs will hang.
add_custom_target (install-peejay
    COMMAND "${CMAKE_COMMAND}"
            -D CMAKE_INSTALL_COMPONENT=peejay
            -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    USES_TERMINAL
)

set (PEEJAY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# setup target
# ~~~~~~~~~~~~
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
    -wd4068 # 4068: unknown pragma
    -wd4324 # 4324: structure was padded due to alignment specifier
  )

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    check_cxx_compiler_flag (-Wno-c++20-compat CLANG_W_NO_CXX20_COMPAT)
    if (${CLANG_W_NO_CXX20_COMPAT})
      list (APPEND clang_options -Wno-c++20-compat)
    endif ()
    check_cxx_compiler_flag (-Wc++2a-compat CLANG_W_NO_CXX2A_COMPAT)
    if (${CLANG_W_NO_CXX2A_COMPAT})
      list (APPEND clang_options -Wno-c++2a-compat)
    endif ()
  endif ()

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


if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
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

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
      check_cxx_compiler_flag (-Wno-implicit-int-float-conversion CLANG_W_NO_IMPLICIT_INT_FLOAT_CONVERSION)
      if (${CLANG_W_NO_IMPLICIT_INT_FLOAT_CONVERSION})
        list (APPEND gclang_options -Wno-implicit-int-float-conversion)
      endif ()
    endif ()

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

if (TARGET gtest)
  set (prelink_target json-main)
  set (test_target pj-unittests)
  set (out_xml "${CMAKE_BINARY_DIR}/${test_target}.xml")
  add_custom_command (
    TARGET ${prelink_target}
    PRE_LINK
    COMMAND "$<TARGET_FILE:${test_target}>" "--gtest_output=xml:${out_xml}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running PJ unit tests"
    DEPENDS ${test_target}
    BYPRODUCTS ${out_xml}
    VERBATIM
  )
endif ()


add_executable (main-klee main_klee.cpp)
target_link_libraries (main-klee PUBLIC peejay)
setup_target (main-klee)

add_subdirectory (tools)


include (GNUInstallDirs)
install (
  EXPORT peejay
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/peejay"
  NAMESPACE peejay::
)
# List of four values that specify what project to install. The four values are:
# - Build directory
# - Project Name
# - Project Component
# - Directory.
set (CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_PROJECT_NAME};ALL;/")
