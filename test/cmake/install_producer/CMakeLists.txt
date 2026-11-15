cmake_minimum_required(VERSION 4.2)

project(eversion_install_producer
  VERSION 4.5.6
  LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
find_package(eversion CONFIG REQUIRED)

add_library(producer INTERFACE)
target_link_libraries(producer INTERFACE eversion::eversion)
eversion_declare(producer
  NAMESPACE producer::version_info
  MAJOR ${PROJECT_VERSION_MAJOR}
  MINOR ${PROJECT_VERSION_MINOR}
  PATCH ${PROJECT_VERSION_PATCH}
  OUTPUT producer/version_info.h
)

install(TARGETS producer
  EXPORT ProducerTargets
)
configure_package_config_file(
  ProducerConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/ProducerConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Producer"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ProducerConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ProducerConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ProducerConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Producer"
)
install(EXPORT ProducerTargets
  NAMESPACE producer::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Producer"
)
