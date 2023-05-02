# CMakeLists.txt
#
# Top-level CMake file for RoostaBoosta.

cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR) # project homepage_url

# MbedOS CMake helpers. Have to be included before project() in order for proper
# toolchain selection.
include(cmake/mbed-setup.cmake)

project(
  RoostaBoosta
  VERSION 0.0.4
  DESCRIPTION
    "IoT Weather Alarm Clock - Georgia Tech ECE 4180 Spring 2023 Final Project"
  HOMEPAGE_URL https://github.com/mshakula/RoostaBoosta
  LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================================================
# External libraries.

add_subdirectory(third_party/mbed-os)
add_subdirectory(third_party/4DGL-uLCD-144-MbedOS6)
add_subdirectory(third_party/MODDMA)
add_subdirectory(third_party/RTC)

# ======================================================
# Main target.

add_executable(RoostaBoosta)

file(GLOB sources CONFIGURE_DEPENDS src/*.cpp src/*.h)

target_sources(RoostaBoosta PRIVATE ${sources})

set_target_properties(
  RoostaBoosta PROPERTIES ARTIFACT_NAME "RoostaBoosta-${CMAKE_PROJECT_VERSION}"
                          OUTPUT_NAME "RoostaBoosta-${CMAKE_PROJECT_VERSION}")

# also have to enable in mbed_app.json
target_link_libraries(
  RoostaBoosta PRIVATE mbed-os mbed-storage-blockdevice mbed-storage-filesystem
                       mbed-storage-sd mbed-storage-fat)
target_link_libraries(RoostaBoosta PRIVATE 4DGL-uLCD-144-MbedOS6)
target_link_libraries(RoostaBoosta PRIVATE MODDMA)
target_link_libraries(RoostaBoosta PRIVATE RTC)

mbed_set_post_build(RoostaBoosta)
