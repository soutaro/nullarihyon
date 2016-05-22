cmake_minimum_required (VERSION 2.6)
project (Nullabilint)

set (LLVM_CONFIG /Users/soutaro/src/nullabilint/build/bin/llvm-config)
execute_process (
  COMMAND ${LLVM_CONFIG} --src-root
  OUTPUT_VARIABLE LLVM_BASE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process (
  COMMAND ${LLVM_CONFIG} --obj-root
  OUTPUT_VARIABLE LLVM_BUILD
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process (
  COMMAND ${LLVM_CONFIG} --libs
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process (
  COMMAND ${LLVM_CONFIG} --cxxflags
  OUTPUT_VARIABLE LLVM_CXX_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set (CLANG_LIBS
  clang
  clangFrontend
  clangAST
  clangAnalysis
  clangBasic
  clangCodeGen
  clangDriver
  clangFrontendTool
  clangLex
  clangParse
  clangSema
  clangEdit
  clangSerialization
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangStaticAnalyzerFrontend
  clangTooling
)

set (USER_LIBS
  pthread
  curses
  z
)

include_directories("${LLVM_BASE}/include")
include_directories("${LLVM_BASE}/tools/clang/include")
include_directories("${LLVM_BUILD}/include")
include_directories("${LLVM_BUILD}/tools/clang/include")

link_directories("${LLVM_BUILD}/lib")
link_directories("${LLVM_BUILD}/tools/clang/lib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXX_FLAGS}")

file(GLOB_RECURSE CORE_SOURCES src/*.cpp src/*.h)

add_executable(nullabilint-core ${CORE_SOURCES})
target_link_libraries(nullabilint-core ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS})
