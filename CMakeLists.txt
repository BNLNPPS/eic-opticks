cmake_minimum_required(VERSION 3.5)

project(esi-opticks VERSION 0.1.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

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

