#===- klee/CMakeLists.txt -------------------------------------------------===//
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
function (klee_is_available result)
  find_program (KLEE_PATH klee)
  if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang$") AND (NOT "${KLEE_PATH}" STREQUAL
                                                    "KLEE_PATH-NOTFOUND")
  )
    set (${result} Yes PARENT_SCOPE)
  else ()
    set (${result} No PARENT_SCOPE)
  endif ()
endfunction (klee_is_available)

function(setup_klee_target target)
  setup_target(${target})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  set(flags -g -fno-threadsafe-statics -target x86_64-pc-linux-gnu
    -Wno-weak-vtables
    -Wno-padded
    -Wno-documentation-unknown-command
    -Wno-switch-enum
    -Wno-covered-switch-default
    -Wno-exit-time-destructors
    -Wno-ctad-maybe-unsupported
    -Wno-documentation
  )
  target_compile_options(${target} PRIVATE ${flags})
endfunction(setup_klee_target)

# The KLEE targets are a bit complicated. There are three of them:
#
# 1. The bitcode target. This is used for the KLEE run. The tool will
#    symbolically execute the bitcode and generate a series of test cases that
#    execute the various control paths through the code.
# 2. The executable target This is an executable program that is used to exercise
#    the test cases on the target. We compile it with santizers enabled to try
#    and shake out any latent bugs.
# 3. The run target Coordinates the running of the above processes.

function (add_klee_target)
  cmake_parse_arguments (
    klee_prefix
    "" # options
    "TARGET" # one-value keywords
    "DEPENDS;SOURCES" # multi-value keywords.
    ${ARGN}
  )

  if (NOT klee_available)
    return ()
  endif ()

  set (depends ${klee_prefix_DEPENDS})
  set (sources ${klee_prefix_SOURCES})

  # Target names derived from the TARGET argument.
  set (target_base ${klee_prefix_TARGET})
  set (bitcode_target "${target_base}-bc")
  set (exe_target "${target_base}-exe")
  set (run_target "${target_base}-run")

  message (STATUS "Adding KLEE targets klee-${target_base}-bc/exe/run")

  # Bitcode Target

  add_library (${bitcode_target} OBJECT ${sources})
  setup_klee_target (${bitcode_target})
  target_compile_definitions (${bitcode_target} PRIVATE NDEBUG NO_INLINE_ASM)
  target_compile_options (${bitcode_target} PRIVATE -emit-llvm)
  target_link_libraries (${bitcode_target} PRIVATE peejay)

  # Executable Target

  add_executable (${exe_target} ${sources})
  setup_klee_target (${exe_target})
  target_compile_definitions (${exe_target} PRIVATE KLEE_RUN=1)
  target_compile_options (${exe_target} PRIVATE -fsanitize=undefined)
  target_link_options (
    ${exe_target} PRIVATE -fsanitize=undefined --rtlib=compiler-rt -lgcc_s
  )
  target_link_libraries (
    ${exe_target} PRIVATE /home/klee/klee_build/lib/libkleeRuntest.so
  )
  target_link_libraries (${exe_target} PRIVATE ${depends})
  target_link_libraries (${exe_target} PRIVATE peejay)

  # Run Target

  set (link_llvm_lib "") # collection of dependent libraries.
  foreach (dependent ${depends})
    list (APPEND link_llvm_lib "--link-llvm-lib=$<TARGET_FILE:${dependent}-bc>")
  endforeach (dependent)

  add_custom_target (
    ${run_target}
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/kout
    COMMAND rm -r -f ${CMAKE_CURRENT_BINARY_DIR}/kout/${target_base}
    COMMAND
      klee --only-output-states-covering-new
      --libc=uclibc --libcxx --posix-runtime --exit-on-error-type=Abort
      --exit-on-error-type=Assert --exit-on-error-type=BadVectorAccess
      --exit-on-error-type=Execution --exit-on-error-type=External
      --exit-on-error-type=Free --exit-on-error-type=Model
      --exit-on-error-type=Overflow --exit-on-error-type=Ptr
      --exit-on-error-type=ReadOnly --exit-on-error-type=ReportError
      --exit-on-error-type=User
      --output-dir=${CMAKE_CURRENT_BINARY_DIR}/kout/${target_base} --watchdog
      --max-time=1000 ${link_llvm_lib} $<TARGET_OBJECTS:${bitcode_target}>
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/run_klee_test $<TARGET_FILE:${exe_target}>
      ${CMAKE_CURRENT_BINARY_DIR}/kout/${target_base}/*.ktest
    DEPENDS ${bitcode_target} ${exe_target} ${link_llvm_lib}
    COMMENT "Running KLEE for '${bitcode_target}'"
  )

  add_dependencies (klee-all ${run_target})
  add_dependencies (klee-build-all ${bitcode_target} ${exe_target})
endfunction (add_klee_target)

klee_is_available (klee_available)
message (STATUS "KLEE available? ${klee_available}")
if (klee_available)
  add_custom_target(klee-all)
  add_custom_target(klee-build-all)

  # Calls to add_klee_target of the form:
  set(TESTS
    av_assign_count
    av_assign_range
    av_ctor_count
    av_ctor_count_value
    av_ctor_first_last
    av_emplace_pos_args
    av_erase_first_last
    av_erase_pos
    av_insert_pos_count_lvalue
    av_insert_pos_first_last
    av_insert_pos_lvalue
    av_insert_pos_rvalue
    av_insert_range
    av_resize_count
    av_resize_count_value
    keyword
    quote
  )
  foreach (T ${TESTS})
    add_klee_target (TARGET klee-${T} SOURCES ${T}.cpp)
  endforeach ()

endif ()
