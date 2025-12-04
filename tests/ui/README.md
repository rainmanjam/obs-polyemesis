# UI Workflow Tests

This directory contains tests for UI operations in the obs-polyemesis plugin.

## Overview

Since Qt UI tests require special setup and can be difficult to run in automated environments, these tests verify UI functionality by:

1. **Testing Backend Functions**: The C test suite (`test_ui_workflows.c`) tests the backend functions that UI components call
2. **Verifying State Changes**: Tests verify that state changes triggered by UI actions work correctly
3. **API Simulation**: The bash script (`/tmp/test_ui_workflows.sh`) simulates UI operations by calling the API directly

## Test Coverage

### 1. Channel UI Operations
- ✅ Channel creation through UI flow
- ✅ Channel editing (name, input URL, settings)
- ✅ Channel deletion via context menu
- ✅ Start/Stop button functionality
- ✅ Context menu operations (duplicate, restart)
- ✅ Preview mode (start, go live, cancel)

### 2. Output UI Operations
- ✅ Adding output through dialog
- ✅ Editing output settings (encoding, stream key)
- ✅ Deleting output via context menu
- ✅ Enable/disable toggle
- ✅ Stream key validation

### 3. Dock UI Operations
- ✅ Connection configuration dialog
- ✅ Connect/disconnect flow
- ✅ Start All / Stop All buttons
- ✅ Channel list updates
- ✅ Status indicators

### 4. Error Handling
- ✅ UI response to invalid server connection
- ✅ UI response to invalid credentials
- ✅ UI response to invalid input
- ✅ UI state after failed operations

## Running the Tests

### C Test Suite (Backend Functions)

```bash
# Build the test
cd /Users/rainmanjam/Documents/GitHub/obs-polyemesis
mkdir -p build
cd build
cmake ..
make test_ui_workflows

# Run the test
./tests/ui/test_ui_workflows
```

### Bash Script (API Simulation)

```bash
# Run the script
/tmp/test_ui_workflows.sh
```

The bash script will:
1. Connect to the Restreamer server
2. Simulate various UI workflows
3. Verify expected behavior
4. Clean up test resources
5. Display a summary of results

## Test Server

Tests use the live Restreamer server:
- **URL**: https://rs2.rainmanjam.com
- **Username**: admin
- **Password**: tenn2jagWEE@##$

## Test Architecture

### C Tests (`test_ui_workflows.c`)

These tests exercise the same code paths that the Qt UI uses:

```c
// Example: Channel creation flow
stream_channel_t *channel = channel_manager_create_channel(manager, "My Channel");
// ^ This is exactly what ChannelEditDialog calls when user clicks "Save"

// Example: Start button flow
bool started = channel_start(manager, channel->channel_id);
// ^ This is exactly what happens when user clicks "Start" button
```

### Bash Tests (`test_ui_workflows.sh`)

These tests simulate complete user workflows:

```bash
# Simulates: User opens Create Channel dialog, fills form, clicks Save
api_create_process "$CHANNEL_ID" "$OUTPUT_URL"

# Simulates: User clicks Start button
api_start_process "$CHANNEL_ID"

# Simulates: User clicks Stop button
api_stop_process "$CHANNEL_ID"
```

## Test Scenarios

### Scenario 1: Create and Start Channel
```
1. User clicks "Create Channel"
2. User enters channel name
3. User adds YouTube output
4. User enters stream key
5. User clicks "Save"
6. User clicks "Start" button
7. Verify channel is streaming
```

### Scenario 2: Edit Running Channel
```
1. User right-clicks running channel
2. User selects "Edit Channel"
3. User changes stream key
4. User clicks "Save"
5. Verify changes are applied
```

### Scenario 3: Multi-Output Streaming
```
1. User creates channel with YouTube output
2. User clicks "Add Output"
3. User adds Twitch output
4. User clicks "Start All"
5. Verify both outputs are streaming
```

### Scenario 4: Preview Mode
```
1. User right-clicks channel
2. User selects "Start Preview (60s)"
3. Verify preview mode is active
4. User clicks "Go Live"
5. Verify channel transitions to live
```

### Scenario 5: Error Handling
```
1. User enters invalid server URL
2. User clicks "Test Connection"
3. Verify error message is displayed
4. User corrects URL
5. Verify connection succeeds
```

## Implementation Notes

### Why Not Full Qt UI Tests?

Full Qt UI tests (QTest framework) require:
- X11/Wayland display server (for widget rendering)
- Qt Test framework integration
- Mock OBS environment
- Complex setup for CI/CD

Instead, we test:
- The backend logic that UI calls
- State changes that UI triggers
- API interactions that UI performs

This provides excellent coverage without the complexity of widget testing.

### What UI Code is NOT Tested?

These tests do NOT verify:
- Widget rendering
- Layout positioning
- Mouse click handling
- Keyboard shortcuts
- Visual appearance
- Theme compatibility

These aspects should be tested manually during UI development.

### What IS Tested?

These tests DO verify:
- Channel creation/deletion logic
- Output management logic
- Start/stop operations
- State transitions
- Error handling
- API communication
- Data validation
- Multi-channel operations

## Adding New Tests

To add a new UI workflow test:

### In C (`test_ui_workflows.c`):

```c
static bool test_ui_new_workflow(void)
{
    print_test("UI Workflow: New Feature");

    // 1. Setup test data
    stream_channel_t *channel = channel_manager_create_channel(g_manager, "Test");

    // 2. Call backend functions (as UI would)
    bool result = some_backend_function(channel);

    // 3. Verify expected behavior
    ASSERT_TRUE(result, "Function should succeed");

    // 4. Return success
    return true;
}

// Add to test suite:
RUN_TEST(test_ui_new_workflow, "UI Workflow: New feature description");
```

### In Bash (`test_ui_workflows.sh`):

```bash
function test_new_workflow() {
    print_test "UI Workflow: New Feature"

    # Simulate user actions
    print_info "User clicks button X"
    print_info "User enters value Y"

    # Call API
    if api_some_operation "$PARAM"; then
        test_passed "New workflow completed"
    else
        test_failed "New workflow failed"
    fi
}

# Add to main():
test_new_workflow
```

## Debugging Failed Tests

### C Tests

If a C test fails:

1. Check the assertion message for details
2. Run with verbose logging:
   ```bash
   VERBOSE=1 ./tests/ui/test_ui_workflows
   ```
3. Use GDB for debugging:
   ```bash
   gdb ./tests/ui/test_ui_workflows
   (gdb) run
   (gdb) bt  # backtrace on crash
   ```

### Bash Tests

If a bash test fails:

1. Check the curl response:
   ```bash
   bash -x /tmp/test_ui_workflows.sh  # Run with tracing
   ```
2. Manually test the API endpoint:
   ```bash
   curl -v https://rs2.rainmanjam.com/api/v3/about
   ```
3. Check server logs on the Restreamer instance

## Related Files

- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/restreamer-dock.cpp` - Main dock UI implementation
- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/channel-widget.cpp` - Channel widget UI
- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/output-widget.cpp` - Output widget UI
- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/channel-edit-dialog.cpp` - Channel edit dialog
- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/connection-config-dialog.cpp` - Connection config dialog
- `/Users/rainmanjam/Documents/GitHub/obs-polyemesis/src/restreamer-channel.c` - Channel management backend

## Future Enhancements

- [ ] Add performance benchmarks for UI operations
- [ ] Add stress tests (many channels, rapid operations)
- [ ] Add memory leak detection for UI workflows
- [ ] Add integration tests with real OBS instance
- [ ] Add automated screenshot tests for visual regression
- [ ] Add accessibility tests (keyboard navigation, screen readers)

## Contributing

When adding new UI features:

1. Write tests FIRST (TDD approach)
2. Test both success and failure cases
3. Test edge cases (empty input, invalid data)
4. Update this README with new test coverage
5. Ensure all tests pass before committing

## License

Same as obs-polyemesis plugin (GPL-2.0)
