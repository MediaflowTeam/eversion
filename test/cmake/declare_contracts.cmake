cmake_minimum_required(VERSION 4.2)

if(NOT DEFINED EVERSION_SOURCE_DIR)
  message(FATAL_ERROR "EVERSION_SOURCE_DIR is required")
endif()
if(NOT DEFINED EVERSION_BUILD_DIR)
  message(FATAL_ERROR "EVERSION_BUILD_DIR is required")
endif()

set(work_dir "${EVERSION_BUILD_DIR}/cmake_contracts/declare_contracts")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

set(eversion_configure_args)
if(NOT "${EVERSION_CMAKE_GENERATOR}" STREQUAL "")
  list(APPEND eversion_configure_args -G "${EVERSION_CMAKE_GENERATOR}")
endif()
if(NOT "${EVERSION_CMAKE_MAKE_PROGRAM}" STREQUAL "")
  list(APPEND eversion_configure_args
    "-DCMAKE_MAKE_PROGRAM=${EVERSION_CMAKE_MAKE_PROGRAM}")
endif()
if(NOT "${EVERSION_CXX_COMPILER}" STREQUAL "")
  list(APPEND eversion_configure_args
    "-DCMAKE_CXX_COMPILER=${EVERSION_CXX_COMPILER}")
endif()

function(_eversion_write_project case_name body)
  set(case_dir "${work_dir}/${case_name}")
  file(MAKE_DIRECTORY "${case_dir}")
  file(WRITE "${case_dir}/CMakeLists.txt"
"cmake_minimum_required(VERSION 4.2)\n"
"project(${case_name} LANGUAGES CXX)\n"
"add_subdirectory(\"${EVERSION_SOURCE_DIR}\" eversion EXCLUDE_FROM_ALL)\n"
"${body}\n")
  set(${case_name}_source_dir "${case_dir}" PARENT_SCOPE)
  set(${case_name}_build_dir "${work_dir}/${case_name}-build" PARENT_SCOPE)
endfunction()

function(_eversion_join_body output_var)
  set(body "")
  foreach(part IN LISTS ARGN)
    string(APPEND body "${part}")
  endforeach()
  set("${output_var}" "${body}" PARENT_SCOPE)
endfunction()

function(_eversion_expect_file_line file_path expected_line)
  file(STRINGS "${file_path}" lines)
  foreach(line IN LISTS lines)
    string(REGEX REPLACE "\r$" "" line "${line}")
    if(line STREQUAL expected_line)
      return()
    endif()
  endforeach()
  message(FATAL_ERROR
    "expected line '${expected_line}' was not found in '${file_path}'")
endfunction()

function(_eversion_expect_generated_source_line case_name glob_expression
  expected_line)
  file(GLOB_RECURSE generated_files
    "${${case_name}_build_dir}/${glob_expression}")
  list(LENGTH generated_files generated_file_count)
  if(NOT generated_file_count EQUAL 1)
    message(FATAL_ERROR
      "${case_name}: expected exactly one generated file matching "
      "'${glob_expression}' under '${${case_name}_build_dir}', found "
      "${generated_file_count}: ${generated_files}")
  endif()
  list(GET generated_files 0 generated_file)
  _eversion_expect_file_line("${generated_file}" "${expected_line}")
endfunction()

function(_eversion_expect_configure_failure case_name)
  _eversion_expect_configure_failure_containing("${case_name}" "" ${ARGN})
endfunction()

function(_eversion_expect_configure_failure_containing case_name
  expected_texts)
  _eversion_join_body(body ${ARGN})
  _eversion_write_project("${case_name}" "${body}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${${case_name}_source_dir}"
            -B "${${case_name}_build_dir}"
            ${eversion_configure_args}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(result EQUAL 0)
    message(FATAL_ERROR
      "${case_name} unexpectedly configured successfully\n"
      "stdout:\n${output}\n"
      "stderr:\n${error}")
  endif()

  string(REGEX REPLACE "[ \t\r\n]+" " " normalized_error "${error}")
  foreach(expected_text IN LISTS expected_texts)
    string(REPLACE "<CASE_DIR>" "${${case_name}_source_dir}"
      expected_text_with_case_dir "${expected_text}")
    string(REGEX REPLACE "[ \t\r\n]+" " " normalized_expected
      "${expected_text_with_case_dir}")
    string(FIND "${normalized_error}" "${normalized_expected}" position)
    if(position LESS 0)
      message(FATAL_ERROR
        "${case_name} failed to configure, but stderr does not contain "
        "'${expected_text}'\n"
        "stdout:\n${output}\n"
        "stderr:\n${error}")
    endif()
  endforeach()
endfunction()

function(_eversion_expect_configure_success case_name)
  _eversion_join_body(body ${ARGN})
  _eversion_write_project("${case_name}" "${body}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${${case_name}_source_dir}"
            -B "${${case_name}_build_dir}"
            ${eversion_configure_args}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR
      "${case_name} failed to configure\n"
      "stdout:\n${output}\n"
      "stderr:\n${error}")
  endif()

  set("${case_name}_source_dir" "${${case_name}_source_dir}" PARENT_SCOPE)
  set("${case_name}_build_dir" "${${case_name}_build_dir}" PARENT_SCOPE)
endfunction()

_eversion_expect_configure_failure_containing("duplicate_namespace"
"NAMESPACE='duplicate::version_info';'first';OUTPUT='first/version_info.h';<CASE_DIR>/CMakeLists.txt"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE duplicate::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT first/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE duplicate::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT second/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("duplicate_output"
"OUTPUT='shared/version_info.h';'first';<CASE_DIR>/CMakeLists.txt"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE first::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT shared/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE second::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT shared/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("duplicate_header_guard"
"HEADER_GUARD='FFMPEG_WRAP_VERSION_INFO_H_';'ffmpeg-wrap/version_info.h';'ffmpeg_wrap/version_info.h';'first';<CASE_DIR>/CMakeLists.txt"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE first::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT ffmpeg-wrap/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE second::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT ffmpeg_wrap/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("duplicate_header_guard_case"
"HEADER_GUARD='MEDIA_VERSION_INFO_H_';'Media/version_info.h';'media/version_info.h';'first';<CASE_DIR>/CMakeLists.txt"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE first::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT Media/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE second::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT media/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("reserved_namespace"
"NAMESPACE='eversion::tools';reserved for EVersion"
"add_library(reserved_namespace INTERFACE)\n"
"eversion_declare(reserved_namespace NAMESPACE eversion::tools MAJOR 1 MINOR 0 PATCH 0 OUTPUT reserved_namespace/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("reserved_output"
"OUTPUT='eversion/tools.h';reserved for EVersion"
"add_library(reserved_output INTERFACE)\n"
"eversion_declare(reserved_output NAMESPACE reserved_output::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT eversion/tools.h NO_INSTALL)\n")

_eversion_expect_configure_failure_containing("reserved_header_guard"
"OUTPUT='eversion_version.h';EVERSION_VERSION_H_;reserved for EVersion"
"add_library(reserved_header_guard INTERFACE)\n"
"eversion_declare(reserved_header_guard NAMESPACE reserved_header_guard::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT eversion_version.h NO_INSTALL)\n")

_eversion_expect_configure_success("digit_output"
"add_library(digit_output INTERFACE)\n"
"eversion_declare(digit_output NAMESPACE digit_output::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT 3d/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure("invalid_output"
"add_library(bad INTERFACE)\n"
"eversion_declare(bad NAMESPACE bad::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT ../bad/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure("invalid_namespace"
"add_library(bad INTERFACE)\n"
"eversion_declare(bad NAMESPACE 1bad::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT bad/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_success("no_install"
"add_library(no_install INTERFACE)\n"
"eversion_declare(no_install NAMESPACE no_install::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT no_install/version_info.h NO_INSTALL)\n"
"get_target_property(include_dirs no_install INTERFACE_INCLUDE_DIRECTORIES)\n"
"foreach(include_dir IN LISTS include_dirs)\n"
"  if(include_dir MATCHES \\\"INSTALL_INTERFACE\\\")\n"
"    message(FATAL_ERROR \\\"NO_INSTALL must not add INSTALL_INTERFACE include dirs\\\")\n"
"  endif()\n"
"endforeach()\n")

_eversion_expect_configure_success("leading_zero_versions"
"add_library(leading_zero INTERFACE)\n"
"eversion_declare(leading_zero NAMESPACE leading_zero::version_info MAJOR 010 MINOR 09 PATCH 0000000000000000000007 OUTPUT leading_zero/version_info.h NO_INSTALL)\n"
"get_target_property(version_string leading_zero EVERSION_VERSION_STRING)\n"
"if(NOT version_string STREQUAL \"10.9.7\")\n"
"  message(FATAL_ERROR \"EVERSION_VERSION_STRING was '\${version_string}', expected '10.9.7'\")\n"
"endif()\n")

_eversion_expect_configure_success("maximum_version"
"add_library(maximum_version INTERFACE)\n"
"eversion_declare(maximum_version NAMESPACE maximum_version::version_info MAJOR 4294967295 MINOR 0 PATCH 0 OUTPUT maximum_version/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure("version_number_overflow"
"add_library(version_number_overflow INTERFACE)\n"
"eversion_declare(version_number_overflow NAMESPACE version_number_overflow::version_info MAJOR 4294967296 MINOR 0 PATCH 0 OUTPUT version_number_overflow/version_info.h NO_INSTALL)\n")

set(plugin_overflow_case_dir "${work_dir}/plugin_version_overflow")
file(MAKE_DIRECTORY "${plugin_overflow_case_dir}")
file(WRITE "${plugin_overflow_case_dir}/plugin_overflow.cpp" "")
_eversion_expect_configure_failure("plugin_version_overflow"
"add_library(plugin_overflow MODULE plugin_overflow.cpp)\n"
"eversion_declare_plugin(plugin_overflow ID test.plugin NAME \"Test Plugin\" NAMESPACE plugin_overflow::version_info MAJOR 0 MINOR 4294967296 PATCH 0 OUTPUT plugin_overflow/version_info.h NO_INSTALL)\n")

set(subdirectory_case_name "source_line_from_subdirectory")
set(subdirectory_case_dir "${work_dir}/${subdirectory_case_name}")
file(MAKE_DIRECTORY "${subdirectory_case_dir}/core")
file(WRITE "${subdirectory_case_dir}/core/CMakeLists.txt"
"add_library(core_version_info INTERFACE)\n"
"eversion_declare(core_version_info NAMESPACE source_line_from_subdirectory::core_version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT core/version_info.h NO_INSTALL)\n")
_eversion_expect_configure_success("${subdirectory_case_name}"
"add_subdirectory(core)\n")
_eversion_expect_generated_source_line("${subdirectory_case_name}"
  "core/eversion_generated/*/include/*/version_info.h"
  " * Source: source_line_from_subdirectory/core/CMakeLists.txt")
_eversion_expect_generated_source_line("${subdirectory_case_name}"
  "eversion/eversion_generated/*/include/eversion/version_info.h"
  " * Source: eversion/CMakeLists.txt")

set(outside_case_name "source_line_outside_project")
set(outside_file_dir "${work_dir}/outside")
file(MAKE_DIRECTORY "${outside_file_dir}")
file(WRITE "${outside_file_dir}/declare.cmake"
"add_library(outside_version_info INTERFACE)\n"
"eversion_declare(outside_version_info NAMESPACE source_line_outside_project::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT outside/version_info.h NO_INSTALL)\n")
_eversion_expect_configure_success("${outside_case_name}"
"include(\"${outside_file_dir}/declare.cmake\")\n")
_eversion_expect_generated_source_line("${outside_case_name}"
  "eversion_generated/*/include/outside/version_info.h"
  " * Source: declare.cmake")
