cmake_minimum_required (VERSION 3.0)
project (Nullarihyon)

if (NOT LLVM_ROOT)
  message (FATAL_ERROR "Specify location of llvm from command line: -DLLVM_ROOT=/opt/local/clang+llvm-3.8.0-x86_64-apple-darwin")
endif()

if (NOT IS_ABSOLUTE ${LLVM_ROOT})
  set (LLVM_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${LLVM_ROOT})
  get_filename_component(LLVM_ROOT ${LLVM_ROOT} ABSOLUTE)
endif()

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

if (NOT CLANG_COMPILER_LIB_PATH)
  set (CLANG_COMPILER_LIB_PATH ${LLVM_ROOT}/lib/clang)
  message (STATUS "Clang compiler libs are supporsed in ${CLANG_COMPILER_LIB_PATH}; you can overwrite from command line: -DCLANG_COMPILER_LIB_PATH")
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
  clangASTMatchers
)

set (USER_LIBS
  pthread
  curses
  z
)

FOREACH (header_path ${LLVM_INCLUDE_PATHS})
  get_filename_component(header_path ${header_path} ABSOLUTE)
  include_directories(${header_path})
ENDFOREACH(header_path)

FOREACH (lib_path ${LLVM_LIB_PATHS})
  get_filename_component(lib_path ${lib_path} ABSOLUTE)
  link_directories(${lib_path})
ENDFOREACH(lib_path)

add_definitions (-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS)
add_definitions (-D_GNU_SOURCE -DHAVE_CLANG_CONFIG_H)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXX_FLAGS} -O0")

# Version of this software
set (NULL_VERSION 1.6.1)

# Setup RPATH for installation
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/nullarihyon/${NULL_VERSION}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Setup precompiled header
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(cotire)
set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 0)

# Add source code
add_subdirectory (src)
add_subdirectory (driver)

enable_testing ()
add_subdirectory (vendor/googletest)
add_subdirectory (unittest)

# Install frontend scripts
install (DIRECTORY frontend/lib frontend/bundle DESTINATION lib/nullarihyon/${NULL_VERSION}/frontend)
# Install analyzer command
install (DIRECTORY ${CLANG_COMPILER_LIB_PATH} DESTINATION lib/nullarihyon/${NULL_VERSION})

# Copy dynamic link libraries
FOREACH (lib_path ${LLVM_LIB_PATHS})
  get_filename_component(lib_path ${lib_path} ABSOLUTE)
  file(GLOB dylibs ${lib_path}/*.dylib)
  install (FILES ${dylibs} DESTINATION lib/nullarihyon/${NULL_VERSION})
ENDFOREACH(lib_path)

# Generate nullarihyon command ruby script
set (INSTALL_LIBEXEC_PREFIX ${CMAKE_INSTALL_PREFIX}/libexec/nullarihyon/${NULL_VERSION})
set (INSTALL_LIB_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/nullarihyon/${NULL_VERSION})
install(CODE "execute_process(COMMAND ruby ${PROJECT_SOURCE_DIR}/frontend/bin/make_frontend.rb ${CMAKE_INSTALL_PREFIX}/bin ${INSTALL_LIBEXEC_PREFIX}/nullarihyon-core ${INSTALL_LIB_PREFIX}/clang ${INSTALL_LIB_PREFIX}/frontend ${NULL_VERSION})")
