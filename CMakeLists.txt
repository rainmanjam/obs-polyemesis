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
# Try to find static library first for better portability
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(JANSSON jansson)
endif()

if(NOT JANSSON_FOUND)
  # Fallback: try to find jansson via find_library
  # Look for static library first, then shared
  find_path(JANSSON_INCLUDE_DIR jansson.h)
  find_library(JANSSON_LIBRARY NAMES libjansson.a jansson)

  if(JANSSON_INCLUDE_DIR AND JANSSON_LIBRARY)
    set(JANSSON_FOUND TRUE)
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
    set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
    message(STATUS "Found jansson: ${JANSSON_LIBRARY}")
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
    # Always use OBS pre-built Qt6 for better compatibility
    # OBS bundles its own Qt, so we should use that instead of Homebrew
    message(STATUS "Using OBS pre-built Qt6 for plugin compatibility")
  endif()

  find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

  # Remove deprecated AGL framework from Qt's link libraries (removed in modern macOS SDKs)
  if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_qt_opengl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(_qt_opengl_libs)
      list(FILTER _qt_opengl_libs EXCLUDE REGEX ".*AGL.*")
      set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_qt_opengl_libs}")
    endif()
  endif()

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

# Platform-specific RPATH configuration
if(APPLE)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
      BUILD_WITH_INSTALL_RPATH FALSE
      INSTALL_RPATH "@loader_path/../Frameworks;@executable_path/../Frameworks"
      INSTALL_RPATH_USE_LINK_PATH FALSE
  )

  # Remove AGL framework completely - it's deprecated and removed from modern macOS SDKs
  # This is a workaround for OBS Studio < 29.1 and Qt < 6.2, which may reference deprecated AGL symbols.
  # Only apply if OBS/Qt versions are known to require it.
  if(TARGET OBS::libobs)
    get_target_property(_obs_version OBS::libobs VERSION)
  endif()
  if(TARGET Qt6::Core)
    get_target_property(_qt_version Qt6::Core VERSION)
  elseif(TARGET Qt5::Core)
    get_target_property(_qt_version Qt5::Core VERSION)
  endif()
  # Set default values if not found
  if(NOT _obs_version)
    set(_obs_version "0.0.0")
  endif()
  if(NOT _qt_version)
    set(_qt_version "0.0.0")
  endif()
  # Apply workaround only for OBS < 29.1 and Qt < 6.2
  if(
    (ENABLE_QT)
    AND (
      ("${_obs_version}" VERSION_LESS "29.1")
      OR ("${_qt_version}" VERSION_LESS "6.2")
    )
  )
    message(WARNING "Applying AGL linker workaround for OBS Studio < 29.1 or Qt < 6.2. This may be fragile; upgrade dependencies if possible.")
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "LINKER:-U,_CGLChoosePixelFormat"  "LINKER:-U,_aglChoosePixelFormat")
  endif()

  # Post-build step to fix library dependencies on macOS
  add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Checking library dependencies for ${CMAKE_PROJECT_NAME}..."
    COMMAND bash -c "otool -L $<TARGET_FILE:${CMAKE_PROJECT_NAME}> || true"
    COMMENT "Displaying library dependencies for verification"
    VERBATIM
  )

elseif(UNIX AND NOT APPLE)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
      BUILD_WITH_INSTALL_RPATH FALSE
      INSTALL_RPATH "$ORIGIN"
      INSTALL_RPATH_USE_LINK_PATH FALSE
  )
endif()

# Add tests (optional, controlled by ENABLE_TESTING option)
option(ENABLE_TESTING "Build test suite" OFF)
if(ENABLE_TESTING)
  enable_testing() # Required for CTest to find tests
  add_subdirectory(tests)
endif()
