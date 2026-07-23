add_executable(fvm_solver main.cpp)

target_include_directories(fvm_solver PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(fvm_solver PRIVATE numerical_toolkit)
