# pycfd Python module — built only when BUILD_PYTHON=ON (needs pybind11 + a matching Python).
# pybind11 is fetched in the top-level CMakeLists when BUILD_PYTHON is set.
pybind11_add_module(pycfd bindings.cpp)
target_link_libraries(pycfd PRIVATE numerical_toolkit)
target_include_directories(pycfd PRIVATE ${PROJECT_SOURCE_DIR}/include)
