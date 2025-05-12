add_executable(consgeo consgeo.cc)

target_link_libraries(consgeo G4CX)
target_include_directories(consgeo PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>)
install(TARGETS consgeo RUNTIME DESTINATION bin)
