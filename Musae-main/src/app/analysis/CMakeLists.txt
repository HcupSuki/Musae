file(GLOB_RECURSE MusaeAppAnalysis_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppAnalysis STATIC ${MusaeAppAnalysis_srcs})
target_include_directories(MusaeAppAnalysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(ROOT)
target_link_libraries(MusaeAppAnalysis Mustard::Mustard ROOT::HistPainter MusaeData MusaeDetector)
