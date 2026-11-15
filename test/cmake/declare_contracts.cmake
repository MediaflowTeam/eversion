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

function(_eversion_expect_configure_failure case_name)
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
endfunction()

_eversion_expect_configure_failure("duplicate_namespace"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE duplicate::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT first/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE duplicate::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT second/version_info.h NO_INSTALL)\n")

_eversion_expect_configure_failure("duplicate_output"
"add_library(first INTERFACE)\n"
"eversion_declare(first NAMESPACE first::version_info MAJOR 1 MINOR 0 PATCH 0 OUTPUT shared/version_info.h NO_INSTALL)\n"
"add_library(second INTERFACE)\n"
"eversion_declare(second NAMESPACE second::version_info MAJOR 2 MINOR 0 PATCH 0 OUTPUT shared/version_info.h NO_INSTALL)\n")

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
