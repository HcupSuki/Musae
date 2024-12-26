file(GLOB_RECURSE ReconEvent_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.c++)
add_executable(ReconLGA ${ReconEvent_srcs})
find_package(ROOT)
target_link_libraries(ReconLGA Mustard::Mustard ROOT::Minuit2 MusaeData MusaeDetector)

install(TARGETS ReconLGA DESTINATION ${CMAKE_INSTALL_BINDIR})
