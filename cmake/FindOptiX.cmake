# SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

if(TARGET OptiX::OptiX)
    return()
endif()

find_package(CUDAToolkit REQUIRED)

set(OptiX_INSTALL_DIR "OptiX_INSTALL_DIR-NOTFOUND" CACHE PATH "Path to the installed location of the OptiX SDK.")

if(NOT OptiX_FIND_VERSION)
    set(OptiX_FIND_VERSION "*")
endif()

# If they haven't specified a specific OptiX SDK install directory, search likely default locations for SDKs.
if(NOT OptiX_INSTALL_DIR)
    if(CMAKE_HOST_WIN32)
        # This is the default OptiX SDK install location on Windows.
        file(GLOB OPTIX_SDK_DIR "$ENV{ProgramData}/NVIDIA Corporation/OptiX SDK ${OptiX_FIND_VERSION}*")
    else()
        # On linux, there is no default install location for the SDK, but it does have a default subdir name.
        foreach(dir "/opt" "/usr/local" "$ENV{HOME}" "$ENV{HOME}/Downloads")
            file(GLOB OPTIX_SDK_DIR "${dir}/NVIDIA-OptiX-SDK-${OptiX_FIND_VERSION}*")
            if(OPTIX_SDK_DIR)
                break()
            endif()
        endforeach()
    endif()

    # If we found multiple SDKs, try to pick the one with the highest version number
    list(LENGTH OPTIX_SDK_DIR len)
    if(${len} GREATER 0)
        list(SORT OPTIX_SDK_DIR)
        list(REVERSE OPTIX_SDK_DIR)
        list(GET OPTIX_SDK_DIR 0 OPTIX_SDK_DIR)
    endif()
endif()

find_path(OptiX_ROOT_DIR NAMES include/optix.h PATHS ${OptiX_INSTALL_DIR} ${OPTIX_SDK_DIR} REQUIRED)
file(READ "${OptiX_ROOT_DIR}/include/optix.h" header)
string(REGEX REPLACE "^.*OPTIX_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])[^0-9].*$" "\\1.\\2.\\3" OPTIX_VERSION ${header})
string(REGEX REPLACE "^.*OPTIX_VERSION ([0-9]+)[^0-9].*$" "\\1" OptiX_VERSION ${header})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OptiX
    FOUND_VAR OptiX_FOUND
    VERSION_VAR OPTIX_VERSION
    REQUIRED_VARS
        OptiX_ROOT_DIR
    REASON_FAILURE_MESSAGE
        "OptiX installation not found. Please use CMAKE_PREFIX_PATH or OptiX_INSTALL_DIR to locate 'include/optix.h'."
)

set(OptiX_INCLUDE_DIR ${OptiX_ROOT_DIR}/include)

add_library(OptiX::OptiX INTERFACE IMPORTED)
target_include_directories(OptiX::OptiX INTERFACE ${OptiX_INCLUDE_DIR} ${CUDAToolkit_INCLUDE_DIRS})
target_link_libraries(OptiX::OptiX INTERFACE ${CMAKE_DL_LIBS})
