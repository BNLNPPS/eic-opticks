set(name GDXML)
set(desc "XML Level xercesc fixing GDML")

find_package(XercesC REQUIRED)

set(SOURCES
    GDXML_LOG.cc 
    GDXML.cc
    GDXMLRead.cc
    GDXMLWrite.cc
) 

set(HEADERS
    GDXML_LOG.hh
    GDXML_API_EXPORT.hh
    GDXML.hh
    GDXMLRead.hh
    GDXMLWrite.hh
    GDXMLErrorHandler.hh
)

add_library( ${name}  SHARED ${SOURCES} ${HEADERS} )

target_link_libraries( ${name} PUBLIC
    SysRap 
    XercesC::XercesC
)

target_include_directories( ${name} PUBLIC 
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
   $<INSTALL_INTERFACE:include>
   $<INSTALL_INTERFACE:include/GDXML>
)

target_compile_definitions( ${name} PUBLIC OPTICKS_GDXML )

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${name})

install(TARGETS ${name}
        EXPORT EicOpticksTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(tests)
