set(name G4CX)

set(SOURCES
    G4CX_LOG.cc 
    G4CXOpticks.cc
) 

set(HEADERS
    G4CX_LOG.hh
    G4CX_API_EXPORT.hh
    G4CXOpticks.hh
)

add_library(${name} SHARED ${SOURCES} ${HEADERS})

target_link_libraries(${name} PUBLIC U4 CSGOptiX)

target_include_directories( ${name} PUBLIC 
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
   $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CSGOptiX>
)

target_compile_definitions( ${name} PUBLIC OPTICKS_G4CX )

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${name})

install(TARGETS ${name}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(tests)
