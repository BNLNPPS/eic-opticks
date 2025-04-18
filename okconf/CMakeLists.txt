set(name OKConf)
project(${name} VERSION 0.1.0)
set(FLAGS_VERBOSE ON)
include(OpticksBuildOptions)
set(FLAGS_VERBOSE OFF)

set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})  

#[=[
OKConf
========

OKConf is unusual : it find_packages OptiX and G4, see::

    cmake/Modules/FindOptiX.cmake 
    cmake/Modules/FindG4.cmake 

Within those the location of the INCLUDE_DIR is used 
to access headers which are parsed to extract version integers. 
But the OptiX and G4 targets are not linked against.
The below default RPATH setup from OpticksBuildOptions of
does not work for OKConfTest ? But the above does::

   set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) 


On Linux could extract driver version

    nvidia-smi --query-gpu=driver_version --format=csv,noheader
    418.56

BUT the assumption that the build machine is the same as the 
running one is a bad one.  So paths need to be overridden.


#]=]


set(OptiX_VERBOSE ON)
find_package(OptiX) 
find_package(Geant4) 
#find_package(Custom4     MODULE QUIET) 

message(STATUS "OKCONF_OPTIX_INSTALL_DIR : ${OKCONF_OPTIX_INSTALL_DIR}")
message(STATUS "OptiX_VERSION_INTEGER : ${OptiX_VERSION}")
message(STATUS "G4_VERSION_INTEGER      : ${G4_VERSION_INTEGER}")




# generate define header for hardcoding in the library 

string(REPLACE "." "" OKCONF_CUDA_API_VERSION_INTEGER "${CMAKE_CUDA_COMPILER_VERSION}")
set(OKCONF_OPTIX_VERSION_INTEGER  "${OptiX_VERSION}")
set(OKCONF_OPTIX_INSTALL_DIR      "${OptiX_INSTALL_DIR}")
string(REPLACE "." "" OKCONF_GEANT4_VERSION_INTEGER "${Geant4_VERSION}")

set(OKCONF_OPTICKS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

set(OKCONF_COMPUTE_CAPABILITY_INTEGER "${CMAKE_CUDA_ARCHITECTURES}")
#set(OKCONF_CUDA_NVCC_FLAGS     "${CUDA_NVCC_FLAGS}")
set(OKCONF_CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS}")


set(OKConf_GENERATED_HEADER  "OKConf_Config.hh")

#configure_file( OKCONF_OpticksCMakeConfig.hh.in inc/${OKCONF_GENERATED_HEADER} )
configure_file( ${OKConf_GENERATED_HEADER}.in inc/${OKConf_GENERATED_HEADER} )


# generate opticks-config sh script into lib dir
#include(OpticksConfigureConfigScript)


set(SOURCES
    OKConf.cc
    OpticksVersionNumber.cc
)

set(HEADERS
    OKConf.hh
    OKConf.h
    OpticksVersionNumber.hh
    OKCONF_API_EXPORT.hh
)

add_library( ${name}  SHARED ${SOURCES} ${HEADERS} )

# hmm reluctant to make PUBLIC as just want to communicate to the tests 
# but for integrated it seems need to, unless just use ".." from the tests




target_include_directories( ${name} PRIVATE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/inc>
   PUBLIC
   $<INSTALL_INTERFACE:include>
   $<INSTALL_INTERFACE:include/OKConf>
)

target_compile_definitions( ${name} PUBLIC OPTICKS_OKCONF )
add_subdirectory(tests)



# <findname>_targets is my convention
message(STATUS "OptiX_targets:${OptiX_targets}")
message(STATUS "G4_targets:${G4_targets}")

set(PC_VERBOSE ON)

bcm_deploy(TARGETS ${name} NAMESPACE Opticks:: SKIP_HEADER_INSTALL)

install(FILES ${HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/inc/${OKConf_GENERATED_HEADER}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

