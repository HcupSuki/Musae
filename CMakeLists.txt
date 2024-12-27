cmake_minimum_required(VERSION 3.21)

# Enable MSVC_RUNTIME_LIBRARY target property
# See https://cmake.org/cmake/help/latest/policy/CMP0091.html
cmake_policy(SET CMP0091 NEW)
# Set CMP0077 to new in order to configure options for build-in libraries
# See https://discourse.cmake.org/t/what-is-the-correct-way-to-set-options-of-a-project-before-fetch-content/268/6
# See also https://cmake.org/cmake/help/latest/policy/CMP0077.html
cmake_policy(SET CMP0077 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    # ExternalProject_Add sets the timestamps of extracted contents to the time of extraction
    # See https://cmake.org/cmake/help/latest/policy/CMP0135.html
    cmake_policy(SET CMP0135 NEW)
endif()

project(Musae VERSION 0.0.0
              LANGUAGES C CXX)

# =============================================================================
# Variables
# =============================================================================

# Dirs
set(MUSAE_PROJECT_ROOT_DIR ${PROJECT_SOURCE_DIR})
set(MUSAE_PROJECT_CMAKE_DIR ${MUSAE_PROJECT_ROOT_DIR}/cmake)
set(PROJECT_SOURCE_DIR ${MUSAE_PROJECT_ROOT_DIR}/src)
set(MUSAE_PROJECT_3RDPARTY_DIR ${MUSAE_PROJECT_ROOT_DIR}/thirdparty)

# Install dirs
include(GNUInstallDirs)

# =============================================================================
# Options
# =============================================================================

include(CMakeDependentOption)

# Basic
# Options (ON/OFF)
option(MUSAE_BUILTIN_MUSTARD "Use built-in Mustard (network or pre-downloaded source is required)." OFF)
option(MUSAE_BUILTIN_TURTLE "Use built-in turtle (network or pre-downloaded source is required)." OFF)
option(MUSAE_BUILTIN_PMP "Use built-in pmp (network or pre-downloaded source is required)." OFF)

# Advance
# Options (ON/OFF)
option(MUSAE_ENABLE_ASAN_IN_DEBUG_BUILD "Enable AddressSanitizer in debug build" OFF)
option(MUSAE_ENABLE_IPO "Build Musae with interprocedural optimization" ON)
option(MUSAE_ENABLE_UBSAN_IN_DEBUG_BUILD "Enable UndefinedBehaviorSanitizer in debug build" ON)
option(MUSAE_FULL_UNITY_BUILD "Building Musae use unity build technique COMPLETELY." OFF)
option(MUSAE_USE_SHARED_MSVC_RT "Select the MSVC runtime library for use (MSVC only). ON for shared (.dll), OFF for static (.lib). If you run into link error related to MSVC runtime, you can try flip this." ON)
cmake_dependent_option(MUSAE_ENABLE_UNITY_BUILD "Build Musae with unity build technique." OFF "NOT MUSAE_FULL_UNITY_BUILD" ON)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build using shared libraries")
set(MUSAE_BUILTIN_MUSTARD_VERSION 0.8.0 CACHE STRING "Set built-in Mustard version (if required).")
set(MUSAE_BUILTIN_TURTLE_VERSION 0.11 CACHE STRING "Set built-in turtle version (if required).")
set(MUSAE_BUILTIN_PMP_VERSION 3.0.0 CACHE STRING "Set built-in pmp version (if required).")

# =============================================================================
# Installation path
# =============================================================================

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${MUSAE_PROJECT_ROOT_DIR}/install)
endif()
message(STATUS "Install path: ${CMAKE_INSTALL_PREFIX} (add -DCMAKE_INSTALL_PREFIX=<install_path> into CMake command if you wish to change it)")

# =============================================================================
# Global compile options
# =============================================================================

include(${MUSAE_PROJECT_CMAKE_DIR}/GlobalCompileConfig.cmake)

# =============================================================================
# Dependencies
# =============================================================================

# Required:

# Mustard
include(${MUSAE_PROJECT_CMAKE_DIR}/LookForMustard.cmake)

# Required, built-in if not found:

# Turtle
include(${MUSAE_PROJECT_CMAKE_DIR}/LookForTurtle.cmake)
# PMP
include(${MUSAE_PROJECT_CMAKE_DIR}/LookForPMP.cmake)

# =============================================================================
# Let's go!
# =============================================================================

# Config for Musae
include(${MUSAE_PROJECT_CMAKE_DIR}/MusaeCompileConfig.cmake)
# libraries
add_subdirectory(${PROJECT_SOURCE_DIR})
# main program
add_executable(Musae Musae.c++)
target_link_libraries(Musae MusaeAppReconstruction MusaeAppSimulation MusaeAppUtility)
install(TARGETS Musae DESTINATION ${CMAKE_INSTALL_BINDIR})
