# datarhei/Restreamer Core API - Gap Analysis

## Overview
This document analyzes the current obs-polyemesis implementation against the complete datarhei/Restreamer Core API (v3) to identify missing functionality that should be exposed to users.

---

## Current Implementation Status

### ✅ Currently Implemented (restreamer-api.h/c)

| Feature | Endpoint | Status |
|---------|----------|--------|
| **Authentication** | `/api/login` | ✅ Implemented with JWT |
| **Connection Test** | `/ping` | ✅ Basic health check |
| **Process List** | `GET /api/v3/process` | ✅ Implemented |
| **Process Details** | `GET /api/v3/process/{id}` | ✅ Implemented |
| **Start Process** | `PUT /api/v3/process/{id}/command` (start) | ✅ Implemented |
| **Stop Process** | `PUT /api/v3/process/{id}/command` (stop) | ✅ Implemented |
| **Restart Process** | `PUT /api/v3/process/{id}/command` (restart) | ✅ Implemented |
| **Create Process** | `POST /api/v3/process` | ✅ Implemented (multistream) |
| **Delete Process** | `DELETE /api/v3/process/{id}` | ✅ Implemented |
| **Get Sessions** | `GET /api/v3/session/active` | ✅ Implemented |
| **Process Logs** | `GET /api/v3/process/{id}/report` | ✅ Implemented |

---

## ❌ Missing Critical API Endpoints

### 1. **Configuration Management** (Priority: HIGH)
Users cannot view or modify Restreamer configuration from the plugin.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/config` | GET | Retrieve active configuration | View current Restreamer settings |
| `/api/v3/config` | PUT | Update configuration | Modify settings without web UI |
| `/api/v3/config/reload` | GET | Reload configuration | Apply config changes |

**Impact**: Users must use web UI for configuration changes.

---

### 2. **Process State & Monitoring** (Priority: HIGH)
Users cannot monitor detailed process state or probe input streams.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/process/{id}/state` | GET | Get process progress/state | Real-time process monitoring |
| `/api/v3/process/{id}/probe` | GET | Analyze input stream details | Stream diagnostics |
| `/api/v3/process/{id}/config` | GET | Get process config | View/verify process settings |

**Impact**: Limited visibility into process health and stream quality.

---

### 3. **Playout Management** (Priority: MEDIUM)
Users cannot manage input streams dynamically.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/process/{id}/playout/{inputid}/status` | GET | Get playout status | Monitor input health |
| `/api/v3/process/{id}/playout/{inputid}/stream` | PUT | Switch input stream URL | Dynamic source switching |
| `/api/v3/process/{id}/playout/{inputid}/reopen` | GET | Reopen input connection | Recover from input failures |
| `/api/v3/process/{id}/playout/{inputid}/keyframe/{name}` | GET | Get last keyframe | Thumbnail/preview generation |
| `/api/v3/process/{id}/playout/{inputid}/errorframe/{name}` | POST | Upload error placeholder | Custom error screens |

**Impact**: Cannot dynamically manage inputs or recover from failures.

---

### 4. **Metadata Storage** (Priority: MEDIUM)
Users cannot store custom metadata for processes.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/metadata/{key}` | GET/PUT | Global metadata storage | Store plugin settings |
| `/api/v3/process/{id}/metadata/{key}` | GET/PUT | Process-specific metadata | Store profile notes/tags |

**Impact**: No way to store custom data or annotations.

---

### 5. **File System Access** (Priority: LOW)
Users cannot manage recorded files or access storage.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/fs` | GET | List filesystems | View available storage |
| `/api/v3/fs/{storage}` | GET | List files | Browse recordings |
| `/api/v3/fs/{storage}/{filepath}` | GET/PUT/DELETE | File operations | Manage recordings |

**Impact**: Cannot manage recordings from OBS.

---

### 6. **Metrics & Monitoring** (Priority: MEDIUM)
Users cannot access detailed performance metrics.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/metrics` | GET | List available metrics | Know what's measurable |
| `/api/v3/metrics` | POST | Query collected metrics | Performance graphs/stats |
| `/metrics` | GET | Prometheus metrics | Integration with monitoring |
| `/profiling` | GET | Application profiling | Debug performance issues |

**Impact**: Limited performance visibility.

---

### 7. **Protocol-Specific Monitoring** (Priority: LOW)
Users cannot monitor RTMP/SRT streams independently.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/rtmp` | GET | List active RTMP streams | Monitor RTMP inputs |
| `/api/v3/srt` | GET | List active SRT streams | Monitor SRT inputs |

**Impact**: Cannot monitor protocol-specific streams.

---

### 8. **FFmpeg Capabilities** (Priority: LOW)
Users cannot query FFmpeg capabilities.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/skills` | GET | List FFmpeg capabilities | Know available codecs/formats |
| `/api/v3/skills/reload` | GET | Refresh capabilities | Detect new FFmpeg features |

**Impact**: Cannot validate codec/format support.

---

### 9. **Session Management** (Priority: LOW)
Enhanced session details not available.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/v3/session` | GET | Session summary | Overall viewer stats |

**Impact**: Limited session analytics.

---

### 10. **Advanced Authentication** (Priority: VERY LOW)
Alternative auth methods not implemented.

| Endpoint | Method | Purpose | User Benefit |
|----------|--------|---------|--------------|
| `/api/login/refresh` | GET | Refresh access token | Extended sessions |
| `/api/graph` | GET | GraphQL playground | Advanced queries |
| `/api/graph/query` | POST | Execute GraphQL | Custom data fetching |

**Impact**: No token refresh (re-login required), no GraphQL support.

---

## User Workflow Analysis

### Current User Flow (What Works)
1. ✅ User connects to Restreamer instance (host, port, credentials)
2. ✅ User creates output profile with name and settings
3. ✅ User adds multiple stream destinations (Twitch, YouTube, etc.)
4. ✅ User starts profile → creates FFmpeg process
5. ✅ User monitors basic process status (running/stopped)
6. ✅ User stops/restarts processes
7. ✅ User views active sessions

### Missing Workflow Elements (What Doesn't Work)
1. ❌ User cannot view/modify Restreamer configuration
2. ❌ User cannot probe input stream quality/codec info
3. ❌ User cannot see detailed process progress/state
4. ❌ User cannot switch input streams dynamically
5. ❌ User cannot recover from input failures without restart
6. ❌ User cannot access performance metrics
7. ❌ User cannot manage recorded files
8. ❌ User cannot store custom notes/metadata per profile
9. ❌ User cannot validate codec support before starting
10. ❌ User cannot monitor RTMP/SRT streams independently

---

## Recommended Implementation Priorities

### Phase 1: Essential Monitoring (HIGH PRIORITY)
These are critical for production use:

1. **Process State API** (`/api/v3/process/{id}/state`)
   - Real-time progress tracking
   - Frame counts, bitrate, dropped frames
   - Essential for quality monitoring

2. **Input Probe API** (`/api/v3/process/{id}/probe`)
   - Stream codec detection
   - Resolution/framerate validation
   - Input health checks

3. **Configuration API** (`/api/v3/config` GET/PUT)
   - View Restreamer settings
   - Modify configuration
   - Reload without restart

4. **Metrics API** (`/api/v3/metrics`)
   - CPU/Memory usage
   - Bandwidth tracking
   - Performance analytics

### Phase 2: Advanced Control (MEDIUM PRIORITY)
Enhances user experience:

1. **Playout Management**
   - Dynamic input switching
   - Input recovery
   - Error frame handling

2. **Metadata Storage**
   - Profile annotations
   - Custom settings storage

3. **Session Analytics**
   - Enhanced viewer stats
   - Bandwidth breakdown

### Phase 3: Nice to Have (LOW PRIORITY)
Optional enhancements:

1. File system access
2. RTMP/SRT monitoring
3. FFmpeg capabilities detection
4. GraphQL support

---

## Recommended UI Additions

### Connection Tab Enhancements
```
┌─ Connection Settings ────────────────────────┐
│ Host: [localhost          ] Port: [8080]    │
│ ☑ Use HTTPS                                 │
│ Username: [admin         ]                  │
│ Password: [••••••••••    ]                  │
│                                             │
│ [ Test Connection ] [View Config]          │
│ Status: ● Connected                         │
└─────────────────────────────────────────────┘

┌─ Restreamer Configuration ──────────────────┐
│ ► General Settings                          │
│ ► RTMP Settings                             │
│ ► Storage Settings                          │
│ ► Resource Limits                           │
│                                             │
│ [ Refresh ] [ Save Changes ]                │
└─────────────────────────────────────────────┘
```

### Process Details Enhancements
```
┌─ Process: profile_123 ──────────────────────┐
│ Status: ● Running                           │
│ Uptime: 02:15:33                           │
│ CPU: 45% | Memory: 512 MB                  │
│                                             │
│ ┌─ Input Stream ─────────────────────────┐ │
│ │ Codec: H.264 (High Profile)           │ │
│ │ Resolution: 1920x1080 @ 60fps         │ │
│ │ Bitrate: 6000 kbps                    │ │
│ │ Audio: AAC 192 kbps                   │ │
│ │ [ Probe Input ] [ Switch Source ]     │ │
│ └───────────────────────────────────────┘ │
│                                             │
│ ┌─ Performance ──────────────────────────┐ │
│ │ Frames: 8,100 / 8,112 (99.85%)        │ │
│ │ Dropped: 12 frames                    │ │
│ │ Output Bitrate: 5,875 kbps            │ │
│ │ Network: ↑ 6.2 Mbps ↓ 50 Kbps         │ │
│ └───────────────────────────────────────┘ │
│                                             │
│ [ Start ] [ Stop ] [ Restart ] [ Logs ]    │
└─────────────────────────────────────────────┘
```

### Profile Manager Enhancements
```
┌─ Profile: Multistream Setup ────────────────┐
│ Input URL: rtmp://localhost/live/obs       │
│ Orientation: Horizontal (16:9)             │
│                                             │
│ ┌─ Destinations (3) ──────────────────────┐│
│ │ ☑ Twitch (1920x1080 @ 60fps, 6000kbps) ││
│ │   Frames: 8,100 | Dropped: 5           ││
│ │   Status: ● Streaming                   ││
│ │                                         ││
│ │ ☑ YouTube (1920x1080 @ 60fps, 8000kbps)││
│ │   Frames: 8,098 | Dropped: 7           ││
│ │   Status: ● Streaming                   ││
│ │                                         ││
│ │ ☐ Facebook (Disabled)                   ││
│ │   [ Edit ] [ Remove ]                   ││
│ │                                         ││
│ │ [ Add Destination ]                     ││
│ └─────────────────────────────────────────┘│
│                                             │
│ ┌─ Notes & Metadata ──────────────────────┐│
│ │ [Production stream for gaming channel]  ││
│ │ [ Save Notes ]                          ││
│ └─────────────────────────────────────────┘│
│                                             │
│ [ Start Profile ] [ Stop Profile ]         │
└─────────────────────────────────────────────┘
```

---

## Implementation Recommendations

### 1. Extend restreamer-api.h with new functions:
```c
/* Configuration API */
bool restreamer_api_get_config(restreamer_api_t *api, char **config_json);
bool restreamer_api_set_config(restreamer_api_t *api, const char *config_json);
bool restreamer_api_reload_config(restreamer_api_t *api);

/* Process State API */
bool restreamer_api_get_process_state(restreamer_api_t *api, const char *process_id,
                                      restreamer_process_state_t *state);

/* Input Probe API */
bool restreamer_api_probe_input(restreamer_api_t *api, const char *process_id,
                                restreamer_probe_info_t *info);

/* Metrics API */
bool restreamer_api_get_metrics(restreamer_api_t *api, restreamer_metrics_t *metrics);
bool restreamer_api_query_metrics(restreamer_api_t *api, const char *query_json,
                                  char **result_json);

/* Metadata API */
bool restreamer_api_get_metadata(restreamer_api_t *api, const char *key, char **value);
bool restreamer_api_set_metadata(restreamer_api_t *api, const char *key, const char *value);
bool restreamer_api_get_process_metadata(restreamer_api_t *api, const char *process_id,
                                         const char *key, char **value);
bool restreamer_api_set_process_metadata(restreamer_api_t *api, const char *process_id,
                                         const char *key, const char *value);

/* Playout API */
bool restreamer_api_get_playout_status(restreamer_api_t *api, const char *process_id,
                                       const char *input_id, restreamer_playout_status_t *status);
bool restreamer_api_switch_input_stream(restreamer_api_t *api, const char *process_id,
                                        const char *input_id, const char *new_url);
bool restreamer_api_reopen_input(restreamer_api_t *api, const char *process_id,
                                 const char *input_id);
```

### 2. Add new UI tabs in restreamer-dock.cpp:
- Configuration tab for Restreamer settings
- Metrics tab for performance monitoring
- Advanced process details with probe info

### 3. Enhance profile configuration dialog:
- Add metadata/notes field
- Show per-destination statistics
- Add input stream probe button

---

## Conclusion

The current implementation covers **basic process management** (create, start, stop, list). However, it's missing **critical monitoring and configuration APIs** that are essential for production use.

**Key Gaps:**
1. No process state/progress monitoring
2. No input stream probing/diagnostics
3. No configuration management
4. No performance metrics
5. No metadata storage
6. No dynamic input management

**Recommendation:** Prioritize Phase 1 (Essential Monitoring) APIs to provide users with the visibility they need for production streaming workflows.
