file(GLOB_RECURSE SOURCES *.cpp *.h)

include_directories(../src)
add_executable(nullabilint-core ${SOURCES})
target_link_libraries(nullabilint-core ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS} analyzer)
