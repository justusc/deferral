cmake_minimum_required(VERSION 3.15)

if(PROJECT_NAME)
  set(DEFERRAL_IS_SUBPROJECT TRUE)
endif ()

set(DEFERRAL_VERSION_MAJOR 1)
set(DEFERRAL_VERSION_MINOR 0)
set(DEFERRAL_VERSION_PATCH 0)
set(DEFERRAL_VERSION "${DEFERRAL_VERSION_MAJOR}.${DEFERRAL_VERSION_MINOR}.${DEFERRAL_VERSION_PATCH}")

project(deferral VERSION ${DEFERRAL_VERSION} LANGUAGES CXX)

# Create inferface library
add_library(deferral INTERFACE)
target_include_directories(deferral INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Testing setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(DEFERRAL_IS_SUBPROJECT)
  add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(DEFERRAL_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY "SameMajorVersion"
)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${DEFERRAL_CMAKE_CONFIG_DESTINATION}"
)

# Targets
install(
  TARGETS "deferral"
  EXPORT "${PROJECT_NAME}Targets"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
  EXPORT "${PROJECT_NAME}Targets"
  NAMESPACE "${PROJECT_NAME}::"
  DESTINATION "${DEFERRAL_CMAKE_CONFIG_DESTINATION}"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION "${DEFERRAL_CMAKE_CONFIG_DESTINATION}"
)

# Install the header file
install(FILES include/deferral.hh DESTINATION include)
