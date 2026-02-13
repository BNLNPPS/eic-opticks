find_package(Geant4 REQUIRED ui_all vis_all)

configure_file(config_path.h.in config_path.h @ONLY)

# gphox is the primary library
add_library(gphox config.cpp torch.cpp)
target_link_libraries(gphox PUBLIC ${Geant4_LIBRARIES} G4CX SysRap)
target_include_directories(gphox
    INTERFACE $<INSTALL_INTERFACE:include>
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# consgeo is a tool for converting GDML into CSG
add_executable(consgeo consgeo.cc)

target_link_libraries(consgeo G4CX)

target_include_directories(consgeo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# simg4ox runs Geant4 and OptiX simulations
add_executable(simg4ox simg4ox.cpp g4app.h)

target_link_libraries(simg4ox gphox)

target_include_directories(simg4ox PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# GPUCerenkov - Cerenkov-only example with Opticks GPU raytracing
add_executable(GPUCerenkov GPUCerenkov.cpp GPUCerenkov.h)
target_link_libraries(GPUCerenkov gphox)
target_include_directories(GPUCerenkov PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# GPURaytrace example - Cerenkov + Scintillation with Opticks GPU raytracing
add_executable(GPURaytrace GPURaytrace.cpp GPURaytrace.h)
target_link_libraries(GPURaytrace gphox)
target_include_directories(GPURaytrace PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# GPUPhotonSource - optical photon input via SetInputPhoton with Opticks GPU simulation
add_executable(GPUPhotonSource GPUPhotonSource.cpp GPUPhotonSource.h)
target_link_libraries(GPUPhotonSource gphox)
target_include_directories(GPUPhotonSource PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# GPUPhotonSourceMinimal - GPU-only ray tracing (no G4 optical photon tracking)
add_executable(GPUPhotonSourceMinimal GPUPhotonSourceMinimal.cpp GPUPhotonSourceMinimal.h)
target_link_libraries(GPUPhotonSourceMinimal gphox)
target_include_directories(GPUPhotonSourceMinimal PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

# simtox creates a numpy file with initial photons for simulation
add_executable(simtox simtox.cpp)

target_link_libraries(simtox gphox)

target_include_directories(simtox PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

install(TARGETS consgeo simg4ox GPUCerenkov GPURaytrace GPUPhotonSource GPUPhotonSourceMinimal simtox gphox
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES config.h torch.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
