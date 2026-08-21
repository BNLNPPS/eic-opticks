cmake_minimum_required(VERSION 3.22)
project(Geant4andSimphony LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(simphony REQUIRED)
find_package(Geant4 REQUIRED)

add_executable(Geant4andSimphony Geant4andSimphony.cpp)
target_link_libraries(Geant4andSimphony PRIVATE
    simphony::G4CX
    simphony::U4
    simphony::SysRap
    ${Geant4_LIBRARIES}
)
target_link_options(Geant4andSimphony PRIVATE LINKER:--disable-new-dtags)
