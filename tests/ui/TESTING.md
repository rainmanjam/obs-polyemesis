# UI Workflow Testing Guide

## Overview

This document describes how to test UI workflows in the obs-polyemesis plugin. Since full Qt UI tests are complex and require special setup, we use a hybrid approach:

1. **Backend Function Tests** (C) - Test the logic that UI calls
2. **API Simulation Tests** (Bash) - Simulate complete user workflows via API

## Quick Start

### Run C Tests (Backend Functions)

```bash
cd /Users/rainmanjam/Documents/GitHub/obs-polyemesis
./build.sh
cd build
ctest -R ui_workflows -V
```

### Run Bash Tests (API Simulation)

```bash
/tmp/test_ui_workflows.sh
```

## Test Files

### 1. C Test Suite

**File**: `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/tests/ui/test_ui_workflows.c`

**Purpose**: Tests backend functions that UI components invoke

**Tests**:
- Channel creation flow (14 tests)
- Channel editing flow
- Channel deletion flow
- Channel start/stop flow
- Output addition flow
- Output editing flow
- Output deletion flow
- Output enable/disable flow
- Connection configuration flow
- Start All / Stop All flow
- Error handling (invalid input, server errors)
- Context menu operations
- Preview mode operations

**Build Command**:
```bash
cd build
make test_ui_workflows
```

**Run Command**:
```bash
./tests/ui/test_ui_workflows
```

### 2. Bash Test Script

**File**: `/tmp/test_ui_workflows.sh`

**Purpose**: Simulates complete UI workflows by calling the API

**Tests**:
- Connection config dialog flow
- Channel create flow
- Channel edit flow
- Channel start/stop flow
- Channel delete flow
- Multi-destination streaming
- Output enable/disable
- Start All / Stop All
- Preview mode operations
- Error handling (invalid server, credentials)

**Run Command**:
```bash
bash /tmp/test_ui_workflows.sh
```

**Run with Verbose Output**:
```bash
bash -x /tmp/test_ui_workflows.sh
```

## Test Coverage Matrix

| UI Component | Operation | C Test | Bash Test | Manual Test Required |
|--------------|-----------|--------|-----------|---------------------|
| **Connection Dialog** | | | | |
| | Test Connection | ✅ | ✅ | Layout/Appearance |
| | Save Settings | ✅ | ✅ | Layout/Appearance |
| | Error Display | ✅ | ✅ | Layout/Appearance |
| **Channel Widget** | | | | |
| | Create Channel | ✅ | ✅ | Layout/Appearance |
| | Edit Channel | ✅ | ✅ | Layout/Appearance |
| | Delete Channel | ✅ | ✅ | Confirmation Dialog |
| | Start/Stop Button | ✅ | ✅ | Button State |
| | Status Indicator | ✅ | ✅ | Color/Animation |
| | Expand/Collapse | ❌ | ❌ | **Manual** |
| | Context Menu | ✅ | ✅ | Menu Display |
| | Duplicate | ✅ | ✅ | Layout |
| | Hover State | ❌ | ❌ | **Manual** |
| **Output Widget** | | | | |
| | Add Output | ✅ | ✅ | Dialog Display |
| | Edit Output | ✅ | ✅ | Dialog Display |
| | Delete Output | ✅ | ✅ | Confirmation |
| | Enable Toggle | ✅ | ✅ | Toggle State |
| | Status Indicator | ✅ | ✅ | Color Display |
| | Stats Display | ✅ | ✅ | Formatting |
| **Dock** | | | | |
| | Connection Status | ✅ | ✅ | Indicator Color |
| | Channel List | ✅ | ✅ | List Rendering |
| | Start All Button | ✅ | ✅ | Button State |
| | Stop All Button | ✅ | ✅ | Button State |
| | Create Channel Btn | ✅ | ✅ | Button State |
| | Dock Resize | ❌ | ❌ | **Manual** |
| | Dock Float | ❌ | ❌ | **Manual** |
| **Preview Mode** | | | | |
| | Start Preview | ✅ | ✅ | UI Feedback |
| | Go Live | ✅ | ✅ | UI Transition |
| | Cancel Preview | ✅ | ✅ | UI Reset |
| | Duration Timer | ✅ | ✅ | Timer Display |

Legend:
- ✅ = Automated test coverage
- ❌ = No automated test (requires manual testing)

## Testing Scenarios

### Scenario 1: First Time User Setup

```
1. User opens OBS
2. User opens Polyemesis dock
3. User sees "Not Connected" indicator
4. User clicks connection config button
5. User enters server details
6. User clicks "Test Connection"
   → EXPECT: Success message
7. User clicks "Save"
   → EXPECT: Connected indicator
```

**Test Coverage**:
- C Test: `test_ui_connection_config_flow()`
- Bash Test: `test_connection_config_dialog()`

### Scenario 2: Create First Channel

```
1. User clicks "Create Channel"
2. User enters channel name: "My First Stream"
3. User adds YouTube output
4. User enters stream key
5. User sets resolution to 1920x1080
6. User clicks "Save"
   → EXPECT: Channel appears in list
7. User clicks "Start" button
   → EXPECT: Channel status shows "Streaming"
```

**Test Coverage**:
- C Test: `test_ui_channel_create_flow()`, `test_ui_output_add_flow()`, `test_ui_channel_start_stop_flow()`
- Bash Test: `test_channel_create_flow()`, `test_channel_start_stop_flow()`

### Scenario 3: Edit Existing Channel

```
1. User right-clicks channel
2. User selects "Edit Channel"
3. User changes stream key
4. User changes bitrate to 6000
5. User clicks "Save"
   → EXPECT: Changes applied
6. User restarts channel
   → EXPECT: New settings active
```

**Test Coverage**:
- C Test: `test_ui_channel_edit_flow()`, `test_ui_output_edit_flow()`
- Bash Test: `test_channel_edit_flow()`

### Scenario 4: Multi-Platform Streaming

```
1. User creates channel
2. User adds YouTube output
3. User adds Twitch output
4. User adds Facebook output
5. User clicks "Start"
   → EXPECT: All 3 outputs streaming
6. User checks status indicators
   → EXPECT: All show "Active"
```

**Test Coverage**:
- C Test: `test_ui_output_add_flow()` (multiple outputs)
- Bash Test: `test_multistream_flow()`

### Scenario 5: Preview Mode Testing

```
1. User right-clicks channel
2. User selects "Start Preview (60s)"
   → EXPECT: Preview mode active
3. User verifies stream quality
4. User clicks "Go Live"
   → EXPECT: Transitions to live mode
```

**Test Coverage**:
- C Test: `test_ui_preview_mode_flow()`
- Bash Test: `test_preview_mode_flow()`

### Scenario 6: Error Handling

```
1. User enters invalid server URL
2. User clicks "Test Connection"
   → EXPECT: Error message displayed
3. User enters invalid stream key
4. User clicks "Start"
   → EXPECT: Appropriate error shown
5. Network disconnects during stream
   → EXPECT: Reconnect attempt shown
```

**Test Coverage**:
- C Test: `test_ui_error_invalid_input()`, `test_ui_error_server_unavailable()`
- Bash Test: `test_error_handling_invalid_server()`, `test_error_handling_invalid_credentials()`

## Running Tests

### All UI Tests

```bash
# Run all UI-related tests
cd build
ctest -L ui -V
```

### Individual Test Suites

```bash
# Backend function tests only
./tests/ui/test_ui_workflows

# API simulation tests only
/tmp/test_ui_workflows.sh

# Specific test pattern
ctest -R ui_workflows -V
```

### With Debugging

```bash
# Run with GDB
gdb ./tests/ui/test_ui_workflows
(gdb) run
(gdb) bt  # on crash

# Run bash script with tracing
bash -x /tmp/test_ui_workflows.sh

# Run with verbose logging
VERBOSE=1 ./tests/ui/test_ui_workflows
```

## Interpreting Results

### C Test Output

```
[TEST] Running: UI Workflow: Channel creation through UI
[PASS] UI Workflow: Channel creation through UI

[TEST] Running: UI Workflow: Channel editing
[FAIL] test_ui_workflows.c:234: Channel name should be updated

TEST SUMMARY
============
Total:   14
Passed:  13
Failed:  1
Crashed: 0
```

### Bash Test Output

```
[TEST] UI Workflow: Channel Creation
    User clicks 'Create Channel' button
    User fills dialog: Name='ui_test_channel_1234'
    User clicks 'Save'
    Channel created successfully
[PASS] Channel creation flow completed

[FAIL] Failed to start channel
```

## Common Issues

### Issue: Connection Timeout

**Symptom**: Tests fail with "connection timeout"

**Solution**:
1. Verify server is running: `curl https://rs2.rainmanjam.com/api/v3/about`
2. Check network connectivity
3. Increase timeout in test

### Issue: Stream Key Invalid

**Symptom**: Channel starts but immediately stops

**Solution**:
1. Verify stream key format
2. Check service-specific requirements
3. Test with known-good key

### Issue: Memory Leaks

**Symptom**: Tests report memory leaks

**Solution**:
1. Run with AddressSanitizer: `ENABLE_ASAN=1 ./build.sh`
2. Check cleanup code in tests
3. Verify all malloc/free pairs

## Test Data

### Test Server
- **URL**: https://rs2.rainmanjam.com
- **Port**: 443
- **Protocol**: HTTPS
- **Username**: admin
- **Password**: tenn2jagWEE@##$

### Test URLs
- **Input**: rtmp://localhost:1935/live/test
- **YouTube**: rtmp://a.rtmp.youtube.com/live2/
- **Twitch**: rtmp://live.twitch.tv/app/
- **Facebook**: rtmps://live-api-s.facebook.com:443/rtmp/

## Adding New Tests

### Add C Test

1. Open `tests/ui/test_ui_workflows.c`
2. Add test function:

```c
static bool test_ui_new_feature(void)
{
    print_test("UI Workflow: New Feature");

    // Setup
    stream_channel_t *channel =
        channel_manager_create_channel(g_manager, "Test");

    // Action
    bool result = new_feature_function(channel);

    // Verify
    ASSERT_TRUE(result, "Feature should work");

    return true;
}
```

3. Add to test suite:

```c
RUN_TEST(test_ui_new_feature, "UI Workflow: New feature");
```

### Add Bash Test

1. Open `/tmp/test_ui_workflows.sh`
2. Add test function:

```bash
function test_new_feature() {
    print_test "UI Workflow: New Feature"

    # Simulate UI actions
    print_info "User clicks button X"

    # Call API
    if api_new_operation "$PARAM"; then
        test_passed "New feature works"
    else
        test_failed "New feature failed"
    fi
}
```

3. Add to main():

```bash
test_new_feature
```

## CI/CD Integration

### GitHub Actions

```yaml
- name: Run UI Tests
  run: |
    cd build
    ctest -R ui_workflows -V
```

### Local Pre-Commit

```bash
#!/bin/bash
# .git/hooks/pre-commit

cd build
ctest -R ui_workflows --output-on-failure
if [ $? -ne 0 ]; then
    echo "UI tests failed!"
    exit 1
fi
```

## Performance Benchmarks

Expected test execution times:

| Test Suite | Expected Time | Timeout |
|------------|--------------|---------|
| C Tests (all) | 60-90s | 300s |
| Bash Tests (all) | 30-60s | 120s |
| Single test | 2-5s | 30s |

If tests take significantly longer, investigate:
- Network latency to server
- Server load
- System resource constraints

## Troubleshooting

### Tests Won't Compile

```bash
# Check dependencies
cmake --build build --target test_ui_workflows --verbose

# Verify OBS headers
find /Applications/OBS.app -name "obs.h"
```

### Tests Crash

```bash
# Run with crash handler
./tests/ui/test_ui_workflows

# Run with GDB
gdb ./tests/ui/test_ui_workflows
(gdb) run
(gdb) bt
```

### Server Not Available

```bash
# Check server status
curl -v https://rs2.rainmanjam.com/api/v3/about

# Use alternate server (edit test files)
TEST_SERVER_URL="your-server.com"
```

## Related Documentation

- [README.md](README.md) - Test overview
- [Main README](/Users/rainmanjam/Documents/GitHub/obs-polyemesis/README.md) - Plugin documentation
- [Test Framework](/Users/rainmanjam/Documents/GitHub/obs-polyemesis/tests/test_framework.h) - Assertion macros

## Support

For issues or questions:

1. Check existing test output
2. Review test logs in build/Testing/Temporary/
3. Run with verbose output
4. Check server logs if using live server
5. File issue with test output attached
