cmake_minimum_required(VERSION 3.15)
project(arritmic3d CXX)

# Find dependencies
find_package(pybind11 REQUIRED)

# Fetch Eigen automatically if not found
include(FetchContent)
if (NOT TARGET Eigen3::Eigen)
  if (NOT DEFINED EIGEN_FALLBACK_VERSION)
    set(EIGEN_FALLBACK_VERSION "3.4.0" CACHE STRING "Version of Eigen to fetch if missing")
  endif()
  message(STATUS "Eigen not found: fetching Eigen ${EIGEN_FALLBACK_VERSION}")
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        ${EIGEN_FALLBACK_VERSION}
  )
  FetchContent_MakeAvailable(eigen)
endif()

# Find Eigen3
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# Build Python extension module
pybind11_add_module(_core src/bindings.cpp)

# Link Eigen3
target_link_libraries(_core PRIVATE Eigen3::Eigen)

# Include directories
target_include_directories(_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${EIGEN3_INCLUDE_DIRS}
)

# C++ standard
target_compile_features(_core PRIVATE cxx_std_17)

# Cross-platform compiler flags
if(MSVC)
    target_compile_options(_core PRIVATE /W4 /O2)
else()
    target_compile_options(_core PRIVATE -Wall -O3)
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
