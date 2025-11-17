# OBS Polyemesis - End-to-End Testing Framework

## Overview

This directory contains comprehensive end-to-end (E2E) tests that verify the complete plugin lifecycle on all supported platforms:

1. **Build** - Compile the plugin from source
2. **Install** - Install plugin to OBS directories
3. **Load** - Verify OBS loads the plugin successfully
4. **Functionality** - Test all plugin features
5. **Integration** - Test Restreamer connectivity
6. **Cleanup** - Remove test artifacts

## Test Coverage

### Platform Tests

- **macOS** (arm64, x86_64, universal)
- **Linux** (Ubuntu, Debian, Fedora)
- **Windows** (x64)

### Functionality Tests

1. **Plugin Loading**
   - Binary compatibility
   - Library dependencies
   - OBS module registration
   - Code signing verification

2. **UI Components**
   - Restreamer Control dock
   - Source properties dialog
   - Output settings dialog
   - Configuration persistence

3. **Source Functionality**
   - Restreamer source creation
   - Stream URL validation
   - Video/audio playback
   - Error handling

4. **Output Functionality**
   - Restreamer output creation
   - Multi-destination streaming
   - Connection management
   - Stream state monitoring

5. **API Integration**
   - Restreamer API connectivity
   - Process CRUD operations
   - Authentication
   - Error handling

## Test Structure

```
tests/e2e/
├── README.md                     # This file
├── common/
│   ├── test-framework.sh         # Shared test utilities
│   ├── obs-launcher.sh           # OBS automation helpers
│   └── assertions.sh             # Test assertion functions
├── macos/
│   ├── e2e-test-macos.sh        # macOS E2E test suite
│   ├── install-test.sh           # Installation verification
│   └── uninstall-test.sh         # Cleanup verification
├── linux/
│   ├── e2e-test-linux.sh        # Linux E2E test suite
│   ├── install-test.sh           # Installation verification
│   └── uninstall-test.sh         # Cleanup verification
├── windows/
│   ├── e2e-test-windows.ps1     # Windows E2E test suite
│   ├── install-test.ps1          # Installation verification
│   └── uninstall-test.ps1        # Cleanup verification
└── run-all-e2e-tests.sh          # Multi-platform test runner
```

## Quick Start

### Run All Platform Tests

```bash
cd tests/e2e
./run-all-e2e-tests.sh
```

### Run Platform-Specific Tests

**macOS:**
```bash
cd tests/e2e/macos
./e2e-test-macos.sh
```

**Linux:**
```bash
cd tests/e2e/linux
./e2e-test-linux.sh
```

**Windows:**
```powershell
cd tests\e2e\windows
.\e2e-test-windows.ps1
```

## Test Stages

### Stage 1: Build Verification
- Compile plugin from source
- Verify binary output
- Check architecture compatibility
- Validate dependencies

### Stage 2: Installation
- Create required directories
- Copy plugin files
- Set permissions
- Verify installation paths

### Stage 3: Plugin Loading
- Launch OBS (headless or with display)
- Check plugin registration
- Verify no load errors
- Validate module exports

### Stage 4: UI Testing
- Verify dock registration
- Check source availability
- Validate output registration
- Test configuration dialogs

### Stage 5: Functional Testing
- Create Restreamer source
- Configure streaming settings
- Test connection to Restreamer
- Verify stream state management

### Stage 6: Integration Testing
- Full Restreamer API workflow
- Multi-destination streaming
- Process lifecycle management
- Error recovery

### Stage 7: Cleanup
- Stop OBS gracefully
- Remove test artifacts
- Uninstall plugin
- Restore system state

## Requirements

### macOS
- macOS 11.0 or higher
- OBS Studio 28.0 or higher
- Xcode Command Line Tools
- CMake 3.16+

### Linux
- Ubuntu 20.04+ / Debian 11+ / Fedora 35+
- OBS Studio 28.0 or higher
- Build essentials
- CMake 3.16+

### Windows
- Windows 10/11
- OBS Studio 28.0 or higher
- Visual Studio 2019+ or Build Tools
- CMake 3.16+

## Test Execution Modes

### 1. Full E2E Test
Runs complete lifecycle: build → install → test → uninstall
```bash
./e2e-test-macos.sh --full
```

### 2. Quick Test
Assumes plugin is already built
```bash
./e2e-test-macos.sh --quick
```

### 3. Install Only
Just tests installation
```bash
./e2e-test-macos.sh --install-only
```

### 4. CI Mode
Optimized for continuous integration
```bash
./e2e-test-macos.sh --ci
```

## Configuration

### Environment Variables

```bash
# OBS installation path (auto-detected if not set)
export OBS_INSTALL_PATH="/Applications/OBS.app"

# Plugin installation directory (auto-detected if not set)
export OBS_PLUGIN_PATH="$HOME/Library/Application Support/obs-studio/plugins"

# Test workspace
export E2E_WORKSPACE="/tmp/obs-polyemesis-e2e"

# Restreamer test instance
export RESTREAMER_URL="http://localhost:8080"
export RESTREAMER_USER="admin"
export RESTREAMER_PASS="admin"

# Test verbosity (0=quiet, 1=normal, 2=verbose)
export E2E_VERBOSITY=1

# Keep artifacts on failure
export E2E_KEEP_ON_FAILURE=true
```

## Test Output

### Success Output
```
========================================
 OBS Polyemesis - End-to-End Tests
========================================

Platform: macOS (arm64)
OBS Version: 30.0.0
Date: 2025-11-16

[STAGE 1/7] Build Verification
  ✓ Plugin binary exists
  ✓ Architecture: arm64, x86_64
  ✓ Dependencies linked

[STAGE 2/7] Installation
  ✓ Plugin directory created
  ✓ Files copied successfully
  ✓ Permissions set correctly

[STAGE 3/7] Plugin Loading
  ✓ OBS launched successfully
  ✓ Plugin loaded without errors
  ✓ Module registered: obs-polyemesis

[STAGE 4/7] UI Testing
  ✓ Restreamer Control dock registered
  ✓ Restreamer source available
  ✓ Restreamer output available

[STAGE 5/7] Functional Testing
  ✓ Source creation works
  ✓ Output configuration works
  ✓ Connection to Restreamer successful

[STAGE 6/7] Integration Testing
  ✓ API authentication works
  ✓ Process creation successful
  ✓ Multi-destination streaming works

[STAGE 7/7] Cleanup
  ✓ OBS stopped gracefully
  ✓ Plugin uninstalled
  ✓ Workspace cleaned

========================================
 Test Summary
========================================

Total Stages: 7
Passed: 7
Failed: 0
Duration: 45.2 seconds

✅ All end-to-end tests passed!
```

## Troubleshooting

### OBS Won't Launch
```bash
# Check OBS installation
which obs
ls -la /Applications/OBS.app

# Try launching manually
/Applications/OBS.app/Contents/MacOS/OBS --verbose
```

### Plugin Won't Load
```bash
# Check plugin binary
file build/Release/obs-polyemesis.plugin/Contents/MacOS/obs-polyemesis

# Check dependencies
otool -L build/Release/obs-polyemesis.plugin/Contents/MacOS/obs-polyemesis

# Check OBS logs
tail -f "$HOME/Library/Application Support/obs-studio/logs/"*.txt
```

### Tests Timeout
```bash
# Increase timeout
export E2E_TIMEOUT=120

# Run with verbose output
export E2E_VERBOSITY=2
./e2e-test-macos.sh
```

## CI/CD Integration

### GitHub Actions

```yaml
name: End-to-End Tests

on: [push, pull_request]

jobs:
  e2e-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install OBS
        run: brew install obs
      - name: Run E2E Tests
        run: |
          cd tests/e2e/macos
          ./e2e-test-macos.sh --ci

  e2e-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install OBS
        run: sudo apt-get install obs-studio
      - name: Run E2E Tests
        run: |
          cd tests/e2e/linux
          ./e2e-test-linux.sh --ci

  e2e-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install OBS
        run: choco install obs-studio
      - name: Run E2E Tests
        run: |
          cd tests/e2e/windows
          .\e2e-test-windows.ps1 -CI
```

## Best Practices

1. **Always run in clean environment** - Use fresh OBS install or VM
2. **Check prerequisites** - Verify OBS and dependencies before running
3. **Review logs on failure** - Check OBS logs for detailed error information
4. **Use CI mode in automation** - Optimized for headless execution
5. **Keep artifacts on failure** - Set `E2E_KEEP_ON_FAILURE=true` for debugging

## Related Documentation

- [Integration Tests](../README_INTEGRATION_TESTS.md) - Restreamer API integration tests
- [Plugin Tests](../README_PLUGIN_TESTS.md) - Plugin loading and UI tests
- [Windows Testing](../README_WINDOWS_TESTING.md) - Windows-specific testing guide
