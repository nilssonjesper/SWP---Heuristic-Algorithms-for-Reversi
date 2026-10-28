cmake_minimum_required(VERSION 3.16)
project(swp2025_group7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Output binaries into bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Base flags
set(BASE_FLAGS
    "-Ofast -flto=auto -finline-functions -funroll-all-loops -finline-functions-called-once -march=native -Wall -Wextra"
)
# Architecture dependent flags
set(X86_FLAGS "-mssse3 -mbmi2")
set(ARM_FLAGS "")

# Configure flags based on architecture
if(APPLE)
  message(STATUS "Configuring for ARM.")
  set(CMAKE_CXX_FLAGS "${BASE_FLAGS} ${APPLE_ARM_FLAGS}")
else()
  message(STATUS "Configuring for x86-64.")
  set(CMAKE_CXX_FLAGS "${BASE_FLAGS} ${X86_FLAGS}")
endif()

# Debug configuration
option(ENABLE_ASSERTIONS "When enabled, includes (runtime) assert statements."
       OFF)
if(ENABLE_ASSERTIONS)
  message(STATUS "Enabled debug mode.")
else()
  add_compile_definitions(NDEBUG)
endif()

# Test library
option(LINK_TESTS "When enabled, links GoogleTest library." OFF)
if(LINK_TESTS)
  message(STATUS "Linking test library.")

  # Link test library
  enable_testing()
  add_subdirectory(tests)
endif()

# Sanitizer library
option(LINK_SANITIZERS
       "When enables, links address and undefined behavior sanitizers." OFF)
if(LINK_SANITIZERS)
  message(STATUS "Linking libasan, libubsan.")

  # Link libasan, libubsan
  add_compile_options(-fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
endif()

if(NOT
   (ENABLE_ASSERTIONS
    OR LINK_TESTS
    OR LINK_SANITIZERS))
  message(STATUS "Release build.")
endif()

add_subdirectory(src)
