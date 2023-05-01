# CMakeLists.txt
#
# Top-level CMake file for RoostaBoosta.

cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR) # project homepage_url

# MbedOS CMake helpers. Have to be included before project() in order for proper
# toolchain selection.
include(cmake/mbed-setup.cmake)

project(
  RoostaBoosta
  VERSION 0.0.5
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

# ======================================================
# Main target.

add_executable(RoostaBoosta)

file(GLOB sources CONFIGURE_DEPENDS src/*.cpp src/*.c)

target_sources(RoostaBoosta PRIVATE ${sources})

set_target_properties(
  RoostaBoosta
  PROPERTIES ARTIFACT_NAME "RoostaBoosta-${CMAKE_PROJECT_VERSION}-networking"
             OUTPUT_NAME "RoostaBoosta-${CMAKE_PROJECT_VERSION}-networking")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(
    RoostaBoosta PUBLIC "-Wno-unknown-pragmas"
                        $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND SILENCE_WARNINGS)
  target_compile_options(RoostaBoosta PUBLIC "-Wno-narrowing"
                                             "-Wno-sign-compare")
endif()

# also have to enable in mbed_app.json
target_link_libraries(
  RoostaBoosta PRIVATE mbed-os mbed-storage-blockdevice mbed-storage-filesystem
                       mbed-storage-sd mbed-storage-fat)
target_link_libraries(RoostaBoosta PRIVATE 4DGL-uLCD-144-MbedOS6)
target_link_libraries(RoostaBoosta PRIVATE MODDMA)

mbed_set_post_build(RoostaBoosta)
