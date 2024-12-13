file(GLOB_RECURSE Musae_RECONSTRUCTION_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeReconstruction STATIC ${Musae_RECONSTRUCTION_SRC})
target_include_directories(MusaeReconstruction PUBLIC ${PROJECT_SOURCE_DIR}/lib/reconstruction)
target_link_libraries(MusaeReconstruction PUBLIC MusaeData MusaeDetector Mustard::Mustard)
