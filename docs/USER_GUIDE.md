# OBS Polyemesis User Guide

**Control Datarhei Restreamer directly from OBS Studio**

## Table of Contents

- [What is OBS Polyemesis?](#what-is-obs-polyemesis)
- [Architecture Overview](#architecture-overview)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

## What is OBS Polyemesis?

OBS Polyemesis is a plugin that integrates Datarhei Restreamer control into OBS Studio, allowing you to:

- ✅ Start/stop Restreamer streams directly from OBS
- ✅ Monitor stream status in real-time
- ✅ Control multiple Restreamer instances
- ✅ Manage multistreaming configurations
- ✅ View stream health and statistics

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         OBS Studio                               │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                  OBS Polyemesis Plugin                     │  │
│  │                                                            │  │
│  │  ┌──────────────┐  ┌───────────────┐  ┌──────────────┐  │  │
│  │  │   UI Dock    │  │  Config       │  │  API Client  │  │  │
│  │  │  (Qt/C++)    │◄─┤  Manager      │◄─┤   (REST)     │  │  │
│  │  │              │  │   (C)         │  │    (C)       │  │  │
│  │  └──────┬───────┘  └───────────────┘  └──────┬───────┘  │  │
│  │         │                                      │          │  │
│  └─────────┼──────────────────────────────────────┼──────────┘  │
│            │                                      │             │
└────────────┼──────────────────────────────────────┼─────────────┘
             │                                      │
             │         User Interactions            │ HTTPS/REST
             ▼                                      ▼
        ┌────────┐                        ┌──────────────────┐
        │  User  │                        │   Restreamer     │
        └────────┘                        │   HTTP API       │
                                          │  (Port 8080)     │
                                          └────────┬─────────┘
                                                   │
                                          ┌────────▼─────────┐
                                          │   Restreamer     │
                                          │   Streaming      │
                                          │    Engine        │
                                          └──────────────────┘
                                                   │
                                          ┌────────▼─────────┐
                                          │  YouTube/Twitch  │
                                          │  Other Platforms │
                                          └──────────────────┘
```

### Component Flow

```
User Action → UI Dock → Config Manager → API Client → Restreamer API
                ▲                                            │
                │                                            │
                └────────────── Response ◄───────────────────┘
```

## Installation

### Prerequisites

- OBS Studio 28.0 or newer
- Operating System:
  - Ubuntu 22.04+ / Debian 11+
  - macOS 12+ (Intel or Apple Silicon)
  - Windows 10/11 (64-bit)
- Datarhei Restreamer instance (local or remote)

### Option 1: Install from Release (Recommended)

1. **Download the plugin**:
   - Go to [Releases](https://github.com/rainmanjam/obs-polyemesis/releases)
   - Download the appropriate package for your OS:
     - **Linux**: `obs-polyemesis-X.Y.Z-linux-x86_64.tar.gz`
     - **macOS**: `obs-polyemesis-X.Y.Z-macos.pkg`
     - **Windows**: `obs-polyemesis-X.Y.Z-windows-x64.zip`

2. **Install the plugin**:

   **Linux (Ubuntu/Debian)**:
   ```bash
   tar -xzf obs-polyemesis-*-linux-x86_64.tar.gz
   sudo cp -r obs-polyemesis /usr/lib/obs-plugins/
   sudo cp -r data/* /usr/share/obs/obs-plugins/obs-polyemesis/
   ```

   **macOS**:
   ```bash
   # Double-click the .pkg file and follow the installer
   # Or via terminal:
   sudo installer -pkg obs-polyemesis-*.pkg -target /
   ```

   **Windows**:
   ```
   1. Extract the ZIP file
   2. Copy the contents to: C:\Program Files\obs-studio\
   3. Merge folders when prompted
   ```

3. **Restart OBS Studio**

### Option 2: Build from Source

See [BUILDING.md](BUILDING.md) for detailed build instructions.

## Quick Start

### 1. Set Up Restreamer

If you don't have Restreamer running:

```bash
# Using Docker
docker run -d \
  --name restreamer \
  -p 8080:8080 \
  -p 8181:8181 \
  -p 1935:1935 \
  datarhei/restreamer:latest
```

Access Restreamer at: `http://localhost:8080`

### 2. Configure OBS Polyemesis

1. **Open OBS Studio**

2. **Find the Restreamer Control Dock**:
   - Menu: `Docks` → `Restreamer Control`
   - The dock will appear in your OBS layout

3. **Connect to Restreamer**:
   ```
   ┌─────────────────────────────────────────┐
   │      Restreamer Connection Setup        │
   ├─────────────────────────────────────────┤
   │                                         │
   │  URL:  http://localhost:8080           │
   │                                         │
   │  API Key: [your-api-key-here]         │
   │                                         │
   │  [Test Connection]  [Save]             │
   │                                         │
   └─────────────────────────────────────────┘
   ```

4. **Get Your API Key**:
   - Go to Restreamer web interface
   - Navigate to: `Settings` → `API`
   - Copy the API key
   - Paste it into OBS Polyemesis

### 3. Start Streaming

1. **Configure Your Stream** in Restreamer:
   - Add your streaming platforms (YouTube, Twitch, etc.)
   - Set up video/audio sources
   - Save the configuration

2. **Control from OBS**:
   ```
   ┌─────────────────────────────────────────┐
   │        Restreamer Control Panel         │
   ├─────────────────────────────────────────┤
   │                                         │
   │  Status: ● Connected                   │
   │                                         │
   │  Active Streams:                       │
   │  ┌─────────────────────────────────┐  │
   │  │ ▶ YouTube - Main Channel        │  │
   │  │   ● Live (00:45:23)             │  │
   │  ├─────────────────────────────────┤  │
   │  │ ▶ Twitch - Gaming               │  │
   │  │   ● Live (00:45:23)             │  │
   │  └─────────────────────────────────┘  │
   │                                         │
   │  [Start All] [Stop All] [Refresh]     │
   │                                         │
   └─────────────────────────────────────────┘
   ```

3. **Click "Start All"** to begin streaming to all platforms

## Configuration

### Connection Settings

The plugin stores configuration in:
- **Linux**: `~/.config/obs-studio/plugin_config/obs-polyemesis/config.json`
- **macOS**: `~/Library/Application Support/obs-studio/plugin_config/obs-polyemesis/config.json`
- **Windows**: `%APPDATA%\obs-studio\plugin_config\obs-polyemesis\config.json`

Example configuration:

```json
{
  "restreamer_url": "http://localhost:8080",
  "api_key": "your-api-key-here",
  "auto_connect": true,
  "poll_interval_ms": 2000,
  "timeout_ms": 5000
}
```

### Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `restreamer_url` | Restreamer API URL | `http://localhost:8080` |
| `api_key` | Authentication key | Required |
| `auto_connect` | Connect on OBS startup | `true` |
| `poll_interval_ms` | Status update frequency | `2000` (2 seconds) |
| `timeout_ms` | API request timeout | `5000` (5 seconds) |

### Multi-Instance Setup

You can control multiple Restreamer instances:

```
┌────────────────────────────────────────────────────┐
│  Instance 1: Production (http://prod:8080)         │
│  Status: ● Connected                              │
│  Streams: YouTube, Twitch, Facebook               │
├────────────────────────────────────────────────────┤
│  Instance 2: Backup (http://backup:8080)          │
│  Status: ● Connected                              │
│  Streams: YouTube (backup), Twitch (backup)       │
└────────────────────────────────────────────────────┘
```

## Usage

### Stream Control Workflow

```
Start OBS Studio
       │
       ▼
Plugin Auto-Connects to Restreamer
       │
       ▼
View Stream Status in Dock
       │
       ├──► Start Individual Stream  ──► Stream Goes Live
       │
       ├──► Start All Streams  ──────► All Streams Go Live
       │
       ├──► Stop Stream  ──────────────► Stream Stops
       │
       └──► Monitor Status  ───────────► View Live Stats
```

### Common Tasks

#### Start a Specific Stream

1. Click the stream in the list
2. Click `▶ Start` button
3. Monitor status indicator

#### Stop All Streams

1. Click `Stop All` button
2. Confirm in dialog
3. All streams stop simultaneously

#### Monitor Stream Health

The dock shows real-time status:

```
● Green  = Stream is live and healthy
● Yellow = Stream is starting/buffering
● Red    = Stream error or stopped
● Gray   = Disconnected from Restreamer
```

## Troubleshooting

### Connection Issues

**Problem**: Cannot connect to Restreamer

**Solutions**:
1. Verify Restreamer is running:
   ```bash
   curl http://localhost:8080/api/v3/config
   ```

2. Check firewall settings:
   ```bash
   # Linux
   sudo ufw allow 8080/tcp

   # macOS
   # System Preferences → Security & Privacy → Firewall
   ```

3. Verify API key is correct

4. Check OBS logs:
   - **Linux**: `~/.config/obs-studio/logs/`
   - **macOS**: `~/Library/Application Support/obs-studio/logs/`
   - **Windows**: `%APPDATA%\obs-studio\logs\`

### Stream Won't Start

**Problem**: Stream fails to start from OBS

**Solutions**:
1. Verify stream is configured in Restreamer web interface
2. Check Restreamer logs:
   ```bash
   docker logs restreamer
   ```
3. Test stream manually in Restreamer interface
4. Verify network connectivity to streaming platforms

### Plugin Not Loading

**Problem**: Plugin doesn't appear in OBS

**Solutions**:
1. Verify installation path is correct
2. Check OBS version compatibility (28.0+)
3. Review OBS logs for errors
4. Reinstall the plugin

### Performance Issues

**Problem**: OBS becomes slow with plugin enabled

**Solutions**:
1. Increase `poll_interval_ms` to reduce API calls:
   ```json
   {"poll_interval_ms": 5000}
   ```
2. Disable auto-connect if not needed
3. Update to latest plugin version

## FAQ

### Q: Does this replace OBS's built-in streaming?

**A**: No. This plugin controls **Restreamer**, which is a separate multistreaming service. OBS still handles your video capture and encoding.

### Q: Can I use this with OBS Websocket?

**A**: Yes, they work independently.

### Q: Does this work with Restreamer Core?

**A**: Yes, both Restreamer and Restreamer Core are supported.

### Q: What's the latency impact?

**A**: Minimal. The plugin only controls Restreamer via API; it doesn't affect video encoding or transmission.

### Q: Can I run Restreamer on a different machine?

**A**: Yes! Just update the `restreamer_url` to point to your Restreamer instance:
```json
{"restreamer_url": "http://192.168.1.100:8080"}
```

### Q: Is HTTPS supported?

**A**: Yes, use HTTPS URLs:
```json
{"restreamer_url": "https://restreamer.example.com"}
```

### Q: How do I update the plugin?

**A**: Download the latest release and reinstall. Your configuration is preserved.

### Q: Where can I get help?

**A**:
- [GitHub Issues](https://github.com/rainmanjam/obs-polyemesis/issues) - Bug reports
- [Discussions](https://github.com/rainmanjam/obs-polyemesis/discussions) - Questions
- [Wiki](https://github.com/rainmanjam/obs-polyemesis/wiki) - Advanced guides

## Advanced Usage

### Custom API Endpoints

The plugin uses these Restreamer API endpoints:

```
GET  /api/v3/config              - Get configuration
GET  /api/v3/process             - List streams
POST /api/v3/process/{id}/start  - Start stream
POST /api/v3/process/{id}/stop   - Stop stream
GET  /api/v3/process/{id}/state  - Get stream state
```

### Logging

Enable debug logging:

1. Set environment variable before starting OBS:
   ```bash
   export OBS_POLYEMESIS_DEBUG=1
   obs
   ```

2. Check logs for detailed API communication

### Performance Tuning

For high-load scenarios:

```json
{
  "poll_interval_ms": 10000,
  "timeout_ms": 10000,
  "max_retries": 3,
  "connection_pool_size": 5
}
```

## Getting Started Checklist

- [ ] Install OBS Studio 28.0+
- [ ] Install Restreamer (Docker or standalone)
- [ ] Install OBS Polyemesis plugin
- [ ] Get Restreamer API key
- [ ] Configure connection in OBS
- [ ] Test connection
- [ ] Set up streams in Restreamer
- [ ] Control streams from OBS
- [ ] Monitor stream status
- [ ] Join community for support

## Next Steps

- Read [CONTRIBUTING.md](../CONTRIBUTING.md) to contribute
- See [BUILDING.md](BUILDING.md) to build from source
- Check [CHANGELOG.md](../CHANGELOG.md) for updates
- Visit [Restreamer Documentation](https://docs.datarhei.com/)

---

**Need help?** Open an issue on [GitHub](https://github.com/rainmanjam/obs-polyemesis/issues)
