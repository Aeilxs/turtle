cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Dependencies
include(FetchContent)

# SFML
FetchContent_Declare(SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.1
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(SFML)

# Catch2
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(Catch2)

# --- mu
add_library(mu STATIC
  src/core/mu.cpp
)
target_include_directories(mu PUBLIC src)
target_compile_features(mu PUBLIC cxx_std_17)

# --- geom (header-only)
add_library(geom INTERFACE)
target_include_directories(geom INTERFACE src)
target_link_libraries(geom INTERFACE mu)

# --- gfx
add_library(gfx STATIC
  src/gfx/triangulate.cpp
  src/gfx/frame.cpp
)
target_include_directories(gfx PUBLIC src)
target_link_libraries(gfx PUBLIC mu geom)

# --- turtle (header-only)
add_library(turtle INTERFACE)
target_include_directories(turtle INTERFACE src)
target_link_libraries(turtle INTERFACE mu geom gfx)

# --- backend SFML
add_library(gfx_sfml STATIC
  src/backends/sfml_renderer.cpp
  src/backends/sfml_renderer.hpp
)
target_include_directories(gfx_sfml PUBLIC src)
target_link_libraries(gfx_sfml PUBLIC gfx SFML::Graphics)

# --- app
add_executable(main
  src/main.cpp
)
target_link_libraries(main PRIVATE gfx_sfml turtle)

# --- tests
enable_testing()
add_executable(tests
  tests/mu_tests.cpp
)
target_link_libraries(tests PRIVATE mu Catch2::Catch2WithMain)
target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME mu_test COMMAND tests)
