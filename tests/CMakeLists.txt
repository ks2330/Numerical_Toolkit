# Create the test executable
add_executable(matrix_test test_matrix.cpp)

# Link Google Test (gtest_main provides the main() function for the tests)
target_link_libraries(matrix_test PRIVATE gtest_main)

# Discover and register the tests with CTest
include(GoogleTest)
gtest_discover_tests(matrix_test)