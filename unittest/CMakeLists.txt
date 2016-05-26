file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src ${googletest_SOURCE_DIR})
add_executable (UnitTest ${SOURCES})
target_link_libraries (
  UnitTest
  analyzer
  gtest
  gtest_main
)

