add_executable(1d_heat_equation main.cpp)

# Add the project's main include directory so we can find "nt/grid.h", etc.
target_include_directories(1d_heat_equation PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link the executable against our toolkit library. This resolves the undefined reference.
target_link_libraries(1d_heat_equation PRIVATE numerical_toolkit)