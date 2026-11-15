cmake_minimum_required(VERSION 4.2)

project(version_report VERSION 1.4.2 LANGUAGES CXX)

# The example does not use C++20 modules. Module scanning would add GCC-only
# flags to compile_commands.json, which clang-tidy rejects.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(version_report_WARNINGS_AS_ERRORS "Treat version_report compiler warnings as errors" OFF)

find_package(eversion CONFIG REQUIRED)

add_executable(version_report_cli
  src/main.cc
)

target_link_libraries(version_report_cli
  PRIVATE
    eversion::eversion
    ${CMAKE_DL_LIBS}
)

eversion_declare(version_report_cli
  NAMESPACE version_report::version_info
  MAJOR ${PROJECT_VERSION_MAJOR}
  MINOR ${PROJECT_VERSION_MINOR}
  PATCH ${PROJECT_VERSION_PATCH}
  BUILD_METADATA example.1
  OUTPUT version_report/version_info.h
  NO_INSTALL
)

set_target_properties(version_report_cli PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

# The example builds against the installed package only, so it sets its own
# warning options instead of using the library's CMake modules.
target_compile_options(version_report_cli PRIVATE -Wall -Wextra -Wpedantic)
if(version_report_WARNINGS_AS_ERRORS)
  target_compile_options(version_report_cli PRIVATE -Werror)
endif()

add_library(version_report_plugin MODULE)

eversion_declare_plugin(version_report_plugin
  ID example.report.plugin
  NAME "Version Report Plugin"
  NAMESPACE version_report_plugin::version_info
  MAJOR 2
  MINOR 0
  PATCH 1
  PRERELEASE beta.1
  OUTPUT version_report_plugin/version_info.h
  NO_INSTALL
)

set_target_properties(version_report_plugin PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(version_report_plugin PRIVATE -Wall -Wextra -Wpedantic)
if(version_report_WARNINGS_AS_ERRORS)
  target_compile_options(version_report_plugin PRIVATE -Werror)
endif()

enable_testing()

add_test(NAME version_report_cli COMMAND version_report_cli $<TARGET_FILE:version_report_plugin>)
set_tests_properties(version_report_cli PROPERTIES
  PASS_REGULAR_EXPRESSION "version_report 1\\.4\\.2\\+example\\.1[\n]+eversion [0-9]+\\.[0-9]+\\.[0-9]+[\n]+plugin example\\.report\\.plugin \\(Version Report Plugin\\) 2\\.0\\.1-beta\\.1"
)
