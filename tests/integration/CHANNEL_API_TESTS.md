# Channel-API Integration Tests Summary

## Overview

This document describes the comprehensive integration tests for Channel/API/Multistream interactions in the obs-polyemesis plugin. These tests verify that multiple modules work together correctly.

## Test File

**Location:** `tests/integration/test_channel_api_integration.c`
**Total Tests:** 12
**Execution Modes:** Mock (default) + Live (LIVE_TEST_SERVER=1)

## Test Categories

### 1. Channel to API Integration (3 tests)

#### Test 1: Channel Start Creates API Calls
- Verifies channel start creates correct process on server
- Tests process creation with multiple outputs
- Validates process appears in server's process list
- Checks process reference is stored correctly

#### Test 2: Channel Stop Cleanup
- Verifies channel stop deletes process from server
- Tests complete resource cleanup
- Validates process no longer exists after stop
- Checks channel status changes to inactive

#### Test 3: Channel State Reflects Server State
- Verifies channel state accurately reflects server
- Tests stats updates from server
- Validates output connection states
- Checks bytes sent/received tracking

### 2. Multistream to API Integration (3 tests)

#### Test 4: Multistream Config Creates JSON
- Verifies multistream creates correct process JSON
- Tests multiple destination configuration
- Validates output array in process JSON
- Checks JSON structure correctness

#### Test 5: Output URL Construction
- Tests URL generation for different services
- Validates YouTube, Twitch, TikTok URLs
- Checks service name retrieval
- Verifies URL format correctness

#### Test 6: Orientation Video Filtering
- Tests orientation detection (horizontal, vertical, square)
- Validates video filter generation
- Checks filter correctness for conversions
- Tests same-orientation handling

### 3. Configuration Integration (3 tests)

#### Test 7: Config Change Propagation
- Verifies config changes update API client
- Tests channel manager API updates
- Validates connection with new config
- Checks manager uses updated API

#### Test 8: Reconnection on Config Change
- Tests force re-login functionality
- Validates token refresh mechanism
- Checks connection persistence
- Verifies API stays connected

#### Test 9: Invalid Server Config
- Tests invalid hostname handling
- Validates invalid port handling
- Checks error message generation
- Tests invalid credentials (mock mode)

### 4. Error Recovery (3 tests)

#### Test 10: Recovery from API Errors
- Tests 404 error handling
- Validates API recovery after errors
- Checks continued operation
- Tests error state management

#### Test 11: Server Disconnection
- Tests server shutdown handling (mock only)
- Validates disconnection detection
- Checks error messages
- Tests reconnection after restart

#### Test 12: Automatic Reconnection
- Tests auto-reconnect configuration
- Validates health monitoring setup
- Checks reconnect delays and attempts
- Tests manual reconnect functionality

## Execution Modes

### Mock Mode (Default)
```bash
# Fast, deterministic, no network
./test_channel_api_integration

# Via CTest
ctest -R channel_api_integration
```

**Characteristics:**
- Local mock server on port 9500
- < 5 seconds execution time
- No external dependencies
- Suitable for CI/CD

### Live Mode
```bash
# Against real server
LIVE_TEST_SERVER=1 ./test_channel_api_integration

# Via CTest
LIVE_TEST_SERVER=1 ctest -R channel_api_integration -V
```

**Characteristics:**
- Real Restreamer server (rs2.rainmanjam.com)
- 10-30 seconds execution time
- Full integration validation
- Pre-release testing

## Architecture

### Helper Functions

```c
bool use_live_server(void)
    // Checks LIVE_TEST_SERVER environment variable

restreamer_connection_t get_test_connection(void)
    // Returns config for mock or live server

bool setup_test_server(void)
    // Starts mock server or validates live server

void teardown_test_server(void)
    // Stops mock server
```

### Test Structure

Each test follows this pattern:
```c
static bool test_feature(void)
{
    // 1. Print test description
    // 2. Setup test server
    // 3. Create API client
    // 4. Perform test operations
    // 5. Assert expectations
    // 6. Cleanup resources
    // 7. Teardown server
    return true;
}
```

## Dependencies

- **libcurl:** HTTP client
- **jansson:** JSON parsing
- **OBS libobs:** API stubs
- **pthread:** Threading (Unix)

## Build Configuration

Added to `tests/CMakeLists.txt`:
```cmake
add_executable(test_channel_api_integration
    integration/test_channel_api_integration.c
    mock_restreamer.c
    obs_stubs.c
)
```

Linked sources:
- restreamer-api.c
- restreamer-api-utils.c
- restreamer-channel.c
- restreamer-multistream.c
- restreamer-config.c

## Running Tests

### Quick Commands

```bash
# Build
make test_channel_api_integration

# Run mock mode
./tests/test_channel_api_integration

# Run live mode
LIVE_TEST_SERVER=1 ./tests/test_channel_api_integration

# CTest
ctest -R channel_api_integration -V

# All integration tests
ctest -L integration
```

### Expected Output

**Success:**
```
[TEST] Running: Test 1: Channel start creates correct API calls
  Using MOCK server: localhost:9500
  Channel start succeeded
  Process reference: test-process-1234567890
  Found process on server: proc_abc123 (state: running)
[PASS] Test 1: Channel start creates correct API calls
```

**Failure:**
```
[TEST] Running: Test 1: Channel start creates correct API calls
  Using MOCK server: localhost:9500
  Connection error: Connection refused
[FAIL] Test 1: Channel start creates correct API calls
```

## Test Coverage

### Covered Scenarios

✅ Channel creation and management
✅ Channel start/stop operations
✅ Process creation on server
✅ Process deletion on server
✅ Multistream configuration
✅ Output URL generation
✅ Orientation detection
✅ Configuration updates
✅ Reconnection logic
✅ Error handling
✅ API error recovery
✅ Server disconnection

### Not Covered

❌ Actual video encoding
❌ Network bandwidth monitoring
❌ Long-running stability
❌ Concurrent multi-channel ops
❌ UI components

## Performance

### Mock Mode
- Setup: < 100ms
- Per test: 10-50ms
- Total: < 5s
- Memory: ~10MB

### Live Mode
- Setup: < 1000ms
- Per test: 100-500ms
- Total: 10-30s
- Memory: ~15MB

## CI/CD Integration

```yaml
# GitHub Actions
- name: Run Integration Tests
  run: |
    cd build
    ctest -L integration --output-on-failure
```

Exit codes:
- 0: All passed
- 1: Some failed
- 2: Crashed

## Troubleshooting

### Port in Use
```bash
lsof -ti:9500 | xargs kill -9
```

### Connection Issues
```bash
# Check mock server
netstat -an | grep 9500

# Check live server
curl -k https://rs2.rainmanjam.com/api/v3/
```

### SSL Errors
```bash
curl --version  # Check SSL support
```

## Documentation

- **README.md** - Overview and instructions
- **TESTING_GUIDE.md** - Comprehensive guide
- **CHANNEL_API_TESTS.md** - This file

## Future Enhancements

1. Stress testing
2. Performance metrics
3. Extended scenarios
4. Live streaming tests

## Related Files

- `test_restreamer_api_integration.c` - Basic API tests
- `mock_restreamer.c` - Mock server implementation
- `obs_stubs.c` - OBS API stubs
- `test_framework.h` - Test macros and utilities
