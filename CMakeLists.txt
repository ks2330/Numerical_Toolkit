cmake_minimum_required(VERSION 3.15)
project(NumericalToolkit LANGUAGES CXX)

# --- Locate External Dependencies ---
# Fetch the Eigen library automatically
include(FetchContent)
FetchContent_Declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)

# When including Eigen via FetchContent, we only want the header-only library.
# Explicitly disable building Eigen's tests, docs, demos, and blas components
# to prevent it from running configuration checks that are failing in your
# environment. This is a robust way to include header-only dependencies.
set(EIGEN_BUILD_TESTING OFF)
set(EIGEN_BUILD_DOCS OFF) # Note: In Eigen 3.4.0 it's EIGEN_BUILD_DOCS
set(EIGEN_BUILD_DEMOS OFF)
set(EIGEN_BUILD_BLAS OFF)

FetchContent_MakeAvailable(Eigen)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-modules-ts")
endif()

# The 'file(GLOB_RECURSE ... "src/*.cpp")' command you had would not find the
# source files in subdirectories like 'src/nt'.
#
# While you could fix this with a pattern like "src/**/*.cpp", it is much more
# robust to list source files explicitly. This ensures that your build system
# is correctly updated when you add or remove files.
add_library(numerical_toolkit
    src/nt/setup/build_matrix.cpp
    src/nt/setup/grid.cpp
    src/nt/setup/init_matrix.cpp
    src/nt/setup/p_cal.cpp

    src/nt/finite_methods/forward_euler.cpp
    src/nt/finite_methods/crank_nicholson.cpp

    src/nt/setup/finite_element_methods/FEM_Global_Stiffness_Matrix.cpp   

    src/mesh_generation/mesh_generation.cpp
    src/mesh_generation/mesh_triangulation_algorithm.cpp
    src/mesh_generation/shape_generators.cpp

    src/app_support/app_forward_euler.cpp
    src/app_support/app_FEM.cpp
    src/app_support/app_FEM_UI.cpp
)

# This tells any target linking to 'numerical_toolkit' to also:
# 1. Add the project's 'include' directory to its include paths.
# 2. Link against the Eigen library.
target_include_directories(numerical_toolkit PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:src>
)

target_link_libraries(numerical_toolkit PUBLIC Eigen3::Eigen)


# Enable testing and fetch Google Test

# enable_testing()
# include(FetchContent)
# FetchContent_Declare(
#  googletest
#  GIT_REPOSITORY https://github.com/google/googletest.git
#  GIT_TAG        v1.14.0
# )
# FetchContent_MakeAvailable(googletest)

# add_subdirectory(tests)

# add_subdirectory(tests)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/results/metrics)

add_subdirectory(apps/lid_driven_cavity)
add_subdirectory(apps/1d_heat_equation)
add_subdirectory(apps/UI)
# ad
