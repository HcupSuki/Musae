file(GLOB_RECURSE SimFlux_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.c++)
add_executable(SimFlux ${SimFlux_srcs})
target_link_libraries(SimFlux MusaeData MusaeDetector MusaeReconstruction MusaeSimulation Mustard::Mustard)

file(GLOB SimFlux_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*)
foreach(_scripts ${SimFlux_SCRIPTS})
    configure_file(${_scripts} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endforeach()

install(TARGETS SimFlux DESTINATION ${CMAKE_INSTALL_BINDIR})
foreach(_scripts ${SimFlux_SCRIPTS})
    install(FILES ${_scripts} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/SimFlux)
endforeach()
