cmake_minimum_required(VERSION 3.15)
project(NumericalToolkit LANGUAGES CXX)


include(FetchContent)
FetchContent_Declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)

set(EIGEN_BUILD_TESTING OFF)
set(EIGEN_BUILD_DOCS OFF) #
set(EIGEN_BUILD_DEMOS OFF)
set(EIGEN_BUILD_BLAS OFF)

FetchContent_MakeAvailable(Eigen)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-modules-ts")
    set(NT_STATIC_RUNTIME_FLAGS -static -static-libgcc -static-libstdc++)
endif()

add_library(numerical_toolkit
    src/nt/setup/build_matrix.cpp
    src/nt/setup/grid.cpp
    src/nt/setup/init_matrix.cpp
    src/nt/setup/p_cal.cpp

    src/nt/finite_methods/forward_euler.cpp
    src/nt/finite_methods/crank_nicholson.cpp

    src/mesh_generation/mesh_init.cpp
    src/mesh_generation/mesh_sampling.cpp
    src/mesh_generation/mesh_triangulation.cpp
    src/mesh_generation/mesh_Bowyer_Watson.cpp
    src/mesh_generation/mesh_cleanup.cpp
    src/mesh_generation/mesh_metrics.cpp
    src/mesh_generation/shape_generators.cpp
    src/mesh_generation/quadtree.cpp
    src/mesh_generation/mesh_Advancing_Front.cpp

    src/app_support/app_forward_euler.cpp
    src/app_support/app_FEM.cpp
    src/app_support/app_FEM_UI.cpp

    src/app_support/solver_api.cpp
)


target_include_directories(numerical_toolkit PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:src>
)

target_link_libraries(numerical_toolkit PUBLIC Eigen3::Eigen)

if(MSVC)
    target_compile_definitions(numerical_toolkit PUBLIC _USE_MATH_DEFINES)
endif()

# --- Optional components (all default ON; the Python build turns tests/apps OFF) ---
option(BUILD_TESTS  "Build the Google Test suite"        ON)
option(BUILD_APPS   "Build the CLI apps"                 ON)
option(BUILD_PYTHON "Build the pycfd Python bindings"    OFF)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/results/metrics)

if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

if(BUILD_APPS)
    add_subdirectory(apps/UI)
    add_subdirectory(apps/FVM_solver)
endif()

# --- Python bindings (pycfd) — opt-in; needs a matching Python + pybind11 ---
# Uses the pip-installed pybind11 that matches the venv interpreter (not FetchContent, so the
# version always tracks the Python). Configure with, e.g.:
#   cmake -S . -B build-py -G "Visual Studio 17 2022" -A x64 \
#         -DBUILD_PYTHON=ON -DBUILD_TESTS=OFF -DBUILD_APPS=OFF \
#         -DPython_EXECUTABLE=<venv>/Scripts/python.exe \
#         -Dpybind11_DIR=<venv>/Lib/site-packages/pybind11/share/cmake/pybind11
if(BUILD_PYTHON)
    find_package(pybind11 CONFIG REQUIRED)
    add_subdirectory(bindings)
endif()
