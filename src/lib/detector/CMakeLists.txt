file(GLOB_RECURSE MUSAE_DETECTOR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Musae/*.c++)

add_library(MusaeDetector STATIC ${MUSAE_DETECTOR_SRC})
target_include_directories(MusaeDetector PUBLIC ${PROJECT_SOURCE_DIR}/lib/detector
                                         PRIVATE ${TURTLE_INCLUDE_DIR})
target_link_libraries(MusaeDetector PUBLIC Mustard::Mustard
                                    PRIVATE turtle pmp)
target_compile_definitions(MusaeDetector PRIVATE PMP_SCALAR_TYPE_64=1
                                                 PMP_INDEX_TYPE_64=1)
