cmake_minimum_required(VERSION 3.20)
project(common LANGUAGES CXX)

add_library(common INTERFACE)
add_library(common::common ALIAS common)

target_include_directories(
  common INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(common INTERFACE cxx_std_23)

# for FetchContent
include(GNUInstallDirs)
install(
  TARGETS common
  EXPORT commonTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT commonTargets
  FILE commonTargets.cmake
  NAMESPACE common::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
