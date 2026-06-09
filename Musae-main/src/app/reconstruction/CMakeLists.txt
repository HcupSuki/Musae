file(GLOB_RECURSE MusaeAppReconstruction_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppReconstruction STATIC ${MusaeAppReconstruction_srcs})
target_include_directories(MusaeAppReconstruction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(ROOT)
target_link_libraries(MusaeAppReconstruction Mustard::Mustard ROOT::Minuit2 MusaeData MusaeDetector)
