file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src ${googletest_SOURCE_DIR})
add_executable (UnitTest ${SOURCES})
target_link_libraries (
  UnitTest
  analyzer
  gtest
  gtest_main
)

set_target_properties(UnitTest PROPERTIES
  COTIRE_PREFIX_HEADER_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/vendor
  COTIRE_ADD_UNITY_BUILD FALSE)
cotire(UnitTest)
