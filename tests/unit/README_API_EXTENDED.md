# Extended API Unit Tests

This directory contains comprehensive unit tests for the obs-polyemesis Restreamer API client.

## Test Suite: test_api_extended.c

### Overview
Comprehensive testing of the API client module with focus on:
- Token management
- Process JSON creation
- HTTP method verification
- Error handling
- Security features
- JSON parsing

### Test Categories

#### 1. Token Management Tests (4 tests)
- **Token Expiry Detection**: Verifies that tokens expiring within 60 seconds are detected
- **Token Refresh Structure**: Tests the refresh token API call structure
- **Login Throttling**: Tests exponential backoff on failed login attempts
- **Force Login Token Clearing**: Verifies force_login invalidates existing tokens

#### 2. Process JSON Creation Tests (3 tests)
- **Cleanup Arrays**: Verifies input/output objects contain cleanup arrays as per API spec
- **Limits Object**: Verifies process creation includes resource limits (CPU, memory, waitfor)
- **Complete Structure**: Tests that all required fields are present in process creation JSON

#### 3. HTTP Method Verification Tests (2 tests)
- **PUT Method for Commands**: Verifies process commands (start/stop/restart) use PUT method
- **Content-Type Headers**: Verifies all requests include proper `Content-Type: application/json` header

#### 4. Error Handling Tests (5 tests)
- **HTTP 400 Handling**: Tests proper error reporting for bad requests
- **HTTP 401 Retry Logic**: Verifies automatic retry with token refresh on 401 Unauthorized
- **Network Timeout**: Confirms 10-second timeout is configured
- **Error Message Propagation**: Tests that error messages are stored and retrievable
- **NULL Pointer Safety**: Verifies all API functions handle NULL pointers gracefully

#### 5. Security Tests (3 tests)
- **Secure Memory Zeroing**: Tests that `secure_memzero()` properly clears sensitive data
- **Secure String Freeing**: Verifies `secure_free()` clears memory before freeing
- **HTTPS Certificate Verification**: Confirms SSL verification is enabled (prevents MITM attacks)

#### 6. JSON Parsing Tests (3 tests)
- **Valid JSON Parsing**: Tests parsing of well-formed JSON responses
- **Invalid JSON Handling**: Verifies malformed JSON is properly rejected with error messages
- **Process Field Parsing**: Tests that process JSON is correctly parsed into structs

### Total Test Count
**20 comprehensive unit tests** covering critical API client functionality

## Building and Running

### Build Tests
```bash
cd /Users/rainmanjam/Documents/GitHub/obs-polyemesis
cmake -B build -DENABLE_TESTING=ON
cmake --build build --target test_api_extended
```

### Run Tests
```bash
# Run all extended API tests
./build/tests/test_api_extended

# Or use CTest
cd build
ctest -R api_extended_tests -V
```

### Expected Output
```
================================================================================
TEST SUITE: Extended API Client Tests
================================================================================

[TEST] Running: Token Expiry Detection
[PASS] Token Expiry Detection

[TEST] Running: Token Refresh Structure
[PASS] Token Refresh Structure

... (all 20 tests)

================================================================================
TEST SUMMARY
================================================================================
Total:   20
Passed:  20
Failed:  0
Crashed: 0
Skipped: 0
================================================================================
Result: PASSED
================================================================================
```

## Test Framework

These tests use a custom lightweight test framework (`test_framework.h`) that provides:
- Crash detection with signal handlers
- Memory leak tracking
- Colored console output
- Test statistics and reporting
- Assertion macros (ASSERT, ASSERT_EQ, ASSERT_STR_EQ, etc.)

## Coverage Areas

### Token Management
- Token expiry detection (60-second proactive refresh window)
- Refresh token API call structure
- Exponential backoff on login failures (1000ms → 2000ms → 4000ms)
- Token invalidation on force login

### Process JSON API Specification
Tests verify compliance with datarhei Restreamer API v3 specification:
```json
{
  "id": "process_id",
  "type": "ffmpeg",
  "reference": "description",
  "input": [{
    "id": "input_0",
    "address": "rtmp://...",
    "options": [...],
    "cleanup": []
  }],
  "output": [{
    "id": "output_0",
    "address": "rtmp://...",
    "options": [...],
    "cleanup": []
  }],
  "options": [...],
  "autostart": true,
  "reconnect": true,
  "reconnect_delay_seconds": 15,
  "stale_timeout_seconds": 30,
  "limits": {
    "cpu_usage": 0,
    "memory_mbytes": 0,
    "waitfor_seconds": 0
  }
}
```

### HTTP Method Verification
- Process commands (start/stop/restart) use PUT method (idempotent operations)
- Process creation uses POST method
- All requests include `Content-Type: application/json`
- All authenticated requests include `Authorization: Bearer <token>`

### Error Handling
- HTTP 400 errors properly reported
- HTTP 401 triggers automatic token refresh and retry
- Network timeouts configured (10 seconds)
- Error messages stored in `api->last_error`
- NULL pointer checks on all public API functions

### Security Features
- `secure_memzero()` uses volatile pointer to prevent compiler optimization
- `secure_free()` clears memory before calling `bfree()`
- Passwords and tokens cleared from memory on API destruction
- SSL certificate verification enabled (CURLOPT_SSL_VERIFYPEER = 1L)
- Hostname verification enabled (CURLOPT_SSL_VERIFYHOST = 2L)

## Integration with CMake

The test suite is integrated into the CMake build system:

```cmake
# Extended API tests executable (standalone unit tests)
add_executable(
  test_api_extended
  unit/test_api_extended.c
  obs_stubs.c
)

target_sources(
  test_api_extended
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/restreamer-api.c
    ${CMAKE_SOURCE_DIR}/src/restreamer-api-utils.c
)

# Define TESTING_MODE to expose internal functions
target_compile_definitions(test_api_extended PRIVATE TESTING_MODE)

# Add to CTest
add_test(NAME api_extended_tests COMMAND $<TARGET_FILE:test_api_extended>)
```

## TESTING_MODE

The tests use `TESTING_MODE` compile definition to access internal functions:
- `secure_memzero()` - Security function for clearing sensitive memory
- `secure_free()` - Security function for clearing and freeing strings
- `parse_json_response()` - JSON parsing helper
- `parse_process_fields()` - Process object parsing
- `handle_login_failure()` - Login retry logic
- `is_login_throttled()` - Login throttling check

## Future Enhancements

Potential additions to the test suite:
1. Mock server integration for testing actual HTTP requests
2. Coverage for dynamic output management APIs
3. Tests for encoding parameter updates
4. File system API testing
5. Metrics API testing
6. Integration tests with live Restreamer server

## Dependencies

- **libobs**: OBS Studio library (for memory management: bzalloc, bfree, dstr)
- **jansson**: JSON parsing library
- **libcurl**: HTTP client library
- **test_framework.h**: Custom test framework with crash detection

## Author Notes

This test suite was designed to provide comprehensive coverage of the API client without requiring a running Restreamer server. It focuses on:
- Unit-level testing of individual functions
- Validation of data structures and JSON formatting
- Security and error handling verification
- Compliance with API specifications

For integration tests against a live server, see `/tests/integration/test_restreamer_api_integration.c`.
