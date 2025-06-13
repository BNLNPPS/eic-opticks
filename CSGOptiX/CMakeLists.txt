set(name CSGOptiX)

find_package(OptiX)
set(SOURCES)
set(OPTIX_SOURCES)


list(APPEND SOURCES
    Params.cc
    CSGOptiX.cc
    Frame.cc
    CSGOPTIX_LOG.cc

    Ctx.cc
    Properties.cc
    GAS.cc
    GAS_Builder.cc
    PIP.cc
    IAS.cc
    IAS_Builder.cc
    SBT.cc
)

list(APPEND HEADERS
    CSGOptiX.h
    CSGOPTIX_API_EXPORT.hh
    CSGOPTIX_LOG.hh
)

list(APPEND CU_SOURCES 
     CSGOptiX7.cu
     Check.cu
)

list(APPEND SOURCES  
     Ctx.cc 
     Properties.cc 
     GAS.cc 
     GAS_Builder.cc 
     PIP.cc 
     IAS.cc 
     IAS_Builder.cc 
     SBT.cc 
)


set(_generated_PTX_files "")

foreach(cu_src IN LISTS CU_SOURCES)
    message(STATUS "Processing ${cu_src}")
    add_library(${cu_src} OBJECT ${cu_src})
    set_target_properties(${cu_src} PROPERTIES CUDA_PTX_COMPILATION ON)
    target_include_directories(${cu_src} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CSG>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sysrap>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/qudarap>)
    target_compile_definitions(${cu_src} PUBLIC WITH_PRD WITH_SIMULATE WITH_SIMTRACE WITH_RENDER
            OPTICKS_CSGOPTIX WITH_THRUST OPTICKS_CSG WITH_CONTIGUOUS WITH_S_BB CONFIG_Debug OPTICKS_SYSRAP WITH_CHILD
            PLOG_LOCAL RNG_PHILOX DEBUG_TAG DEBUG_PIDX DEBUG_PIDXYZ WITH_STTF WITH_SLOG OPTICKS_OKCONF OPTICKS_QUDARAP NVCC)
    target_link_libraries(${cu_src} PUBLIC OptiX::OptiX)

    list(APPEND _generated_PTX_files "${cu_src}.ptx")
endforeach()

message(STATUS "_generated_PTX_files ${_generated_PTX_files}") 

add_library( ${name} SHARED ${SOURCES} ${HEADERS} )

target_compile_options( ${name}_OPTIX
   PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-O2 --use_fast_math -Xcompiler -fPIC>
)

#[=[

CUDA_ARCHITECTURES
--------------------

Note that for ptx generation must specify a single GPU architecture otherwise::

    nvcc fatal   : Option '--ptx (-ptx)' is not allowed when compiling for multiple GPU architectures

Compile Definition Notes
----------------------------


WITH_PRD
~~~~~~~~~

using Pointer trick means can reduce attrib and payload to 2


WITH_THRUST
~~~~~~~~~~~~

* WITH_THRUST is needed for Custom4 on GPU complex math functions used by qpmt.h

* However Custom4 also used on CPU without WITH_THRUST (eg by junosw), so
  the WITH_THRUST definition must be PRIVATE to avoid interference with other
  Custom4 usage

#]=]



#[=[
      ${CSG_INCLUDE_DIRS}
/usr/local/cuda-12.4/include;/data1/blyth/local/opticks_Debug/include/CSG;/data1/blyth/local/opticks_Debug/externals/glm/glm;/data1/blyth/local/opticks_Debug/include/CSG

      $<BUILD_INTERFACE:/dummy/cx/begin/build>
      $<INSTALL_INTERFACE:/dummy/cx/begin/install>

      $<BUILD_INTERFACE:/dummy/cx/end/build>
      $<INSTALL_INTERFACE:/dummy/cx/end/install>
      ${CUDA_INCLUDE_DIRS}
#]=]


target_include_directories(${name}
   PUBLIC
      $<BUILD_INTERFACE:${OPTICKS_PREFIX}/externals/glm/glm>
      $<INSTALL_INTERFACE:externals/glm/glm>
      ${OpticksOptiX_INCLUDE}
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(${name}_OPTIX
   PUBLIC
      ${OpticksOptiX_INCLUDE}
      $<BUILD_INTERFACE:${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}>
      $<BUILD_INTERFACE:${OPTICKS_PREFIX}/include/SysRap>
      $<BUILD_INTERFACE:${OPTICKS_PREFIX}/include/CSG>
      $<BUILD_INTERFACE:${OPTICKS_PREFIX}/include/QUDARap>
)

if(Custom4_FOUND)
   target_include_directories( ${name}_OPTIX PUBLIC ${Custom4_INCLUDE_DIR})
endif()

target_include_directories( ${name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CSGOptiX>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${name} PUBLIC OptiX::OptiX CSG QUDARap SysRap)

install(FILES $<TARGET_OBJECTS:CSGOptiX7.cu> DESTINATION ptx RENAME CSGOptiX_generated_CSGOptiX7.cu.ptx)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${name})

install(TARGETS ${name}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(tests)
