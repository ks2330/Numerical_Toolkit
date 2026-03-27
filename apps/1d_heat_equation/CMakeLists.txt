# Create the app directly from main.cpp
add_executable(1d_heat_equation main.cpp)

target_include_directories(1d_heat_equation PRIVATE ${CMAKE_SOURCE_DIR}/external/eigen)

include(FetchContent)

FetchContent_Declare(
  fkYAML
  GIT_REPOSITORY https://github.com/fktn-k/fkYAML.git
  GIT_TAG        v0.4.2
)
FetchContent_MakeAvailable(fkYAML)

target_include_directories(1d_heat_equation PRIVATE ${fkyaml_SOURCE_DIR}/include)