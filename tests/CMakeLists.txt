

# Create the test executable
add_executable(matrix_test test_matrix.cpp)
add_executable(fe_test test_fe.cpp)
add_executable(cn_test test_cn.cpp)
add_executable(GFEM_test test_Global_Stiffness_Matrix.cpp)
add_executable(Triangulation_test test_triangulation.cpp)
add_executable(Shape_Gen_test test_shape_generation.cpp)

# Include the project headers for the tests
target_include_directories(matrix_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(fe_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(cn_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(GFEM_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(Triangulation_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(Shape_Gen_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link against Google Test, our toolkit library, and Eigen (via the toolkit).
# gtest_main provides the main() function for the tests.
target_link_libraries(matrix_test      PRIVATE gtest_main numerical_toolkit)
target_link_libraries(fe_test          PRIVATE gtest_main numerical_toolkit)
target_link_libraries(cn_test          PRIVATE gtest_main numerical_toolkit)
target_link_libraries(GFEM_test        PRIVATE gtest_main numerical_toolkit)
target_link_libraries(Triangulation_test PRIVATE gtest_main numerical_toolkit)
target_link_libraries(Shape_Gen_test   PRIVATE gtest_main numerical_toolkit)

# Discover and register all tests with CTest
include(GoogleTest)
gtest_discover_tests(matrix_test)
gtest_discover_tests(fe_test)
gtest_discover_tests(cn_test)
gtest_discover_tests(GFEM_test)
gtest_discover_tests(Triangulation_test)
gtest_discover_tests(Shape_Gen_test)
