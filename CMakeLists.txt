cmake_minimum_required(VERSION 3.15)
project(arritmic3d CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type and flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Fast CACHE STRING "Choose the type of build." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel Fast)

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
    add_subdirectory(${eigen_SOURCE_DIR} ${eigen_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# Build Python extension module
pybind11_add_module(_core src/bindings.cpp)

# Link Eigen3
target_link_libraries(_core PRIVATE Eigen3::Eigen)

# Include directories
target_include_directories(_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# C++ standard
target_compile_features(_core PRIVATE cxx_std_17)

# Cross-platform compiler flags
if(MSVC)
    target_compile_options(_core PRIVATE /W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Fast")
        target_compile_options(_core PRIVATE /O2 /DNDEBUG /arch:AVX)
        message(STATUS "Build configuration: FAST (Windows AVX)")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(_core PRIVATE /Od /Zi /DDEBUG)
        message(STATUS "Build configuration: DEBUG (Windows)")
    else() # Release
        target_compile_options(_core PRIVATE /O2 /DNDEBUG)
        message(STATUS "Build configuration: RELEASE (Windows)")
    endif()
else()
    target_compile_options(_core PRIVATE -Wall)
    if(CMAKE_BUILD_TYPE STREQUAL "Fast")
        target_compile_options(_core PRIVATE -O3 -DNDEBUG -mavx -ffast-math)
        message(STATUS "Build configuration: FAST (Linux/macOS AVX)")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(_core PRIVATE -O0 -g -DDEBUG -fno-omit-frame-pointer -fsanitize=address,undefined)
        message(STATUS "Build configuration: DEBUG (Linux/macOS)")
    else() # Release
        target_compile_options(_core PRIVATE -O3 -DNDEBUG)
        message(STATUS "Build configuration: RELEASE (Linux/macOS)")
    endif()
endif()

# Install C++ module into arritmic/
install(
    TARGETS _core
    DESTINATION arritmic
)

# Install CSV files (always from restitutionModels/ in project root)
install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/restitutionModels/"
    DESTINATION "arritmic/restitutionModels"
    FILES_MATCHING PATTERN "*.csv"
    PATTERN "*/.git" EXCLUDE
)
