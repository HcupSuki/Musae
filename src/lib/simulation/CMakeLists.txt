file(GLOB_RECURSE MUSAE_SIMULATION_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeSimulation STATIC ${MUSAE_SIMULATION_SRC})
target_include_directories(MusaeSimulation PUBLIC ${PROJECT_SOURCE_DIR}/lib/simulation)
target_link_libraries(MusaeSimulation PUBLIC MusaeData MusaeDetector MusaeReconstruction Mustard::Mustard)
