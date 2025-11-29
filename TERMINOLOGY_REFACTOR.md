# Terminology Refactor: Profile → Channel, Destination → Output

This document tracks all changes made during the terminology refactor to align with Restreamer conventions.

## Overview

| Old Term | New Term | Rationale |
|----------|----------|-----------|
| Profile | Channel | More user-friendly, aligns with streaming terminology |
| Destination | Output | Matches Restreamer's "Outputs" terminology |

## Files Changed

### Core Header Files
- [x] `src/restreamer-output-profile.h` → `src/restreamer-channel.h`
- [x] `src/restreamer-output-profile.c` → `src/restreamer-channel.c`

### UI Files
- [x] `src/restreamer-dock.cpp`
- [x] `src/restreamer-dock.h`
- [x] `src/profile-widget.cpp` → `src/channel-widget.cpp`
- [x] `src/profile-widget.h` → `src/channel-widget.h`
- [x] `src/profile-edit-dialog.cpp` → `src/channel-edit-dialog.cpp`
- [x] `src/profile-edit-dialog.h` → `src/channel-edit-dialog.h`
- [x] `src/destination-widget.cpp` → `src/output-widget.cpp`
- [x] `src/destination-widget.h` → `src/output-widget.h`

### Locale Files
- [x] `data/locale/en-US.ini`
- [x] `data/locale/de-DE.ini`
- [x] `data/locale/es-ES.ini`
- [x] `data/locale/fr-FR.ini`
- [x] `data/locale/it-IT.ini`
- [x] `data/locale/ja-JP.ini`
- [x] `data/locale/ko-KR.ini`
- [x] `data/locale/pt-BR.ini`
- [x] `data/locale/ru-RU.ini`
- [x] `data/locale/zh-CN.ini`
- [x] `data/locale/zh-TW.ini`

### Test Files
- [x] `tests/test_output_profile.c` → `tests/test_channel.c`
- [x] `tests/test_profile_coverage.c` → `tests/test_channel_coverage.c`
- [x] Other test files referencing profiles/destinations

### Build Files
- [x] `CMakeLists.txt` (update source file references)

### Plugin Files
- [x] `src/plugin-main.c` (updated function calls)

## Type Renames

| Old Type | New Type |
|----------|----------|
| `output_profile_t` | `stream_channel_t` |
| `profile_destination_t` | `channel_output_t` |
| `profile_manager_t` | `channel_manager_t` |
| `profile_status_t` | `channel_status_t` |
| `PROFILE_STATUS_*` | `CHANNEL_STATUS_*` |
| `encoding_settings_t` | (unchanged) |

## Function Renames

### Profile Manager Functions
| Old Function | New Function |
|--------------|--------------|
| `profile_manager_create` | `channel_manager_create` |
| `profile_manager_destroy` | `channel_manager_destroy` |
| `profile_manager_create_profile` | `channel_manager_create_channel` |
| `profile_manager_get_profile` | `channel_manager_get_channel` |
| `profile_manager_delete_profile` | `channel_manager_delete_channel` |
| `profile_manager_duplicate_profile` | `channel_manager_duplicate_channel` |
| `profile_manager_start_all` | `channel_manager_start_all` |
| `profile_manager_stop_all` | `channel_manager_stop_all` |
| `profile_manager_save` | `channel_manager_save` |
| `profile_manager_load` | `channel_manager_load` |

### Profile/Channel Functions
| Old Function | New Function |
|--------------|--------------|
| `profile_add_destination` | `channel_add_output` |
| `profile_remove_destination` | `channel_remove_output` |
| `profile_update_destination` | `channel_update_output` |
| `profile_get_destination` | `channel_get_output` |
| `profile_start` | `channel_start` |
| `profile_stop` | `channel_stop` |
| `profile_set_destination_backup` | `channel_set_output_backup` |
| `profile_trigger_failover` | `channel_trigger_failover` |
| `profile_restore_primary` | `channel_restore_primary` |
| `profile_check_failover` | `channel_check_failover` |
| `profile_bulk_*` | `channel_bulk_*` |
| `profile_get_default_encoding` | `channel_get_default_encoding` |
| `stream_channel_start` | `channel_start` |
| `stream_channel_stop` | `channel_stop` |
| `stream_channel_start_preview` | `channel_start_preview` |
| `stream_channel_preview_to_live` | `channel_preview_to_live` |
| `stream_channel_cancel_preview` | `channel_cancel_preview` |
| `stream_channel_check_preview_timeout` | `channel_check_preview_timeout` |

## Variable Renames

| Old Variable | New Variable |
|--------------|--------------|
| `profileManager` | `channelManager` |
| `profile_count` | `channel_count` |
| `profile_name` | `channel_name` |
| `profile_id` | `channel_id` |
| `destination_count` | `output_count` |
| `destinations` | `outputs` |
| `profileListContainer` | `channelListContainer` |
| `profileListLayout` | `channelListLayout` |
| `profileStatusLabel` | `channelStatusLabel` |
| `profileWidgets` | `channelWidgets` |
| `createProfileButton` | `createChannelButton` |
| `startAllProfilesButton` | `startAllChannelsButton` |
| `stopAllProfilesButton` | `stopAllChannelsButton` |
| `profile1/profile2/profile3` | `channel1/channel2/channel3` |
| `output_channel_t` | `stream_channel_t` |

## UI Text Changes

### Buttons & Labels
| Old Text | New Text |
|----------|----------|
| `+ New Profile` | `+ New Channel` |
| `No profiles` | `No channels` |
| `X profile(s)` | `X channel(s)` |
| `Start all profiles` | `Start all channels` |
| `Stop all profiles` | `Stop all channels` |
| `1 destination` | `1 output` |
| `X destinations` | `X outputs` |

### Dialog Titles
| Old Text | New Text |
|----------|----------|
| `Create Profile` | `Create Channel` |
| `Delete Profile` | `Delete Channel` |
| `Duplicate Profile` | `Duplicate Channel` |
| `Edit Profile` | `Edit Channel` |
| `Profile Created` | `Channel Created` |
| `Profile Settings` | `Channel Settings` |
| `Profile Statistics` | `Channel Statistics` |
| `Export Profile Configuration` | `Export Channel Configuration` |
| `Add Streaming Destination` | `Add Output` |
| `Destination Settings` | `Output Settings` |
| `Edit Destination - X` | `Edit Output - X` |
| `Cannot Start Destination` | `Cannot Start Output` |
| `Cannot Stop Destination` | `Cannot Stop Output` |

### Context Menu Items
| Old Text | New Text |
|----------|----------|
| `▶ Start Profile` | `▶ Start Channel` |
| `■ Stop Profile` | `■ Stop Channel` |
| `↻ Restart Profile` | `↻ Restart Channel` |
| `✎ Edit Profile...` | `✎ Edit Channel...` |
| `📋 Duplicate Profile` | `📋 Duplicate Channel` |
| `🗑️ Delete Profile` | `🗑️ Delete Channel` |
| `⚙️ Profile Settings...` | `⚙️ Channel Settings...` |

### Tooltips
| Old Text | New Text |
|----------|----------|
| `Create new streaming profile` | `Create new streaming channel` |
| `Start all profiles` | `Start all channels` |
| `Stop all profiles` | `Stop all channels` |
| `Auto-start profile when OBS streaming starts` | `Auto-start channel when OBS streaming starts` |
| `Default maximum reconnect attempts for new profiles` | `Default maximum reconnect attempts for new channels` |

## Signal Renames

| Old Signal | New Signal |
|------------|------------|
| `destinationStartRequested` | `outputStartRequested` |
| `destinationStopRequested` | `outputStopRequested` |
| `destinationEditRequested` | `outputEditRequested` |

## Test Status

- [x] macOS tests passing (22/22 tests)
- [ ] Windows tests passing (Docker not running locally)
- [ ] Linux tests passing (Docker not running locally)

## Notes

- Log messages may retain some "profile" references for debugging backward compatibility
- Config file keys remain unchanged for settings migration compatibility
- The refactor maintains API compatibility where possible
- `multistream_config_t` retains `destination_count` and `destinations` as it's a separate API structure
- `websocket-api.cpp` not yet fully refactored (separate API layer)

## Additional Build Fixes

During the refactoring process, additional fixes were required:
1. Fixed inconsistent type names (`output_channel_t` → `stream_channel_t`)
2. Fixed struct member references (`->destination_count` → `->output_count`, `->destinations[` → `->outputs[`)
3. Fixed function calls in plugin-main.c (`output_channel_start` → `channel_start`)
4. Fixed test variable names (`profile1/profile2` → `channel1/channel2`)
5. Fixed test function calls (`stream_channel_start` → `channel_start`)
