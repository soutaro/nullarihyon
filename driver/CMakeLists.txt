file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src)
add_executable(nullarihyon-core ${SOURCES})
target_link_libraries(nullarihyon-core ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS} analyzer)

install (TARGETS nullarihyon-core DESTINATION libexec/nullarihyon/${NULL_VERSION})

set_target_properties(nullarihyon-core PROPERTIES
  COTIRE_PREFIX_HEADER_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/vendor
  COTIRE_ADD_UNITY_BUILD FALSE)
cotire(nullarihyon-core)
