# BuildJanssonUniversal.cmake
# Builds jansson library as a universal binary (arm64 + x86_64) on macOS

include(ExternalProject)

set(JANSSON_VERSION "2.14")
set(JANSSON_URL "https://github.com/akheron/jansson/releases/download/v${JANSSON_VERSION}/jansson-${JANSSON_VERSION}.tar.gz")
set(JANSSON_HASH "5798d010e41cf8d76b66236cfb2f2543c8d082181d16bc3085ab49538d4b9929")

if(APPLE)
  # Build jansson as universal binary for macOS
  ExternalProject_Add(
    jansson_external
    URL ${JANSSON_URL}
    URL_HASH SHA256=${JANSSON_HASH}
    PREFIX ${CMAKE_BINARY_DIR}/external/jansson
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/jansson/install
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_OSX_ARCHITECTURES=arm64$<SEMICOLON>x86_64
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
      -DJANSSON_BUILD_DOCS=OFF
      -DJANSSON_BUILD_SHARED_LIBS=OFF
      -DJANSSON_EXAMPLES=OFF
      -DJANSSON_WITHOUT_TESTS=ON
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config Release
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
  )

  # Set variables for FindJansson to use our built library
  set(JANSSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/external/jansson/install/include PARENT_SCOPE)
  set(JANSSON_LIBRARY ${CMAKE_BINARY_DIR}/external/jansson/install/lib/libjansson.a PARENT_SCOPE)
  set(JANSSON_FOUND TRUE PARENT_SCOPE)

  # Create imported target
  add_library(Jansson::Jansson STATIC IMPORTED GLOBAL)
  set_target_properties(Jansson::Jansson PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/external/jansson/install/lib/libjansson.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/external/jansson/install/include
  )

  # Make sure our plugin depends on jansson being built
  add_dependencies(Jansson::Jansson jansson_external)

  message(STATUS "Building jansson ${JANSSON_VERSION} as universal binary (arm64 + x86_64)")
  message(STATUS "  Include: ${CMAKE_BINARY_DIR}/external/jansson/install/include")
  message(STATUS "  Library: ${CMAKE_BINARY_DIR}/external/jansson/install/lib/libjansson.a")
endif()
