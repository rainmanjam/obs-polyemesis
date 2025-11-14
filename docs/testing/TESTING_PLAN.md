# OBS Polyemesis - Comprehensive Testing Plan

## Overview
This document outlines comprehensive testing workflows to ensure full Restreamer create/monitor/control functionality.

---

## Test Environment Setup

### Prerequisites
- **OBS Studio**: Latest stable version (30.x+)
- **Restreamer Server**: datarhei/core v3.x running locally or remote
- **Test Accounts**: Valid stream keys for at least 2 platforms (Twitch, YouTube, etc.)
- **Network**: Stable internet connection for streaming tests

### Restreamer Server Configuration
```bash
# Option 1: Docker local server
docker run -d \
  --name restreamer \
  -p 8080:8080 \
  -p 1935:1935 \
  -p 6000:6000/udp \
  datarhei/restreamer:latest

# Option 2: Use existing Restreamer server
# Configure connection details: host, port, username, password
```

---

## Phase 1: Connection & Authentication Tests

### Test 1.1: Initial Connection
**Objective**: Verify plugin can connect to Restreamer server

**Steps**:
1. Open OBS Studio
2. Navigate to View → Docks → Restreamer Dock
3. In "Connection" tab, enter:
   - Host: `localhost` (or server IP)
   - Port: `8080`
   - HTTPS: Unchecked (for local) or Checked (for production)
   - Username: `admin`
   - Password: (your password)
4. Click "Test" button

**Expected Results**:
- ✅ Connection status changes to "● Connected" (green)
- ✅ Status message displays server info
- ✅ No errors in OBS log

**Log Verification**:
```bash
grep -i "polyemesis.*login\|connected" ~/Library/Application\ Support/obs-studio/logs/*.txt
```

### Test 1.2: Invalid Credentials
**Objective**: Verify proper error handling for auth failures

**Steps**:
1. Enter incorrect password
2. Click "Test" button

**Expected Results**:
- ❌ Connection status shows "● Authentication failed" (red)
- ❌ Error message displayed to user
- ✅ Plugin remains functional (no crash)

### Test 1.3: Connection Persistence
**Objective**: Verify connection settings persist across OBS restarts

**Steps**:
1. Connect successfully
2. Quit OBS
3. Restart OBS
4. Check connection status

**Expected Results**:
- ✅ Connection credentials remembered
- ✅ Auto-reconnect on startup (if enabled)

### Test 1.4: Network Failure Recovery
**Objective**: Test plugin behavior during network issues

**Steps**:
1. Connect successfully
2. Disconnect network OR stop Restreamer server
3. Wait 30 seconds
4. Restore network OR restart Restreamer

**Expected Results**:
- ⚠️ Connection status shows "● Disconnected"
- ✅ Plugin doesn't crash
- ✅ Auto-reconnect works when network restored
- ✅ Error messages are user-friendly

---

## Phase 2: Profile Management Tests

### Test 2.1: Create New Profile
**Objective**: Verify profile creation workflow

**Steps**:
1. Click "Create Profile" button
2. Enter profile name: "Test_Profile_1"
3. Click OK

**Expected Results**:
- ✅ Profile appears in profile list
- ✅ Profile saved to settings
- ✅ Profile status shows "INACTIVE"
- ✅ Buttons enabled: Edit, Duplicate, Start

### Test 2.2: Configure Profile Basic Settings
**Objective**: Test profile configuration dialog

**Steps**:
1. Select "Test_Profile_1"
2. Click "Configure" button
3. Modify settings:
   - Profile Name: "Horizontal_Stream"
   - Source Orientation: Horizontal (16:9)
   - Auto-detect: Checked
   - Auto-start: Unchecked
   - Auto-reconnect: Checked
   - Input URL: `rtmp://localhost/live/obs_input`
4. Click "Save"

**Expected Results**:
- ✅ Settings saved successfully
- ✅ Profile name updated in list
- ✅ Changes persist after OBS restart

### Test 2.3: Add Multiple Destinations
**Objective**: Test multi-destination streaming

**Steps**:
1. Configure profile "Horizontal_Stream"
2. Click "Add Destination":
   - **Destination 1**:
     - Service: Twitch
     - Stream Key: (valid Twitch key)
     - Target Orientation: Horizontal
     - Enabled: ✓
   - **Destination 2**:
     - Service: YouTube
     - Stream Key: (valid YouTube key)
     - Target Orientation: Horizontal
     - Enabled: ✓
3. Save profile

**Expected Results**:
- ✅ Both destinations appear in destinations table
- ✅ Profile summary shows "2 destinations"
- ✅ Stream keys masked in UI (only show first 4 and last 4 chars)

### Test 2.4: Edit Existing Destination
**Objective**: Test destination modification

**Steps**:
1. Select destination in table
2. Click "Edit Destination"
3. Change target orientation to Vertical
4. Save changes

**Expected Results**:
- ✅ Destination updated successfully
- ✅ Changes reflected in table
- ✅ Profile marked as modified

### Test 2.5: Remove Destination
**Objective**: Test destination deletion

**Steps**:
1. Select destination
2. Click "Remove Destination"
3. Confirm deletion

**Expected Results**:
- ✅ Destination removed from table
- ✅ Profile destination count decremented
- ✅ No crashes or errors

### Test 2.6: Duplicate Profile
**Objective**: Test profile cloning

**Steps**:
1. Select "Horizontal_Stream"
2. Click "Duplicate"
3. Enter name: "Horizontal_Stream_Copy"

**Expected Results**:
- ✅ New profile created with all settings copied
- ✅ Destinations copied correctly
- ✅ New profile has unique ID

### Test 2.7: Delete Profile
**Objective**: Test profile deletion

**Steps**:
1. Select profile (must be INACTIVE)
2. Click "Delete"
3. Confirm deletion

**Expected Results**:
- ✅ Profile removed from list
- ✅ Settings file updated
- ✅ Cannot delete ACTIVE profile (button disabled)

### Test 2.8: Profile Context Menu
**Objective**: Test right-click menu

**Steps**:
1. Right-click on profile in list
2. Test each menu option

**Expected Results**:
- ✅ Context menu appears
- ✅ Menu items enabled/disabled based on profile status
- ✅ All actions work correctly

---

## Phase 3: Stream Control Tests

### Test 3.1: Start Single Profile
**Objective**: Verify profile can start streaming

**Pre-conditions**:
- OBS is streaming (to generate source content)
- Profile has at least 1 destination configured

**Steps**:
1. Start OBS streaming first
2. Select profile
3. Click "▶ Start" button
4. Monitor status for 30 seconds

**Expected Results**:
- ✅ Profile status changes: INACTIVE → STARTING → ACTIVE
- ✅ Start button disabled, Stop button enabled
- ✅ Process created on Restreamer server
- ✅ No errors in logs

**Verification Commands**:
```bash
# Check Restreamer processes
curl -u admin:password http://localhost:8080/api/v3/process | jq

# Check OBS logs
tail -f ~/Library/Application\ Support/obs-studio/logs/*.txt | grep polyemesis
```

### Test 3.2: Monitor Active Stream
**Objective**: Verify real-time monitoring works

**Steps**:
1. With stream active, observe metrics:
   - Process state (ACTIVE/STARTING/ERROR)
   - Uptime counter
   - CPU usage
   - Memory usage
   - Frames processed
   - Dropped frames
   - FPS
   - Bitrate
   - Progress indicator

**Expected Results**:
- ✅ All metrics update every 2-5 seconds
- ✅ Values are realistic (FPS ~30, bitrate matches settings)
- ✅ No negative values or errors

### Test 3.3: Stop Single Profile
**Objective**: Verify clean shutdown

**Steps**:
1. With profile ACTIVE
2. Click "■ Stop" button
3. Wait for state transition

**Expected Results**:
- ✅ Profile status: ACTIVE → STOPPING → INACTIVE
- ✅ Process removed from Restreamer
- ✅ Metrics cleared/reset
- ✅ No hanging processes

### Test 3.4: Start All Profiles
**Objective**: Test bulk start operation

**Steps**:
1. Create 3 profiles with different destinations
2. Click "▶ All" button
3. Monitor all profiles

**Expected Results**:
- ✅ All profiles start simultaneously
- ✅ Each profile transitions independently
- ✅ No race conditions or conflicts
- ✅ System remains responsive

### Test 3.5: Stop All Profiles
**Objective**: Test bulk stop operation

**Steps**:
1. With all profiles ACTIVE
2. Click "■ All" button
3. Wait for completion

**Expected Results**:
- ✅ All profiles stop cleanly
- ✅ All processes removed
- ✅ No orphaned streams

### Test 3.6: Restart Streaming (OBS Stop/Start)
**Objective**: Test profile behavior when OBS stops streaming

**Steps**:
1. Start OBS streaming
2. Start profile
3. Stop OBS streaming
4. Wait 10 seconds
5. Start OBS streaming again

**Expected Results**:
- ⚠️ Profile shows error or stops (no input source)
- ✅ Auto-reconnect triggers when OBS resumes
- ✅ Or profile remains stopped (depending on auto-reconnect setting)

### Test 3.7: Stream Quality Verification
**Objective**: Verify streams actually reach destinations

**Steps**:
1. Start profile with Twitch destination
2. Open Twitch dashboard
3. Check "Stream Manager" for live indicator
4. Verify video appears

**Expected Results**:
- ✅ Stream goes live on Twitch
- ✅ Video quality matches settings
- ✅ Audio present (if configured)
- ✅ No buffering or stuttering

---

## Phase 4: Error Handling Tests

### Test 4.1: Invalid Stream Key
**Objective**: Test handling of authentication failures at destination

**Steps**:
1. Configure destination with invalid stream key
2. Start profile
3. Monitor status

**Expected Results**:
- ❌ Profile shows ERROR status
- ❌ Error message indicates auth failure
- ✅ Plugin remains stable (no crash)
- ✅ Other destinations continue if valid

### Test 4.2: Network Interruption During Stream
**Objective**: Test resilience to network issues

**Steps**:
1. Start profile
2. Disable network for 10 seconds
3. Re-enable network

**Expected Results**:
- ⚠️ Profile shows error/disconnected
- ✅ Auto-reconnect attempts (if enabled)
- ✅ Stream resumes when network restored
- ✅ Or clean error state if auto-reconnect off

### Test 4.3: Restreamer Server Crash
**Objective**: Test plugin behavior when server goes down

**Steps**:
1. Start profile
2. Stop Restreamer server: `docker stop restreamer`
3. Wait 30 seconds
4. Restart server: `docker start restreamer`

**Expected Results**:
- ❌ Connection status shows disconnected
- ✅ Plugin doesn't crash
- ✅ Auto-reconnect when server returns
- ✅ Profiles show correct state after reconnect

### Test 4.4: OBS Crash During Active Stream
**Objective**: Test cleanup on unexpected exit

**Steps**:
1. Start profile
2. Force quit OBS (kill -9)
3. Restart OBS
4. Check Restreamer processes

**Expected Results**:
- ✅ Profile state restored correctly
- ✅ No orphaned processes on Restreamer
- ⚠️ Or orphaned process cleaned up after timeout

### Test 4.5: Invalid Input URL
**Objective**: Test handling of bad configuration

**Steps**:
1. Configure profile with invalid Input URL: `rtmp://invalid:1935/live/test`
2. Start profile

**Expected Results**:
- ❌ Profile shows ERROR status
- ❌ Error message: "Input source unreachable" or similar
- ✅ Plugin remains stable

### Test 4.6: Insufficient Bandwidth
**Objective**: Test behavior under network constraints

**Steps**:
1. Configure high bitrate (6000+ kbps)
2. Start stream on limited connection
3. Monitor dropped frames

**Expected Results**:
- ⚠️ High dropped frame count displayed
- ⚠️ Warning indicator (if implemented)
- ✅ Stream continues (degraded quality)
- ✅ Metrics accurately reflect issues

---

## Phase 5: Performance & Load Tests

### Test 5.1: Multiple Concurrent Profiles
**Objective**: Test system limits

**Steps**:
1. Create 5 profiles with 2 destinations each (10 streams total)
2. Start all profiles simultaneously
3. Monitor for 5 minutes

**Expected Results**:
- ✅ All streams start successfully
- ✅ CPU usage reasonable (<50%)
- ✅ No memory leaks
- ✅ UI remains responsive
- ✅ All streams stable

**Monitoring**:
```bash
# Monitor OBS CPU/memory
top -pid $(pgrep OBS)

# Monitor Restreamer processes
watch -n 1 'curl -s -u admin:password http://localhost:8080/api/v3/process | jq ".[].state"'
```

### Test 5.2: Long-Running Stream (24h)
**Objective**: Test stability over extended period

**Steps**:
1. Start profile
2. Leave running for 24 hours
3. Monitor metrics periodically

**Expected Results**:
- ✅ No crashes or hangs
- ✅ Memory usage stable (no leaks)
- ✅ Stream quality consistent
- ✅ Metrics continue updating

### Test 5.3: Rapid Start/Stop Cycles
**Objective**: Test state management under stress

**Steps**:
1. Create script to start/stop profile every 10 seconds
2. Run for 50 cycles (10 minutes)

**Expected Results**:
- ✅ All state transitions complete
- ✅ No race conditions
- ✅ No orphaned processes
- ✅ Plugin remains stable

---

## Phase 6: Integration Tests

### Test 6.1: OBS Profile Switching (Future Feature)
**Objective**: Verify settings follow OBS profile changes

**Steps**:
1. Configure plugin in OBS Profile 1
2. Switch to OBS Profile 2
3. Configure different settings
4. Switch back to Profile 1

**Expected Results**:
- ✅ Settings restore correctly for each OBS profile
- ✅ Active streams maintain state during switch

### Test 6.2: OBS Scene Changes
**Objective**: Test behavior during scene transitions

**Steps**:
1. Start profile with RTMP input from OBS
2. Change OBS scenes during stream

**Expected Results**:
- ✅ Stream continues without interruption
- ✅ Scene changes reflected in output
- ✅ No dropped frames during transition

### Test 6.3: WebSocket API (Future Feature)
**Objective**: Test remote control via obs-websocket

**Steps**:
1. Connect obs-websocket client
2. Send vendor request: `GetProfiles`
3. Send vendor request: `StartProfile {profileId: "..."}`
4. Send vendor request: `StopProfile {profileId: "..."}`

**Expected Results**:
- ✅ API responds with correct data
- ✅ Profile control works remotely
- ✅ Events emitted for state changes

---

## Phase 7: UI/UX Tests

### Test 7.1: Theme Compatibility
**Objective**: Verify UI in both light and dark themes

**Steps**:
1. Test in OBS Dark theme (default)
2. Switch to Light theme (Settings → General → Theme)
3. Verify all UI elements

**Expected Results**:
- ✅ All text readable (sufficient contrast)
- ✅ Buttons/inputs styled correctly
- ✅ Status colors visible
- ✅ No visual glitches

### Test 7.2: Window Resizing
**Objective**: Test responsive layout

**Steps**:
1. Resize OBS window to minimum size
2. Resize to maximum
3. Resize dock panel specifically

**Expected Results**:
- ✅ No content cutoff
- ✅ Scrollbars appear when needed
- ✅ Layout remains usable

### Test 7.3: Tab Navigation
**Objective**: Test keyboard navigation

**Steps**:
1. Use Tab key to navigate between fields
2. Test in Connection tab
3. Test in Output Profiles tab
4. Test in Process List tab

**Expected Results**:
- ✅ Tab order logical (top-to-bottom, left-to-right)
- ✅ All interactive elements reachable
- ✅ Visual focus indicator present

### Test 7.4: Tooltips
**Objective**: Verify helpful tooltips present

**Steps**:
1. Hover over each button/field
2. Check for tooltips

**Expected Results**:
- ✅ All buttons have tooltips
- ✅ Tooltips are descriptive
- ✅ Technical fields have explanations

---

## Phase 8: Security Tests

### Test 8.1: Password Storage
**Objective**: Verify credentials stored securely

**Steps**:
1. Configure connection with password
2. Check settings file: `~/Library/Application Support/obs-studio/plugin_config/obs-polyemesis/config.json`

**Expected Results**:
- ✅ Password not stored in plaintext
- ✅ Or file permissions restrictive (600)
- ⚠️ Consider keychain integration

### Test 8.2: Stream Key Masking
**Objective**: Verify stream keys not exposed

**Steps**:
1. Configure destination with stream key
2. Check UI displays
3. Check logs
4. Check screenshots

**Expected Results**:
- ✅ Keys masked in UI (e.g., "abc***xyz")
- ✅ Keys not logged in plaintext
- ✅ Keys not visible in profile list

### Test 8.3: HTTPS/TLS Connection
**Objective**: Test secure connection to Restreamer

**Steps**:
1. Configure HTTPS endpoint
2. Enable HTTPS checkbox
3. Test connection

**Expected Results**:
- ✅ TLS handshake succeeds
- ✅ Certificate validation works
- ✅ No certificate warnings (if valid cert)

---

## Phase 9: Cross-Platform Tests

### Test 9.1: macOS Testing
**Platform**: macOS 12+ (Intel and Apple Silicon)

**Key Tests**:
- ✅ Plugin loads correctly
- ✅ All UI elements render
- ✅ Streaming works
- ✅ No crashes on quit

### Test 9.2: Windows Testing
**Platform**: Windows 10/11

**Key Tests**:
- ✅ Plugin installation successful
- ✅ DLL dependencies resolved
- ✅ Paths correct (Windows-style)
- ✅ All functionality works

### Test 9.3: Linux Testing
**Platform**: Ubuntu 22.04+ / Fedora 38+

**Key Tests**:
- ✅ Plugin loads
- ✅ Library dependencies met
- ✅ X11/Wayland compatibility
- ✅ All tests pass

---

## Phase 10: Regression Testing

### Critical Path Tests (Run Before Each Release)

**Connection**:
- [ ] Connect to Restreamer server
- [ ] Test connection button works
- [ ] Credentials persist across restarts

**Profile Management**:
- [ ] Create profile
- [ ] Configure profile settings
- [ ] Add destination
- [ ] Edit destination
- [ ] Remove destination
- [ ] Delete profile
- [ ] Duplicate profile

**Stream Control**:
- [ ] Start single profile
- [ ] Monitor active stream
- [ ] Stop profile
- [ ] Start all profiles
- [ ] Stop all profiles

**Error Handling**:
- [ ] Invalid credentials
- [ ] Invalid stream key
- [ ] Network failure
- [ ] Server unavailable

**UI/UX**:
- [ ] Dark theme
- [ ] Light theme
- [ ] Window resize
- [ ] Tab navigation

---

## Automated Test Suite

### Test Script Template

```bash
#!/bin/bash
# scripts/test-polyemesis.sh

set -e

echo "=========================================="
echo " OBS Polyemesis Automated Test Suite"
echo "=========================================="

# Check prerequisites
echo "[1/10] Checking prerequisites..."
command -v obs >/dev/null 2>&1 || { echo "Error: OBS not installed"; exit 1; }
command -v docker >/dev/null 2>&1 || { echo "Error: Docker not installed"; exit 1; }
command -v curl >/dev/null 2>&1 || { echo "Error: curl not installed"; exit 1; }
command -v jq >/dev/null 2>&1 || { echo "Error: jq not installed"; exit 1; }

# Start Restreamer server
echo "[2/10] Starting Restreamer server..."
docker run -d --rm --name restreamer-test \
  -p 8080:8080 \
  -p 1935:1935 \
  datarhei/restreamer:latest
sleep 10

# Wait for server ready
echo "[3/10] Waiting for Restreamer API..."
for i in {1..30}; do
  if curl -s http://localhost:8080/api/v3/about >/dev/null; then
    echo "Server ready!"
    break
  fi
  echo "Waiting... ($i/30)"
  sleep 1
done

# Test 1: API health check
echo "[4/10] Testing API health..."
curl -s -u admin:admin http://localhost:8080/api/v3/about | jq .name

# Test 2: Login endpoint
echo "[5/10] Testing login..."
TOKEN=$(curl -s -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}' \
  | jq -r .access_token)

if [ -n "$TOKEN" ] && [ "$TOKEN" != "null" ]; then
  echo "✅ Login successful"
else
  echo "❌ Login failed"
  exit 1
fi

# Test 3: Process list
echo "[6/10] Testing process list..."
PROCESSES=$(curl -s -u admin:admin http://localhost:8080/api/v3/process | jq length)
echo "Found $PROCESSES processes"

# Test 4: Create process
echo "[7/10] Testing process creation..."
PROCESS_JSON=$(cat <<EOF
{
  "id": "test_obs_stream",
  "reference": "obs-polyemesis-test",
  "input": [
    {
      "address": "rtmp://localhost/live/obs_input",
      "id": "input_0"
    }
  ],
  "output": [
    {
      "address": "rtmp://localhost/live/test_output",
      "id": "output_0"
    }
  ],
  "options": ["-f", "flv"]
}
EOF
)

curl -s -X POST http://localhost:8080/api/v3/process \
  -u admin:admin \
  -H "Content-Type: application/json" \
  -d "$PROCESS_JSON" | jq .id

# Test 5: Check process exists
echo "[8/10] Verifying process created..."
PROCESS_ID=$(curl -s -u admin:admin http://localhost:8080/api/v3/process | jq -r '.[0].id')
echo "Process ID: $PROCESS_ID"

# Test 6: Start process (will fail without real input, that's OK)
echo "[9/10] Testing process control..."
curl -s -X PUT http://localhost:8080/api/v3/process/$PROCESS_ID/command \
  -u admin:admin \
  -H "Content-Type: application/json" \
  -d '{"command":"start"}' | jq .

# Cleanup
echo "[10/10] Cleaning up..."
curl -s -X DELETE http://localhost:8080/api/v3/process/$PROCESS_ID \
  -u admin:admin >/dev/null

docker stop restreamer-test

echo ""
echo "=========================================="
echo " Test suite completed successfully!"
echo "=========================================="
```

### Save and run:
```bash
chmod +x scripts/test-polyemesis.sh
./scripts/test-polyemesis.sh
```

---

## Manual Test Checklist

Use this checklist for pre-release validation:

### Installation & Setup
- [ ] Plugin installs correctly
- [ ] Dock appears in View menu
- [ ] No errors on first launch
- [ ] Settings file created correctly

### Connection
- [ ] Can connect to local Restreamer
- [ ] Can connect to remote Restreamer
- [ ] HTTPS connections work
- [ ] Invalid credentials handled gracefully
- [ ] Connection persists across restarts

### Profile Management
- [ ] Can create profiles
- [ ] Can configure all profile settings
- [ ] Can add destinations (all service types)
- [ ] Can edit destinations
- [ ] Can remove destinations
- [ ] Can duplicate profiles
- [ ] Can delete profiles
- [ ] Settings persist correctly

### Stream Control
- [ ] Can start single profile
- [ ] Can stop single profile
- [ ] Can start all profiles
- [ ] Can stop all profiles
- [ ] Profile states transition correctly
- [ ] Buttons enable/disable properly

### Monitoring
- [ ] Process state updates
- [ ] Metrics display correctly
- [ ] Uptime counter increments
- [ ] CPU/memory stats realistic
- [ ] Frame counts update
- [ ] Bitrate displayed

### Error Handling
- [ ] Invalid stream keys detected
- [ ] Network failures handled
- [ ] Server unavailable handled
- [ ] Auto-reconnect works
- [ ] Error messages user-friendly

### UI/UX
- [ ] Dark theme looks good
- [ ] Light theme looks good
- [ ] Window resizing works
- [ ] Tooltips helpful
- [ ] Tab navigation logical
- [ ] Context menus work

### Performance
- [ ] No memory leaks (24h test)
- [ ] CPU usage reasonable
- [ ] Multiple profiles work
- [ ] UI remains responsive

### Security
- [ ] Passwords not in plaintext
- [ ] Stream keys masked
- [ ] TLS connections work
- [ ] File permissions correct

### Cross-Platform
- [ ] macOS (Intel)
- [ ] macOS (Apple Silicon)
- [ ] Windows 10/11
- [ ] Linux (Ubuntu/Fedora)

---

## Test Results Template

### Test Execution Log

**Date**: YYYY-MM-DD
**OBS Version**: X.X.X
**Plugin Version**: X.X.X
**OS**: macOS/Windows/Linux
**Restreamer Version**: X.X.X

| Test ID | Test Name | Status | Notes |
|---------|-----------|--------|-------|
| 1.1 | Initial Connection | ✅ PASS | Connected in 2.3s |
| 1.2 | Invalid Credentials | ✅ PASS | Error displayed correctly |
| 2.1 | Create Profile | ✅ PASS | Profile created successfully |
| 3.1 | Start Profile | ❌ FAIL | Timeout after 30s |
| 3.2 | Monitor Stream | ⚠️ PARTIAL | FPS metric not updating |

**Issues Found**:
1. **Profile start timeout**: Server returned 500 error. Needs investigation.
2. **FPS metric**: Value stays at 0 despite active stream. Check API polling.

**Performance Metrics**:
- Memory usage: 45MB (idle) → 120MB (5 active streams)
- CPU usage: 2% (idle) → 15% (5 streams)
- UI response time: <100ms for all actions

---

## Continuous Testing Strategy

### Pre-Commit Tests
Run before every git commit:
```bash
# Quick smoke test
./scripts/test-polyemesis.sh

# Build test
cmake --build build-test

# Check for common issues
grep -r "TODO\|FIXME" src/
```

### Pre-Release Tests
Run before version bump:
- [ ] Full manual checklist
- [ ] All automated tests
- [ ] 24-hour stability test
- [ ] Cross-platform testing
- [ ] Security audit

### Post-Release Monitoring
- Monitor GitHub issues for bug reports
- Check user feedback on forums/Discord
- Track crash reports (if telemetry implemented)

---

## Tools & Resources

### Testing Tools
- **OBS Studio**: https://obsproject.com/download
- **Restreamer**: https://datarhei.github.io/restreamer/
- **Postman/curl**: API testing
- **jq**: JSON parsing for scripts
- **Valgrind**: Memory leak detection (Linux)
- **Instruments**: Performance profiling (macOS)

### Documentation
- Restreamer API Docs: https://docs.datarhei.com/
- OBS Plugin Docs: https://obsproject.com/docs/plugins.html
- Qt Testing: https://doc.qt.io/qt-6/qtest-overview.html

---

## Contributing Tests

If you find bugs or want to add test cases:

1. **Document the scenario** in this file
2. **Provide reproduction steps** (exact button clicks)
3. **Include expected vs actual results**
4. **Attach logs** if possible
5. **Submit GitHub issue** with "test" label

---

**Last Updated**: 2025-11-09
**Maintained By**: OBS Polyemesis Development Team
