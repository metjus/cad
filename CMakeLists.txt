cmake_minimum_required(VERSION 3.21)
project(HybridCAD VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HCAD_BUILD_APP   "Build the Qt desktop proof of concept" ON)
option(HCAD_BUILD_TESTS "Build unit and geometry tests" ON)
set(HCAD_RUNTIME_DIRS "" CACHE STRING "Extra directories searched for runtime DLLs when installing on Windows")

find_package(OpenCASCADE REQUIRED)
message(STATUS "OpenCASCADE ${OpenCASCADE_VERSION} from ${OpenCASCADE_DIR}")

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /bigobj)
  add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
else()
  add_compile_options(-Wall -Wextra -Wno-deprecated-declarations)
endif()

add_subdirectory(src)

if(HCAD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
