cmake_minimum_required(VERSION 4.2)

project(eversion_install_final_consumer LANGUAGES CXX)

find_package(Producer CONFIG REQUIRED)

add_executable(final_consumer
  main.cc
)
target_link_libraries(final_consumer PRIVATE producer::producer)
target_compile_features(final_consumer PRIVATE cxx_std_20)
set_target_properties(final_consumer PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
