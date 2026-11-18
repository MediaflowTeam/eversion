cmake_minimum_required(VERSION 4.2)

foreach(required_var IN ITEMS EVERSION_SOURCE_DIR EVERSION_BUILD_DIR)
  if(NOT DEFINED ${required_var})
    message(FATAL_ERROR "${required_var} is required")
  endif()
endforeach()

set(work_dir "${EVERSION_BUILD_DIR}/cmake_contracts/install_generated_header_chain")
set(eversion_prefix "${work_dir}/eversion-prefix")
set(producer_build "${work_dir}/producer-build")
set(producer_prefix "${work_dir}/producer-prefix")
set(consumer_build "${work_dir}/consumer-build")
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

function(_eversion_check_configure_cache description build_dir)
  load_cache("${build_dir}" READ_WITH_PREFIX cached_
    CMAKE_CXX_COMPILER CMAKE_GENERATOR)

  if(NOT "${EVERSION_CXX_COMPILER}" STREQUAL ""
      AND NOT "${cached_CMAKE_CXX_COMPILER}" STREQUAL
        "${EVERSION_CXX_COMPILER}")
    message(FATAL_ERROR
      "${description} used CMAKE_CXX_COMPILER="
      "'${cached_CMAKE_CXX_COMPILER}', expected "
      "'${EVERSION_CXX_COMPILER}'")
  endif()
  if(NOT "${EVERSION_CMAKE_GENERATOR}" STREQUAL ""
      AND NOT "${cached_CMAKE_GENERATOR}" STREQUAL
        "${EVERSION_CMAKE_GENERATOR}")
    message(FATAL_ERROR
      "${description} used CMAKE_GENERATOR="
      "'${cached_CMAKE_GENERATOR}', expected "
      "'${EVERSION_CMAKE_GENERATOR}'")
  endif()
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

function(_eversion_expect_one_file description glob_expression output_variable)
  file(GLOB_RECURSE matched_files "${glob_expression}")
  list(LENGTH matched_files matched_file_count)
  if(NOT matched_file_count EQUAL 1)
    message(FATAL_ERROR
      "${description}: expected exactly one file matching "
      "'${glob_expression}', found ${matched_file_count}: ${matched_files}")
  endif()
  list(GET matched_files 0 matched_file)
  set("${output_variable}" "${matched_file}" PARENT_SCOPE)
endfunction()

function(_eversion_contract_run description)
  execute_process(
    COMMAND ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR
      "${description} failed with exit code ${result}\n"
      "stdout:\n${output}\n"
      "stderr:\n${error}")
  endif()
endfunction()

_eversion_contract_run("install eversion"
  "${CMAKE_COMMAND}" --install "${EVERSION_BUILD_DIR}"
  --prefix "${eversion_prefix}")

_eversion_contract_run("configure producer"
  "${CMAKE_COMMAND}"
  -S "${EVERSION_SOURCE_DIR}/test/cmake/install_producer"
  -B "${producer_build}"
  ${eversion_configure_args}
  "-DCMAKE_PREFIX_PATH=${eversion_prefix}"
  "-DCMAKE_INSTALL_PREFIX=${producer_prefix}")

_eversion_check_configure_cache("producer" "${producer_build}")

_eversion_contract_run("build producer"
  "${CMAKE_COMMAND}" --build "${producer_build}")

_eversion_contract_run("install producer"
  "${CMAKE_COMMAND}" --install "${producer_build}")

_eversion_expect_one_file("producer installed generated header"
  "${producer_prefix}/*/producer/version_info.h" producer_installed_header)
_eversion_expect_file_line("${producer_installed_header}"
  " * Source: eversion_install_producer/CMakeLists.txt")
file(READ "${producer_installed_header}" producer_header_contents)
foreach(forbidden_text IN ITEMS "${EVERSION_SOURCE_DIR}" "${work_dir}")
  string(FIND "${producer_header_contents}" "${forbidden_text}"
    forbidden_position)
  if(NOT forbidden_position EQUAL -1)
    message(FATAL_ERROR
      "producer installed generated header '${producer_installed_header}' "
      "contains build-machine path '${forbidden_text}'")
  endif()
endforeach()

_eversion_expect_one_file("eversion installed generated header"
  "${eversion_prefix}/*/eversion/version_info.h" eversion_installed_header)
_eversion_expect_file_line("${eversion_installed_header}"
  " * Source: eversion/CMakeLists.txt")
file(READ "${eversion_installed_header}" eversion_header_contents)
string(FIND "${eversion_header_contents}" "${EVERSION_SOURCE_DIR}"
  eversion_source_dir_position)
if(NOT eversion_source_dir_position EQUAL -1)
  message(FATAL_ERROR
    "eversion installed generated header '${eversion_installed_header}' "
    "contains build-machine path '${EVERSION_SOURCE_DIR}'")
endif()

set(prefix_path "${producer_prefix};${eversion_prefix}")
string(REPLACE ";" "\;" prefix_path_arg "${prefix_path}")
_eversion_contract_run("configure final consumer"
  "${CMAKE_COMMAND}"
  -S "${EVERSION_SOURCE_DIR}/test/cmake/install_final_consumer"
  -B "${consumer_build}"
  ${eversion_configure_args}
  "-DCMAKE_PREFIX_PATH=${prefix_path_arg}")

_eversion_check_configure_cache("final consumer" "${consumer_build}")

_eversion_contract_run("build final consumer"
  "${CMAKE_COMMAND}" --build "${consumer_build}")

_eversion_contract_run("run final consumer"
  "${consumer_build}/final_consumer")

set(eversion_prefix_b "${work_dir}/eversion-prefix-b")
_eversion_contract_run("install eversion to second prefix"
  "${CMAKE_COMMAND}" --install "${EVERSION_BUILD_DIR}"
  --prefix "${eversion_prefix_b}")

file(REMOVE_RECURSE "${eversion_prefix}")

file(GLOB eversion_package_dirs
  "${eversion_prefix_b}/lib*/cmake/eversion")
list(LENGTH eversion_package_dirs eversion_package_dir_count)
if(NOT eversion_package_dir_count EQUAL 1)
  message(FATAL_ERROR
    "expected one eversion package directory under '${eversion_prefix_b}', "
    "found ${eversion_package_dir_count}: ${eversion_package_dirs}")
endif()
list(GET eversion_package_dirs 0 eversion_package_dir)

_eversion_contract_run("reconfigure producer with second eversion prefix"
  "${CMAKE_COMMAND}"
  -S "${EVERSION_SOURCE_DIR}/test/cmake/install_producer"
  -B "${producer_build}"
  ${eversion_configure_args}
  "-Deversion_DIR=${eversion_package_dir}")

load_cache("${producer_build}" READ_WITH_PREFIX producer_cached_
  EVERSION_TEMPLATE_DIR)
set(producer_template_dir "${producer_cached_EVERSION_TEMPLATE_DIR}")
string(FIND "${producer_template_dir}" "${eversion_prefix_b}/"
  template_prefix_position)
if(template_prefix_position LESS 0)
  message(FATAL_ERROR
    "producer EVERSION_TEMPLATE_DIR='${producer_template_dir}' is not "
    "under the second eversion prefix '${eversion_prefix_b}'")
endif()

_eversion_contract_run("rebuild producer with second eversion prefix"
  "${CMAKE_COMMAND}" --build "${producer_build}")
