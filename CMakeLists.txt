cmake_minimum_required(VERSION 3.15)
project(NumericalToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(external/Eigen)

add_subdirectory(apps/lid_driven_cavity)
