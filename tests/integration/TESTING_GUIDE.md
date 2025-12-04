# Integration Testing Guide

This guide explains how to run and interpret the integration tests for the obs-polyemesis plugin.

## Overview

Integration tests verify that multiple modules work together correctly. The test suite includes:

- **21 total tests** across 2 test files
- **Mock mode** for fast CI/CD testing
- **Live mode** for full validation against real servers
- Coverage of channel, API, and multistream interactions

## Quick Start

### Run All Tests (Mock Mode)
```bash
cd build
ctest -L integration
```

### Run All Tests (Live Mode)
```bash
cd build
LIVE_TEST_SERVER=1 ctest -L integration -V
```

## Test Modes

### Mock Mode (Default)
- Uses local mock server
- Fast execution
- Deterministic behavior
- Suitable for CI/CD pipelines
- No network dependencies

**When to use:** Development, CI/CD, rapid iteration

### Live Mode (LIVE_TEST_SERVER=1)
- Uses real Restreamer server
- Full integration validation
- Tests real network behavior
- Validates against production-like environment

**When to use:** Pre-release validation, feature verification, debugging production issues

## Test Categories

### 1. Channel to API Integration (3 tests)
Tests that channel operations correctly interact with the API:

```bash
# These tests verify:
# - Channel start creates processes on server
# - Channel stop deletes processes from server
# - Channel state reflects actual server state
```

**Key scenarios:**
- Starting a channel with multiple outputs
- Stopping a channel and verifying cleanup
- Syncing channel state with server

### 2. Multistream to API Integration (3 tests)
Tests that multistream configuration creates correct API calls:

```bash
# These tests verify:
# - Correct process JSON generation
# - Proper URL construction for different services
# - Orientation-based video filtering
```

**Key scenarios:**
- Creating multistream with YouTube, Twitch, TikTok
- Detecting and converting between orientations
- Building FFmpeg video filters

### 3. Configuration Integration (3 tests)
Tests that configuration changes propagate correctly:

```bash
# These tests verify:
# - API client updates when config changes
# - Reconnection after config change
# - Handling of invalid configurations
```

**Key scenarios:**
- Changing server connection settings
- Invalid host/port handling
- Token refresh and re-authentication

### 4. Error Recovery (3 tests)
Tests that the system recovers from errors gracefully:

```bash
# These tests verify:
# - Recovery from API errors (404, connection errors)
# - Handling server disconnection
# - Automatic reconnection features
```

**Key scenarios:**
- 404 errors for non-existent resources
- Network disconnection and reconnection
- Invalid credentials handling

## Running Specific Test Categories

### Channel Tests Only
```bash
ctest -R channel_api_integration -V
```

### API Tests Only
```bash
ctest -R restreamer_api_integration -V
```

### Live Server Tests
```bash
LIVE_TEST_SERVER=1 ctest -R channel_api_integration -V
```

## Interpreting Results

### Success Output
```
[TEST] Running: Test 1: Channel start creates correct API calls
  Using MOCK server: localhost:9500
  Channel start succeeded
  Process reference: test-process-1234567890
  Found process on server: proc_abc123 (state: running)
[PASS] Test 1: Channel start creates correct API calls
```

### Failure Output
```
[TEST] Running: Test 1: Channel start creates correct API calls
  Using MOCK server: localhost:9500
  Connection error: Connection refused
[FAIL] Test 1: Channel start creates correct API calls
```

### Expected Warnings
Some tests may show warnings that are expected:
```
WARNING: Process still exists: running
```
This means cleanup didn't complete, which could be a real issue.

## CI/CD Integration

### GitHub Actions Example
```yaml
- name: Run Integration Tests
  run: |
    cd build
    ctest -L integration --output-on-failure
```

### GitLab CI Example
```yaml
test:integration:
  script:
    - cd build
    - ctest -L integration -V
  tags:
    - docker
```

### Jenkins Example
```groovy
stage('Integration Tests') {
    steps {
        sh '''
            cd build
            ctest -L integration --output-on-failure
        '''
    }
}
```

## Debugging Failed Tests

### Enable Verbose Output
```bash
ctest -R channel_api_integration -V
```

### Run Single Test
```bash
./build/tests/test_channel_api_integration
```

### Check Mock Server
```bash
# Mock server runs on port 9500 by default
netstat -an | grep 9500
```

### Check Live Server Connection
```bash
curl -k https://rs2.rainmanjam.com/api/v3/
```

## Common Issues

### Port Already in Use
**Symptom:** Mock server fails to start
**Solution:** Kill existing process or change port
```bash
lsof -ti:9500 | xargs kill -9
```

### Connection Timeout
**Symptom:** Tests hang or timeout
**Solution:** Check network connectivity
```bash
ping rs2.rainmanjam.com
curl -k https://rs2.rainmanjam.com/api/v3/
```

### SSL Certificate Errors
**Symptom:** SSL handshake failed
**Solution:** Tests use `-k` flag to skip verification, but check CURL:
```bash
curl --version  # Ensure SSL support is enabled
```

### Authentication Failures
**Symptom:** 401 Unauthorized errors
**Solution:** Verify credentials in test file or environment

## Performance Expectations

### Mock Mode
- **Setup:** < 100ms
- **Per Test:** 10-50ms
- **Total Suite:** < 5 seconds

### Live Mode
- **Setup:** < 1000ms
- **Per Test:** 100-500ms
- **Total Suite:** 10-30 seconds

## Adding New Tests

### Template
```c
static bool test_new_feature(void)
{
    printf("  Testing new feature...\n");

    if (!setup_test_server()) {
        return false;
    }

    restreamer_connection_t conn = get_test_connection();
    restreamer_api_t *api = restreamer_api_create(&conn);
    ASSERT_NOT_NULL(api, "Should create API client");

    // Your test code here

    restreamer_api_destroy(api);
    teardown_test_server();

    return true;
}
```

### Register Test
```c
RUN_TEST(test_new_feature, "Test: Description of new feature");
```

## Best Practices

1. **Use Mock Mode by Default**
   - Faster iteration
   - More reliable in CI/CD
   - No external dependencies

2. **Test Against Live Server Before Release**
   - Validates real-world behavior
   - Catches integration issues
   - Verifies server compatibility

3. **Clean Up Resources**
   - Always call teardown functions
   - Delete test processes
   - Free allocated memory

4. **Handle Expected Failures**
   - Some operations may fail in mock mode
   - Document expected behavior
   - Don't fail tests on expected errors

5. **Add Descriptive Output**
   - Print what you're testing
   - Show intermediate results
   - Log errors clearly

## Troubleshooting Checklist

- [ ] Is the mock server starting successfully?
- [ ] Are required ports available?
- [ ] Is network connectivity working (for live mode)?
- [ ] Are server credentials correct?
- [ ] Are all dependencies installed (libcurl, jansson)?
- [ ] Is the build up to date?
- [ ] Are environment variables set correctly?

## Support

For issues or questions:
1. Check this guide first
2. Review test output carefully
3. Check server logs (for live mode)
4. Review source code comments
5. File an issue with full test output

## Related Documentation

- [README.md](README.md) - Integration tests overview
- [../../docs/testing-strategy.md](../../docs/testing-strategy.md) - Overall testing strategy
- [../../docs/restreamer-api.md](../../docs/restreamer-api.md) - API documentation
