#===- docs/examples/CMakeLists.txt ----------------------------------------===//
# Copyright © 2026 Paul Bowen-Huggett
# SPDX-License-Identifier: MIT
#===----------------------------------------------------------------------===//
# Builds the example programs that are included in the documentation and runs
# each of them as a test so that the documentation stays correct.

set(peejay_doc_examples
  howto_char_input
  howto_errors
  howto_policies
  howto_reference
  howto_stop_early
  howto_utf8
  howto_walk_dom
  tutorial_1_validate
  tutorial_2_dom
  tutorial_3_backend
)
foreach(example ${peejay_doc_examples})
  add_executable(${example} ${example}.cpp)
  setup_peejay_target(${example})
  target_link_libraries(${example} PRIVATE peejay::peejay)
  add_test(NAME "doc example ${example}" COMMAND ${example})
endforeach()

# This example reads its input from stdin.
add_executable(tutorial_4_stream tutorial_4_stream.cpp)
setup_peejay_target(tutorial_4_stream)
target_link_libraries(tutorial_4_stream PRIVATE peejay::peejay)
add_test(NAME "doc example tutorial_4_stream" COMMAND sh -c "echo '[1, 2]' | $<TARGET_FILE:tutorial_4_stream>")
