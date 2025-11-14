#pragma once

#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Output profile for managing multiple concurrent streams */

typedef enum {
  PROFILE_STATUS_INACTIVE, /* Profile exists but not streaming */
  PROFILE_STATUS_STARTING, /* Profile is starting streams */
  PROFILE_STATUS_ACTIVE,   /* Profile is actively streaming */
  PROFILE_STATUS_STOPPING, /* Profile is stopping streams */
  PROFILE_STATUS_PREVIEW,  /* Profile is in test/preview mode */
  PROFILE_STATUS_ERROR     /* Profile encountered an error */
} profile_status_t;

/* Per-destination encoding settings */
typedef struct {
  /* Video settings */
  uint32_t width;   /* Output width (0 = use source) */
  uint32_t height;  /* Output height (0 = use source) */
  uint32_t bitrate; /* Video bitrate in kbps (0 = use default) */
  uint32_t fps_num; /* FPS numerator (0 = use source) */
  uint32_t fps_den; /* FPS denominator (0 = use source) */

  /* Audio settings */
  uint32_t audio_bitrate; /* Audio bitrate in kbps (0 = use default) */
  uint32_t audio_track;   /* OBS audio track index (1-6, 0 = default) */

  /* Network settings */
  uint32_t max_bandwidth; /* Max bandwidth in kbps (0 = unlimited) */
  bool low_latency;       /* Enable low latency mode */
} encoding_settings_t;

/* Enhanced destination with encoding settings */
typedef struct {
  streaming_service_t service;
  char *service_name;
  char *stream_key;
  char *rtmp_url;
  stream_orientation_t target_orientation;
  encoding_settings_t encoding;
  bool enabled;

  /* Runtime stats */
  uint64_t bytes_sent;
  uint32_t current_bitrate;
  uint32_t dropped_frames;
  bool connected;

  /* Health monitoring */
  time_t last_health_check;
  uint32_t consecutive_failures;
  bool auto_reconnect_enabled;

  /* Backup/Failover */
  bool is_backup;             /* This is a backup destination */
  size_t primary_index;       /* Index of primary (if this is backup) */
  size_t backup_index;        /* Index of backup (if this is primary) */
  bool failover_active;       /* Failover is currently active */
  time_t failover_start_time; /* When failover started */
} profile_destination_t;

/* Output profile structure */
typedef struct output_profile {
  char *profile_name; /* User-friendly name */
  char *profile_id;   /* Unique identifier */

  /* Source configuration */
  stream_orientation_t
      source_orientation; /* Auto, Horizontal, Vertical, Square */
  bool auto_detect_orientation;
  uint32_t source_width;  /* Expected source width */
  uint32_t source_height; /* Expected source height */
  char *input_url;        /* RTMP input URL (rtmp://host/app/key) */

  /* Destinations */
  profile_destination_t *destinations;
  size_t destination_count;

  /* OBS output instance */
  obs_output_t *output;

  /* Status */
  profile_status_t status;
  char *last_error;

  /* Restreamer process reference */
  char *process_reference;

  /* Flags */
  bool auto_start;                 /* Auto-start with OBS streaming */
  bool auto_reconnect;             /* Auto-reconnect on disconnect */
  uint32_t reconnect_delay_sec;    /* Delay before reconnect */
  uint32_t max_reconnect_attempts; /* Max reconnect attempts (0 = unlimited) */

  /* Health monitoring */
  bool health_monitoring_enabled;     /* Enable health checks */
  uint32_t health_check_interval_sec; /* Health check interval */
  uint32_t failure_threshold;         /* Failures before reconnect */

  /* Preview/Test mode */
  bool preview_mode_enabled;     /* Preview mode active */
  uint32_t preview_duration_sec; /* Preview duration (0 = unlimited) */
  time_t preview_start_time;     /* When preview started */
} output_profile_t;

/* Destination template for quick configuration */
typedef struct {
  char *template_name;              /* Template display name */
  char *template_id;                /* Unique identifier */
  streaming_service_t service;      /* Target service */
  stream_orientation_t orientation; /* Recommended orientation */
  encoding_settings_t encoding;     /* Recommended encoding */
  bool is_builtin;                  /* Built-in vs user-created */
} destination_template_t;

/* Profile manager - manages all profiles */
typedef struct {
  output_profile_t **profiles;
  size_t profile_count;
  restreamer_api_t *api; /* Shared API connection */

  /* Destination templates */
  destination_template_t **templates;
  size_t template_count;
} profile_manager_t;

/* Profile Manager Functions */

/* Create profile manager */
profile_manager_t *profile_manager_create(restreamer_api_t *api);

/* Destroy profile manager */
void profile_manager_destroy(profile_manager_t *manager);

/* Profile Management */

/* Create new profile */
output_profile_t *profile_manager_create_profile(profile_manager_t *manager,
                                                 const char *name);

/* Delete profile */
bool profile_manager_delete_profile(profile_manager_t *manager,
                                    const char *profile_id);

/* Get profile by ID */
output_profile_t *profile_manager_get_profile(profile_manager_t *manager,
                                              const char *profile_id);

/* Get profile by index */
output_profile_t *profile_manager_get_profile_at(profile_manager_t *manager,
                                                 size_t index);

/* Get profile count */
size_t profile_manager_get_count(profile_manager_t *manager);

/* Profile Operations */

/* Add destination to profile */
bool profile_add_destination(output_profile_t *profile,
                             streaming_service_t service,
                             const char *stream_key,
                             stream_orientation_t target_orientation,
                             encoding_settings_t *encoding);

/* Remove destination from profile */
bool profile_remove_destination(output_profile_t *profile, size_t index);

/* Update destination encoding settings */
bool profile_update_destination_encoding(output_profile_t *profile,
                                         size_t index,
                                         encoding_settings_t *encoding);

/* Update destination encoding settings during active streaming */
bool profile_update_destination_encoding_live(output_profile_t *profile,
                                              restreamer_api_t *api,
                                              size_t index,
                                              encoding_settings_t *encoding);

/* Enable/disable destination */
bool profile_set_destination_enabled(output_profile_t *profile, size_t index,
                                     bool enabled);

/* Profile Streaming Control */

/* Start streaming for profile */
bool output_profile_start(profile_manager_t *manager, const char *profile_id);

/* Stop streaming for profile */
bool output_profile_stop(profile_manager_t *manager, const char *profile_id);

/* Restart streaming for profile */
bool profile_restart(profile_manager_t *manager, const char *profile_id);

/* Start all profiles */
bool profile_manager_start_all(profile_manager_t *manager);

/* Stop all profiles */
bool profile_manager_stop_all(profile_manager_t *manager);

/* Get active profile count */
size_t profile_manager_get_active_count(profile_manager_t *manager);

/* ========================================================================
 * Preview/Test Mode
 * ======================================================================== */

/* Start profile in preview mode */
bool output_profile_start_preview(profile_manager_t *manager,
                                  const char *profile_id,
                                  uint32_t duration_sec);

/* Stop preview and go live */
bool output_profile_preview_to_live(profile_manager_t *manager,
                                    const char *profile_id);

/* Cancel preview mode */
bool output_profile_cancel_preview(profile_manager_t *manager,
                                   const char *profile_id);

/* Check if preview time has elapsed */
bool output_profile_check_preview_timeout(output_profile_t *profile);

/* ========================================================================
 * Health Monitoring & Auto-Recovery
 * ======================================================================== */

/* Check health of profile destinations */
bool profile_check_health(output_profile_t *profile, restreamer_api_t *api);

/* Attempt to reconnect failed destination */
bool profile_reconnect_destination(output_profile_t *profile,
                                   restreamer_api_t *api, size_t dest_index);

/* Enable/disable health monitoring for profile */
void profile_set_health_monitoring(output_profile_t *profile, bool enabled);

/* Configuration Persistence */

/* Load profiles from OBS settings */
void profile_manager_load_from_settings(profile_manager_t *manager,
                                        obs_data_t *settings);

/* Save profiles to OBS settings */
void profile_manager_save_to_settings(profile_manager_t *manager,
                                      obs_data_t *settings);

/* Load single profile from settings */
output_profile_t *profile_load_from_settings(obs_data_t *settings);

/* Save single profile to settings */
void profile_save_to_settings(output_profile_t *profile, obs_data_t *settings);

/* Utility Functions */

/* Get default encoding settings */
encoding_settings_t profile_get_default_encoding(void);

/* Generate unique profile ID */
char *profile_generate_id(void);

/* Duplicate profile */
output_profile_t *profile_duplicate(output_profile_t *source,
                                    const char *new_name);

/* Update profile stats from restreamer */
bool profile_update_stats(output_profile_t *profile, restreamer_api_t *api);

/* ========================================================================
 * Destination Templates/Presets
 * ======================================================================== */

/* Load built-in templates */
void profile_manager_load_builtin_templates(profile_manager_t *manager);

/* Create custom template from destination */
destination_template_t *profile_manager_create_template(
    profile_manager_t *manager, const char *name, streaming_service_t service,
    stream_orientation_t orientation, encoding_settings_t *encoding);

/* Delete template */
bool profile_manager_delete_template(profile_manager_t *manager,
                                     const char *template_id);

/* Get template by ID */
destination_template_t *profile_manager_get_template(profile_manager_t *manager,
                                                     const char *template_id);

/* Get template by index */
destination_template_t *
profile_manager_get_template_at(profile_manager_t *manager, size_t index);

/* Apply template to profile (add destination) */
bool profile_apply_template(output_profile_t *profile,
                            destination_template_t *tmpl,
                            const char *stream_key);

/* Save custom templates to settings */
void profile_manager_save_templates(profile_manager_t *manager,
                                    obs_data_t *settings);

/* Load custom templates from settings */
void profile_manager_load_templates(profile_manager_t *manager,
                                    obs_data_t *settings);

/* ========================================================================
 * Backup/Failover Destination Support
 * ======================================================================== */

/* Set destination as backup for primary */
bool profile_set_destination_backup(output_profile_t *profile,
                                    size_t primary_index, size_t backup_index);

/* Remove backup relationship */
bool profile_remove_destination_backup(output_profile_t *profile,
                                       size_t primary_index);

/* Manually trigger failover to backup */
bool profile_trigger_failover(output_profile_t *profile, restreamer_api_t *api,
                              size_t primary_index);

/* Restore primary destination after failover */
bool profile_restore_primary(output_profile_t *profile, restreamer_api_t *api,
                             size_t primary_index);

/* Check and auto-failover if primary fails */
bool profile_check_failover(output_profile_t *profile, restreamer_api_t *api);

/* ========================================================================
 * Bulk Destination Operations
 * ======================================================================== */

/* Enable/disable multiple destinations at once */
bool profile_bulk_enable_destinations(output_profile_t *profile,
                                      restreamer_api_t *api, size_t *indices,
                                      size_t count, bool enabled);

/* Delete multiple destinations at once */
bool profile_bulk_delete_destinations(output_profile_t *profile,
                                      size_t *indices, size_t count);

/* Apply encoding settings to multiple destinations */
bool profile_bulk_update_encoding(output_profile_t *profile,
                                  restreamer_api_t *api, size_t *indices,
                                  size_t count, encoding_settings_t *encoding);

/* Start streaming to multiple destinations */
bool profile_bulk_start_destinations(output_profile_t *profile,
                                     restreamer_api_t *api, size_t *indices,
                                     size_t count);

/* Stop streaming to multiple destinations */
bool profile_bulk_stop_destinations(output_profile_t *profile,
                                    restreamer_api_t *api, size_t *indices,
                                    size_t count);

#ifdef __cplusplus
}
#endif
