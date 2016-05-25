file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src ${googletest_SOURCE_DIR})
add_executable (Test ${SOURCES})
target_link_libraries (
  Test
  analyzer
  gtest
  gtest_main
)

