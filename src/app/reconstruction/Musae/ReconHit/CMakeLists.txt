file(GLOB_RECURSE ReconHit_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.c++)
add_executable(ReconHit ${ReconHit_srcs})
target_link_libraries(ReconHit Mustard::Mustard MusaeData MusaeDetector)

install(TARGETS ReconHit DESTINATION ${CMAKE_INSTALL_BINDIR})
