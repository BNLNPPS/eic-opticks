cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(ddeicopticks VERSION 0.2.0 LANGUAGES CXX)

#------------------------------- -*- cmake -*- -------------------------------#
# eic-opticks DD4hep integration plugins
#
# Builds DD4hep action plugins that integrate eic-opticks GPU-accelerated
# optical photon simulation into DD4hep/Geant4.
#
# Usage from a DD4hep steering file:
#   OpticsSteppingAction -- collects gensteps from standard G4Cerenkov/G4Scintillation
#   OpticsRun            -- initializes/finalizes G4CXOpticks per run
#   OpticsEvent          -- triggers GPU simulation per event
#
# Cerenkov/Scintillation physics should be registered via DD4hep's built-in
# Geant4CerenkovPhysics and Geant4OpticalPhotonPhysics plugins.
#----------------------------------------------------------------------------#

find_package(DD4hep REQUIRED COMPONENTS DDG4 DDCore)
find_package(eic-opticks REQUIRED)
find_package(Geant4 REQUIRED)

dd4hep_set_compiler_flags()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

dd4hep_add_plugin(ddeicopticks
  SOURCES
    OpticsSteppingAction.cc
    OpticsRun.cc
    OpticsEvent.cc
  USES
    DD4hep::DDG4
    DD4hep::DDCore
    eic-opticks::G4CX
    eic-opticks::U4
    eic-opticks::SysRap
)

install(TARGETS ddeicopticks
  LIBRARY DESTINATION lib
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_MODULE_PREFIX}ddeicopticks.components
  DESTINATION lib
)
