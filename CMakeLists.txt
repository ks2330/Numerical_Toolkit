cmake_minimum_required(VERSION 3.15)
project(NumericalToolkit LANGUAGES CXX)

# --- Locate External Dependencies ---
# Add the Eigen library from the external directory
add_subdirectory(external/Eigen)

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

    src/app_support/app_forward_euler.cpp
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
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(tests)

# add_subdirectory(tests)
add_subdirectory(apps/lid_driven_cavity)
add_subdirectory(apps/1d_heat_equation)
add_subdirectory(apps/UI)
