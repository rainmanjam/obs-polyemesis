# Qt Unit Tests for OBS Polyemesis

## Overview

This directory contains Qt Test Framework-based unit tests for UI components and business logic.

## Current Status

Qt Test Framework is **configured but not fully implemented** due to architectural constraints:

- Plugin code is tightly coupled to OBS callbacks
- ProfileManager and RestreamerDock are not separated into testable classes
- Refactoring required to enable comprehensive UI testing

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
