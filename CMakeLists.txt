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

# --- Define the Toolkit Library ---
# This creates a static library named 'numerical_toolkit' that other executables can link to.
add_library(numerical_toolkit
    include/nt/grid.cpp
    include/nt/build_matrix.cpp
    include/nt/init_matrix.cpp
    include/nt/p_cal.cpp
)

# This tells any target linking to 'numerical_toolkit' to also:
# 1. Add the project's 'include' directory to its include paths.
# 2. Link against the Eigen library.
target_include_directories(numerical_toolkit PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
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
