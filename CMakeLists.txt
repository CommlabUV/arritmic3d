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
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(_core PRIVATE /W4 /Od /Zi /DDEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Fast")
        target_compile_options(_core PRIVATE /W4 /O2 /DNDEBUG /arch:AVX)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(_core PRIVATE /W4 /O2 /Zi)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(_core PRIVATE /W4 /O1)
    else() # Release
        target_compile_options(_core PRIVATE /W4 /O2 /DNDEBUG)
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(_core PRIVATE -Wall -O0 -g -DDEBUG -fno-omit-frame-pointer -fsanitize=address,undefined)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Fast")
        target_compile_options(_core PRIVATE -Wall -O3 -DNDEBUG -mavx -ffast-math)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(_core PRIVATE -Wall -O2 -g)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(_core PRIVATE -Wall -Os)
    else() # Release
        target_compile_options(_core PRIVATE -Wall -O3)
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
