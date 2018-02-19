cmake_minimum_required(VERSION 3.0)

project (json C CXX)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# Tell gtest to link against the "Multi-threaded Debug DLL runtime
# library" on Windows.
set (gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
add_subdirectory (googletest)


add_library (json-parser STATIC
    dom_types.cpp
    dom_types.hpp
    json.cpp
    json.hpp
    json_error.cpp
    json_error.hpp
    maybe.hpp
    utf.cpp
    utf.hpp
)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options (json-parser PRIVATE
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-exit-time-destructors
    )
elseif (CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options (json-parser PRIVATE -Wall -Wextra -pedantic)
elseif (MSVC)
  target_compile_options (json-parser PRIVATE /W4)
endif ()
set_property (TARGET json-parser PROPERTY CXX_STANDARD 11)
set_property (TARGET json-parser PROPERTY CXX_STANDARD_REQUIRED Yes)



add_executable (json-tests
    "${PROJECT_SOURCE_DIR}/googletest/googlemock/src/gmock_main.cc"
    test_json.cpp
    test_utf.cpp
    test_number.cpp
)
set_property (TARGET json-tests PROPERTY CXX_STANDARD 11)
set_property (TARGET json-tests PROPERTY CXX_STANDARD_REQUIRED Yes)
target_link_libraries (json-tests PRIVATE json-parser gtest gmock)



add_executable (json-main main.cpp)
target_link_libraries (json-main PUBLIC json-parser)
set_property (TARGET json-main PROPERTY CXX_STANDARD 11)
set_property (TARGET json-main PROPERTY CXX_STANDARD_REQUIRED Yes)

add_custom_command (TARGET json-main
    PRE_LINK
    COMMAND json-tests "--gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/output.xml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running json unit tests"
    VERBATIM
)



add_executable (main-klee main_klee.cpp)
target_link_libraries (main-klee PUBLIC json-parser)
set_property (TARGET main-klee PROPERTY CXX_STANDARD 11)
set_property (TARGET main-klee PROPERTY CXX_STANDARD_REQUIRED Yes)

#eof:CMakeLists.txt
