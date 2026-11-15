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
