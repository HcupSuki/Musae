file(GLOB_RECURSE GenCRMu_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.c++)
add_executable(GenCRMu ${GenCRMu_srcs})
target_link_libraries(GenCRMu Mustard::Mustard)

install(TARGETS GenCRMu DESTINATION ${CMAKE_INSTALL_BINDIR})
