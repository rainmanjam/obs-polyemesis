# Changelog

All notable changes to OBS Polyemesis will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.9.0] - 2025-11-12

### Added
- **OBS Integration Improvements**
  - Hotkey support for quick profile control
    - Start/Stop all profiles via keyboard shortcuts
    - Start specific horizontal/vertical profiles
    - Configurable in OBS Settings → Hotkeys
  - Tools menu integration
    - Quick access to "Start All Profiles", "Stop All Profiles", and "Open Settings"
    - Convenient shortcuts without opening the dock
  - Pre-load callbacks for better state management
    - Plugin initializes before scene collections load
    - Improved reliability on OBS startup

- **UI/UX Enhancements**
  - Color-coded status indicators with emoji icons
    - Profile status: 🟢 Active, 🟡 Starting, 🟠 Stopping, 🔴 Error, ⚫ Inactive
    - Process state: 🟢 Running, 🟡 Starting, 🟠 Stopping, 🔴 Failed
    - CPU usage: Green (<50%), Orange (50-80%), Red (>80%)
    - Memory usage: Green (<1GB), Orange (1-2GB), Red (>2GB)
    - Dropped frames: Green (<1%), Orange (1-5%), Red (>5%)
  - Improved dialog alignment across all forms
    - Converted QFormLayout to QGridLayout for precise control
    - Right-aligned labels with consistent spacing
    - Professional appearance with 10px spacing
  - Enhanced scene collection integration
    - Saves last active profile for quick restoration
    - Saves last selected process
    - Tracks profile active states for potential auto-restart

- **Streaming Service Integration**
  - OBS Service Loader - Full access to OBS's streaming service database
    - 100+ streaming services automatically loaded from OBS
    - Common services (Twitch, YouTube, Facebook) shown first
    - Regional server selection for each service (e.g., Twitch: Tokyo, Seoul, Singapore, etc.)
  - Enhanced destination dialogs with server selection
    - Service dropdown with full OBS service list
    - Automatic server list population per service
    - Direct links to get stream keys for each service
    - Custom RTMP server option with full URL support
    - Dynamic UI that shows/hides relevant fields
  - Consistent alignment across all destination forms
    - "Add Destination" (multistream)
    - "Add Destination" (output profiles)
    - "Edit Destination" (output profiles)
    - "Configure Profile" basic settings

### Improved
- Dialog layouts now use QGridLayout for better alignment
- Labels are consistently right-aligned with proper vertical centering
- Input fields have consistent minimum widths (250-300px)
- All dialogs follow professional spacing standards (10px)
- Dynamic field visibility (custom RTMP vs regular services)

### Technical Details
- Added `obs-service-loader.h` and `obs-service-loader.cpp` for OBS service integration
- Loads services from `/Applications/OBS.app/Contents/PlugIns/rtmp-services.plugin/Contents/Resources/services.json` (macOS)
- Parses service name, servers, stream key links, and supported codecs
- Hotkeys registered via `obs_hotkey_register_frontend()`
- Tools menu items added via `obs_frontend_add_tools_menu_item()`
- Pre-load callback registered via `obs_frontend_add_preload_callback()`

- **Core Features**
  - Full datarhei Restreamer API integration (v3)
  - REST API client with connection management
  - HTTP Basic Authentication support
  - Comprehensive error handling and logging

- **Plugin Types**
  - Source plugin for displaying Restreamer streams in OBS
  - Output plugin for streaming to Restreamer
  - Dock UI plugin with Qt6-based control panel

- **Multistreaming Engine**
  - Automatic video orientation detection (landscape/portrait/square)
  - Intelligent stream routing based on orientation
  - Support for multiple simultaneous destinations
  - Automatic video transcoding between orientations
  - FFmpeg filter chain generation

- **Platform Support**
  - Pre-configured endpoints for major platforms:
    - Twitch (landscape)
    - YouTube (all orientations)
    - Facebook (all orientations)
    - Kick (landscape)
    - TikTok (vertical preferred)
    - Instagram (square/vertical)
    - X/Twitter (all orientations)
  - Custom RTMP destination support

- **UI/UX**
  - Qt6-based control dock panel
  - Real-time process monitoring
  - CPU and memory usage display
  - Process control buttons (start/stop/restart)
  - Active session viewer
  - Connection status indicators
  - Settings management interface

- **Configuration**
  - Global connection settings
  - Per-source configuration options
  - Persistent settings storage
  - Secure credentials handling

- **Testing Infrastructure**
  - Custom C test framework (zero external dependencies)
  - Mock Restreamer HTTP server for integration tests
  - 13 comprehensive test cases:
    - 5 API client tests
    - 3 configuration tests
    - 5 multistream tests
  - CTest integration with all tests passing
  - Code coverage support (optional --coverage flag)
  - AddressSanitizer support for memory leak detection

- **Cross-Platform Support**
  - macOS (Universal: arm64 + x86_64)
  - Windows (x64)
  - Linux (Ubuntu 24.04, x86_64)
  - Platform-specific build configurations
  - Automated CI/CD workflows for all platforms

- **Build System**
  - CMake 3.28+ configuration
  - Cross-platform compilation tested
  - Dependency detection (libcurl, jansson, Qt6)
  - Optional build flags:
    - `ENABLE_FRONTEND_API` (default: ON)
    - `ENABLE_QT` (default: ON)
    - `ENABLE_TESTING` (default: OFF)

- **Installation Packages**
  - macOS .pkg installer with pre/post-install scripts
  - Windows .exe installer (NSIS)
  - Linux .deb package (Debian/Ubuntu)
  - Linux .rpm package (Fedora/RHEL)

- **Documentation**
  - Comprehensive README
  - Plugin feature documentation
  - Building instructions for all platforms
  - Code style guide
  - ACT local testing guide
  - Cross-platform status tracking
  - Deployment guide with Docker instructions
  - API compatibility notes

- **Code Quality**
  - clang-format configuration for C/C++
  - gersemi configuration for CMake
  - Automated formatting checks in CI/CD
  - Local format verification script

- **Localization**
  - English (en-US.ini) with 60+ localized strings
  - Framework ready for additional languages

### Technical Details

**Architecture:**
- Modular design with separated concerns
- API client abstraction layer
- Configuration management system
- Event-driven UI updates

**Performance:**
- Efficient network I/O with libcurl
- Lightweight JSON parsing with jansson
- Minimal memory footprint
- Background threading for non-blocking operations

**Security:**
- HTTPS/TLS support
- Secure credential storage
- Input validation
- No hardcoded secrets

**Compatibility:**
- OBS Studio 28.0+ required
- macOS 11.0 (Big Sur) or later
- Windows 10/11 (x64)
- Ubuntu 24.04 or compatible Linux distros

### Development Stats

- **Lines of Code**: 3,686 total
  - Production code: 2,667 lines
  - Test code: 1,019 lines
- **Files Created**: 30+
- **Documentation Pages**: 9
- **Test Coverage**: 13 test cases, 100% pass rate
- **Platforms**: 3 (macOS, Windows, Linux)
- **CI/CD Jobs**: 8 (formatting + builds + tests)
- **Build Artifacts**: 4 per run

### Known Limitations

1. **API Version**: Currently supports datarhei-core v16.x API structure
2. **Qt Version**: Requires Qt6 (Qt5 compatibility not tested)
3. **Code Signing**: macOS/Windows packages are unsigned (requires certificates)
4. **Real-World Testing**: Integration with live Restreamer instances pending

### Dependencies

**Runtime:**
- OBS Studio >= 28.0
- libcurl
- jansson (JSON library)
- Qt6 Core and Widgets

**Build-time:**
- CMake >= 3.28
- C/C++ compiler (Clang, GCC, or MSVC)
- pkg-config
- OBS Studio development files

### Contributors

- rainmanjam - Initial development and implementation
- Claude (Anthropic) - Development assistance

### Bug Fixes

- **Memory Management**
  - Fixed critical allocator mismatch bug causing crashes on Ubuntu/Windows
  - Fixed heap-use-after-free in profile deletion tests
  - Fixed Windows heap corruption in multistream tests
  - Enabled AddressSanitizer for comprehensive memory testing

- **Security**
  - Fixed Bearer Security Scan SARIF validation handling
  - All security scans now passing

- **Cross-Platform**
  - Fixed MSVC printf format warnings on Windows
  - Fixed memory corruption issues on Linux
  - All unit tests now passing on macOS, Windows, and Ubuntu

## Future Releases

### Planned for 1.0.0
- UI tests for Qt dock panel
- Performance benchmark suite
- Additional platform integrations
- Enhanced error recovery
- Improved documentation with screenshots

### Under Consideration
- WebSocket support for real-time updates
- Process template system
- Batch operations
- Advanced monitoring dashboard
- Plugin API for extensions

---

## Release Links

- [GitHub Releases](https://github.com/rainmanjam/obs-polyemesis/releases)
- [Issue Tracker](https://github.com/rainmanjam/obs-polyemesis/issues)
- [Documentation](https://github.com/rainmanjam/obs-polyemesis/blob/main/docs/developer/PLUGIN_DOCUMENTATION.md)

---

**Note**: This is the initial v1.0 release. Feedback and contributions are welcome!
