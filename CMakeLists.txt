cmake_minimum_required(VERSION 3.21)
project(HybridCAD VERSION 0.1.0 LANGUAGES CXX)

# Full SemVer of the build. CI overrides it to produce consecutive releases
# from one commit (e.g. the 0.1.1-alpha used by the update round-trip test).
set(HCAD_VERSION_FULL "${PROJECT_VERSION}-alpha" CACHE STRING "Full SemVer version, e.g. 0.1.0-alpha")
if(NOT HCAD_VERSION_FULL MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
  message(FATAL_ERROR "HCAD_VERSION_FULL must start with MAJOR.MINOR.PATCH, got '${HCAD_VERSION_FULL}'")
endif()
set(HCAD_VERSION_MAJOR ${CMAKE_MATCH_1})
set(HCAD_VERSION_MINOR ${CMAKE_MATCH_2})
set(HCAD_VERSION_PATCH ${CMAKE_MATCH_3})
set(HCAD_PACK_ID "HybridCAD.App" CACHE STRING "Velopack package id (install folder name)")
set(HCAD_UPDATE_FEED "https://github.com/metjus/cad" CACHE STRING "Default update feed URL")
message(STATUS "HybridCAD version ${HCAD_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HCAD_BUILD_APP   "Build the Qt desktop proof of concept" ON)
option(HCAD_BUILD_TESTS "Build unit and geometry tests" ON)
option(HCAD_WITH_VELOPACK "Link the Velopack C library for installer hooks and in-app updates (Windows)" OFF)
set(VELOPACK_DIR "" CACHE PATH "Extracted velopack_libc release (contains include/ and lib/)")
set(HCAD_RUNTIME_DIRS "" CACHE STRING "Extra directories searched for runtime DLLs when installing on Windows")

find_package(OpenCASCADE REQUIRED)
message(STATUS "OpenCASCADE ${OpenCASCADE_VERSION} from ${OpenCASCADE_DIR}")

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /bigobj)
  add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
else()
  add_compile_options(-Wall -Wextra -Wno-deprecated-declarations -Wno-missing-field-initializers)
endif()

add_subdirectory(src)

if(HCAD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
