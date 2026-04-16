add_executable(fem_steady_state main.cpp)

target_include_directories(fem_steady_state PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(fem_steady_state PRIVATE numerical_toolkit)
