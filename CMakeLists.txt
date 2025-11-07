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

# Find CURL
find_package(CURL REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE CURL::libcurl)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

# Find jansson for JSON parsing
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(JANSSON REQUIRED jansson)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${JANSSON_LIBRARIES})
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${JANSSON_INCLUDE_DIRS})
endif()

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.c
          src/restreamer-api.c
          src/restreamer-api.h
          src/restreamer-config.c
          src/restreamer-config.h
          src/restreamer-source.c
          src/restreamer-output.c
          src/restreamer-multistream.c
          src/restreamer-multistream.h)

if(ENABLE_QT)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/restreamer-dock.cpp src/restreamer-dock.h src/restreamer-dock-bridge.cpp)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
