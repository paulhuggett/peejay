#===- tools/check/CMakeLists.txt ------------------------------------------===//
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
add_executable (pj-check check.cpp)
target_link_libraries (pj-check PUBLIC peejay)
setup_target (pj-check)

function(run_test prelink_target test_target)
  set (out_xml "${CMAKE_BINARY_DIR}/${test_target}.xml")
  add_dependencies (${prelink_target} ${test_target})
  add_custom_command (
    TARGET ${prelink_target}
    PRE_LINK
    COMMAND "$<TARGET_FILE:${test_target}>" "--gtest_output=xml:${out_xml}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running PJ unit tests"
    BYPRODUCTS ${out_xml}
    VERBATIM
  )
endfunction(run_test)

if (TARGET gtest)
  run_test(pj-check pj-unittests)
  run_test(pj-check pj-schema-unittests)
  run_test(pj-check pj-uri-unittests)
endif ()
