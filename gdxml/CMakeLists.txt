set(name GDXML)
set(desc "XML Level xercesc fixing GDML")
project(${name} VERSION 0.1.0)
include(OpticksBuildOptions NO_POLICY_SCOPE)

#[=[


#]=]


#set(G4_VERBOSE YES)
#set(OpticksXercesC_VERBOSE YES)

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
    Opticks::SysRap 
    XercesC::XercesC
)

target_include_directories( ${name} PUBLIC 
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
   $<INSTALL_INTERFACE:include>
   $<INSTALL_INTERFACE:include/GDXML>
)

target_compile_definitions( ${name} PUBLIC OPTICKS_GDXML )
target_compile_definitions( ${name} PUBLIC G4USE_STD11 ) 

install(FILES ${HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
bcm_deploy(TARGETS ${name} NAMESPACE Opticks:: SKIP_HEADER_INSTALL)

add_subdirectory(tests)



