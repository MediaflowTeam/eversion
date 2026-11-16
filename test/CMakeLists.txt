find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  message(FATAL_ERROR
    "GoogleTest was not found via find_package(GTest CONFIG). "
    "Install GoogleTest with your system package manager, such as apt, "
    "or build GoogleTest yourself and pass its package location to "
    "CMake with CMAKE_PREFIX_PATH or GTest_DIR."
  )
endif()
include(GoogleTest)

add_library(eversion_test_consumer INTERFACE)
eversion_declare(eversion_test_consumer
  NAMESPACE eversion_demo::version_info
  MAJOR 1
  MINOR 2
  PATCH 3
  PRERELEASE rc.1
  BUILD_METADATA git.deadbeef
  OUTPUT eversion_demo/version_info.h
  NO_INSTALL
)

add_library(eversion_leading_zero_consumer INTERFACE)
eversion_declare(eversion_leading_zero_consumer
  NAMESPACE eversion_leading_zero::version_info
  MAJOR 010
  MINOR 09
  PATCH 000
  OUTPUT eversion_leading_zero/version_info.h
  NO_INSTALL
)

add_executable(${PROJECT_NAME}_test
  version_test.cc
  version_declare_test.cc
  component_info_test.cc
)

target_link_libraries(${PROJECT_NAME}_test
  PRIVATE
    ${PROJECT_NAME}::${PROJECT_NAME}
    eversion_test_consumer
    eversion_leading_zero_consumer
    GTest::gtest_main
)

eversion_configure_cxx_target(${PROJECT_NAME}_test)

gtest_discover_tests(${PROJECT_NAME}_test)

add_library(eversion_test_plugin MODULE)
eversion_configure_cxx_target(eversion_test_plugin)
set_target_properties(eversion_test_plugin PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/plugins"
)
# Leading zeros are intentional here: they exercise plugin path normalization.
eversion_declare_plugin(eversion_test_plugin
  ID test.plugin
  NAME "EVersion Test Plugin"
  NAMESPACE eversion_test_plugin::version_info
  MAJOR 09
  MINOR 08
  PATCH 07
  PRERELEASE beta.2
  BUILD_METADATA build.42
  OUTPUT eversion_test_plugin/version_info.h
  NO_INSTALL
)

add_executable(eversion_plugin_info_test
  plugin_info_test.cc
)

target_link_libraries(eversion_plugin_info_test
  PRIVATE
    ${PROJECT_NAME}::${PROJECT_NAME}
    GTest::gtest_main
    ${CMAKE_DL_LIBS}
)

eversion_configure_cxx_target(eversion_plugin_info_test)

target_compile_definitions(eversion_plugin_info_test
  PRIVATE
    "EVERSION_TEST_PLUGIN_PATH=\"$<TARGET_FILE:eversion_test_plugin>\""
)

add_dependencies(eversion_plugin_info_test eversion_test_plugin)

gtest_discover_tests(eversion_plugin_info_test)

set(eversion_contract_cmake_args
  "-DEVERSION_CMAKE_GENERATOR=${CMAKE_GENERATOR}"
  "-DEVERSION_CMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
  "-DEVERSION_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
)

add_test(NAME CMakeContract.Declare
  COMMAND "${CMAKE_COMMAND}"
    "-DEVERSION_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
    "-DEVERSION_BUILD_DIR=${PROJECT_BINARY_DIR}"
    ${eversion_contract_cmake_args}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/declare_contracts.cmake"
)

add_test(NAME CMakeContract.InstallGeneratedHeaderChain
  COMMAND "${CMAKE_COMMAND}"
    "-DEVERSION_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
    "-DEVERSION_BUILD_DIR=${PROJECT_BINARY_DIR}"
    ${eversion_contract_cmake_args}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_generated_header_chain.cmake"
)
