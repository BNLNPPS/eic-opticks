cmake_minimum_required(VERSION 3.22)
project(AdePTandSimphony LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(simphony REQUIRED)
find_package(Geant4 REQUIRED)
find_package(AdePT REQUIRED)
find_package(VecGeom REQUIRED)

include(${VecGeom_DIR}/CudaRdcUtils.cmake)

set(ADEPT_SRC "" CACHE PATH "")
set(ADEPT_INSTALL "" CACHE PATH "")
add_library(adept_physics_list STATIC ${ADEPT_SRC}/examples/common/src/FTFP_BERT_AdePT.cc)
target_include_directories(adept_physics_list PUBLIC
    ${ADEPT_SRC}/examples/common/include
    ${ADEPT_INSTALL}/include
)
target_link_libraries(adept_physics_list PUBLIC ${Geant4_LIBRARIES})

add_executable(AdePTandSimphony AdePTandSimphony.cpp)
target_link_libraries(AdePTandSimphony PRIVATE
    simphony::G4CX
    simphony::U4
    simphony::SysRap
    adept_physics_list
    ${Geant4_LIBRARIES}
)
cuda_rdc_target_link_libraries(AdePTandSimphony PRIVATE
    AdePT::G4Integration
    AdePT::Transport
)
