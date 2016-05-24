cmake_minimum_required (VERSION 2.6)
project (Nullabilint)

if (NOT LLVM_ROOT)
  message (FATAL_ERROR "Specify location of llvm from command line: -DLLVM_ROOT=/opt/local/clang+llvm-3.8.0-x86_64-apple-darwin")
endif()

get_filename_component(LLVM_ROOT ${LLVM_ROOT} ABSOLUTE)

if (NOT LLVM_CONFIG)
  set (LLVM_CONFIG ${LLVM_ROOT}/bin/llvm-config)
  message (STATUS "llvm-config is supporsed in ${LLVM_CONFIG}; you can overwrite from command line: -DLLVM_CONFIG")
endif()

if (NOT LLVM_INCLUDE_PATHS)
  set (LLVM_INCLUDE_PATHS ${LLVM_ROOT}/include)
  message (STATUS "LLVM/Clang header files are supporsed in ${LLVM_INCLUDE_PATHS}; you can overwrite from command line: -DLLVM_INCLUDE_PATHS")
endif()

if (NOT LLVM_LIB_PATHS)
  set (LLVM_LIB_PATHS ${LLVM_ROOT}/lib)
  message (STATUS "LLVM/Clang libs are supporsed in ${LLVM_LIB_PATHS}; you can overwrite from command line: -DLLVM_LIB_PATHS")
endif()

if (NOT LLVM_LIBS)
  execute_process (
    COMMAND ${LLVM_CONFIG} --libs
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if (NOT LLVM_CXX_FLAGS)
  execute_process (
    COMMAND ${LLVM_CONFIG} --cxxflags
    OUTPUT_VARIABLE LLVM_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# Remove unnecesary -isysroot from llvm-config --cxxflags
string(REGEX REPLACE "-isysroot +[^ ]+ " "" LLVM_CXX_FLAGS ${LLVM_CXX_FLAGS})

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

FOREACH (header_path ${LLVM_INCLUDE_PATHS})
  include_directories(${header_path})
ENDFOREACH(header_path)

FOREACH (lib_path ${LLVM_LIB_PATHS})
  link_directories(${lib_path})
ENDFOREACH(lib_path)

add_definitions (-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions (-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXX_FLAGS}")

file(GLOB_RECURSE CORE_SOURCES src/*.cpp src/*.h)

add_executable(nullabilint-core ${CORE_SOURCES})
target_link_libraries(nullabilint-core ${LLVM_LIBS} ${CLANG_LIBS} ${USER_LIBS})
