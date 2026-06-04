file(GLOB_RECURSE MusaeAppVisualization_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppVisualization STATIC ${MusaeAppVisualization_srcs})
target_include_directories(MusaeAppVisualization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MusaeAppVisualization Mustard::Mustard MusaeData MusaeDetector)
