# OBS Polyemesis - Comprehensive Local Testing Plan

## Overview
This document outlines comprehensive local testing procedures for OBS Polyemesis across macOS, Linux, and Windows using Docker and act.

## Table of Contents
1. [Test Categories](#test-categories)
2. [Platform Coverage](#platform-coverage)
3. [Test Scenarios](#test-scenarios)
4. [Execution Instructions](#execution-instructions)
5. [Restreamer Integration Tests](#restreamer-integration-tests)

---

## Test Categories

### 1. Build Tests
- ✅ CMake configuration
- ✅ Compilation (Debug + Release)
- ✅ Binary architecture verification (arm64/x86_64)
- ✅ Dependency linking (OBS, libcurl, jansson)
- ✅ Plugin bundle structure
- ✅ Code signing verification

### 2. Unit Tests
- ✅ Profile validation
- ✅ Destination management
- ✅ URL validation
- ✅ Process ID generation
- ✅ UI widget functionality
- ✅ Configuration file handling

### 3. Integration Tests
- 🔄 Restreamer API connectivity
- 🔄 Process creation and management
- 🔄 Streaming session lifecycle
- 🔄 Error handling and recovery
- 🔄 Authentication (HTTP Basic Auth)
- 🔄 SSL/TLS connectivity

### 4. End-to-End Tests
- 🔄 Plugin installation
- 🔄 OBS loading and initialization
- 🔄 UI component registration
- 🔄 Profile creation and management
- 🔄 Stream start/stop operations
- 🔄 Real streaming to Restreamer server

### 5. Platform-Specific Tests
**macOS:**
- Bundle structure (.plugin format)
- Info.plist validation
- Framework linking (@rpath resolution)
- Keychain integration (future)

**Linux:**
- Shared library (.so) loading
- System dependencies (apt/yum packages)
- AppImage compatibility
- Wayland/X11 compatibility

**Windows:**
- DLL loading and dependencies
- Registry integration (if any)
- Windows Defender compatibility
- Visual C++ redistributable requirements

### 6. Security Tests
- ✅ XSS vulnerability scanning
- ✅ Code analysis (Bearer, Semgrep, SonarCloud)
- ✅ Dependency vulnerability scanning (Snyk, Grype, Trivy)
- ✅ Secret scanning (Gitleaks)
- 🔄 Input validation tests
- 🔄 Credential storage security

### 7. Performance Tests
- 🔄 Memory leak detection (Valgrind)
- 🔄 CPU usage profiling
- 🔄 Network bandwidth monitoring
- 🔄 Concurrent stream handling
- 🔄 Long-running stability tests

---

## Platform Coverage

### macOS (Native + act)
```bash
# Native macOS tests
./scripts/macos-test.sh

# E2E tests
./tests/e2e/macos/e2e-test-macos.sh

# Plugin installation test
./tests/test-plugin-automated.sh
```

### Linux (Docker + act)
```bash
# Build and test via act
act -W .github/workflows/automated-tests.yml -j build-linux

# Linux-specific E2E
./tests/e2e/linux/e2e-test-linux.sh

# Docker-based unit tests
./scripts/test-linux-docker.sh
```

### Windows (Remote + act)
```bash
# Remote Windows tests (requires Windows machine with SSH)
./scripts/windows-test.sh

# Or use act with Windows containers (experimental)
act -W .github/workflows/automated-tests.yml -j build-windows
```

### All Platforms
```bash
# Run all platform tests sequentially
./scripts/test-all-platforms.sh

# Skip specific platforms
./scripts/test-all-platforms.sh --skip-windows

# With verbose output
./scripts/test-all-platforms.sh -v
```

---

## Test Scenarios

### Scenario 1: Vertical Streaming Test
**Objective:** Verify plugin can handle vertical video (9:16) streaming

**Test Steps:**
1. Create profile with auto-orientation detection enabled
2. Add vertical video source (1080x1920 or 720x1280)
3. Configure Restreamer destination
4. Start streaming
5. Verify video orientation is detected correctly
6. Verify Restreamer receives correct resolution

**Expected Results:**
- Auto-detection identifies vertical orientation
- Stream metadata shows correct resolution
- Restreamer process uses correct encoding parameters
- Video plays correctly in portrait mode

**Test Script:**
```bash
./tests/scenarios/test-vertical-streaming.sh
```

### Scenario 2: Horizontal Streaming Test
**Objective:** Verify plugin can handle horizontal video (16:9) streaming

**Test Steps:**
1. Create profile with auto-orientation detection enabled
2. Add horizontal video source (1920x1080 or 1280x720)
3. Configure Restreamer destination
4. Start streaming
5. Verify video orientation is detected correctly
6. Verify Restreamer receives correct resolution

**Expected Results:**
- Auto-detection identifies horizontal orientation
- Stream metadata shows correct resolution
- Restreamer process uses correct encoding parameters
- Video plays correctly in landscape mode

**Test Script:**
```bash
./tests/scenarios/test-horizontal-streaming.sh
```

### Scenario 3: Multi-Destination Streaming
**Objective:** Verify plugin can stream to multiple Restreamer destinations simultaneously

**Test Steps:**
1. Create single profile
2. Add 3+ destinations with different resolutions/bitrates
3. Start streaming
4. Monitor all streams
5. Verify no dropped frames
6. Stop streams individually and together

**Expected Results:**
- All streams start successfully
- Independent stream control works
- No resource exhaustion
- Graceful shutdown of all streams

**Test Script:**
```bash
./tests/scenarios/test-multi-destination.sh
```

### Scenario 4: Reconnection and Error Recovery
**Objective:** Verify plugin handles network interruptions and errors gracefully

**Test Steps:**
1. Start streaming to Restreamer
2. Simulate network interruption (firewall rule)
3. Verify auto-reconnect attempts
4. Restore network
5. Verify stream resumes
6. Test invalid credentials
7. Test invalid server URL

**Expected Results:**
- Reconnection attempts logged
- UI shows connection status
- Stream resumes after network restore
- Appropriate error messages for invalid config

**Test Script:**
```bash
./tests/scenarios/test-error-recovery.sh
```

### Scenario 5: Profile Import/Export
**Objective:** Verify profile configuration can be saved and restored

**Test Steps:**
1. Create profile with multiple destinations
2. Configure custom settings
3. Export profile to JSON
4. Delete profile
5. Import from JSON
6. Verify all settings restored

**Expected Results:**
- Export produces valid JSON
- Import recreates exact configuration
- No data loss in round-trip

**Test Script:**
```bash
./tests/scenarios/test-profile-import-export.sh
```

### Scenario 6: Cross-Platform Installation
**Objective:** Verify plugin installs and works on all supported platforms

**Test Steps (macOS):**
1. Build plugin bundle
2. Install to `~/Library/Application Support/obs-studio/plugins/`
3. Launch OBS
4. Verify plugin in View → Docks menu
5. Create test profile
6. Verify functionality

**Test Steps (Linux):**
1. Build .so plugin
2. Install to `~/.config/obs-studio/plugins/` or `/usr/share/obs/obs-plugins/`
3. Launch OBS
4. Verify plugin loads
5. Test functionality

**Test Steps (Windows):**
1. Build .dll plugin
2. Install to `C:\Program Files\obs-studio\obs-plugins\64bit\`
3. Launch OBS
4. Verify plugin loads
5. Test functionality

**Expected Results:**
- Plugin installs without errors
- OBS recognizes plugin on all platforms
- UI renders correctly
- Core functionality works identically

**Test Script:**
```bash
./tests/scenarios/test-cross-platform-install.sh
```

---

## Restreamer Integration Tests

### Prerequisites
- Datarhei Restreamer server running and accessible
- Valid credentials (username/password)
- Network connectivity to server

### Configuration
```json
{
  "host": "rs.rainmanjam.com",
  "port": 443,
  "use_https": true,
  "username": "admin",
  "password": "your-password-here"
}
```

### Test 1: Connection Test
**Objective:** Verify plugin can connect to Restreamer server

```bash
# Test connection settings
./test-connection-settings.sh

# Expected: 200 OK response from /api/v3/process
# Expected: Authentication successful
```

### Test 2: Process Creation
**Objective:** Verify plugin can create Restreamer processes

**API Endpoint:** `POST /api/v3/process`

**Test Steps:**
1. Configure profile with valid destination
2. Start streaming
3. Verify process created on Restreamer
4. Check process status via API
5. Stop streaming
6. Verify process removed

**Verification:**
```bash
# Check process exists
curl -u admin:password https://rs.rainmanjam.com/api/v3/process/{id}

# Should return process with status "running"
```

### Test 3: RTMP Ingest
**Objective:** Verify Restreamer can receive RTMP stream from OBS

**Test Steps:**
1. Create profile with RTMP URL
2. Start OBS recording/streaming
3. Start plugin stream
4. Verify Restreamer receives stream
5. Check stream quality metrics

**RTMP URL Format:**
```
rtmp://rs.rainmanjam.com/live/stream-key
```

### Test 4: HLS Output
**Objective:** Verify Restreamer can transcode and serve HLS

**Test Steps:**
1. Start streaming to Restreamer
2. Wait for HLS segments to generate
3. Access HLS playlist URL
4. Verify playback in browser/VLC

**HLS URL Format:**
```
https://rs.rainmanjam.com/memfs/{id}.m3u8
```

### Test 5: SSL/TLS Connectivity
**Objective:** Verify plugin works with HTTPS Restreamer servers

**Test Steps:**
1. Configure with `use_https: true`
2. Test API connectivity
3. Verify certificate validation
4. Test with self-signed certificate (should warn/fail)

### Test 6: Authentication
**Objective:** Verify HTTP Basic Auth works correctly

**Test Steps:**
1. Test with valid credentials (should succeed)
2. Test with invalid credentials (should fail gracefully)
3. Test with missing credentials (should prompt/fail)
4. Test credential persistence

---

## Execution Instructions

### Quick Test (5 minutes)
```bash
# Run unit tests only
cd build && ctest --output-on-failure

# Or via CMake
cmake --build build --target test
```

### Medium Test (15 minutes)
```bash
# Build + Unit tests + Integration tests
./scripts/test-all-platforms.sh --skip-windows --build-first
```

### Full Test Suite (30-60 minutes)
```bash
# All platforms, all tests, with coverage
./scripts/test-all-platforms.sh --build-first -v

# Generate coverage report
./scripts/generate-coverage.sh
```

### Continuous Integration Simulation
```bash
# Run exact same tests as GitHub Actions
act push -W .github/workflows/automated-tests.yml

# Run specific job
act -j build-linux
act -j unit-tests-macos
act -j security-scan
```

### Docker-Based Testing
```bash
# Linux build and test in Docker
act -W .github/workflows/automated-tests.yml \
    -j build-linux \
    --platform ubuntu-latest=ubuntu:22.04

# Use full Ubuntu image for better compatibility
act -W .github/workflows/automated-tests.yml \
    -j build-linux \
    --platform ubuntu-latest=catthehacker/ubuntu:full-latest
```

### Restreamer Live Test
```bash
# Test connection to Restreamer server
./test-connection-settings.sh

# Run integration test with real server
E2E_RESTREAMER_HOST="rs.rainmanjam.com" \
E2E_RESTREAMER_PORT=443 \
E2E_RESTREAMER_HTTPS=true \
E2E_RESTREAMER_USER="admin" \
E2E_RESTREAMER_PASS="password" \
./tests/scenarios/test-restreamer-integration.sh
```

---

## Test Matrix

| Test Type | macOS | Linux | Windows | Duration | Importance |
|-----------|-------|-------|---------|----------|------------|
| Unit Tests | ✅ | ✅ | ✅ | 2 min | Critical |
| Build Tests | ✅ | ✅ | ✅ | 5 min | Critical |
| Integration Tests | ✅ | ✅ | 🔄 | 10 min | High |
| E2E Tests | ✅ | 🔄 | 🔄 | 15 min | High |
| Security Scans | ✅ | ✅ | ✅ | 3 min | Critical |
| Performance Tests | 🔄 | ✅ | 🔄 | 20 min | Medium |
| Live Streaming | ✅ | 🔄 | 🔄 | 5 min | High |

**Legend:**
- ✅ Implemented and passing
- 🔄 In progress or needs work
- ❌ Not implemented

---

## Test Results Location

- **Unit Test Results:** `build/Testing/`
- **Coverage Reports:** `build/coverage/`
- **E2E Test Logs:** `/tmp/obs-polyemesis-e2e/`
- **Integration Test Logs:** `build/integration-tests/`
- **CI Artifacts:** `.github/workflows/artifacts/`

---

## Troubleshooting

### Plugin Doesn't Load in OBS
1. Check OBS logs: `~/Library/Application Support/obs-studio/logs/`
2. Verify binary architecture matches OBS (arm64 vs x86_64)
3. Check code signature: `codesign -dv /path/to/plugin`
4. Verify dependencies: `otool -L /path/to/plugin`

### Restreamer Connection Fails
1. Test connectivity: `curl -u user:pass https://server/api/v3/process`
2. Check firewall rules
3. Verify SSL certificate
4. Check server logs on Restreamer

### Tests Fail in Docker
1. Ensure Docker has enough resources (8GB+ RAM)
2. Check Docker network settings
3. Use full Ubuntu image for better package availability
4. Check file permissions in mounted volumes

---

## Next Steps

1. ✅ Run comprehensive test suite locally
2. 🔄 Set up automated nightly tests
3. 🔄 Implement missing test scenarios
4. 🔄 Add performance benchmarks
5. 🔄 Create test data generator
6. 🔄 Set up test Restreamer server

---

## Contributing

When adding new tests:
1. Follow existing test framework conventions
2. Add test to appropriate category (unit/integration/e2e)
3. Update this document with new scenarios
4. Ensure tests are cross-platform compatible
5. Add CI workflow if needed
