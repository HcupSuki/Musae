file(GLOB_RECURSE MusaeAppSimulation_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppSimulation STATIC ${MusaeAppSimulation_srcs})
target_include_directories(MusaeAppSimulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MusaeAppSimulation Mustard::Mustard MusaeData MusaeDetector MusaeSimulation) 

# file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/scripts)
# file(GLOB MUSAE_SIMULATION_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Musae/SimFlux/scripts/*)
# foreach(_scripts ${MUSAE_SIMULATION_SCRIPTS})
#     file(COPY ${_scripts} DESTINATION ${CMAKE_BINARY_DIR}/scripts)  
# endforeach()


