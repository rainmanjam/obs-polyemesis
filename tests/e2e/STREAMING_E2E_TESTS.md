# Comprehensive End-to-End Streaming Tests

## Overview

This test suite provides comprehensive coverage of the obs-polyemesis plugin's streaming functionality through end-to-end tests against a live Restreamer server. These tests simulate real-world streaming workflows from channel creation through streaming to cleanup.

**Test File:** `tests/e2e/test_streaming_e2e.c`
**Test Count:** 16 comprehensive E2E tests across 5 test suites
**Live Server:** https://rs2.rainmanjam.com

## Test Server Configuration

### Server Details
- **URL:** rs2.rainmanjam.com
- **Port:** 443 (HTTPS)
- **Username:** admin
- **Password:** tenn2jagWEE@##$
- **Protocol:** HTTPS with TLS

### Test Destinations
- **YouTube RTMP:** `rtmp://a.rtmp.youtube.com/live2/`
- **Twitch RTMP:** `rtmp://live.twitch.tv/app/`
- **Facebook RTMPS:** `rtmps://live-api-s.facebook.com:443/rtmp/`
- **Custom RTMP:** `rtmp://custom.example.com/live/`
- **Input URL:** `rtmp://localhost:1935/live/test`

### Test Timing Configuration
- **Timeout:** 15 seconds
- **Polling Interval:** 1 second
- **Startup Delay:** 2 seconds
- **Stream Duration:** 3 seconds

## Test Suites

### Suite 1: Complete Streaming Workflow (3 tests)

Tests the full lifecycle of streaming channels with various configurations.

#### Test 1.1: Basic Channel Workflow
**Function:** `test_basic_channel_workflow()`

**What it tests:**
1. Create channel with single output destination
2. Verify channel creation and existence
3. Start streaming
4. Monitor process state (FPS, bitrate, running status)
5. Stop streaming gracefully
6. Verify process stopped
7. Delete channel
8. Verify complete deletion

**Expected Results:**
- ✓ Channel creates successfully with unique ID
- ✓ Process starts and reaches running state
- ✓ State shows valid metrics (FPS > 0, bitrate > 0)
- ✓ Process stops cleanly when requested
- ✓ Channel deletes without leaving artifacts
- ✓ Deleted channel is no longer retrievable

#### Test 1.2: Multi-Destination Workflow
**Function:** `test_multi_destination_workflow()`

**What it tests:**
1. Create channel with 4 output destinations (YouTube, Twitch, Facebook, Custom)
2. Start multistream
3. Verify all 4 outputs are registered and active
4. Monitor concurrent streaming
5. Stop multistream cleanly

**Expected Results:**
- ✓ All 4 outputs created successfully
- ✓ Process starts with all outputs active
- ✓ API returns all 4 output IDs
- ✓ All streams receive data concurrently
- ✓ Process stops cleanly with all outputs

#### Test 1.3: Encoding Settings Workflow
**Function:** `test_encoding_settings_workflow()`

**What it tests:**
1. Create channel with custom video filter (scale=1280:720)
2. Verify FFmpeg configuration includes custom filter
3. Start process with custom encoding
4. Verify stream runs with custom settings
5. Stop and cleanup

**Expected Results:**
- ✓ Custom video filter appears in config JSON
- ✓ Process starts with custom encoding
- ✓ Stream runs successfully with custom settings
- ✓ Settings persist through process lifecycle

---

### Suite 2: Multi-Channel Scenarios (3 tests)

Tests behavior with multiple independent channels running simultaneously.

#### Test 2.1: Multiple Channels Simultaneously
**Function:** `test_multiple_channels_simultaneous()`

**What it tests:**
1. Create 3 independent channels
2. Start all channels at once
3. Verify all are running concurrently
4. Monitor each channel's state independently
5. Stop all channels

**Expected Results:**
- ✓ All 3 channels create successfully
- ✓ All channels start without interfering with each other
- ✓ Each channel shows independent running state
- ✓ All channels stream simultaneously
- ✓ All channels stop cleanly

#### Test 2.2: Stop All Channels
**Function:** `test_stop_all_channels()`

**What it tests:**
1. Create and start 2 channels
2. Stop both channels simultaneously
3. Verify both channels stopped

**Expected Results:**
- ✓ Both channels start successfully
- ✓ Simultaneous stop command succeeds for both
- ✓ Both channels show stopped state
- ✓ No interference between stop operations

#### Test 2.3: Independent Channel Management
**Function:** `test_independent_channel_management()`

**What it tests:**
1. Create 2 independent channels
2. Start only first channel
3. Verify second channel remains stopped
4. Start second channel, stop first
5. Verify independent operation

**Expected Results:**
- ✓ Starting channel 1 doesn't affect channel 2
- ✓ Channel 2 remains stopped when not explicitly started
- ✓ Can start channel 2 while channel 1 runs
- ✓ Can stop channel 1 without affecting channel 2
- ✓ Channels operate completely independently

---

### Suite 3: Live Operations (3 tests)

Tests dynamic modifications while streaming is active.

#### Test 3.1: Add Destination Live
**Function:** `test_add_destination_live()`

**What it tests:**
1. Create channel with 1 output
2. Start streaming
3. Add second output while stream is running
4. Verify both outputs are now active
5. Verify stream continues without interruption

**Expected Results:**
- ✓ Channel starts with 1 output
- ✓ Second output adds successfully during streaming
- ✓ Output count increases to 2
- ✓ Stream continues uninterrupted
- ✓ Both outputs receive data after addition

#### Test 3.2: Remove Destination Live
**Function:** `test_remove_destination_live()`

**What it tests:**
1. Create channel with 2 outputs
2. Start streaming
3. Remove first output while streaming
4. Verify remaining output continues
5. Verify stream uninterrupted

**Expected Results:**
- ✓ Channel starts with 2 outputs
- ✓ Output removal succeeds during streaming
- ✓ Output count decreases to 1
- ✓ Stream continues to remaining output
- ✓ No data loss or interruption

#### Test 3.3: Encoding Change Live
**Function:** `test_encoding_change_live()`

**What it tests:**
1. Create and start channel
2. Attempt to update encoding settings (bitrate, resolution, preset) while streaming
3. Verify stream continues regardless of update support
4. Handle both success and unsupported scenarios

**Expected Results:**
- ✓ Channel starts successfully
- ✓ Encoding update attempt doesn't crash
- ✓ Stream continues whether update succeeds or not
- ✓ API handles unsupported operations gracefully

---

### Suite 4: Error Scenarios (4 tests)

Tests error handling and graceful degradation.

#### Test 4.1: Invalid Server
**Function:** `test_invalid_server()`

**What it tests:**
1. Attempt connection to non-existent server
2. Verify connection fails gracefully
3. Verify proper error handling

**Expected Results:**
- ✓ API client creates without crashing
- ✓ Connection test fails as expected
- ✓ No resource leaks or crashes
- ✓ Error can be recovered from

#### Test 4.2: Invalid Credentials
**Function:** `test_invalid_credentials()`

**What it tests:**
1. Attempt connection with invalid username/password
2. Verify authentication fails
3. Verify proper error handling

**Expected Results:**
- ✓ API client creates without crashing
- ✓ Authentication fails as expected
- ✓ Error message indicates credential problem
- ✓ Can recover and retry with correct credentials

#### Test 4.3: Invalid Stream Keys
**Function:** `test_invalid_stream_keys()`

**What it tests:**
1. Create channel with obviously invalid stream key
2. Start process (will fail to connect to destination)
3. Check process state and logs
4. Verify process handles connection failure gracefully

**Expected Results:**
- ✓ Channel creates with invalid key
- ✓ Process starts but fails to connect
- ✓ Logs show connection errors
- ✓ Process can be stopped cleanly
- ✓ No crashes or resource leaks

#### Test 4.4: Invalid Input URL
**Function:** `test_invalid_input_url()`

**What it tests:**
1. Create channel with non-existent input URL
2. Attempt to start process
3. Verify process handles missing input gracefully
4. Check state shows no input data (FPS = 0)

**Expected Results:**
- ✓ Channel creates with invalid input
- ✓ Process may start but receives no input
- ✓ FPS remains 0 or very low
- ✓ Process doesn't crash waiting for input
- ✓ Can be stopped cleanly

---

### Suite 5: Persistence Scenarios (3 tests)

Tests that configuration and state persist correctly.

#### Test 5.1: Channel Persistence
**Function:** `test_channel_persistence()`

**What it tests:**
1. Create and start channel
2. Get process details
3. Stop and restart (simulating app restart)
4. Verify process ID and configuration persist
5. Verify channel can restart successfully

**Expected Results:**
- ✓ Channel creates with unique ID
- ✓ Process ID remains same after stop/start
- ✓ Configuration persists across restart
- ✓ Channel can be restarted multiple times

#### Test 5.2: Stopped State Reload
**Function:** `test_stopped_state_reload()`

**What it tests:**
1. Create and start channel
2. Verify running state
3. Stop channel
4. Reload state multiple times
5. Verify stopped state persists correctly

**Expected Results:**
- ✓ Channel starts successfully
- ✓ Running state is accurate
- ✓ Stopped state is accurate after stop
- ✓ State remains consistent on reload
- ✓ Process exists but is not running

#### Test 5.3: Outputs Persistence
**Function:** `test_outputs_persistence()`

**What it tests:**
1. Create channel with 3 outputs
2. Verify output count
3. Start, run, then stop
4. Reload and verify outputs persist
5. Verify output configuration unchanged

**Expected Results:**
- ✓ All 3 outputs created initially
- ✓ Outputs remain after stop
- ✓ Output count correct on reload
- ✓ Output configurations unchanged
- ✓ Outputs can be reactivated after reload

---

## Running the Tests

### Quick Start

```bash
# Navigate to E2E test directory
cd tests/e2e

# Run the test suite
./run-streaming-e2e-tests.sh
```

### Build and Run

```bash
# From project root
cd obs-polyemesis

# Build the test binary
mkdir -p build && cd build
cmake ..
make test_streaming_e2e

# Run the tests
../tests/e2e/run-streaming-e2e-tests.sh
```

### Custom Configuration

```bash
# Run with custom server
RESTREAMER_URL=my-server.com \
RESTREAMER_USER=myuser \
RESTREAMER_PASS=mypass \
./run-streaming-e2e-tests.sh

# Run with custom build directory
BUILD_DIR=/path/to/build ./run-streaming-e2e-tests.sh
```

### Direct Binary Execution

```bash
# Run test binary directly
cd build
./tests/test_streaming_e2e
```

---

## Expected Test Output

### Successful Run

```
========================================================================
 OBS Polyemesis - Comprehensive E2E Streaming Tests
========================================================================

Date: Mon Dec  2 22:00:00 PST 2024
Host: test-machine

========================================================================
 Checking Prerequisites
========================================================================

[✓] Test binary found: ../../build/tests/test_streaming_e2e
[✓] Test binary is executable
[i] Checking connectivity to rs2.rainmanjam.com:443...
[✓] Server is reachable

========================================================================
 Running E2E Streaming Tests
========================================================================

[i] Test Configuration:
  Server:   rs2.rainmanjam.com:443
  Username: admin
  Protocol: HTTPS

[i] Starting test execution...

========================================================================
TEST SUITE: Comprehensive End-to-End Streaming Tests
========================================================================

========================================================================
 TEST CONFIGURATION
========================================================================
Server:   rs2.rainmanjam.com:443
Protocol: HTTPS
Username: admin
========================================================================

[SETUP] Connecting to Restreamer server...
    [OK] Connected to Restreamer server
[SETUP] Connection successful

========================================================================
 TEST SUITE 1: Complete Streaming Workflow
========================================================================

[TEST] Running: 1.1: Basic channel lifecycle (create → start → stop → delete)
    Creating channel: basic_workflow_1701547234_5432
    Starting stream...
    Stream status - Running: YES, FPS: 29.97, Bitrate: 2500 kbps
    Stopping stream...
    Deleting channel...
[PASS] 1.1: Basic channel lifecycle (create → start → stop → delete)

[TEST] Running: 1.2: Multi-destination streaming workflow
    Creating multi-destination channel: multi_dest_1701547242_8721
    Starting multi-destination stream...
    Active outputs: 4
      [0] output_0
      [1] output_1
      [2] output_2
      [3] output_3
    Stopping multi-destination stream...
[PASS] 1.2: Multi-destination streaming workflow

[TEST] Running: 1.3: Custom encoding settings workflow
    Creating channel with custom encoding: encoding_1701547250_1234
    Video filter present in config: YES
    Starting stream with custom encoding...
[PASS] 1.3: Custom encoding settings workflow

========================================================================
 TEST SUITE 2: Multi-Channel Scenarios
========================================================================

[TEST] Running: 2.1: Start multiple channels simultaneously
    Creating 3 channels...
      Created: multi_chan_0_1701547258_5678
      Created: multi_chan_1_1701547258_9012
      Created: multi_chan_2_1701547258_3456
    Starting all 3 channels...
      Started: multi_chan_0_1701547258_5678
      Started: multi_chan_1_1701547258_9012
      Started: multi_chan_2_1701547258_3456
    Verifying all channels are running...
      [0] multi_chan_0_1701547258_5678 - Running: YES
      [1] multi_chan_1_1701547258_9012 - Running: YES
      [2] multi_chan_2_1701547258_3456 - Running: YES
    Stopping all channels...
[PASS] 2.1: Start multiple channels simultaneously

[... additional test output ...]

========================================================================
TEST SUMMARY
========================================================================
Total:   16
Passed:  16
Failed:  0
Crashed: 0
Skipped: 0
========================================================================
Result: PASSED
========================================================================

[CLEANUP] Removing 16 test processes...
  Cleaning up: basic_workflow_1701547234_5432
  Cleaning up: multi_dest_1701547242_8721
  [... additional cleanup ...]

[TEARDOWN] Disconnecting from server...
[TEARDOWN] Complete

========================================================================
 Test Execution Complete
========================================================================
[✓] All E2E streaming tests passed!
```

---

## Test Features

### Automatic Cleanup
- All test processes are tracked and cleaned up automatically
- Cleanup occurs even if tests fail
- Manual cleanup script available if needed

### Comprehensive Coverage
- **16 tests** covering all major workflows
- **5 test suites** organized by functionality
- Real-world streaming scenarios
- Error handling and edge cases
- State persistence verification

### Helper Functions
- `setup_api_client()` - Initialize and authenticate
- `cleanup_api_client()` - Clean disconnect
- `generate_process_id()` - Unique process IDs
- `register_test_process()` - Track for cleanup
- `cleanup_test_process()` - Remove single process
- `cleanup_all_test_processes()` - Batch cleanup
- `wait_for_process_state()` - Poll for state
- `wait_for_running()` - Wait for running state
- `verify_process_exists()` - Check existence

### Robust Error Handling
- Timeout protection for all operations
- Graceful handling of API limitations
- Connection verification before tests
- Cleanup on test failure
- Informative error messages

---

## Requirements

### Server Requirements
- Live Restreamer server running and accessible
- Valid credentials configured
- Network connectivity to server
- HTTPS/TLS support
- Sufficient server resources for concurrent tests

### Build Dependencies
- libcurl (HTTP/HTTPS requests)
- jansson (JSON parsing)
- C compiler (GCC/Clang/MSVC)
- CMake 3.16 or higher

### Runtime Requirements
- Network access to rs2.rainmanjam.com:443
- Permissions to create/delete processes on Restreamer
- Sufficient bandwidth for streaming tests (minimal actual data)
- 2-5 minutes for full test suite execution

---

## Troubleshooting

### Connection Failures

**Error:**
```
[ERROR] Failed to connect to Restreamer server
```

**Solutions:**
1. Verify server URL: `ping rs2.rainmanjam.com`
2. Check port accessibility: `nc -zv rs2.rainmanjam.com 443`
3. Verify credentials are correct
4. Check firewall/proxy settings
5. Confirm HTTPS certificate is valid

### Test Timeouts

**Error:**
```
[FAIL] Should get process state
```

**Solutions:**
1. Increase `TEST_TIMEOUT_MS` in source file (currently 15000ms)
2. Check server load and performance
3. Verify network latency: `ping rs2.rainmanjam.com`
4. Run fewer concurrent tests
5. Check for rate limiting

### Process Not Starting

**Error:**
```
[FAIL] Process should be running
```

**Solutions:**
1. Check Restreamer server logs
2. Verify input URL is accessible
3. Confirm FFmpeg is available on server
4. Check server resource limits (CPU, memory)
5. Review process configuration in UI

### Build Failures

**Error:**
```
Test binary not found
```

**Solutions:**
```bash
# Clean and rebuild
cd build
rm -rf *
cmake ..
make test_streaming_e2e

# Check for errors
make VERBOSE=1 test_streaming_e2e
```

---

## Manual Cleanup

If tests fail and processes remain on the server:

### Using curl

```bash
# List all processes
curl -u admin:tenn2jagWEE@##$ \
  https://rs2.rainmanjam.com/api/v3/process

# Delete specific test process
curl -X DELETE -u admin:tenn2jagWEE@##$ \
  https://rs2.rainmanjam.com/api/v3/process/e2e_test_123
```

### Using Restreamer UI

1. Navigate to https://rs2.rainmanjam.com
2. Login with credentials
3. Go to Processes page
4. Find processes starting with `basic_workflow_`, `multi_`, etc.
5. Delete each test process

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: E2E Streaming Tests

on: [push, pull_request]

jobs:
  e2e-streaming:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libcurl4-openssl-dev libjansson-dev

      - name: Build tests
        run: |
          mkdir build && cd build
          cmake ..
          make test_streaming_e2e

      - name: Run E2E tests
        env:
          RESTREAMER_URL: ${{ secrets.RESTREAMER_URL }}
          RESTREAMER_USER: ${{ secrets.RESTREAMER_USER }}
          RESTREAMER_PASS: ${{ secrets.RESTREAMER_PASS }}
        run: |
          cd tests/e2e
          ./run-streaming-e2e-tests.sh
        timeout-minutes: 10

      - name: Upload test logs on failure
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: test-logs
          path: build/Testing/Temporary/
```

---

## Best Practices

### When Running Tests

1. **Clean environment** - Ensure no leftover test processes
2. **Stable network** - Run on reliable network connection
3. **Monitor server** - Check server health before running
4. **Review logs** - Always check output for warnings
5. **Cleanup verification** - Confirm all test processes removed

### When Writing New Tests

1. **Use helper functions** - Leverage existing test infrastructure
2. **Register processes** - Always register for cleanup
3. **Unique IDs** - Use `generate_process_id()` for uniqueness
4. **Verify cleanup** - Ensure test cleans up on success AND failure
5. **Independent tests** - Don't depend on other test state
6. **Meaningful assertions** - Clear error messages
7. **Document behavior** - Explain what test validates

### When Debugging Failures

1. **Check logs first** - Test output shows detailed information
2. **Verify server** - Ensure server is operational
3. **Run single test** - Isolate failing test
4. **Check network** - Verify connectivity is stable
5. **Review server UI** - Check process state in Restreamer
6. **Manual cleanup** - Remove test processes if needed

---

## Test Comparison

### vs. test_streaming_workflow.c

The original `test_streaming_workflow.c` has 6 tests focused on:
- Basic lifecycle
- Multistream
- Failover
- Live output management
- Encoding settings
- Reconnection

This new `test_streaming_e2e.c` has 16 tests with expanded coverage:
- ✓ All original scenarios
- ✓ Multi-channel concurrent scenarios
- ✓ Independent channel management
- ✓ Error scenario testing
- ✓ Persistence verification
- ✓ Invalid input handling
- ✓ More comprehensive assertions

---

## Related Documentation

- [E2E Test Framework](README.md) - Platform E2E testing
- [Streaming Workflow Tests](STREAMING_WORKFLOW_TESTS.md) - Original E2E tests
- [Restreamer API](../../src/restreamer-api.h) - API reference
- [Test Framework](../test_framework.h) - Test utilities

---

## Maintenance

### Updating Server Configuration

Edit constants in `test_streaming_e2e.c`:

```c
#define TEST_SERVER_URL "your-server.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "your-username"
#define TEST_SERVER_PASSWORD "your-password"
```

Or use environment variables in the run script.

### Adding New Tests

1. Write test function following the pattern:
```c
static bool test_your_new_test(void)
{
    char process_id[128];
    generate_process_id(process_id, sizeof(process_id), "your_test");
    register_test_process(process_id);

    // Test implementation

    return true;
}
```

2. Add to appropriate test suite:
```c
RUN_TEST(test_your_new_test, "X.X: Your test description");
```

3. Update documentation with test details

---

## License

Same as parent project (GPL-2.0)

## Support

For issues or questions:
1. Check this documentation
2. Review test output logs
3. Check Restreamer server logs
4. Open an issue on GitHub

---

**Last Updated:** December 2, 2024
**Version:** 1.0
**Test Count:** 16 tests across 5 suites
