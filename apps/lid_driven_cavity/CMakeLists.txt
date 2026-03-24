# Create the app directly from main.cpp
add_executable(lid_driven_cavity main.cpp)

# No need for nt_lib yet, just tell it where Eigen is
target_include_directories(lid_driven_cavity PRIVATE ${CMAKE_SOURCE_DIR}/external/eigen)