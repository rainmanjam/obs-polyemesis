# OBS Polyemesis - WebSocket Vendor API

## Overview

The WebSocket Vendor API provides remote control and automation capabilities for the OBS Polyemesis plugin through the obs-websocket protocol. This enables automated testing, CI/CD integration, and external control applications.

## Implementation Status

### ✅ Completed
- WebSocket API architecture designed
- 16 vendor request handlers implemented
- Event emission system designed
- Integration with plugin main module
- Global state accessor functions
- Type-safe API client access

### ⚠️ Pending
- **obs-websocket-api headers required**: The implementation is complete but requires obs-websocket development headers to compile
- Installation of obs-websocket development package
- Build system integration with obs-websocket

## Requirements

To use the WebSocket API, you need:

1. **obs-websocket plugin** (installed) ✅
2. **obs-websocket development headers** (not available) ❌

### Installing obs-websocket Development Headers

#### Option 1: From Source
```bash
git clone https://github.com/obsproject/obs-websocket.git
cd obs-websocket
# Copy headers to your OBS installation
cp -r src/obs-websocket-api.h /path/to/obs/include/
```

#### Option 2: Package Manager (if available)
```bash
# Example for Linux
sudo apt-get install obs-websocket-dev

# Example for macOS with Homebrew
brew install obs-websocket --HEAD
```

## API Endpoints

### Profile Management
- `CreateProfile` - Create a new streaming profile
- `DeleteProfile` - Delete an existing profile
- `DuplicateProfile` - Duplicate a profile with a new name
- `GetProfiles` - List all profiles with status

### Destination Management
- `AddDestination` - Add streaming destination to profile
- `RemoveDestination` - Remove destination from profile
- `EditDestination` - Modify existing destination

### Stream Control
- `StartProfile` - Start streaming for a profile
- `StopProfile` - Stop streaming for a profile
- `StartAllProfiles` - Start all inactive profiles
- `StopAllProfiles` - Stop all active profiles

### State Queries
- `GetPluginState` - Get overall plugin state
- `GetProfileState` - Get detailed state of specific profile
- `GetConnectionStatus` - Get Restreamer connection status
- `GetButtonStates` - Get UI button states (for testing)

### Connection
- `ConnectToServer` - Connect to Restreamer server

## Events Emitted

- `ProfileCreated` - Fired when profile is created
- `ProfileDeleted` - Fired when profile is deleted
- `ProfileStateChanged` - Fired when profile status changes
- `ConnectionStatusChanged` - Fired when Restreamer connection changes
- `ButtonStatesChanged` - Fired when UI button states change
- `ErrorOccurred` - Fired on errors

## Usage Example (Python)

```python
from obswebsocket import obsws, requests

# Connect to OBS WebSocket
ws = obsws("localhost", 4455, "your-password")
ws.connect()

# Create a profile
response = ws.call(requests.CallVendorRequest(
    vendorName="polyemesis",
    requestType="CreateProfile",
    requestData={"profileName": "My Profile"}
))

profile_id = response.datain['responseData']['profileId']
print(f"Created profile: {profile_id}")

# Add destination
ws.call(requests.CallVendorRequest(
    vendorName="polyemesis",
    requestType="AddDestination",
    requestData={
        "profileId": profile_id,
        "name": "Twitch",
        "url": "rtmp://live.twitch.tv/app/",
        "streamKey": "your_stream_key"
    }
))

# Start streaming
ws.call(requests.CallVendorRequest(
    vendorName="polyemesis",
    requestType="StartProfile",
    requestData={"profileId": profile_id}
))

# Get profile state
state = ws.call(requests.CallVendorRequest(
    vendorName="polyemesis",
    requestType="GetProfileState",
    requestData={"profileId": profile_id}
))

print(f"Profile status: {state.datain['responseData']['status']}")

ws.disconnect()
```

## Files Created

- `src/websocket-api.h` - WebSocket API header declarations
- `src/websocket-api.cpp` - WebSocket API implementation
- `src/plugin-main.h` - Global accessor functions
- Updated `src/plugin-main.c` - WebSocket initialization/shutdown
- Updated `src/restreamer-dock.h` - Public accessor methods
- Updated `src/restreamer-dock-bridge.cpp` - Accessor implementation
- Updated `CMakeLists.txt` - Build configuration

## Building with WebSocket API

Once obs-websocket development headers are available:

```bash
cmake --build build-test --config Release
```

The plugin will automatically register the "polyemesis" vendor with obs-websocket on startup.

## Testing

See `tests/websocket/` directory for automated test suite (pending completion of Python test framework).

## Troubleshooting

### Build Error: obs-websocket-api.h not found

**Cause**: obs-websocket development headers not installed

**Solution**:
1. Install obs-websocket development package
2. OR temporarily disable WebSocket API by commenting out initialization in `plugin-main.c`

### Runtime Warning: WebSocket Vendor API initialization failed

**Cause**: obs-websocket plugin not loaded or incompatible version

**Solution**:
1. Ensure obs-websocket 5.x+ is installed
2. Check OBS logs for obs-websocket errors
3. Verify obs-websocket is enabled in OBS settings

## Next Steps

1. Install obs-websocket development headers
2. Build plugin with WebSocket API enabled
3. Create Python test automation framework
4. Implement automated workflow tests
5. Add CI/CD integration tests

## References

- [obs-websocket Documentation](https://github.com/obsproject/obs-websocket)
- [obs-websocket Protocol](https://github.com/obsproject/obs-websocket/blob/master/docs/generated/protocol.md)
- [Python obs-websocket Client](https://github.com/Elektordi/obs-websocket-py)
