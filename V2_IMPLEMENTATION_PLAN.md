# Implementation Plan: OBS Polyemesis v2.0 - Complete Modernization

## Overview

This plan outlines the complete modernization of OBS Polyemesis to v2.0, incorporating OBS native theming, comprehensive API coverage, recording capabilities, hybrid UI redesign, and expanded platform support.

**Target Version:** 2.0.0
**Estimated Duration:** 6-8 weeks
**Current Version:** 0.9.0

---

## Phase 1: Cross-Platform Build Infrastructure (Week 1)

### 1.1 macOS Universal Binary Support
**Priority:** HIGH
**Blockers:** Homebrew jansson is arm64-only

**Tasks:**
- Research jansson universal binary build options
  - Option A: Build jansson from source in CI as universal binary
  - Option B: Use pre-built universal jansson from alternate source
  - Option C: Statically link jansson (increase binary size but eliminate dependency)
- Update CMakeLists.txt to support multi-architecture builds
- Modify `.github/workflows/build-macos.yaml` to build universal binary
- Test universal binary on both Intel and Apple Silicon Macs
- Update installation validation to verify both architectures present

**Deliverables:**
- Universal macOS binary (arm64 + x86_64)
- Updated CI/CD workflow
- Documentation on dependency management

**Success Criteria:**
- `lipo -info` shows both arm64 and x86_64
- Plugin loads successfully on Intel Mac
- Plugin loads successfully on Apple Silicon Mac

---

### 1.2 Linux ARM64 Build Support
**Priority:** MEDIUM
**Blockers:** None (build already successful in testing)

**Tasks:**
- Add ARM64 build job to `.github/workflows/build-linux.yaml`
- Configure cross-compilation or use ARM64 runners
- Test on actual ARM64 hardware (Raspberry Pi 4/5, ARM server)
- Create `.deb` and `.rpm` packages for ARM64
- Update installation instructions for ARM64

**Deliverables:**
- Linux ARM64 `.deb` package
- Linux ARM64 `.rpm` package
- ARM64 build artifacts in releases

**Success Criteria:**
- Plugin installs on Ubuntu 24.04 ARM64
- Plugin loads in OBS on ARM64
- All tests pass on ARM64

---

### 1.3 Build System Enhancements
**Priority:** MEDIUM

**Tasks:**
- Add build matrix documentation showing all supported platforms
- Create platform-specific build scripts for local development
- Add architecture detection to CMake configuration
- Update buildspec.json with platform-specific metadata
- Create unified release workflow that builds all platforms

**Deliverables:**
- Comprehensive build documentation
- Platform detection in build system
- Unified release process

---

## Phase 2: OBS Theme Integration (Week 1-2)

### 2.1 Theme System Audit
**Priority:** HIGH

**Tasks:**
- Document all current custom styles in restreamer-dock.cpp (lines 912-1169)
- Catalog all color definitions from shadcn/ui implementation
- List all styled widgets and their purposes
- Identify functional vs aesthetic styling
- Map current colors to semantic meanings (error, warning, success, info)
- Test current UI with all OBS themes (Yami, Grey, Acri, Light, Dark, Rachni)

**Deliverables:**
- Style audit document
- Widget inventory spreadsheet
- Color semantic mapping
- Theme compatibility matrix

---

### 2.2 Custom Style Removal
**Priority:** HIGH
**Dependencies:** 2.1 complete

**Tasks:**
- Remove all hardcoded color values
- Remove custom stylesheet definitions
- Delete shadcn/ui color system
- Remove custom font definitions
- Remove custom spacing overrides
- Preserve only functional requirements (status indicators)

**Deliverables:**
- Cleaned restreamer-dock.cpp with no custom styles
- Reduced code size by ~250 lines

---

### 2.3 OBS Palette Integration
**Priority:** HIGH
**Dependencies:** 2.2 complete

**Tasks:**
- Implement QPalette-based coloring for all widgets
- Use `obs_frontend_get_theme()` to detect active theme
- Create helper functions for semantic colors:
  - `get_success_color()` - green variants per theme
  - `get_error_color()` - red variants per theme
  - `get_warning_color()` - orange/yellow variants per theme
  - `get_info_color()` - blue variants per theme
- Apply palette colors to status indicators dynamically
- Implement theme change listener to update colors when user switches themes

**Deliverables:**
- Theme utility functions in dedicated header
- Dynamic color system
- Theme change event handler

---

### 2.4 Widget Theme Compliance
**Priority:** HIGH
**Dependencies:** 2.3 complete

**Tasks:**
- Verify each widget type inherits OBS theme correctly:
  - QPushButton
  - QComboBox
  - QLineEdit
  - QTableWidget
  - QGroupBox
  - QLabel
  - QCheckBox
  - QProgressBar
  - Custom widgets (process status, profile cards)
- Fix widgets that don't inherit properly
- Test transparency and alpha blending
- Test dark/light theme transitions
- Test high contrast modes

**Deliverables:**
- 100% theme-compliant UI
- Widget compatibility checklist
- Edge case documentation

---

### 2.5 Theme Testing & Validation
**Priority:** MEDIUM
**Dependencies:** 2.4 complete

**Tasks:**
- Create visual regression test suite
- Capture screenshots of UI with each OBS theme
- Compare with native OBS widgets for consistency
- Test custom status indicators remain readable in all themes
- Validate accessibility (contrast ratios, color blindness)
- Get community feedback on theme integration

**Deliverables:**
- Screenshot library for all themes
- Accessibility report
- User acceptance testing results

---

## Phase 3: Restreamer API Completion (Week 2-4)

### 3.1 Configuration Management API
**Priority:** HIGH
**API Endpoints:** `/api/v3/config/*`

**Tasks:**
- Define `restreamer_config_t` structure in restreamer-api.h
- Implement `restreamer_api_get_config()`
- Implement `restreamer_api_set_config()`
- Implement `restreamer_api_reload_config()`
- Add JSON parsing for nested config structure
- Handle config validation errors from server
- Write unit tests for config API (15+ test cases)
- Add integration test with real Restreamer instance

**Deliverables:**
- Configuration management API
- Config data structures
- 15+ unit tests
- Integration test

**Impact:** Enables viewing and editing global Restreamer settings from OBS

---

### 3.2 Process State & Monitoring API
**Priority:** HIGH
**API Endpoints:** `/api/v3/process/{id}/state`, `/api/v3/process/{id}/probe`

**Tasks:**
- Define `restreamer_process_state_t` structure
  - Fields: frames_in, frames_out, frames_dropped, fps, bitrate, progress, uptime
- Define `restreamer_probe_info_t` structure
  - Fields: codec, resolution, pixel_format, framerate, audio_codec, sample_rate
- Implement `restreamer_api_get_process_state()`
- Implement `restreamer_api_probe_input()`
- Implement `restreamer_api_get_process_config()`
- Add real-time state polling with configurable interval
- Parse FFmpeg probe output JSON
- Calculate derived metrics (drop percentage, average bitrate)
- Write unit tests (20+ test cases)

**Deliverables:**
- Process state API
- Probe API
- State data structures
- 20+ unit tests

**Impact:** Enables real-time monitoring of stream health and detailed input analysis

---

### 3.3 Metrics & Performance API
**Priority:** MEDIUM
**API Endpoints:** `/api/v3/metrics`, `/metrics` (Prometheus)

**Tasks:**
- Define `restreamer_metrics_t` structure
  - System metrics: CPU, memory, network, disk
  - Process metrics: per-process CPU, memory
  - Stream metrics: bitrate, fps, drop rate
- Define `restreamer_metric_query_t` for custom queries
- Implement `restreamer_api_get_metrics()`
- Implement `restreamer_api_query_metrics()`
- Implement `restreamer_api_get_prometheus_metrics()`
- Parse Prometheus text format
- Convert to structured C data types
- Add time-series data storage (last 60 seconds)
- Write unit tests (10+ test cases)

**Deliverables:**
- Metrics API
- Prometheus parser
- Metrics data structures
- 10+ unit tests

**Impact:** Enables performance monitoring and trend analysis

---

### 3.4 Metadata Storage API
**Priority:** MEDIUM
**API Endpoints:** `/api/v3/metadata/*`

**Tasks:**
- Implement `restreamer_api_get_metadata(key)`
- Implement `restreamer_api_set_metadata(key, value)`
- Implement `restreamer_api_get_process_metadata(id, key)`
- Implement `restreamer_api_set_process_metadata(id, key, value)`
- Implement `restreamer_api_delete_metadata(key)`
- Add metadata cache to reduce API calls
- Support JSON value types (string, number, boolean, object, array)
- Write unit tests (8+ test cases)

**Deliverables:**
- Metadata API
- Metadata cache system
- 8+ unit tests

**Impact:** Enables storing custom notes, tags, and settings per profile

---

### 3.5 File System & Recording API
**Priority:** HIGH (required for recording feature)
**API Endpoints:** `/api/v3/fs/*`

**Tasks:**
- Define `restreamer_file_info_t` structure
  - Fields: name, path, size, modified_time, is_directory, mime_type
- Define `restreamer_file_list_t` structure (array with count)
- Implement `restreamer_api_list_filesystems()`
- Implement `restreamer_api_list_files(storage, path)`
- Implement `restreamer_api_get_file_info(storage, path)`
- Implement `restreamer_api_download_file(storage, path, destination, callback)`
  - Add progress callback system
  - Support resumable downloads
  - Add checksum verification
- Implement `restreamer_api_delete_file(storage, path)`
- Implement `restreamer_api_upload_file(storage, path, source, callback)`
- Add download queue management
- Write unit tests (12+ test cases)

**Deliverables:**
- File system API
- Download manager with progress callbacks
- File data structures
- 12+ unit tests

**Impact:** Enables recording management and file downloads

---

### 3.6 Additional API Coverage
**Priority:** LOW
**API Endpoints:** Various

**Tasks:**
- **FFmpeg Skills API**
  - `restreamer_api_get_skills()`
  - `restreamer_api_reload_skills()`
- **Protocol Monitoring API**
  - `restreamer_api_get_rtmp_streams()`
  - `restreamer_api_get_srt_streams()`
- **Playout Management API**
  - `restreamer_api_get_playout_status(id)`
  - `restreamer_api_switch_input_stream(id, source)`
  - `restreamer_api_reopen_input(id)`
- **Widget System API**
  - `restreamer_api_get_widget_process(id)`
- **Logs & Debugging API**
  - `restreamer_api_get_logs(level, limit)`
  - `restreamer_api_get_process_log(id, limit)`
- Write unit tests for each API group (5+ test cases per group)

**Deliverables:**
- Complete API coverage (100% of Restreamer v3 API)
- 25+ additional unit tests

**Impact:** Full feature parity with Restreamer web UI

---

## Phase 4: Recording Feature Implementation (Week 4-5)

### 4.1 Backend Recording Configuration
**Priority:** HIGH
**Dependencies:** 3.5 (File System API) complete

**Tasks:**
- Extend `output_profile_t` structure to include recording settings:
  - `enable_recording` (bool)
  - `recording_path` (string)
  - `recording_format` (enum: MP4, MKV, FLV, TS)
  - `recording_quality` (enum: COPY, HIGH, MEDIUM, LOW)
  - `recording_encoder` (string, optional override)
- Modify `restreamer_api_create_process()` to include file outputs
- Generate FFmpeg command with both streaming and file outputs
- Implement recording-only profiles (no streaming destinations)
- Add recording path validation
- Support recording naming templates (timestamp, profile name, etc.)
- Handle disk space monitoring
- Implement recording rotation (max file size, max duration)

**Deliverables:**
- Recording-enabled output profiles
- FFmpeg command generation for recording
- Recording configuration structure

---

### 4.2 Recording Lifecycle Management
**Priority:** HIGH
**Dependencies:** 4.1 complete

**Tasks:**
- Implement recording start logic
  - Validate recording path exists and is writable
  - Check available disk space
  - Start process with file output
- Implement recording stop logic
  - Graceful file finalization
  - Verify file integrity
  - Update file list
- Implement recording pause/resume (if supported by Restreamer)
- Add recording state to profile status
- Implement automatic recording on profile start (optional setting)
- Handle recording errors gracefully
  - Disk full
  - Permission denied
  - Invalid codec/format combination

**Deliverables:**
- Recording lifecycle functions
- Error handling for recording scenarios
- Recording state management

---

### 4.3 Recording Browser UI
**Priority:** HIGH
**Dependencies:** 3.5 (File System API), 4.2 complete

**Tasks:**
- Create "Recordings" collapsible section in dock UI
- Add storage location selector (dropdown of available filesystems)
- Implement recording file table widget:
  - Columns: Name, Size, Duration, Date, Actions
  - Sortable by any column
  - Search/filter functionality
  - Multi-select support
- Add action buttons:
  - Refresh list
  - Download selected
  - Delete selected (with confirmation)
  - Play in OBS (add as media source)
- Implement pagination for large recording lists (100+ files)
- Add file context menu (right-click):
  - Download
  - Delete
  - Rename
  - Get shareable link (if supported)
  - Show file info
- Show total storage used and available

**Deliverables:**
- Recording browser UI section
- File table widget
- Storage management UI

---

### 4.4 Download Manager
**Priority:** HIGH
**Dependencies:** 3.5 (File System API), 4.3 complete

**Tasks:**
- Create download queue system
  - Support multiple concurrent downloads
  - Configurable max concurrent downloads (1-5)
- Implement download dialog:
  - Progress bar
  - Speed indicator (MB/s)
  - Time remaining estimate
  - Cancel button
  - Pause/resume button (if supported)
- Add background download support:
  - Minimize dialog to notification
  - Show progress in system tray
  - Notify on completion
- Implement resumable downloads:
  - Save partial download state
  - Resume from last byte on failure
- Add download history tracking:
  - Log successful downloads
  - Log failed downloads with errors
- Implement checksum verification:
  - Request checksum from server if available
  - Verify file integrity after download
  - Warn user if checksum mismatch

**Deliverables:**
- Download manager with queue
- Download progress UI
- Resumable download support
- Download history log

---

### 4.5 Recording Configuration UI
**Priority:** MEDIUM
**Dependencies:** 4.1 complete

**Tasks:**
- Add recording settings to "Configure Profile" dialog:
  - "Enable Recording" checkbox
  - Recording path text field with browse button
  - Recording format dropdown (MP4, MKV, FLV, TS)
  - Recording quality dropdown (Stream Copy, High, Medium, Low)
  - Advanced: Custom encoder settings
  - Template variables for filename (%timestamp%, %profile%, %date%)
- Add recording-only profile option (no streaming destinations)
- Add "Test Recording" button to verify settings
- Show estimated file size based on bitrate and duration
- Validate settings before saving:
  - Path exists and is writable
  - Format compatible with codecs
  - Sufficient disk space
- Add tooltips explaining each setting

**Deliverables:**
- Recording configuration UI
- Settings validation
- Help text and tooltips

---

## Phase 5: Hybrid UI Redesign (Week 5-6)

### 5.1 Collapsible Section Framework
**Priority:** HIGH

**Tasks:**
- Design and implement `CollapsibleSection` custom Qt widget
  - Header with title label
  - Expand/collapse button (chevron icon)
  - Optional action buttons in header (aligned right)
  - Animated expand/collapse transition (200ms)
  - Content container with any child widgets
  - Collapsed state shows only header
- Implement state persistence:
  - Save collapsed/expanded state per section
  - Restore state on dock reopen
  - Store in OBS scene collection data
- Add keyboard navigation:
  - Tab to next section
  - Enter to toggle expand/collapse
  - Arrow keys to navigate within section
- Style to match OBS theme:
  - Use OBS QPalette colors
  - Match OBS widget styling
  - Respect theme changes

**Deliverables:**
- Reusable `CollapsibleSection` widget
- State persistence system
- Keyboard navigation support

---

### 5.2 Remove Tab-Based Layout
**Priority:** HIGH
**Dependencies:** 5.1 complete

**Tasks:**
- Remove `QTabWidget` from dock UI
- Create vertical scroll area as main container
- Replace tabs with collapsible sections:
  - Connection → Connection Section
  - Profiles → Profiles Section
  - Processes → Monitoring Section
  - (new) → Recordings Section
- Preserve all existing functionality
- Maintain logical grouping of related controls
- Test scrolling with many sections
- Optimize scroll performance

**Deliverables:**
- Tab-free dock layout
- Scrollable vertical layout
- All sections as collapsible widgets

---

### 5.3 Section Reorganization
**Priority:** HIGH
**Dependencies:** 5.2 complete

**Tasks:**

#### **Connection Section** (Always visible, compact)
- Move to top of dock
- Compact grid layout (2 columns):
  - Host | Port
  - Protocol (HTTP/HTTPS) | Credentials button
- Inline status indicator (colored dot + text)
- Test Connection button inline with status
- Minimize to just status dot when collapsed
- Quick reconnect on click

#### **Bridge Section** (Collapsible, collapsed by default)
- Horizontal RTMP URL (copy button)
- Vertical RTMP URL (copy button)
- Auto-start checkbox
- Enable/disable toggle

#### **Profiles Section** (Expanded by default)
- Top bar:
  - Profile dropdown (quick switcher)
  - Quick action buttons: ▶ Start | ■ Stop | ⚙ Configure
  - Profile status indicator (🟢 Active / ⚫ Inactive / 🔴 Error)
- Expanded view:
  - Active profile card with live stats:
    - Uptime
    - CPU usage (with sparkline graph)
    - Memory usage (with sparkline graph)
    - Current bitrate (with sparkline graph)
    - Frames processed / dropped
  - Destination list:
    - Per-destination status (🟢/🔴/🟡)
    - Per-destination bitrate
    - Add/Edit/Remove buttons
  - Profile management:
    - Create New Profile button
    - Duplicate Profile button
    - Delete Profile button
- Collapsed view:
  - Just profile name and status icon

#### **Monitoring Section** (Collapsible)
- Process list (table):
  - Columns: ID, Type, State, Actions
  - Select to show details below
- Selected process details (two-column grid):
  - Basic info: ID, type, state, uptime
  - Resources: CPU %, memory, threads
  - Streams: Input status, output status
  - Metrics: Frames in/out, dropped %, FPS, bitrate
- Action buttons:
  - Probe Input
  - View Metrics
  - View Logs
  - Restart Process
- Sessions table:
  - Active viewers/connections
  - Protocol, IP, duration

#### **Recordings Section** (Collapsible, new)
- Storage selector dropdown
- Recording list table (from 4.3)
- Action buttons (Download, Delete, Refresh)
- Recording settings button
- Total storage used/available

#### **System Configuration Section** (Collapsed by default)
- View Global Config button
- Edit Global Config dialog
- Reload Config button
- View FFmpeg Skills button
- Clear Cache button

#### **Advanced Section** (Collapsed by default)
- Manual multistream setup (existing functionality)
- RTMP/SRT stream monitoring
- Protocol viewer
- Debug logs viewer

**Deliverables:**
- 7 reorganized sections
- Improved information density
- Faster access to common actions

---

### 5.4 Profile Quick Switcher
**Priority:** HIGH
**Dependencies:** 5.3 complete

**Tasks:**
- Implement profile dropdown in Profiles Section header
- Populate from profile manager
- Show profile status icon in dropdown items:
  - 🟢 = Active and running
  - 🟡 = Active but stopped
  - 🔴 = Error state
  - ⚫ = Inactive
- Implement quick switch on selection:
  - Save current profile state
  - Load selected profile
  - Update all UI sections
  - Restore profile settings
- Add keyboard shortcut (Ctrl+P / Cmd+P)
- Show recently used profiles at top
- Add "Create New Profile" option in dropdown
- Limit dropdown height, make scrollable if >10 profiles

**Deliverables:**
- Quick profile switcher dropdown
- Profile status indicators
- Keyboard shortcut support

---

### 5.5 View Mode System
**Priority:** MEDIUM
**Dependencies:** 5.3 complete

**Tasks:**
- Design three view modes:

  **Compact Mode:**
  - Show only Profiles Section (expanded)
  - All other sections hidden
  - Single-line stats only
  - Minimal vertical space (~300px)

  **Dashboard Mode (Default):**
  - Show overview cards for all sections
  - Each section shows summary stats when collapsed
  - Expand individual sections on demand
  - Balanced information density

  **Detailed Mode:**
  - All sections expanded by default
  - Maximum information visible
  - For advanced users

- Implement mode switcher:
  - Toggle buttons at bottom of dock
  - Icons: 📱 Compact | 📊 Dashboard | 📋 Detailed
  - Save preferred mode in settings
  - Smooth animated transitions (300ms)
- Persist mode preference per scene collection
- Add keyboard shortcuts (Ctrl+1/2/3 or Cmd+1/2/3)

**Deliverables:**
- Three view modes
- Mode switcher UI
- Mode persistence
- Keyboard shortcuts

---

### 5.6 Sparkline Graphs (Visual Enhancement)
**Priority:** LOW
**Dependencies:** 3.2 (State API), 5.3 complete

**Tasks:**
- Design mini-graph widget:
  - Size: 80px wide × 24px tall
  - Simple line graph using QPainter
  - Show last 60 data points (1 minute at 1Hz)
  - Auto-scaling Y axis
  - Color-coded by threshold:
    - Green: Normal
    - Orange: Warning
    - Red: Critical
- Implement data collection:
  - Store circular buffer of last 60 samples
  - Update every second
  - Metrics to graph: CPU, memory, bitrate, FPS, dropped frames %
- Add sparklines to UI:
  - Next to bitrate label in profile card
  - Next to CPU/memory in process details
  - In compact mode for quick trend visualization
- Make clicking sparkline open detailed graph dialog

**Deliverables:**
- Sparkline graph widget
- Data collection system
- Integration in UI

---

## Phase 6: Enhanced Monitoring & Real-Time Updates (Week 6)

### 6.1 Real-Time State Updates
**Priority:** HIGH
**Dependencies:** 3.2 (State API) complete

**Tasks:**
- Implement background polling system:
  - Create dedicated thread for API polling
  - Configurable poll interval (default: 5 seconds)
  - Separate intervals for different data:
    - Process state: 5 seconds
    - Metrics: 10 seconds
    - File list: 30 seconds
  - Use Qt signals to update UI thread
- Update UI elements with real-time data:
  - Profile status indicators
  - CPU/memory usage
  - Bitrate graphs
  - Frame counts and drop rates
  - Process state
  - Destination health
- Implement smart polling:
  - Poll only active profiles
  - Reduce poll rate when dock hidden
  - Pause polling when OBS minimized
  - Resume polling when dock shown
- Add manual refresh buttons for each section
- Show last update timestamp

**Deliverables:**
- Background polling system
- Real-time UI updates
- Smart polling optimization

---

### 6.2 Health Indicators & Alerts
**Priority:** MEDIUM
**Dependencies:** 6.1 complete

**Tasks:**
- Define health thresholds:
  - **CPU Usage:**
    - Green: < 50%
    - Orange: 50-80%
    - Red: > 80%
  - **Memory Usage:**
    - Green: < 1GB
    - Orange: 1-2GB
    - Red: > 2GB
  - **Dropped Frames:**
    - Green: < 1%
    - Orange: 1-5%
    - Red: > 5%
  - **Bitrate Stability:**
    - Green: ±10% of target
    - Orange: ±25% of target
    - Red: > ±25% of target
- Implement color-coded indicators:
  - Use semantic colors from OBS palette
  - Apply to text, borders, or backgrounds
  - Respect current theme
- Implement alert system:
  - Desktop notification on critical errors
  - OBS log entry for warnings
  - Optional sound alerts (configurable)
  - Alert history log
- Add health summary indicator:
  - Overall system health (all profiles)
  - Show in dock title or status bar
  - Quick tooltip with issues

**Deliverables:**
- Health threshold system
- Color-coded indicators
- Alert notifications
- Health summary

---

### 6.3 Performance Optimization
**Priority:** MEDIUM
**Dependencies:** 6.1 complete

**Tasks:**
- Optimize UI rendering:
  - Only update visible widgets
  - Batch UI updates (debounce)
  - Use QTimer for efficient updates
  - Avoid unnecessary redraws
- Optimize data fetching:
  - Cache API responses
  - Use ETags/If-Modified-Since headers
  - Only fetch changed data
  - Compress API responses if supported
- Optimize memory usage:
  - Limit history buffer sizes
  - Free memory for inactive profiles
  - Use object pooling for widgets
  - Profile memory usage with valgrind
- Optimize network usage:
  - Connection pooling (keep-alive)
  - Request pipelining
  - Reduce polling frequency for stable states
- Add performance monitoring:
  - Log slow API calls
  - Log slow UI updates
  - Add performance metrics to debug mode

**Deliverables:**
- Optimized rendering pipeline
- Efficient data fetching
- Reduced memory footprint
- Performance monitoring

---

## Phase 7: Testing & Quality Assurance (Week 7)

### 7.1 Unit Testing
**Priority:** HIGH

**Tasks:**
- Achieve >85% code coverage for new code
- Test categories:
  - **API Tests (60+ tests):**
    - Configuration API (15 tests)
    - Process state API (20 tests)
    - Metrics API (10 tests)
    - Metadata API (8 tests)
    - File system API (12 tests)
    - Additional APIs (25 tests)
  - **UI Tests (30+ tests):**
    - CollapsibleSection widget (5 tests)
    - Profile switcher (5 tests)
    - Recording browser (10 tests)
    - Download manager (10 tests)
  - **Integration Tests (20+ tests):**
    - End-to-end recording workflow (5 tests)
    - Profile lifecycle (5 tests)
    - Theme switching (5 tests)
    - Multi-platform builds (5 tests)
- Use mock Restreamer HTTP server for API tests
- Use Qt Test framework for UI tests
- Run tests in CI/CD for all platforms
- Generate code coverage reports
- Fix all test failures before merge

**Deliverables:**
- 110+ automated tests
- >85% code coverage
- CI test integration
- Coverage reports

---

### 7.2 Integration Testing
**Priority:** HIGH

**Tasks:**
- Set up test Restreamer instance
- Test complete workflows:
  - Create profile → Configure → Start → Monitor → Stop
  - Enable recording → Start stream → Download recording
  - Switch profiles while streaming
  - Handle Restreamer disconnection gracefully
  - Reconnect after Restreamer restart
- Test edge cases:
  - Invalid credentials
  - Network timeout
  - Malformed API responses
  - Concurrent profile operations
  - Large file downloads (>1GB)
  - 100+ recordings in list
  - 10+ active profiles simultaneously
- Test platform-specific behavior:
  - macOS universal binary on Intel and Apple Silicon
  - Linux ARM64 on Raspberry Pi
  - Windows x64 with different OBS versions
- Performance testing:
  - Monitor CPU/memory usage over 24 hours
  - Test with maximum supported profiles
  - Test with slow network connection
  - Test with unreliable network (packet loss)

**Deliverables:**
- Integration test suite
- Edge case test results
- Platform-specific test results
- Performance test report

---

### 7.3 UI/UX Testing
**Priority:** HIGH

**Tasks:**
- Theme testing:
  - Test with all OBS themes:
    - Yami (dark)
    - Grey (default dark)
    - Acri (blue dark)
    - Dark (pure dark)
    - Rachni (purple dark)
    - Light (light theme)
  - Verify colors match native OBS widgets
  - Test theme switching while plugin running
  - Verify no hardcoded colors remain
- Accessibility testing:
  - Check color contrast ratios (WCAG AA)
  - Test with color blindness simulators
  - Test keyboard navigation
  - Test screen reader compatibility
  - Test high DPI scaling (150%, 200%)
- Usability testing:
  - Get feedback from 5+ users
  - Test common workflows
  - Measure time to complete tasks
  - Identify confusing UI elements
  - Test discoverability of features
- Cross-platform UI testing:
  - Test on Windows 10, 11
  - Test on macOS 12, 13, 14, 15
  - Test on Ubuntu 22.04, 24.04, Fedora
  - Test docked vs floating window
  - Test different screen sizes (1080p, 1440p, 4K)

**Deliverables:**
- Theme compatibility matrix
- Accessibility report
- Usability test results
- Cross-platform UI validation

---

### 7.4 Memory & Performance Testing
**Priority:** HIGH

**Tasks:**
- Memory leak detection:
  - Run with AddressSanitizer (ASAN)
  - Run with Valgrind on Linux
  - Monitor memory usage over 24+ hours
  - Test profile create/delete cycles (100+ iterations)
  - Test recording download cycles
- Performance profiling:
  - Profile CPU usage with perf/Instruments
  - Identify hot paths
  - Optimize bottlenecks
  - Measure UI responsiveness (frame time)
  - Test with 100+ recordings
  - Test with 20+ active profiles
- Resource usage baseline:
  - Document idle memory usage
  - Document CPU usage during streaming
  - Document network bandwidth usage
  - Compare to v0.9.0 baseline

**Deliverables:**
- Zero memory leaks
- Performance profile report
- Resource usage documentation
- Optimization recommendations

---

## Phase 8: Documentation (Week 7-8)

### 8.1 User Documentation
**Priority:** HIGH

**Tasks:**
- Update README.md:
  - Add v2.0 features overview
  - Update screenshots (all view modes, all themes)
  - Add platform support matrix
  - Update installation instructions
  - Add quick start guide
- Create USER_GUIDE.md:
  - Getting started
  - Connecting to Restreamer
  - Creating profiles
  - Configuring recordings
  - Downloading recordings
  - Using view modes
  - Monitoring stream health
  - Troubleshooting common issues
- Create RECORDING_GUIDE.md:
  - Recording setup walkthrough
  - Storage management
  - File format recommendations
  - Quality vs file size tradeoffs
  - Downloading and managing recordings
  - Troubleshooting recording issues
- Create VIDEO_TUTORIAL_SCRIPT.md:
  - Script for video tutorial creation
  - Screen recording notes
  - Narration text
  - Key features to demonstrate

**Deliverables:**
- Updated README
- Comprehensive user guide
- Recording guide
- Video tutorial script

---

### 8.2 Developer Documentation
**Priority:** MEDIUM

**Tasks:**
- Update PLUGIN_DOCUMENTATION.md:
  - Architecture overview
  - New API functions
  - UI component hierarchy
  - Data flow diagrams
  - Thread safety notes
- Update API_GAP_ANALYSIS.md:
  - Mark all implemented APIs as complete
  - Update priority assessments
  - Document any remaining gaps
  - Future API roadmap
- Create THEMING_GUIDE.md:
  - How theme integration works
  - Color palette usage
  - Adding new themed widgets
  - Testing theme compatibility
- Create BUILD_GUIDE.md:
  - Platform-specific build instructions
  - Universal macOS build process
  - Linux ARM64 build process
  - Cross-compilation setup
  - Dependency management
- Update CONTRIBUTING.md:
  - Code style guide
  - How to add new APIs
  - How to add new UI sections
  - Testing requirements
  - PR checklist

**Deliverables:**
- Updated plugin docs
- Theming guide
- Build guide
- Contribution guide

---

### 8.3 Release Documentation
**Priority:** HIGH

**Tasks:**
- Update CHANGELOG.md:
  - Add v2.0.0 section
  - List all new features:
    - OBS theme integration
    - Complete API coverage
    - Recording feature
    - Hybrid UI redesign
    - Linux ARM64 support
    - macOS universal binary
  - List breaking changes:
    - Removed custom theming (auto-upgrades)
    - Changed configuration format (if any)
  - List bug fixes
  - List known limitations
- Create MIGRATION_GUIDE.md:
  - How to upgrade from v0.9.0
  - Settings compatibility
  - Profile migration
  - New configuration options
  - Feature comparison
- Create RELEASE_NOTES_v2.0.0.md:
  - Feature highlights
  - Screenshots/GIFs
  - Installation instructions
  - Known issues
  - Future roadmap
- Update website/landing page (if exists):
  - Feature list
  - Screenshots
  - Download links
  - System requirements

**Deliverables:**
- Complete changelog
- Migration guide
- Release notes
- Updated website

---

## Phase 9: Release Preparation (Week 8)

### 9.1 Beta Release
**Priority:** HIGH

**Tasks:**
- Version bump to v2.0.0-beta.1:
  - Update buildspec.json
  - Update all version strings
  - Update copyright years
- Create beta builds for all platforms:
  - macOS Universal (arm64 + x86_64)
  - Windows x64
  - Linux x86_64 (.deb, .rpm)
  - Linux ARM64 (.deb, .rpm)
- Upload to GitHub Releases as pre-release:
  - Mark as "Pre-release"
  - Add beta release notes
  - Link to migration guide
  - Include SHA256 checksums
- Announce beta to community:
  - OBS Forums
  - Discord/Slack
  - Reddit r/obs
  - GitHub Discussions
- Collect feedback:
  - Monitor GitHub issues
  - Track feature requests
  - Identify critical bugs
  - Gather usability feedback

**Deliverables:**
- Beta builds for all platforms
- Beta release on GitHub
- Community announcement
- Feedback collection

---

### 9.2 Bug Fixes & Refinements
**Priority:** HIGH
**Dependencies:** 9.1 feedback received

**Tasks:**
- Triage beta feedback:
  - Classify bugs by severity (Critical, High, Medium, Low)
  - Identify showstopper issues
  - Prioritize fixes
- Fix critical bugs:
  - Crashes
  - Data loss
  - Security issues
  - Build failures
- Fix high-priority bugs:
  - Feature not working as documented
  - Major UX issues
  - Theme compatibility issues
  - Platform-specific bugs
- Address medium-priority bugs:
  - Minor UX issues
  - Edge case bugs
  - Performance issues
- Refine based on feedback:
  - Improve UI labels/tooltips
  - Add missing validation
  - Enhance error messages
  - Optimize workflows
- Release v2.0.0-beta.2 if needed
- Iterate until stable

**Deliverables:**
- Bug fix releases
- Stable beta candidate
- Issue resolution log

---

### 9.3 Final Release
**Priority:** HIGH
**Dependencies:** 9.2 stable

**Tasks:**
- Final version bump to v2.0.0:
  - Remove beta designation
  - Update all version strings
  - Final CHANGELOG updates
- Create final builds for all platforms:
  - macOS Universal
  - Windows x64
  - Linux x86_64 (.deb, .rpm)
  - Linux ARM64 (.deb, .rpm)
  - Build installers/packages
- Code signing (if certificates available):
  - Sign macOS .pkg
  - Sign Windows .exe
  - Notarize macOS package
- Create GitHub Release:
  - Tag v2.0.0
  - Upload all build artifacts
  - Include comprehensive release notes
  - Include SHA256 checksums
  - Include GPG signatures (optional)
- Update package managers:
  - Submit to OBS plugin browser (if exists)
  - Update Homebrew cask (if exists)
  - Update AUR package (if exists)
- Announce release:
  - OBS Forums (sticky thread)
  - OBS Discord
  - Reddit r/obs
  - Twitter/X announcement
  - Blog post (if exists)
  - Email newsletter (if exists)
- Monitor post-release:
  - Watch for critical issues
  - Respond to questions
  - Prepare hotfix if needed

**Deliverables:**
- v2.0.0 stable release
- All platform builds
- Release announcement
- Package manager updates

---

## Success Criteria

### Functional Requirements
- ✅ OBS theme perfectly matched across all 6 themes
- ✅ 100% of Restreamer v3 API implemented
- ✅ Recording enabled with download capability
- ✅ Hybrid collapsible UI working smoothly
- ✅ Profile quick-switcher functional
- ✅ Real-time monitoring with <5s latency
- ✅ Multiple view modes available
- ✅ macOS universal binary (arm64 + x86_64)
- ✅ Linux ARM64 support
- ✅ All existing v0.9.0 features preserved

### Quality Requirements
- ✅ >85% test coverage for new code
- ✅ Zero memory leaks (ASAN clean)
- ✅ Zero crashes in 24-hour stress test
- ✅ Builds on Windows, macOS (both archs), Linux (x64 + ARM64)
- ✅ All CI/CD pipelines passing
- ✅ Documentation complete and accurate
- ✅ <100ms UI response time
- ✅ <50MB memory footprint (idle)

### User Experience Requirements
- ✅ UI more compact than v1.x (at least 20% less vertical space)
- ✅ Faster access to common actions (max 2 clicks)
- ✅ Visual indicators for all states
- ✅ Seamless integration with OBS
- ✅ Accessibility WCAG AA compliant
- ✅ Intuitive for new users (usability test score >4/5)

---

## Risk Management

### Identified Risks & Mitigation

| Risk | Severity | Probability | Mitigation |
|------|----------|-------------|------------|
| Restreamer API v3 changes mid-development | High | Medium | Pin to specific Restreamer version, add API version detection |
| macOS universal binary dependency issues | High | High | Build dependencies from source, consider static linking |
| Qt6 behavior differences across platforms | Medium | Medium | Test on all platforms early, maintain platform-specific code paths |
| Performance degradation with 100+ recordings | Medium | Medium | Implement pagination, lazy loading, virtual scrolling |
| Large file download failures | Medium | Medium | Add resume capability, chunked downloads, checksum verification |
| Theme inheritance edge cases | Low | High | Test with all themes, create fallback styling system |
| Memory leaks in long-running sessions | High | Low | Run ASAN tests, 24-hour soak tests, valgrind profiling |
| User adoption resistance to UI changes | Medium | Medium | Provide migration guide, video tutorial, responsive to feedback |

---

## Timeline Summary

| Phase | Duration | Start | End | Key Deliverables |
|-------|----------|-------|-----|------------------|
| Phase 1: Cross-Platform Builds | 1 week | Week 1 | Week 1 | Universal macOS, Linux ARM64 |
| Phase 2: OBS Theme Integration | 1-2 weeks | Week 1 | Week 2 | Theme-compliant UI |
| Phase 3: API Completion | 2 weeks | Week 2 | Week 4 | 100% API coverage |
| Phase 4: Recording Feature | 1-2 weeks | Week 4 | Week 5 | Recording & downloads |
| Phase 5: Hybrid UI Redesign | 1-2 weeks | Week 5 | Week 6 | Collapsible layout |
| Phase 6: Enhanced Monitoring | 1 week | Week 6 | Week 6 | Real-time updates |
| Phase 7: Testing & QA | 1 week | Week 7 | Week 7 | 110+ tests passing |
| Phase 8: Documentation | 1-2 weeks | Week 7 | Week 8 | Complete docs |
| Phase 9: Release | 1 week | Week 8 | Week 8 | v2.0.0 shipped |

**Total Estimated Time: 8 weeks**

---

## Dependencies & Blockers

### External Dependencies
- Restreamer v3 API stability
- OBS Studio 28.0+ compatibility
- Qt6 framework availability
- jansson universal binary for macOS
- GitHub Actions ARM64 runners (or cross-compilation)

### Internal Dependencies
- Phase 3 must complete before Phase 4 (recording needs File System API)
- Phase 5.1 must complete before Phase 5.2 (need CollapsibleSection widget)
- Phase 7 must complete before Phase 9 (no release without passing tests)
- Phase 2 should complete before Phase 5 (theme system before UI redesign)

---

## Next Steps

1. **Review and approve this plan** with stakeholders
2. **Create GitHub project board** with all tasks
3. **Create v2.0.0 milestone** on GitHub
4. **Create feature branch** `feature/v2-overhaul`
5. **Begin Phase 1** (Cross-Platform Builds)
6. **Schedule weekly progress reviews** every Friday
7. **Set up automated testing pipeline** for all platforms
8. **Create communication plan** for beta testers

---

## Post-Release Roadmap (v2.1+)

Future enhancements to consider after v2.0 release:

- **WebSocket API integration** for real-time events (no polling)
- **Process templates system** for common configurations
- **Batch operations** (start/stop multiple profiles)
- **Advanced monitoring dashboard** with historical graphs
- **Plugin API** for community extensions
- **Multi-language support** (i18n)
- **Cloud sync** for profiles (optional)
- **Mobile companion app** for monitoring
- **AI-powered quality optimization** suggestions
- **Integration with OBS Advanced Scene Switcher**
