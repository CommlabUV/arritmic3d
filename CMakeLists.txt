cmake_minimum_required(VERSION 3.15)

# --- 1. Custom Configurations (MUST BE BEFORE project()) ---
# We define 'Fast' as a valid configuration for Multi-config generators (Visual Studio)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Fast CACHE STRING "Choose the type of build." FORCE)
endif()

# This tells Visual Studio to actually create a "Fast" profile in the .sln/.vcxproj files
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Fast" CACHE STRING "" FORCE)

project(arritmic3d CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type strings for IDEs
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release Fast)

# Find dependencies
find_package(pybind11 REQUIRED)

# --- EIGEN SETUP ---
if (NOT DEFINED EIGEN_FALLBACK_VERSION)
  set(EIGEN_FALLBACK_VERSION "3.4.0" CACHE STRING "Version of Eigen to fetch if missing")
endif()

find_package(Eigen3 3.4 QUIET)

if (NOT Eigen3_FOUND AND NOT TARGET Eigen3::Eigen)
  message(STATUS "Eigen not found: fetching Eigen ${EIGEN_FALLBACK_VERSION}")
  include(FetchContent)
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        ${EIGEN_FALLBACK_VERSION}
  )
  FetchContent_GetProperties(eigen)
  if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)
    # EXCLUDE_FROM_ALL prevents Eigen's own install rules from polluting the wheel
    add_subdirectory(${eigen_SOURCE_DIR} ${eigen_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# Build Python extension module
pybind11_add_module(_core src/bindings.cpp)

# Link Eigen3
target_link_libraries(_core PRIVATE Eigen3::Eigen)

# Include directories
target_include_directories(_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Cross-platform compiler flags
include(CheckCXXCompilerFlag)

# --- Define Flags by Profile (Lists without quotes) ---
if(MSVC)
    set(FLAGS_DEBUG          /Od /Zi /RTC1 /DDEBUG)
    set(FLAGS_FAST           /O2 /DNDEBUG)
    set(FLAGS_RELEASE        /O2)
    set(AVX_FLAG             "/arch:AVX")
else()
    set(FLAGS_DEBUG          -O0 -g -DDEBUG -fno-omit-frame-pointer)
    set(FLAGS_FAST           -O3 -DNDEBUG -ffast-math)
    set(FLAGS_RELEASE        -O3)
    set(AVX_FLAG             "-mavx")
endif()

# Apply base flags using Generator Expressions
target_compile_options(_core PRIVATE
    $<$<CONFIG:Debug>:${FLAGS_DEBUG}>
    $<$<CONFIG:Fast>:${FLAGS_FAST}>
    $<$<CONFIG:Release>:${FLAGS_RELEASE}>
)

# --- AVX Logic and Report Preparation ---
set(AVX_STATUS "Not applicable")
string(TOUPPER "${CMAKE_BUILD_TYPE}" CURRENT_CONFIG_UPPER)
set(REPORT_FLAGS ${FLAGS_${CURRENT_CONFIG_UPPER}})

if(CMAKE_BUILD_TYPE STREQUAL "Fast")
    check_cxx_compiler_flag("${AVX_FLAG}" COMPILER_SUPPORTS_AVX)
    if(COMPILER_SUPPORTS_AVX)
        target_compile_options(_core PRIVATE ${AVX_FLAG})
        set(AVX_STATUS "Supported (Enabled)")
        list(APPEND REPORT_FLAGS ${AVX_FLAG}) # Add AVX to the report list
    else()
        set(AVX_STATUS "Not supported (Skipped)")
    endif()
endif()

# --- Build Configuration Report ---
string(REPLACE ";" " " PRINTABLE_FLAGS "${REPORT_FLAGS}")

message(STATUS "-----------------------------------------------------------")
message(STATUS " ARRITMIC 3D - Build Configuration Summary:")
message(STATUS "   Build Type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "   Active Profile Flags: ${PRINTABLE_FLAGS}")

if(CMAKE_BUILD_TYPE STREQUAL "Fast")
    message(STATUS "   AVX Optimization:     ${AVX_STATUS}")
endif()

message(STATUS "   Compiler ID:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "   System Architecture:  ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "   Build System:         ${CMAKE_GENERATOR}")
message(STATUS "-----------------------------------------------------------")

# Install C++ module into arritmic3d/
install(
    TARGETS _core
    DESTINATION arritmic3d
)

# Install CSV files (always from restitutionModels/ in project root)
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/restitutionModels/"
    DESTINATION "arritmic3d/restitutionModels"
    FILES_MATCHING PATTERN "*.csv"
    PATTERN "*/.git" EXCLUDE
)
