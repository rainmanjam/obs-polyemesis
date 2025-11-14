# Qt Test Framework for OBS Polyemesis
# Automated UI component testing

cmake_minimum_required(VERSION 3.22)

project(obs-polyemesis-tests)

# Find Qt6 Test module
find_package(Qt6 COMPONENTS Core Test Widgets QUIET)

if(NOT Qt6_FOUND)
    message(WARNING "Qt6 Test module not found - skipping unit tests")
    return()
endif()

# Enable testing
enable_testing()

# Enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include parent source directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
)

# Helper function to add a test
function(add_obs_test test_name source_file)
    add_executable(${test_name} ${source_file})

    target_link_libraries(${test_name}
        PRIVATE
            Qt6::Core
            Qt6::Test
            Qt6::Widgets
    )

    add_test(NAME ${test_name} COMMAND ${test_name})

    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )
endfunction()

# Add individual test files
# add_obs_test(test_profile_manager test_profile_manager.cpp)
# add_obs_test(test_restreamer_dock test_restreamer_dock.cpp)

message(STATUS "Qt Test Framework configured for OBS Polyemesis")
message(STATUS "  To add tests, uncomment test definitions above")
message(STATUS "  Run tests with: ctest --output-on-failure")
