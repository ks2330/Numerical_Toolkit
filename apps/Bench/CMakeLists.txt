add_executable(fem_bench main.cpp)

target_include_directories(fem_bench PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(fem_bench PRIVATE numerical_toolkit)
