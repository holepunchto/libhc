cmake_minimum_required(VERSION 3.25)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(hc C)

add_library(hc OBJECT)

set_target_properties(
  hc
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  hc
  INTERFACE
    include/hc.h
  PRIVATE
)

target_include_directories(
  hc
  PUBLIC
    include
)

add_library(hc_shared SHARED)

set_target_properties(
  hc_shared
  PROPERTIES
  OUTPUT_NAME hc
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  hc_shared
  PUBLIC
    hc
)

add_library(hc_static STATIC)

set_target_properties(
  hc_static
  PROPERTIES
  OUTPUT_NAME hc
  PREFIX lib
)

target_link_libraries(
  hc_static
  PUBLIC
    hc
)

install(TARGETS hc_shared hc_static)

install(FILES include/hc.h DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
