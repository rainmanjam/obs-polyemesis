# OBS Polyemesis - Restreamer Control Plugin

A comprehensive OBS Studio plugin for controlling and monitoring [datarhei Restreamer](https://github.com/datarhei/restreamer) instances with advanced multistreaming capabilities.

## Features

### 🎮 Complete Restreamer Control
- **Process Management**: Start, stop, and restart restreamer processes directly from OBS
- **Real-time Monitoring**: View CPU usage, memory consumption, uptime, and process states
- **Session Tracking**: Monitor active viewer sessions and bandwidth usage
- **Process Logs**: Access stdout/stderr logs for debugging

### 📺 Multiple Plugin Types

#### 1. **Source Plugin**
Add restreamer streams as video sources in OBS
- Connect to any restreamer process
- Display live stream preview
- Support for both global and per-source connection settings

#### 2. **Output Plugin**
Stream directly to restreamer for processing
- Seamless integration with OBS streaming workflow
- Automatic process creation and management
- Support for single or multistream outputs

#### 3. **Dock Panel UI**
Comprehensive control panel with:
- Connection management
- Process list with real-time status
- Process control buttons (start/stop/restart)
- Detailed process metrics
- Active session viewer
- Multistream configuration interface

### 🌐 Advanced Multistreaming

#### Orientation-Aware Streaming
- **Auto-detection**: Automatically detect video orientation (horizontal/vertical/square)
- **Smart Routing**: Route streams to appropriate services based on orientation
- **Adaptive Transcoding**: Apply video filters to convert between orientations
  - Horizontal (16:9) ↔ Vertical (9:16)
  - Square (1:1) to any orientation
  - Crop and scale automatically

#### Multi-Platform Support
Stream simultaneously to multiple platforms with proper orientation:
- **Twitch**: Landscape streaming
- **YouTube**: All orientations
- **Facebook**: All orientations
- **Kick**: Landscape streaming
- **TikTok**: Vertical streaming (with horizontal support)
- **Instagram**: Square and vertical
- **X (Twitter)**: All orientations
- **Custom RTMP**: Any service with custom endpoints

#### Service-Specific Configuration
- Pre-configured RTMP endpoints for major platforms
- Automatic stream key management
- Per-destination enable/disable controls
- Orientation preferences per service

## Installation

### Prerequisites
- OBS Studio 28.0 or later
- datarhei Restreamer instance (local or remote)
- Dependencies:
  - libcurl (HTTP client)
  - jansson (JSON parsing)
  - Qt6 (UI components)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/rainmanjam/obs-polyemesis.git
cd obs-polyemesis

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install
cmake --install build
```

### CMake Build Options
- `ENABLE_FRONTEND_API=ON`: Enable dock UI (default: ON)
- `ENABLE_QT=ON`: Enable Qt-based components (default: ON)

## Configuration

### Global Connection Settings

Configure your restreamer instance in the Dock Panel:

1. **Host**: Restreamer server address (default: `localhost`)
2. **Port**: API port (default: `8080`)
3. **Use HTTPS**: Enable for secure connections
4. **Username**: Optional HTTP basic auth username
5. **Password**: Optional HTTP basic auth password

### Source Configuration

When adding a Restreamer Source:

1. **Use Global Connection**: Toggle to use global settings or per-source settings
2. **Process Selection**: Choose from available restreamer processes
3. **Direct URL**: Or enter a stream URL directly

### Output Configuration

When using Restreamer Output:

1. **Enable Multistreaming**: Toggle multistream mode
2. **Orientation Settings**: Configure auto-detection or manual selection
3. **Destinations**: Add streaming destinations with the Dock Panel

## Usage

### Basic Single Stream

1. Configure connection in the Restreamer Dock
2. Click "Test Connection" to verify connectivity
3. Use "Refresh" to load available processes
4. Select a process and control it with Start/Stop/Restart buttons

### Setting Up Multistreaming

#### Using the Dock Panel:

1. Open the Restreamer Control dock
2. Scroll to "Multistream Configuration"
3. Configure orientation settings:
   - Enable "Auto-detect orientation" for automatic detection
   - Or manually select: Horizontal, Vertical, or Square

4. Add destinations:
   - Click "Add Destination"
   - Select service (Twitch, YouTube, TikTok, etc.)
   - Enter stream key
   - Choose target orientation
   - Click OK

5. Repeat for all desired platforms

6. Click "Start Multistream" to begin

#### Using the Output Plugin:

1. Go to Settings → Stream
2. Select "Restreamer Output" as the service
3. Enable "Enable Multistreaming" in output settings
4. Configure destinations in the Dock Panel
5. Start streaming normally

### Orientation Handling Examples

#### Example 1: Portrait Mobile Stream to Multiple Platforms

**Setup:**
- Source: 1080x1920 (vertical)
- Destinations:
  - TikTok (vertical) - No conversion needed
  - Instagram (vertical) - No conversion needed
  - YouTube (horizontal) - Auto-crop to 1920x1080
  - Twitch (horizontal) - Auto-crop to 1920x1080

**Result:** Plugin automatically crops vertical video for horizontal platforms

#### Example 2: Landscape Gameplay to All Platforms

**Setup:**
- Source: 1920x1080 (horizontal)
- Destinations:
  - Twitch (horizontal) - No conversion
  - YouTube (horizontal) - No conversion
  - TikTok (vertical) - Auto-crop to 1080x1920
  - Instagram (square) - Auto-scale to 1080x1080

**Result:** Plugin crops/scales for vertical and square formats

### Advanced Features

#### Process Monitoring

The Dock Panel provides real-time metrics:
- **Process State**: running, stopped, failed, etc.
- **Uptime**: How long the process has been running
- **CPU Usage**: Current CPU utilization percentage
- **Memory**: RAM usage in MB

#### Session Tracking

View active viewers/connections:
- Session ID
- Remote IP address
- Data transferred (bytes)

#### Custom RTMP Destinations

For services not listed:
1. Select "Custom RTMP" as service
2. Enter full RTMP URL including stream key
3. Choose orientation

## API Integration

The plugin uses the datarhei Restreamer v3 REST API:

### Endpoints Used

- `GET /api/v3/` - Connection test
- `GET /api/v3/process` - List all processes
- `GET /api/v3/process/{id}` - Get process details
- `POST /api/v3/process/{id}/command` - Control process (start/stop/restart)
- `GET /api/v3/process/{id}/log` - Get process logs
- `GET /api/v3/session` - List active sessions
- `POST /api/v3/process` - Create new process
- `DELETE /api/v3/process/{id}` - Delete process

### Authentication

Supports HTTP Basic Authentication:
- Configure username and password in connection settings
- Credentials are stored securely in OBS config

## Multistream Architecture

### Video Filter Pipeline

The plugin builds FFmpeg filter chains for orientation conversion:

```
[Horizontal → Vertical]
crop=ih*9/16:ih,scale=1080:1920

[Vertical → Horizontal]
crop=iw:iw*9/16,scale=1920:1080

[Square → Horizontal]
scale=1920:1080,setsar=1

[Square → Vertical]
scale=1080:1920,setsar=1
```

### Process Creation

When multistreaming:
1. Plugin detects source video dimensions
2. Determines source orientation
3. For each destination:
   - Checks target orientation
   - Generates appropriate video filter if needed
   - Builds RTMP URL with stream key
4. Creates restreamer process with:
   - Input: OBS stream (via local RTMP or SRT)
   - Outputs: All configured destinations
   - Filters: Orientation conversion as needed

## Troubleshooting

### Cannot Connect to Restreamer

**Check:**
- Restreamer is running and accessible
- Correct host and port in settings
- Firewall allows connection
- API authentication credentials (if required)

### Multistream Not Starting

**Check:**
- At least one destination is configured and enabled
- Stream keys are valid
- Restreamer has sufficient resources
- Check restreamer logs for errors

### Orientation Detection Issues

**Solution:**
- Disable auto-detect and manually set orientation
- Verify video resolution matches expected ratio
- Check if source is square (1:1) vs landscape/portrait

### Performance Issues

**Optimize:**
- Reduce number of simultaneous destinations
- Use hardware encoding if available
- Monitor restreamer CPU/memory usage
- Consider using multiple restreamer instances

## Development

### Project Structure

```
obs-polyemesis/
├── src/
│   ├── plugin-main.c              # Main plugin entry
│   ├── restreamer-api.h/c         # REST API client
│   ├── restreamer-config.h/c      # Configuration management
│   ├── restreamer-multistream.h/c # Multistreaming logic
│   ├── restreamer-source.c        # Source plugin
│   ├── restreamer-output.c        # Output plugin
│   ├── restreamer-dock.h/cpp      # Qt dock UI
│   └── restreamer-dock-bridge.cpp # C/C++ bridge
├── CMakeLists.txt                 # Build configuration
├── buildspec.json                 # Plugin metadata
└── data/
    └── locale/
        └── en-US.ini              # Localization strings
```

### Adding New Services

To add a new streaming service:

1. Add to `streaming_service_t` enum in `restreamer-multistream.h`
2. Add RTMP URL in `restreamer_multistream_get_service_url()`
3. Add service name in `restreamer_multistream_get_service_name()`
4. Update dock UI combo box in `restreamer-dock.cpp`

### Contributing

Contributions welcome! Please:
- Follow existing code style
- Add comments for complex logic
- Test with real restreamer instance
- Update documentation

## License

GNU General Public License v2.0 or later

See LICENSE file for details.

## Credits

- **OBS Studio**: https://obsproject.com/
- **datarhei Restreamer**: https://github.com/datarhei/restreamer
- **OBS Plugin Template**: https://github.com/obsproject/obs-plugintemplate

## Support

For issues and feature requests:
- GitHub Issues: https://github.com/rainmanjam/obs-polyemesis/issues
- Restreamer Docs: https://docs.datarhei.com/restreamer/

## Changelog

### Version 1.0.0 (Initial Release)
- Complete restreamer API integration
- Source, output, and dock plugins
- Multistreaming with orientation support
- Support for major streaming platforms
- Real-time monitoring and control
- Session tracking
- Qt-based control panel
