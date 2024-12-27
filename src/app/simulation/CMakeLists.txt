file(GLOB_RECURSE MusaeAppSimulation_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppSimulation STATIC ${MusaeAppSimulation_srcs})
target_include_directories(MusaeAppSimulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MusaeAppSimulation Mustard::Mustard MusaeData MusaeDetector MusaeSimulation) 
