cmake_minimum_required(VERSION 3.18)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 75)
endif()

project(eic-opticks VERSION 0.1.0 LANGUAGES CXX CUDA)

set(BUILD_SHARED_LIBS ON)

include(GNUInstallDirs)
include(CTest)

add_subdirectory(sysrap)
add_subdirectory(CSG)
add_subdirectory(qudarap)
add_subdirectory(CSGOptiX)
add_subdirectory(gdxml)
add_subdirectory(u4)
add_subdirectory(g4cx)
add_subdirectory(src)
