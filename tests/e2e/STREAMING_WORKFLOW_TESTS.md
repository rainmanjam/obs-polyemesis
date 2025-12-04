# End-to-End Streaming Workflow Tests

## Overview

Comprehensive integration tests for the complete streaming workflow using a live Restreamer server. These tests verify the full lifecycle of channel management, multi-destination streaming, failover, dynamic configuration, and auto-reconnection.

**Test File:** `/tests/e2e/test_streaming_workflow.c`
**Test Count:** 6 comprehensive E2E tests
**Server:** https://rs2.rainmanjam.com

## Test Configuration

### Live Server Details
- **URL:** rs2.rainmanjam.com
- **Port:** 443 (HTTPS)
- **Username:** admin
- **Password:** tenn2jagWEE@##$
- **Protocol:** HTTPS

### Test Constants
- **Input URL:** rtmp://localhost:1935/live/test
- **YouTube RTMP:** rtmp://a.rtmp.youtube.com/live2/
- **Twitch RTMP:** rtmp://live.twitch.tv/app/
- **Facebook RTMPS:** rtmps://live-api-s.facebook.com:443/rtmp/
- **Timeout:** 10 seconds
- **Polling Interval:** 1 second

## Test Suite

### Test 1: Complete Channel Lifecycle
**Function:** `test_e2e_channel_create_start_stop()`

Tests the full lifecycle of a streaming channel:
1. Create a new channel with single output destination
2. Verify channel creation and configuration
3. Start streaming
4. Monitor process state (FPS, bitrate, running status)
5. Stop streaming gracefully
6. Delete channel
7. Verify complete cleanup

**Expected Behavior:**
- Channel creates successfully with unique ID
- Process starts and reaches "running" state
- Process state shows valid metrics (FPS, bitrate)
- Process stops cleanly
- Channel deletes without leaving artifacts

### Test 2: Multi-destination Streaming
**Function:** `test_e2e_multistream()`

Tests streaming to multiple platforms simultaneously:
1. Create channel with 3 output destinations (YouTube, Twitch, Facebook)
2. Start multistream
3. Verify all 3 outputs are active
4. Monitor process state
5. Stop multistream
6. Cleanup

**Expected Behavior:**
- All 3 outputs should be created and registered
- Process starts with all outputs active
- API returns all 3 output IDs
- All streams receive data concurrently
- Multistream stops cleanly

### Test 3: Failover Functionality
**Function:** `test_e2e_failover()`

Tests primary/backup failover workflow:
1. Create primary channel with output
2. Create backup channel with output
3. Start primary stream
4. Verify primary is running
5. Simulate primary failure (stop primary)
6. Activate backup stream (failover)
7. Verify backup is now running
8. Restore primary stream
9. Switch back to primary
10. Verify primary is running again

**Expected Behavior:**
- Primary starts and runs successfully
- Backup activates when primary fails
- Backup streams during primary downtime
- Primary can be restored without data loss
- System handles failover transitions smoothly

### Test 4: Live Destination Management
**Function:** `test_e2e_live_output_add_remove()`

Tests dynamic output management while streaming:
1. Create channel with single output
2. Start streaming
3. Verify initial output is active
4. Add second output while stream is running
5. Verify both outputs are active
6. Remove first output while stream continues
7. Verify stream continues with remaining output
8. Cleanup

**Expected Behavior:**
- New outputs can be added without stopping stream
- Outputs can be removed without interrupting stream
- Stream continues seamlessly during modifications
- Output count reflects current active destinations
- Process state remains stable during changes

### Test 5: Custom Encoding Settings
**Function:** `test_e2e_encoding_settings()`

Tests custom encoding configuration:
1. Create channel with custom video filter (scale=1280:720)
2. Verify FFmpeg configuration includes filter
3. Start process with custom encoding
4. Attempt to update encoding settings live (bitrate, resolution, preset)
5. Verify encoding parameter updates
6. Cleanup

**Expected Behavior:**
- Custom video filters are applied to FFmpeg command
- Process configuration JSON contains encoding settings
- Live encoding updates are attempted (may not be supported)
- Encoding parameters are readable via API
- Custom settings persist through process lifecycle

### Test 6: Auto-reconnection on Failure
**Function:** `test_e2e_reconnection()`

Tests automatic reconnection after input loss:
1. Create streaming channel
2. Start streaming
3. Verify stream is running
4. Simulate input loss (restart process)
5. Wait for automatic reconnection
6. Verify process recovers
7. Check logs for reconnection attempts
8. Cleanup

**Expected Behavior:**
- Process starts normally
- Restart triggers reconnection logic
- Process attempts to reconnect automatically
- Logs show reconnection attempts
- Process recovers to running state
- Stream continues after reconnection

## Running the Tests

### Build the Test Binary

```bash
cd /path/to/obs-polyemesis
mkdir -p build && cd build
cmake ..
make test_e2e_streaming_workflow
```

### Run All E2E Streaming Workflow Tests

```bash
# From build directory
./tests/test_e2e_streaming_workflow

# Using CTest
ctest -R e2e_streaming_workflow -V

# With verbose output
ctest -R e2e_streaming_workflow --verbose --output-on-failure
```

### Run Specific Test

To run a specific test, you'll need to modify the source file temporarily or use a debugger to skip to the desired test.

### Expected Output

```
========================================================================
TEST SUITE: End-to-End Streaming Workflow Tests
========================================================================

[SETUP] Connecting to Restreamer server: rs2.rainmanjam.com:443
[SETUP] Successfully connected to Restreamer

[TEST] Running: E2E: Complete channel lifecycle (create → start → stop → delete)
    Creating channel: e2e_lifecycle_1701547234_5432
    Starting stream...
    Process running - FPS: 29.97, Bitrate: 2500 kbps
    Stopping stream...
    Deleting channel...
[PASS] E2E: Complete channel lifecycle (create → start → stop → delete)

[TEST] Running: E2E: Multi-destination streaming (YouTube + Twitch + Facebook)
    Creating multistream channel: e2e_multistream_1701547242_8721
    Starting multistream...
    Active outputs:
      - output_0
      - output_1
      - output_2
    Stopping multistream...
[PASS] E2E: Multi-destination streaming (YouTube + Twitch + Facebook)

[TEST] Running: E2E: Failover functionality (primary → backup → restore)
    Creating primary channel: e2e_primary_1701547250_1234
    Creating backup channel: e2e_backup_1701547250_5678
    Starting primary stream...
    Simulating primary failure...
    Activating backup stream...
    Restoring primary stream...
    Switching back to primary...
[PASS] E2E: Failover functionality (primary → backup → restore)

[TEST] Running: E2E: Live destination management (add/remove while streaming)
    Creating channel with single output: e2e_live_modify_1701547265_9012
    Starting stream...
    Adding second output while streaming...
    Removing first output while streaming...
[PASS] E2E: Live destination management (add/remove while streaming)

[TEST] Running: E2E: Custom encoding settings (resolution, bitrate, filters)
    Creating channel with custom encoding: e2e_encoding_1701547280_3456
    Verifying encoding settings in config...
    Config snippet: {"process":{"input":...,"options":["-vf","scale=1280:720"]...
    Updating encoding settings (bitrate, resolution)...
[PASS] E2E: Custom encoding settings (resolution, bitrate, filters)

[TEST] Running: E2E: Auto-reconnection on failure (input loss recovery)
    Creating channel for reconnection test: e2e_reconnect_1701547295_7890
    Starting stream...
    Simulating input loss (restart process)...
    Waiting for reconnection...
    Process state after restart - Running: YES
    Checking process logs for reconnection attempts...
    Recent log entries (12 total):
      [INFO] Process started
      [INFO] Input connected
      [INFO] Streaming started
      [INFO] Process restarted
      [INFO] Reconnecting to input...
[PASS] E2E: Auto-reconnection on failure (input loss recovery)

[TEARDOWN] Cleaning up...
[TEARDOWN] Complete

========================================================================
TEST SUMMARY
========================================================================
Total:   6
Passed:  6
Failed:  0
Crashed: 0
Skipped: 0
========================================================================
Result: PASSED
========================================================================
```

## Test Features

### Robust Error Handling
- Automatic cleanup of test processes on failure
- Timeout protection for long-running operations
- Connection verification before test execution
- Graceful degradation on API limitations

### Helper Functions
- `setup_api_client()` - Initializes and authenticates API connection
- `cleanup_api_client()` - Safely destroys API client
- `generate_process_id()` - Creates unique process identifiers
- `wait_for_process_state()` - Polls for expected process state
- `cleanup_test_process()` - Ensures process cleanup on test exit

### Test Assertions
Uses the test framework from `test_framework.h`:
- `ASSERT_TRUE()` - Verify condition is true
- `ASSERT_FALSE()` - Verify condition is false
- `ASSERT_NOT_NULL()` - Verify pointer is not NULL
- `ASSERT_EQ()` - Verify equality
- `ASSERT_STR_EQ()` - Verify string equality

## Requirements

### Server Requirements
- Live Restreamer server running and accessible
- Server credentials configured correctly
- Network connectivity to server
- HTTPS/TLS support

### Build Dependencies
- libcurl (for HTTP/HTTPS requests)
- jansson (for JSON parsing)
- C compiler (GCC, Clang, or MSVC)
- CMake 3.16+

### Runtime Requirements
- Network access to rs2.rainmanjam.com:443
- Permissions to create/delete processes on Restreamer
- Sufficient bandwidth for streaming tests

## Troubleshooting

### Connection Failures

```
[ERROR] Failed to connect to Restreamer server
```

**Solutions:**
1. Verify server URL and port are correct
2. Check network connectivity: `ping rs2.rainmanjam.com`
3. Verify HTTPS certificate is valid
4. Check firewall settings
5. Confirm credentials are correct

### Test Timeouts

```
[FAIL] Should get process state
```

**Solutions:**
1. Increase `TEST_TIMEOUT_MS` in source file
2. Check server load/performance
3. Verify network latency: `ping rs2.rainmanjam.com`
4. Reduce concurrent tests

### Process Not Starting

```
[FAIL] Process should be running
```

**Solutions:**
1. Check Restreamer server logs
2. Verify input URL is accessible
3. Check FFmpeg is available on server
4. Review process configuration in server UI
5. Check server resource limits

### Cleanup Issues

**Manual Cleanup:**
```bash
# List all test processes
curl -u admin:password https://rs2.rainmanjam.com/api/v3/process

# Delete specific test process
curl -X DELETE -u admin:password \
  https://rs2.rainmanjam.com/api/v3/process/e2e_test_123
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: E2E Streaming Workflow Tests

on: [push, pull_request]

jobs:
  e2e-streaming-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libcurl4-openssl-dev libjansson-dev cmake

      - name: Build tests
        run: |
          mkdir build && cd build
          cmake ..
          make test_e2e_streaming_workflow

      - name: Run E2E streaming tests
        env:
          RESTREAMER_URL: ${{ secrets.RESTREAMER_URL }}
          RESTREAMER_USER: ${{ secrets.RESTREAMER_USER }}
          RESTREAMER_PASS: ${{ secrets.RESTREAMER_PASS }}
        run: |
          cd build
          ctest -R e2e_streaming_workflow --verbose --output-on-failure
        continue-on-error: true  # Don't fail build on network issues
```

## Best Practices

1. **Always run cleanup** - Use helper functions to ensure processes are deleted
2. **Use unique IDs** - Include timestamp and random number in process IDs
3. **Check server state** - Verify connection before running tests
4. **Monitor timeouts** - Adjust timeouts based on network conditions
5. **Review logs** - Check Restreamer logs for detailed failure information
6. **Isolate tests** - Each test should be independent and not affect others
7. **Handle failures gracefully** - Always attempt cleanup even on test failure

## Related Documentation

- [Restreamer API Documentation](../../docs/api-reference.md)
- [Integration Tests](../README_INTEGRATION_TESTS.md)
- [Test Framework](../test_framework.h)
- [E2E Test Strategy](README.md)

## Maintenance

### Updating Server Credentials

Edit the constants in `test_streaming_workflow.c`:
```c
#define TEST_SERVER_URL "your-server.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "your-username"
#define TEST_SERVER_PASSWORD "your-password"
```

### Adding New Tests

1. Create new test function following pattern:
```c
static bool test_e2e_your_test(void)
{
    // Setup
    // Execute
    // Assert
    // Cleanup
    return true;
}
```

2. Add to test suite at bottom:
```c
RUN_TEST(test_e2e_your_test, "E2E: Your test description");
```

3. Update test count in CMakeLists.txt

## License

Same as parent project (GPL-2.0)
