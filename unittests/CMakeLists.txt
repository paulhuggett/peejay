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
setup_target (json-tests)
target_link_libraries (json-tests PRIVATE json-parser gtest gmock)
