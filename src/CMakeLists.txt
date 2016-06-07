file(GLOB_RECURSE SOURCES *.cpp *.h)

add_library(analyzer ${SOURCES})
target_link_libraries(analyzer ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS})

set_target_properties(analyzer PROPERTIES
  COTIRE_PREFIX_HEADER_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/vendor
  COTIRE_ADD_UNITY_BUILD FALSE)
cotire(analyzer)
