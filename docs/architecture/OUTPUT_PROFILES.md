# Output Profiles - Multi-Stream Management

## Overview

Output Profiles are the core feature of OBS Polyemesis that enable **simultaneous streaming to multiple platforms** with different orientations, resolutions, and encoding settings. Each profile represents a complete streaming configuration that can run independently.

## Key Concepts

### What is an Output Profile?

An Output Profile is a named configuration that defines:
- **Source Orientation**: The orientation of your OBS canvas (horizontal, vertical, or auto-detect)
- **Destinations**: A list of streaming platforms to send to
- **Encoding Settings**: Per-destination video/audio settings
- **Behavior**: Auto-start, auto-reconnect, and other options

### Why Use Profiles?

**Traditional Problem**:
- OBS standard output: ONE stream to ONE platform
- Want to stream vertical to TikTok AND horizontal to Twitch? Not possible!

**Solution with Profiles**:
```
Profile "Horizontal Stream"
├─► Source: Main OBS Canvas (1920x1080)
├─► Destination 1: Twitch (1920x1080, 6000kbps)
└─► Destination 2: YouTube (1920x1080, 8000kbps)

Profile "Vertical Stream"
├─► Source: Aitum Vertical Canvas (1080x1920)
├─► Destination 1: TikTok (1080x1920, 4500kbps)
├─► Destination 2: Instagram (1080x1920, 4000kbps)
└─► Destination 3: YouTube Shorts (1080x1920, 5000kbps)

Both profiles running simultaneously!
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│             OBS Studio Canvases                      │
├─────────────────────────────────────────────────────┤
│                                                      │
│  Main Canvas (1920x1080)    Vertical Canvas (1080x1920)
│         │                            │               │
│         │                            │               │
└─────────┼────────────────────────────┼───────────────┘
          │                            │
          ▼                            ▼
┌──────────────────┐        ┌──────────────────┐
│  Profile:        │        │  Profile:        │
│  "Horizontal"    │        │  "Vertical"      │
├──────────────────┤        ├──────────────────┤
│ Destinations:    │        │ Destinations:    │
│  • Twitch        │        │  • TikTok        │
│  • YouTube       │        │  • Instagram     │
│  • Facebook      │        │  • Shorts        │
└──────────────────┘        └──────────────────┘
          │                            │
          └────────────┬───────────────┘
                       ▼
              ┌─────────────────┐
              │   Restreamer    │
              │                 │
              │ ┌─────────────┐ │
              │ │ Process 1   │ │──► Twitch, YouTube, Facebook
              │ └─────────────┘ │
              │                 │
              │ ┌─────────────┐ │
              │ │ Process 2   │ │──► TikTok, Instagram, Shorts
              │ └─────────────┘ │
              └─────────────────┘
```

## Creating a Profile

### Via Dock UI

1. **Open Restreamer Control Dock**:
   - `Docks` → `Restreamer Control`

2. **Navigate to "Output Profiles" Section**

3. **Click "Create Profile"**

4. **Configure Profile**:
   ```
   Profile Name: "Horizontal Gameplay"
   Source Orientation: Horizontal (16:9)
   ☑ Auto-detect orientation
   ☑ Auto-start with OBS
   ☑ Auto-reconnect on disconnect
   ```

5. **Add Destinations**:
   - Click "Add Destination"
   - Service: Twitch
   - Stream Key: `your-twitch-stream-key`
   - Target Orientation: Horizontal
   - Video Bitrate: 6000 kbps
   - Resolution: 1920x1080 (or 0x0 for source resolution)
   - Click "OK"

6. **Repeat for all platforms**

7. **Click "Save Profile"**

## Profile Settings

### Source Configuration

**Orientation Detection**:
- **Auto-detect** (Recommended): Automatically determines orientation from canvas size
- **Manual Override**: Force a specific orientation
  - Horizontal: 16:9, 21:9, etc.
  - Vertical: 9:16
  - Square: 1:1

**Expected Source Dimensions**:
- Width/Height: Helps profile validate it's receiving correct source

### Destination Settings

Each destination in a profile has independent settings:

#### Basic Settings
- **Service**: Twitch, YouTube, TikTok, Instagram, Facebook, Kick, X, Custom
- **Stream Key**: Your platform-specific stream key
- **Target Orientation**: How this platform should receive the stream
- **Enabled**: Toggle destination on/off without removing it

#### Video Encoding
```
Resolution:
  Width: 1920 (or 0 to use source)
  Height: 1080 (or 0 to use source)

Bitrate: 6000 kbps (0 = use default)
Frame Rate: 60 FPS (0 = use source)
```

#### Audio Encoding
```
Audio Bitrate: 160 kbps (0 = use default)
Audio Track: Track 1 (OBS audio track 1-6, 0 = default)
```

**Example - Send different audio tracks**:
```
Twitch Destination:
  Audio Track: 1 (Game + Mic + Music)

YouTube Destination:
  Audio Track: 2 (Game + Mic only, no music for copyright)
```

#### Network Settings
```
Max Bandwidth: 7000 kbps (0 = unlimited)
☐ Low Latency Mode
```

## Profile Operations

### Starting a Profile

**Manual Start**:
1. Select profile in list
2. Click "Start Profile"

**Auto-Start**:
- Enable "Auto-start with OBS" in profile settings
- Profile starts automatically when you start streaming in OBS

**Start All Profiles**:
- Click "Start All Profiles" button
- Starts every profile with auto-start enabled

### Stopping a Profile

**Manual Stop**:
1. Select profile
2. Click "Stop Profile"

**Stop All**:
- Click "Stop All Profiles"
- Emergency stop for all active streams

### Profile Status

Profiles have five states:

| Status | Icon | Description |
|--------|------|-------------|
| Inactive | ⚫ | Profile exists but not streaming |
| Starting | 🟡 | Profile is connecting to destinations |
| Active | 🟢 | Profile is live streaming |
| Stopping | 🟡 | Profile is disconnecting |
| Error | 🔴 | Profile encountered an error |

## Advanced Features

### Per-Destination Resolution & Bitrate

**Use Case**: Stream 4K to YouTube, 1080p to Twitch, 720p to TikTok

```
Profile "Multi-Quality Stream"
├─► YouTube:  3840x2160 @ 15000 kbps
├─► Twitch:   1920x1080 @ 6000 kbps
└─► TikTok:   1080x1920 @ 4000 kbps (vertical)
```

**How it works**:
1. Restreamer receives your OBS stream at source resolution
2. For each destination, Restreamer transcodes to target resolution/bitrate
3. Each platform gets optimized stream

### Audio Track Selection

**Use Case**: Remove copyrighted music from some platforms

**OBS Setup**:
- Track 1: Game + Microphone + Music
- Track 2: Game + Microphone (no music)

**Profile Setup**:
```
Twitch Destination:
  Audio Track: 1 (everything)

YouTube Destination:
  Audio Track: 2 (no music, copyright-safe)
```

### Automatic Reconnection

**Configuration**:
```
☑ Auto-reconnect on disconnect
Reconnect Delay: 5 seconds
```

**Behavior**:
- If a destination drops, profile auto-reconnects after delay
- Other destinations continue streaming unaffected
- Useful for unstable internet connections

### Bandwidth Management

**Configuration**:
```
Twitch:
  Max Bandwidth: 6500 kbps

YouTube:
  Max Bandwidth: 0 (unlimited)
```

**Use Case**:
- Prevent one destination from consuming all upload bandwidth
- Ensure stable streams to all platforms

## Practical Examples

### Example 1: Dual-Canvas Streaming (Your Workflow!)

**Goal**: Stream horizontal gameplay to Twitch/YouTube, vertical to TikTok/Instagram simultaneously

**Setup**:

**Profile 1: "Gaming - Horizontal"**
```yaml
Source: Main OBS Canvas (1920x1080)
Orientation: Horizontal

Destinations:
  - Twitch:
      Resolution: 1920x1080
      Bitrate: 6000 kbps
      Audio Track: 1

  - YouTube:
      Resolution: 1920x1080
      Bitrate: 8000 kbps
      Audio Track: 1
```

**Profile 2: "Gaming - Vertical"**
```yaml
Source: Aitum Vertical Canvas (1080x1920)
Orientation: Vertical

Destinations:
  - TikTok:
      Resolution: 1080x1920
      Bitrate: 4500 kbps
      Audio Track: 1

  - Instagram:
      Resolution: 1080x1920
      Bitrate: 4000 kbps
      Audio Track: 1

  - YouTube Shorts:
      Resolution: 1080x1920
      Bitrate: 5000 kbps
      Audio Track: 1
```

**Usage**:
1. Start both profiles
2. Stream to 5 platforms simultaneously
3. Each platform gets correct orientation
4. Monitor all streams from one OBS instance

### Example 2: Multi-Quality Streaming

**Goal**: Stream highest quality to YouTube, optimized quality to other platforms

**Profile: "Multi-Quality Broadcast"**
```yaml
Source: Main Canvas (1920x1080)

Destinations:
  - YouTube:
      Resolution: 3840x2160  # Upscale to 4K
      Bitrate: 15000 kbps

  - Twitch:
      Resolution: 1920x1080  # Native
      Bitrate: 6000 kbps

  - Facebook:
      Resolution: 1280x720   # Reduced for bandwidth
      Bitrate: 3500 kbps
```

### Example 3: Copyright-Safe Multi-Platform

**Goal**: Stream with music on Twitch, without music on YouTube

**OBS Audio Tracks**:
- Track 1: Game + Mic + Music
- Track 2: Game + Mic

**Profile: "Copyright-Safe Stream"**
```yaml
Destinations:
  - Twitch:
      Audio Track: 1  # Include music

  - YouTube:
      Audio Track: 2  # No music (copyright-safe)

  - Facebook:
      Audio Track: 2  # No music
```

## Profile Management

### Saving & Loading

**Automatic Save**:
- Profiles save automatically to OBS scene collection
- Persist across OBS restarts

**Manual Save**:
- Changes save when you click "Save Profile"

**Export Profile**:
- (Future feature) Export profile as JSON
- Share profiles with others

### Duplicating Profiles

**Use Case**: Create similar profiles quickly

1. Select existing profile
2. Click "Duplicate"
3. Rename new profile
4. Modify destinations as needed

**Example**:
- Duplicate "Gaming" → "Gaming - No Music"
- Change audio tracks to 2
- Use for copyright-safe streams

### Deleting Profiles

1. Select profile
2. Click "Delete Profile"
3. Confirm deletion

**Note**: Cannot delete active profile. Stop it first.

## Monitoring & Troubleshooting

### Profile Status Dashboard

```
Profile: Gaming - Horizontal        Status: ● Active
─────────────────────────────────────────────────────
Destination         Status    Bitrate    Dropped
─────────────────────────────────────────────────────
Twitch              ● Live    5987 kbps  12 frames
YouTube             ● Live    7893 kbps  0 frames
Facebook            ⚫ Offline -          -
─────────────────────────────────────────────────────
```

### Common Issues

**Profile won't start**:
- ✓ Check: At least one destination enabled
- ✓ Check: Valid stream keys
- ✓ Check: Restreamer connection active

**Poor quality on one platform**:
- Check per-destination bitrate settings
- Check max bandwidth limit
- Monitor upload speed

**Audio out of sync**:
- Check audio track selection
- Ensure correct OBS audio routing

**Frequent disconnects**:
- Enable auto-reconnect
- Increase reconnect delay
- Check internet stability

## Performance Considerations

### CPU/GPU Usage

**Encoding happens on Restreamer**, not OBS:
- OBS sends ONE stream to Restreamer
- Restreamer handles transcoding for all destinations
- Minimal impact on OBS performance

### Upload Bandwidth

**Calculate total bandwidth**:
```
Profile 1:
  Twitch:  6000 kbps
  YouTube: 8000 kbps
  Total:   14000 kbps (14 Mbps upload)

Profile 2:
  TikTok:     4500 kbps
  Instagram:  4000 kbps
  YouTube:    5000 kbps
  Total:      13500 kbps (13.5 Mbps upload)

Grand Total: 27.5 Mbps upload required
```

**Recommendations**:
- Test with `speedtest.net` first
- Leave 20% headroom for fluctuations
- Use bandwidth limits if needed

### Restreamer Resources

Monitor Restreamer server:
- CPU: Transcoding is CPU-intensive
- RAM: ~500MB per active destination
- Network: Must handle total output bandwidth

## Best Practices

### Profile Naming

**Good names**:
- "Gaming - Horizontal"
- "IRL - Vertical"
- "Podcast - Audio Focus"
- "Event - Multi-Platform"

**Bad names**:
- "Profile 1"
- "Test"
- "asdfjkl"

### Destination Organization

**Group by use case**:
```
Gaming Profile:
  ├─► Twitch (primary platform)
  ├─► YouTube (archive)
  └─► Facebook Gaming

IRL Profile:
  ├─► Instagram
  ├─► TikTok
  └─► YouTube Shorts
```

### Testing

**Before going live**:
1. Start profile with all destinations disabled
2. Enable one destination, test
3. Enable next destination, test
4. Repeat until all tested
5. Save working configuration

### Backup Strategy

**Recommended**:
1. Keep profiles simple initially
2. Test one platform before adding more
3. Create backup profiles for important events
4. Document custom encoder settings

## API Reference

For developers integrating with the profile system:

```c
#include "restreamer-output-profile.h"

/* Create manager */
profile_manager_t *manager = profile_manager_create(api);

/* Create profile */
output_profile_t *profile =
    profile_manager_create_profile(manager, "My Profile");

/* Add destination */
encoding_settings_t encoding = profile_get_default_encoding();
encoding.width = 1920;
encoding.height = 1080;
encoding.bitrate = 6000;

profile_add_destination(profile,
    SERVICE_TWITCH,
    "your-stream-key",
    ORIENTATION_HORIZONTAL,
    &encoding);

/* Start streaming */
profile_start(manager, profile->profile_id);

/* Stop all */
profile_manager_stop_all(manager);

/* Cleanup */
profile_manager_destroy(manager);
```

See `restreamer-output-profile.h` for complete API documentation.

## Future Enhancements

Planned features:

- **Stream Scheduling**: Start/stop profiles at specific times
- **Activity Feed**: Unified chat and alerts from all platforms
- **Health Monitoring**: Advanced metrics and alerts
- **Profile Templates**: Pre-configured profiles for common scenarios
- **Cloud Sync**: Sync profiles across multiple OBS installations

---

**Ready to start? Create your first profile and stream to multiple platforms simultaneously!**
