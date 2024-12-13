file(GLOB_RECURSE MUSAE_DATA_MODEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeData STATIC ${MUSAE_DATA_MODEL_SRC})
target_include_directories(MusaeData PUBLIC ${PROJECT_SOURCE_DIR}/lib/data)
target_link_libraries(MusaeData PUBLIC Mustard::Mustard)
