# Test suite for OBS Polyemesis

cmake_minimum_required(VERSION 3.28)

# Add test executable
add_executable(
  obs-polyemesis-tests
  test_main.c
  test_api_client.c
  test_config.c
  test_multistream.c
  mock_restreamer.c
  obs_stubs.c
)

# Link against main project sources (we'll need to adjust the main CMakeLists.txt)
target_sources(
  obs-polyemesis-tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/restreamer-api.c
    ${CMAKE_SOURCE_DIR}/src/restreamer-config.c
    ${CMAKE_SOURCE_DIR}/src/restreamer-multistream.c
)

# Include directories
target_include_directories(
  obs-polyemesis-tests
  PRIVATE ${CMAKE_SOURCE_DIR}/src ${JANSSON_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(obs-polyemesis-tests PRIVATE ${JANSSON_LIBRARIES} CURL::libcurl OBS::libobs)

# Add jansson library directories if available
if(JANSSON_LIBRARY_DIRS)
  target_link_directories(obs-polyemesis-tests PRIVATE ${JANSSON_LIBRARY_DIRS})
endif()

# Platform-specific threading library
if(UNIX)
  target_link_libraries(obs-polyemesis-tests PRIVATE pthread)
endif()

# Add tests with proper executable path handling
# Use TARGET_FILE generator expression to handle multi-config generators (Xcode, Visual Studio)
add_test(NAME api_client_tests COMMAND $<TARGET_FILE:obs-polyemesis-tests> --test-suite=api)
add_test(NAME config_tests COMMAND $<TARGET_FILE:obs-polyemesis-tests> --test-suite=config)
add_test(NAME multistream_tests COMMAND $<TARGET_FILE:obs-polyemesis-tests> --test-suite=multistream)

# Set working directory for tests to ensure they run from the correct location
set_tests_properties(api_client_tests config_tests multistream_tests
  PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Code coverage configuration (optional)
if(ENABLE_COVERAGE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(obs-polyemesis-tests PRIVATE --coverage -O0 -g)
    target_link_options(obs-polyemesis-tests PRIVATE --coverage)
  endif()
endif()

# Memory leak detection with AddressSanitizer (optional)
if(ENABLE_ASAN)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(obs-polyemesis-tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(obs-polyemesis-tests PRIVATE -fsanitize=address)
  endif()
endif()

# Print test configuration
message(STATUS "Test suite configuration:")
message(STATUS "  - Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  - AddressSanitizer: ${ENABLE_ASAN}")
