cmake_minimum_required(VERSION 4.2)

project(eversion
  VERSION 0.1.0
  DESCRIPTION "Easy macro-free version metadata helpers for C++20"
  LANGUAGES CXX
)

# EVersion does not use C++20 modules. Module scanning would add GCC-only
# flags to compile_commands.json, which clang-tidy rejects.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(cmake/eversionOptions.cmake)
include(cmake/eversionCompiler.cmake)
include(cmake/eversionDocs.cmake)
include(cmake/eversionInstall.cmake)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

eversion_configure_cxx_interface_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set(EVERSION_TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates"
  CACHE INTERNAL "Path to EVersion generated-header templates")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/eversionDeclare.cmake")

set(eversion_declare_args
  NAMESPACE eversion::version_info
  MAJOR ${PROJECT_VERSION_MAJOR}
  MINOR ${PROJECT_VERSION_MINOR}
  PATCH ${PROJECT_VERSION_PATCH}
  OUTPUT eversion/version_info.h
)
if(NOT ${PROJECT_NAME}_INSTALL)
  list(APPEND eversion_declare_args NO_INSTALL)
endif()
eversion_declare(${PROJECT_NAME} ${eversion_declare_args})

if(${PROJECT_NAME}_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(${PROJECT_NAME}_INSTALL)
  eversion_install_package(${PROJECT_NAME})
endif()

if(${PROJECT_NAME}_BUILD_DOCS)
  eversion_add_docs_target()
endif()
