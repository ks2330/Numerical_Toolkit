# Create the test executable
add_executable(matrix_test test_matrix.cpp)

# Link against Google Test, our toolkit library, and Eigen (via the toolkit).
# gtest_main provides the main() function for the tests.
target_link_libraries(matrix_test PRIVATE gtest_main numerical_toolkit)

# Discover and register the tests with CTest
include(GoogleTest)
gtest_discover_tests(matrix_test)
