

# Create the test executables
add_executable(matrix_test test_matrix.cpp)
add_executable(fe_test test_fe.cpp)
add_executable(cn_test test_cn.cpp)
add_executable(GFEM_test test_Global_Stiffness_Matrix.cpp)
add_executable(Triangulation_test test_triangulation.cpp)
add_executable(Shape_Gen_test test_shape_generation.cpp)
add_executable(PotentialFlow_test test_FEM_potential_flow.cpp)
add_executable(GasModel_test test_FVM_gas_model.cpp)
add_executable(MeshGeometry_test test_mesh_geometry.cpp)
add_executable(FVM_mesh_test test_FVM_mesh.cpp)
add_executable(FVM_flux_test test_FVM_flux.cpp)

# Include the project headers for the tests
target_include_directories(matrix_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(fe_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(cn_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(GFEM_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(Triangulation_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(Shape_Gen_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(PotentialFlow_test  PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(GasModel_test       PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(MeshGeometry_test   PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(FVM_mesh_test     PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(FVM_flux_test     PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link against Google Test, our toolkit library, and Eigen (via the toolkit).
target_link_libraries(matrix_test        PRIVATE gtest_main numerical_toolkit)
target_link_libraries(fe_test            PRIVATE gtest_main numerical_toolkit)
target_link_libraries(cn_test            PRIVATE gtest_main numerical_toolkit)
target_link_libraries(GFEM_test          PRIVATE gtest_main numerical_toolkit)
target_link_libraries(Triangulation_test PRIVATE gtest_main numerical_toolkit)
target_link_libraries(Shape_Gen_test     PRIVATE gtest_main numerical_toolkit)
target_link_libraries(PotentialFlow_test PRIVATE gtest_main numerical_toolkit)
target_link_libraries(GasModel_test      PRIVATE gtest_main numerical_toolkit)
target_link_libraries(MeshGeometry_test  PRIVATE gtest_main numerical_toolkit)
target_link_libraries(FVM_mesh_test    PRIVATE gtest_main numerical_toolkit)
target_link_libraries(FVM_flux_test    PRIVATE gtest_main numerical_toolkit)

# Discover and register all tests with CTest.
# PRE_TEST defers discovery to run time so MinGW runtime DLLs are in PATH.
# PROPERTIES LABELS "toolkit" lets us filter out Eigen's own 900 registered tests.
include(GoogleTest)
gtest_discover_tests(matrix_test        DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(fe_test            DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(cn_test            DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(GFEM_test          DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(Triangulation_test DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(Shape_Gen_test     DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(PotentialFlow_test DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(GasModel_test      DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(MeshGeometry_test  DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(FVM_mesh_test     DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")
gtest_discover_tests(FVM_flux_test     DISCOVERY_MODE PRE_TEST PROPERTIES LABELS "toolkit")