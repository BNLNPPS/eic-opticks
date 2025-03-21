cmake_minimum_required(VERSION 3.5)

project(eic-opticks VERSION 0.1.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

if(DEFINED ENV{OPTIX_DIR} AND NOT "$ENV{OPTIX_DIR}" STREQUAL "")
    set(OptiX_INSTALL_DIR $ENV{OPTIX_DIR} CACHE PATH "Path to the installed location of the OptiX SDK.")
endif()

add_subdirectory(okconf)
add_subdirectory(sysrap)
add_subdirectory(ana)
add_subdirectory(analytic)
add_subdirectory(bin)
add_subdirectory(CSG)
add_subdirectory(qudarap)
add_subdirectory(CSGOptiX)
add_subdirectory(gdxml)
add_subdirectory(u4)
add_subdirectory(g4cx)

