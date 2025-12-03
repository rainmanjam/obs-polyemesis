# Changelog

All notable changes to OBS Polyemesis will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.9.9] - 2025-12-02

### Fixed
- **Windows Installer: Improved OBS Detection**
  - Added file system checks for OBS in common installation paths
  - Now detects OBS at `Program Files`, `Program Files (x86)`, and `LocalAppData`
  - Works with portable OBS installations that don't have registry entries
  - Fixed existing plugin detection to check correct `bin/64bit/` path
  - Updated success message to show actual installation location

## [0.9.8] - 2025-12-02

### Fixed
- **Critical: Windows Plugin Folder Structure**
  - Fixed incorrect folder structure that prevented OBS from loading the plugin
  - DLL now correctly installed to `bin/64bit/` subdirectory as required by OBS
  - Updated registry paths and uninstaller accordingly

### Added
- Added `.clang-format` configuration (matches OBS Studio code style)

## [0.9.7] - 2025-12-02

### Fixed
- **Critical: Use-After-Free Crash Prevention**
  - Fixed SIGABRT crash when using context menu actions (Add Output, Start/Stop, Edit)
  - Deferred `updateChannelList()` calls using `QTimer::singleShot(0, ...)` pattern
  - Added QPointer guards for delayed timer callbacks
  - Captured channel IDs by value in lambdas instead of pointer access
  - Added `disconnect()` before `deleteLater()` for safe dialog cleanup
  - Added null checks for channel pointer in stats/export actions

### Added
- **Channel Edit Dialog - Outputs Tab**
  - Full CRUD functionality for output management
  - List view showing all configured outputs with status
  - Add/Edit/Remove buttons with dedicated dialogs
  - Output details panel showing configuration
  - Bulk Enable All/Disable All buttons

- **Backup/Failover Settings**
  - "This is a backup output" checkbox in output configuration
  - Primary output selection dropdown
  - Auto-reconnect option

- **Preview Mode Controls**
  - "Start Preview" menu item with duration selection dialog
  - "Go Live" to transition from preview to live streaming
  - "Cancel Preview" to abort preview mode

- **OBS Bridge Settings**
  - Horizontal RTMP URL configuration in Advanced Settings
  - Vertical RTMP URL configuration
  - Auto-start option for bridge

- **Quality Presets**
  - Dropdown in Add Output dialog
  - Options: Auto, 1080p High/Standard, 720p High/Standard, 480p, Low Bandwidth
  - Automatically sets resolution and bitrate

- **UI Improvements**
  - Added tooltips to all buttons (Start/Stop, Edit, Menu, Settings)
  - RTMP URL validation with format guidance
  - Fixed JSON export escaping for special characters
  - Removed duplicate "Channel Settings..." context menu item

### Changed
- **UI Streamlining**
  - Replaced collapsible connection section with persistent connection status bar
    - Shows connection status with visual indicator (⚫ Connected/Disconnected)
    - "Configure" button opens dedicated connection settings dialog
    - More prominent and always-visible connection status
  - Removed CollapsibleSection widget (no longer needed)
    - Simplified codebase by removing ~200 lines of accordion UI code
    - Profiles section now always visible for immediate access
  - Connection Configuration Dialog
    - Dedicated modal dialog for connection settings
    - Fields: Restreamer URL, Username, Password, Connection Timeout
    - Auto-test connection on dialog open if settings exist
    - Real-time connection testing with detailed error messages
    - Context-aware hints for common connection issues (port, auth, network)
    - Support for custom ports (not just 443/80)
    - Flexible URL formats: full URL, host:port, or hostname only
    - Smart protocol detection (HTTPS for domains, HTTP for localhost)
  - Improved port flexibility for non-Let's Encrypt users
    - Tooltip shows port specification examples
    - Help text reminds users about custom ports
    - Default ports: 443 for HTTPS, 80 for HTTP

### Fixed
- Configure button text now fully visible (increased minimum width)
- Connection settings properly isolated in dialog (no longer scattered in dock)

## [0.9.0] - 2025-11-12

### Added
- **Cross-Platform Build Enhancements (Phase 1)**
  - macOS Universal Binary support (arm64 + x86_64)
    - Automatic jansson build from source for universal binaries
    - Single .pkg installer works on all Macs (Intel and Apple Silicon)
  - Linux ARM64 support
    - CI/CD workflow for ARM64 builds
    - Native support for Raspberry Pi and ARM servers
  - Comprehensive build matrix documentation
    - All supported platforms documented
    - Platform-specific build instructions

- **OBS Theme Integration (Phase 2)**
  - Complete removal of 257 lines of custom styling
  - Native QPalette integration for seamless theme matching
  - Support for all 6 OBS themes (Yami, Grey, Acri, Dark, Rachni, Light)
  - Semantic color helpers for status indicators
    - `obs_theme_get_success_color()` - Green variants per theme
    - `obs_theme_get_error_color()` - Red variants per theme
    - `obs_theme_get_warning_color()` - Orange/yellow variants per theme
    - `obs_theme_get_muted_color()` - Subtle text colors per theme
  - Dynamic theme switching support at runtime
  - Simplified theme detection (Dark vs Light)

- **UI Redesign (Phase 5)**
  - CollapsibleSection custom widget
    - Header with title label and expand/collapse chevron
    - Optional quick action buttons in section headers
    - Smooth 200ms expand/collapse animation
    - Keyboard navigation support (Tab, Enter, Arrow keys)
    - State persistence (save/restore collapsed state)
  - Removed tab-based layout in favor of vertical scroll
    - Better information density
    - More natural navigation flow
    - Collapsible sections for each major feature area
  - Dynamic section titles with status indicators
    - Connection: Shows connection state (Connected, Disconnected)
    - Bridge: Shows auto-start state (Active, Inactive)
    - Profiles: Shows selected profile name and status
    - Monitoring: Shows process state (Active, Idle)
  - Quick action buttons in section headers
    - Connection: "Test" button for quick connection testing
    - Bridge: "Enable/Disable" toggle for auto-start
    - Profiles: "Start/Stop" toggle for active profile
  - Copy-to-clipboard buttons for Bridge RTMP URLs
    - One-click copy for horizontal RTMP URL
    - One-click copy for vertical RTMP URL
  - Organized sub-groups within sections
    - Server Configuration and Connection Status
    - Bridge Configuration and Bridge Status
    - Profile Management, Actions, and Details
    - Better visual hierarchy and information grouping

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
- **New Files Created:**
  - `cmake/BuildJanssonUniversal.cmake` - Universal binary build script for jansson
  - `src/collapsible-section.h` and `src/collapsible-section.cpp` - Custom Qt widget for collapsible sections
  - `src/obs-theme-utils.h` and `src/obs-theme-utils.cpp` - OBS theme integration utilities
  - `BUILD_MATRIX.md` - Comprehensive build matrix documentation
  - `THEME_AUDIT.md` - Theme system audit and migration plan
- Added `obs-service-loader.h` and `obs-service-loader.cpp` for OBS service integration
- Loads services from `/Applications/OBS.app/Contents/PlugIns/rtmp-services.plugin/Contents/Resources/services.json` (macOS)
- Parses service name, servers, stream key links, and supported codecs
- Hotkeys registered via `obs_hotkey_register_frontend()`
- Tools menu items added via `obs_frontend_add_tools_menu_item()`
- Pre-load callback registered via `obs_frontend_add_preload_callback()`
- CollapsibleSection uses QPropertyAnimation for smooth transitions
- Theme utilities use `obs_frontend_get_current_theme()` for theme detection
- Dynamic section title updates triggered on state changes

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
