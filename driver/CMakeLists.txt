file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src)
add_executable(nullarihyon-core ${SOURCES})
target_link_libraries(nullarihyon-core ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS} analyzer)

install (TARGETS nullarihyon-core DESTINATION libexec/nullarihyon/${NULL_VERSION})
