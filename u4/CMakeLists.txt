set(name U4)
set(desc "Geant4 Low Level Utilities")

find_package(Geant4 REQUIRED)
find_package(CLHEP  REQUIRED) 
find_package(XercesC REQUIRED)

find_package(Custom4 CONFIG)  # have to be explicit as Custom4 needed without j/PMTSim
find_package(PMTSim_standalone CONFIG)


set(SOURCES
    U4_LOG.cc 
    U4.cc
    U4SolidMaker.cc
    U4SolidTree.cc
    U4Material.cc
    U4VolumeMaker.cc
    U4Recorder.cc
    U4StepPoint.cc
    U4Random.cc
    U4SensitiveDetector.cc

    U4Debug.cc
    U4Scintillation_Debug.cc
    U4Cerenkov_Debug.cc
    U4Hit_Debug.cc

    ShimG4OpAbsorption.cc
    ShimG4OpRayleigh.cc
    Local_G4Cerenkov_modified.cc
    Local_DsG4Scintillation.cc

    U4Physics.cc
) 

set(OTHER_SOURCES
    InstrumentedG4OpBoundaryProcess.cc
)


set(HEADERS
    U4_LOG.hh
    U4_API_EXPORT.hh
    U4.hh
    U4ThreeVector.h
    U4Hit.h 
    U4HitGet.h 
    U4Track.h
    U4Stack.h
    Deprecated_U4PhotonInfo.h
    U4GDML.h
    U4Transform.h
    U4Tree.h
    U4TreeBorder.h
    U4Boundary.h
    U4NistManager.h

    U4App.h 
    U4VPrimaryGenerator.h 

    U4SensorIdentifier.h
    U4SensorIdentifierDefault.h
    U4SensitiveDetector.hh

    U4SolidMaker.hh
    U4SolidTree.hh

    U4Material.hh
    U4Mat.h 
    U4Scint.h

    U4Volume.h
    U4Surface.h
    U4Solid.h
    U4Polycone.h
    U4MaterialPropertiesTable.h
    U4MaterialPropertyVector.h

    U4VolumeMaker.hh
    U4Recorder.hh
    U4OpBoundaryProcess.h
    U4StepPoint.hh
    U4Random.hh

    U4Engine.h
    U4Touchable.h

    U4Navigator.h
    U4Simtrace.h 

    U4Debug.hh
    U4Scintillation_Debug.hh
    U4Cerenkov_Debug.hh
    U4Hit_Debug.hh

    ShimG4OpAbsorption.hh
    ShimG4OpRayleigh.hh

    Local_G4Cerenkov_modified.hh
    Local_DsG4Scintillation.hh
    U4Physics.hh

    U4PMTAccessor.h 

    U4Process.h
    U4PhysicsTable.h
    U4PhysicsVector.h
    U4MaterialTable.h
    U4OpticalSurfaceFinish.h
    U4OpticalSurfaceModel.h 
    U4SurfaceType.h 
    U4SurfacePerfect.h
    U4SurfaceArray.h 

    U4Mesh.h
    U4Version.h 
)

set(OTHER_HEADERS
    InstrumentedG4OpBoundaryProcess.hh
)


add_library(${name} ${SOURCES} ${HEADERS} )

target_link_libraries( ${name} PUBLIC
    SysRap 
    CLHEP::CLHEP
    XercesC::XercesC
    GDXML
    Geant4::G4persistency
)

target_include_directories( ${name} PUBLIC 
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/U4>
)

target_compile_definitions( ${name} PUBLIC OPTICKS_U4 ) # THIS MUST BE PUBLIC FOR SLOG-ING TO WORK FULLY 
target_compile_definitions( ${name} PRIVATE STANDALONE ) 


if(Custom4_FOUND)
    target_link_libraries( ${name} PUBLIC Custom4::Custom4 )
    target_compile_definitions( ${name} PRIVATE WITH_CUSTOM4 )  # MINTING A DEF ? MAYBE COMES WITH THE TARGET ANYHOW
endif()

if(PMTSim_standalone_FOUND)
    target_link_libraries( ${name} PUBLIC Opticks::PMTSim_standalone )
    target_compile_definitions( ${name} PRIVATE WITH_PMTSIM PMTSIM_STANDALONE WITH_NP )
    ## THE DEFS CANNOT BE PUBLIC : AS CAUSE ISSUES FOR JUNOSW BUILD WITH OPTICKS
endif()


install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${name})

install(TARGETS ${name}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(tests)
