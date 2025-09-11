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

# --- Library: mu (your maths etc.)
add_library(mu STATIC
  src/core/mu.cpp
)
target_include_directories(mu PUBLIC src)
target_compile_features(mu PUBLIC cxx_std_17)

# --- App
add_executable(main
  src/main.cpp
)
target_link_libraries(main PRIVATE mu SFML::Graphics)

# --- Tests
enable_testing()

add_executable(tests
  tests/mu_tests.cpp
)
target_link_libraries(tests PRIVATE mu Catch2::Catch2WithMain)
target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME mu_test COMMAND tests)
