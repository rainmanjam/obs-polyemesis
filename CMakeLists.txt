cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# Find CURL - handle cross-platform
find_package(CURL REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE CURL::libcurl)

# Find jansson for JSON parsing - cross-platform approach
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(JANSSON jansson)
endif()

if(NOT JANSSON_FOUND)
  # Fallback: try to find jansson via find_library
  find_path(JANSSON_INCLUDE_DIR jansson.h)
  find_library(JANSSON_LIBRARY NAMES jansson)

  if(JANSSON_INCLUDE_DIR AND JANSSON_LIBRARY)
    set(JANSSON_FOUND TRUE)
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
    set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
  endif()
endif()

if(JANSSON_FOUND)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${JANSSON_LIBRARIES})
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${JANSSON_INCLUDE_DIRS})
  if(JANSSON_LIBRARY_DIRS)
    target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${JANSSON_LIBRARY_DIRS})
  endif()
else()
  message(FATAL_ERROR "jansson library not found. Please install jansson.")
endif()

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  # Platform-specific Qt6 handling
  if(APPLE)
    # On macOS, use Homebrew Qt to avoid deprecated AGL framework in pre-built deps
    # The pre-built OBS Qt6 has AGL framework issues on modern macOS
    if(EXISTS "/opt/homebrew/opt/qt")
      set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" ${CMAKE_PREFIX_PATH})
      message(STATUS "Using Homebrew Qt6 to avoid AGL framework issues")
    endif()
  endif()

  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)

  # Platform-specific compiler options
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
      -Wno-comma>
      $<$<C_COMPILER_ID:MSVC>:/wd4996>
  )

  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.c
    src/restreamer-api.c
    src/restreamer-api.h
    src/restreamer-config.c
    src/restreamer-config.h
    src/restreamer-source.c
    src/restreamer-output.c
    src/restreamer-multistream.c
    src/restreamer-multistream.h
    src/restreamer-output-profile.c
    src/restreamer-output-profile.h
)

if(ENABLE_QT)
  target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE src/restreamer-dock.cpp src/restreamer-dock.h src/restreamer-dock-bridge.cpp
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Add tests (optional, controlled by ENABLE_TESTING option)
option(ENABLE_TESTING "Build test suite" OFF)
if(ENABLE_TESTING)
  enable_testing() # Required for CTest to find tests
  add_subdirectory(tests)
endif()
