#[=[
OpticksBuildOptions.cmake
============================

This is included into all Opticks subproject CMakeLists.txt
Formerly did a conditional find_package for OKConf here::

   if(NOT ${name} STREQUAL "OKConf" AND NOT ${name} STREQUAL "OKConfTest")
      find_package(OKConf     REQUIRED CONFIG)   
   endif()

But it is confusing to hide a package dependency like this, 
it is better to be explicit and have opticks-deps give a true picture 
of the dependencies. BCM is an exception as it is CMake level only 
infrastructure. 

Also formerly included OpticksCUDAFlags which defines CUDA_NVCC_FLAGS here, 
but that depends on the COMPUTE_CAPABILITY that is provided by OKConf 
so have moved that to the OKConf generated TOPMATTER, which generates:

*  $(opticks-prefix)/lib/cmake/okconf/okconf-config.cmake

RPATH setup docs 

* https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling

#]=]

message(STATUS "Configuring ${name}")
if(OBO_VERBOSE)
message(STATUS "OpticksBuildOptions.cmake Configuring ${name} [")
endif()

# for policy to be felt by includer need to : include(OpticksBuildOptions NO_POLICY_SCOPE)  
if(POLICY CMP0077)  # see note/issues/cmake-3.13.4-FindCUDA-warnings.rst
    #cmake_policy(SET CMP0077 OLD)
    cmake_policy(SET CMP0077 NEW)
endif()




if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
   message(STATUS " CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR} ")
   message(STATUS " CMAKE_BINARY_DIR : ${CMAKE_BINARY_DIR} ")
   message(FATAL_ERROR "in-source build detected : DONT DO THAT")
endif()


if(NOT OPTICKS_PREFIX)
    get_filename_component(OBO_MODULE_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    get_filename_component(OBO_MODULE_DIRDIR ${OBO_MODULE_DIR} DIRECTORY)
    get_filename_component(OBO_MODULE_DIRDIRDIR ${OBO_MODULE_DIRDIR} DIRECTORY)
    set(OPTICKS_PREFIX ${OBO_MODULE_DIRDIRDIR})
    # this gives correct prefix when this module is included from installed tree
    # but when included from source tree it gives home
    # hence use -DOPTICKS_PREFIX=$(om-prefix) for cmake internal source builds
    # so that OPTICKS_PREFIX is always correct
endif()


# initialize a list into which targets found by cmake/Modules/FindXXX.cmake are appended
set(OPTICKS_TARGETS_FOUND)


include(CTest)
#add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

include(GNUInstallDirs)
set(CMAKE_INSTALL_INCLUDEDIR "include/${name}")  # override the GNUInstallDirs default of "include"


find_package(BCM CONFIG)

if(NOT BCM_FOUND)
   message(STATUS "CMAKE_MODULE_PATH:${CMAKE_MODULE_PATH}")
   message(STATUS "CMAKE_PREFIX_PATH:${CMAKE_PREFIX_PATH}   expect to find BCM in one of these prefix dirs")
   message(STATUS "see examples/UseBCM to debug failure to find BCM")
   message(FATAL_ERROR "ABORT " )
endif()

include(BCMDeploy)
include(BCMSetupVersion)  # not yet used in anger, see examples/UseGLM
include(EchoTarget)
include(TopMetaTarget)

set(BUILD_SHARED_LIBS ON)

include(OpticksCXXFlags)   

if(OBO_VERBOSE)
message(STATUS "OpticksBuildOptions.cmake Configuring ${name} ]")
endif()


