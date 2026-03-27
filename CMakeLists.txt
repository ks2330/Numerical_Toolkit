cmake_minimum_required(VERSION 3.15)
project(NumericalToolkit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-modules-ts")
endif()



# add_subdirectory(tests)
add_subdirectory(apps/lid_driven_cavity)
add_subdirectory(apps/1d_heat_equation)