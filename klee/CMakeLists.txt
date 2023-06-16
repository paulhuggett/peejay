# Checks that the 'klee' tool is on the path and that the C++ compiler is clang.
function (klee_is_available result)
  find_program (KLEE_PATH klee)
  if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang$") AND (NOT "${KLEE_PATH}" STREQUAL "KLEE_PATH-NOTFOUND"))
    set (${result} Yes PARENT_SCOPE)
  else()
    set (${result} No PARENT_SCOPE)
  endif()
endfunction (klee_is_available)



function (setup_klee_target target)
  setup_target (${target})
  target_include_directories (${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_compile_options (${target} PRIVATE
    -g
    -fno-threadsafe-statics
    -target x86_64-pc-linux-gnu
  )
  target_compile_definitions (${target} PRIVATE HAVE_INT128)
endfunction (setup_klee_target)


# The KLEE targets are a bit complicated. There are three of them:
#
# 1. The bitcode target.
#    This is used for the KLEE run. The tool will symbolically execute the
#    bitcode and generate a series of test cases that execute the various
#    control paths through the code.
#
# 2. The executable target
#    This is an executable program that is used to exercise the test cases on
#    the target. We compile it with santizers enabled to try and shake out any
#    latent bugs.
#
# 3. The run target
#    Coordinates the running of the above processes.

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
  target_link_options (${exe_target} PRIVATE -fsanitize=undefined --rtlib=compiler-rt -lgcc_s)
  target_link_libraries (${exe_target} PRIVATE /home/klee/klee_build/lib/libkleeRuntest.so)
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

    COMMAND klee
            --libc=uclibc
            --libcxx
            --posix-runtime
            --output-dir=${CMAKE_CURRENT_BINARY_DIR}/kout/${target_base}
            --watchdog
            --max-time=1000
            ${link_llvm_lib}
            $<TARGET_OBJECTS:${bitcode_target}>

    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_klee_test
            $<TARGET_FILE:${exe_target}>
            ${CMAKE_CURRENT_BINARY_DIR}/kout/${target_base}/*.ktest

    DEPENDS ${bitcode_target}
            ${exe_target}
            ${link_llvm_lib}

    COMMENT "Running KLEE for '${bitcode_target}'"
  )

  add_dependencies(klee-all ${run_target})
endfunction (add_klee_target)
    

klee_is_available (klee_available)
message (STATUS "KLEE available? ${klee_available}")
if (klee_available)
  add_custom_target (klee-all)

  # Calls to add_klee_target of the form:
  set (TESTS
      av_assign_count
      av_assign_range
      av_insert_pos_lvalue
      av_insert_pos_rvalue
      av_insert_range
      av_resize_count
      av_resize_count_value
      quote
  )
  foreach (T ${TESTS})
    message(STATUS "Adding KLEE test ${T}")
    add_klee_target (TARGET klee-${T} SOURCES ${T}.cpp)
endforeach()


endif ()
