set(name QUDARap)

set(Custom4_VERBOSE ON)
find_package(Custom4 CONFIG)

set(DEV_SOURCES
    QSim_cerenkov_dev.cu
)

set(SOURCES
    QUDARAP_LOG.cc

    QCurandStateMonolithic.cc
    QCurandStateMonolithic.cu
    QCurandState.cu


    QSim.cc
    QSim.cu


    QPMT.cc
    QPMT.cu 

    QEvent.cc
    QEvent.cu

    QDebug.cc
    QState.cc

    QU.cc

    QRng.cc  
    QRng.cu

    QTex.cc
    QTexRotate.cc
    QTex.cu
    QTexMaker.cc
    QTexLookup.cc
    QTexLookup.cu

    QBase.cc
    QBnd.cc
    QBnd.cu


    QOptical.cc
    QOptical.cu 

    QScint.cc
    QScint.cu

    QCerenkovIntegral.cc
    QCerenkov.cc
    QCerenkov.cu 

    QCK.cc
    QCK.hh

    QPoly.cc
    QPoly.cu

    QProp.cc
    QProp.cu

    QMultiFilm.cc
    QMultiFilm.cu

    QBuf.cc
)

SET(HEADERS
    QUDARAP_LOG.hh
    QUDARAP_API_EXPORT.hh
    QUDA_CHECK.h


    QCurandStateMonolithic.hh
    qcurandwrap.h


    QSim.hh
    qsim.h

    QPMT.hh
    qpmt.h

    qgs.h
    qprop.h
 
    QEvent.hh

    QDebug.hh
    qdebug.h

    QU.hh
    QRng.hh
    qrng.h

    QTex.hh
    QTexRotate.hh
    QTexMaker.hh
    QTexLookup.hh

    QState.hh

    qmultifilm.h 

    QBase.hh
    qbase.h 

    QBnd.hh
    qbnd.h



    QOptical.hh

    QScint.hh
    qscint.h

    QCerenkovIntegral.hh
    QCerenkov.hh
    qcerenkov.h

    QPoly.hh

    QProp.hh
    
    QMultiFilm.hh

    QBuf.hh
)

add_library( ${name} ${SOURCES} )

get_target_property(ILL ${name} INTERFACE_LINK_LIBRARIES)
message(STATUS "${name} INTERFACE_LINK_LIBRARIES:${ILL} " )

if(Custom4_FOUND)
   target_compile_definitions( ${name} PUBLIC WITH_CUSTOM4 )
   target_include_directories( ${name} PUBLIC ${Custom4_INCLUDE_DIR})
endif()

target_link_libraries( ${name} SysRap ssl)
target_compile_definitions( ${name} PUBLIC OPTICKS_QUDARAP DEBUG_TAG )


#[=[

Global compile definitions such as DEBUG_TAG and DEBUG_PIDX are defined in sysrap/CMakeLists.txt 
and should be flipped there, NOT HERE. 

For local debugging do flips within files, and do not commit like that 
as they may cause difficult to debug build inconsistencies. 

#]=]


#[=[

WITH_THRUST compile definition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* WITH_THRUST is needed for the Custom4 GPU complex math functions used by qpmt.h 

* However Custom4 also used on CPU without WITH_THRUST (eg by junosw), so 
  the WITH_THRUST definition must be PRIVATE to avoid interference with other
  Custom4 usage

#]=]

target_compile_definitions( ${name} PRIVATE WITH_THRUST )

target_include_directories( ${name} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/sysrap/inc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${name})

install(TARGETS ${name}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(tests)
