set(name CSGOptiXTest)

find_package(CUDAToolkit REQUIRED)

#[=[
Promote sources to ctests by moving to lower list 
generally the problem is required environment 
or doing geometry specific things. 
#]=]

set(SINGLE_SOURCES

    CSGOptiXDescTest.cc 
    CSGOptiXSimtraceTest.cc
    CSGOptiXSimulateTest.cc
    CSGOptiXSimTest.cc
    CXRaindropTest.cc


    CSGOptiXRMTest.cc
    CSGOptiXTMTest.cc
    CSGOptiXSMTest.cc

)


set(VIZ_SOURCES
    CSGOptiXRenderInteractiveTest.cc
)


set(TEST_SOURCES
    CSGOptiXVersion.cc
    CSGOptiXVersionTest.cc
    CSGOptiXRenderTest.cc
    ParamsTest.cc
)

foreach(SRC ${SINGLE_SOURCES})
    get_filename_component(TGT ${SRC} NAME_WE)
    add_executable(${TGT} ${SRC})
    target_link_libraries(${TGT} CSGOptiX CUDA::cudart)
endforeach()


find_package(glew)
find_package(glfw3)

foreach(SRC ${VIZ_SOURCES})
    get_filename_component(TGT ${SRC} NAME_WE)
    add_executable(${TGT} ${SRC})
    target_link_libraries(${TGT} 
          CSGOptiX
          GLEW::glew
          glfw
          CUDA::cudart
         )
endforeach()

foreach(SRC ${TEST_SOURCES})
    get_filename_component(TGT ${SRC} NAME_WE)
    add_executable(${TGT} ${SRC})
    target_link_libraries(${TGT} CSGOptiX CUDA::cudart)

    add_test(
       NAME ${name}.${TGT} 
       COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/CXTestRunner.sh ${CMAKE_CURRENT_BINARY_DIR}/${TGT}
    )
endforeach()
