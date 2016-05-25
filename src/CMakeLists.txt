file(GLOB_RECURSE SOURCES *.cpp *.h)

add_library(analyzer ${SOURCES})
target_link_libraries(analyzer ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS})
