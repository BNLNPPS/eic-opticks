find_package(Geant4 REQUIRED ui_all vis_all)

# consgeo is a tool for converting GDML into CSG
add_executable(consgeo consgeo.cc)

target_link_libraries(consgeo G4CX)

target_include_directories(consgeo PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

install(TARGETS consgeo EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# simg4ox runs Geant4 and OptiX simulations
add_executable(simg4ox simg4ox.cpp g4app.h torch.cpp)

target_link_libraries(simg4ox ${Geant4_LIBRARIES} G4CX)

target_include_directories(simg4ox PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

install(TARGETS simg4ox RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# simtox creates a numpy file with initial photons for simulation
add_executable(simtox simtox.cpp torch.cpp)

target_link_libraries(simtox SysRap)

target_include_directories(simtox PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

install(TARGETS simtox RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
