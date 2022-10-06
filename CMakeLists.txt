cmake_minimum_required(VERSION 3.0)

project (json C CXX)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

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
set_property (TARGET json-parser PROPERTY CXX_STANDARD 20)
set_property (TARGET json-parser PROPERTY CXX_STANDARD_REQUIRED Yes)



add_executable (json-tests
    "${PROJECT_SOURCE_DIR}/googletest/googlemock/src/gmock_main.cc"
    callbacks.cpp
    callbacks.hpp
    test_array.cpp
    test_boolean.cpp
    test_comment.cpp
    test_json.cpp
    test_object.cpp
    test_utf.cpp
    test_number.cpp
    test_string.cpp
)
set_property (TARGET json-tests PROPERTY CXX_STANDARD 20)
set_property (TARGET json-tests PROPERTY CXX_STANDARD_REQUIRED Yes)
target_link_libraries (json-tests PRIVATE json-parser gtest gmock)



add_executable (json-main main.cpp)
target_link_libraries (json-main PUBLIC json-parser)
set_property (TARGET json-main PROPERTY CXX_STANDARD 20)
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
set_property (TARGET main-klee PROPERTY CXX_STANDARD 20)
set_property (TARGET main-klee PROPERTY CXX_STANDARD_REQUIRED Yes)

#eof:CMakeLists.txt
