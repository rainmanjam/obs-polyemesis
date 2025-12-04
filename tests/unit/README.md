# Unit Tests for OBS Polyemesis

## Overview

This directory contains comprehensive unit tests for the OBS Polyemesis plugin:
- **C-based tests**: Channel management module tests
- **Qt-based tests**: UI component and validation tests

## C-Based Tests

### Extended Channel Management Tests (`test_channel_extended.c`)

Comprehensive unit tests for the channel management module covering all critical functionality.

**Total: 21 comprehensive unit tests**

#### Channel Lifecycle Tests (4 tests)
- `test_channel_creation_valid`: Tests creating channels with valid inputs
- `test_channel_creation_invalid`: Tests error handling with NULL/invalid inputs
- `test_channel_deletion`: Tests channel deletion and cleanup
- `test_channel_duplication`: Tests channel duplication with all properties

#### Output Management Tests (4 tests)
- `test_output_addition`: Tests adding outputs to channels with various encoding settings
- `test_output_removal`: Tests removing outputs and array management
- `test_output_enable_disable`: Tests enabling/disabling individual outputs
- `test_bulk_output_operations`: Tests bulk enable/disable, encoding updates, and deletions

#### Start/Stop/Restart Tests (4 tests)
- `test_channel_start_cleanup`: Tests that starting a channel deletes existing process first (critical fix)
- `test_channel_stop_cleanup`: Tests proper cleanup on stop (process reference, error state)
- `test_channel_restart`: Tests restart functionality
- `test_start_stop_all_channels`: Tests batch operations on multiple channels

#### Persistence Tests (4 tests)
- `test_save_channels`: Tests saving channels to OBS settings
- `test_load_channels`: Tests loading channels from settings
- `test_load_missing_settings`: Tests handling of empty/NULL settings
- `test_load_corrupt_settings`: Tests handling of malformed settings data

#### Failover Tests (5 tests)
- `test_backup_configuration`: Tests configuring backup outputs
- `test_remove_backup`: Tests removing backup relationships
- `test_failover_trigger`: Tests manual failover triggering
- `test_primary_restoration`: Tests restoring primary after failover
- Tests automatic failover based on health monitoring

### Building and Running C Tests

```bash
# Build the test executable
cd build
cmake ..
make test_channel_extended

# Run with CTest
ctest -R channel_extended_tests -V

# Or run directly
./test_channel_extended

# Run with sanitizers (detect memory leaks)
cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
make test_channel_extended
./test_channel_extended
```

## Qt-Based Tests

### Current Status

Qt Test Framework is **configured but not fully implemented** due to architectural constraints:

- Plugin code is tightly coupled to OBS callbacks
- ProfileManager and RestreamerDock are not separated into testable classes
- Refactoring required to enable comprehensive UI testing

### Available Qt Tests

The following Qt-based tests are functional:

- `test_url_validation.cpp`: RTMP/SRT URL validation
- `test_profile_validation.cpp`: Profile validation logic
- `test_collapsible_section.cpp`: UI collapsible section widget
- `test_destination_management.cpp`: Destination management logic
- `test_process_id_generation.cpp`: Process ID generation tests
- `test_ui_integration.cpp`: UI integration tests

## Recommended Refactoring

To enable full Qt Test coverage:

1. **Extract Business Logic**
   ```cpp
   // Current: Everything in profile-manager.c
   // Proposed: Separate data model from UI

   class ProfileModel {
   public:
       bool createProfile(const QString& name);
       bool deleteProfile(const QString& name);
       QList<Profile> getProfiles() const;
   };
   ```

2. **Dependency Injection**
   ```cpp
   // Make components testable by injecting dependencies
   class RestreamerClient {
   public:
       virtual bool connect(const QString& url);
       virtual bool createProcess(...);
   };

   class MockRestreamerClient : public RestreamerClient {
       // Test implementation
   };
   ```

3. **Separate UI from Logic**
   ```cpp
   // UI layer (dock.cpp)
   class RestreamerDock : public QDockWidget {
       ProfileModel* model;
   };

   // Testable model (profile-model.cpp)
   class ProfileModel {
       // No Qt UI dependencies
   };
   ```

## Running Tests (When Implemented)

```bash
# Configure with testing enabled
cmake -S . -B build -DENABLE_TESTING=ON

# Build tests
cmake --build build --target all

# Run all tests
cd build
ctest --output-on-failure

# Run specific test
ctest -R test_profile_manager --verbose
```

## Adding New Tests

1. Create test file (e.g., `test_my_feature.cpp`)
2. Include Qt Test headers:
   ```cpp
   #include <QtTest/QtTest>
   ```

3. Define test class:
   ```cpp
   class TestMyFeature : public QObject {
       Q_OBJECT
   private slots:
       void testSomething() {
           QCOMPARE(1 + 1, 2);
       }
   };
   ```

4. Add to CMakeLists.txt:
   ```cmake
   add_obs_test(test_my_feature test_my_feature.cpp)
   ```

5. Include MOC and main:
   ```cpp
   #include "test_my_feature.moc"
   QTEST_MAIN(TestMyFeature)
   ```

## Test Coverage Goals

With proper refactoring, Qt Tests could cover:

- **Profile Management** (5 tests)
  - Create/delete/duplicate profiles
  - Add/remove destinations
  - Profile name validation

- **UI State Management** (3 tests)
  - Button enable/disable logic
  - Profile selection handling
  - Error message display

- **Data Validation** (4 tests)
  - URL validation
  - Stream key validation
  - Process ID generation
  - Configuration persistence

**Potential Coverage**: +10% automated tests (5 additional tests from AUTOMATION_ANALYSIS.md)

## Resources

- [Qt Test Tutorial](https://doc.qt.io/qt-6/qtest-tutorial.html)
- [Qt Test Overview](https://doc.qt.io/qt-6/qtest-overview.html)
- [QTest Class Reference](https://doc.qt.io/qt-6/qtest.html)
