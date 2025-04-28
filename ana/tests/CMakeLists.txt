set(name AnaTests)
include(OpticksBuildOptions)


set(TEST_SOURCES
    AnaTest.cc
)

foreach(SRC ${TEST_SOURCES})
    get_filename_component(TGT ${SRC} NAME_WE)
    add_executable(${TGT} ${SRC})

    add_test(${name}.${TGT} ${TGT})

    target_link_libraries(${TGT} PUBLIC 
          Opticks::SysRap
    )
endforeach()




