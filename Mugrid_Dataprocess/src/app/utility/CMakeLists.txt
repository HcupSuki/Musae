file(GLOB_RECURSE MusaeAppUtility_srcs ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeAppUtility STATIC ${MusaeAppUtility_srcs})
target_include_directories(MusaeAppUtility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MusaeAppUtility Mustard::Mustard)
