

# Create the test executable
add_executable(matrix_test test_matrix.cpp)
add_executable(fe_test test_fe.cpp)
add_executable(cn_test test_cn.cpp)


# Include the project headers for the tests
target_include_directories(matrix_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(fe_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(cn_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    

# Link against Google Test, our toolkit library, and Eigen (via the toolkit).
# gtest_main provides the main() function for the tests.
target_link_libraries(matrix_test PRIVATE gtest_main numerical_toolkit)
target_link_libraries(fe_test PRIVATE gtest_main numerical_toolkit)
target_link_libraries(cn_test PRIVATE gtest_main numerical_toolkit)


# Discover and register the tests with CTest
include(GoogleTest)


# The gtest_discover_tests() function will automatically find all TEST() cases in the specified executable and register them with CTest.
gtest_discover_tests(matrix_test)
gtest_discover_tests(fe_test)
gtest_discover_tests(cn_test)
