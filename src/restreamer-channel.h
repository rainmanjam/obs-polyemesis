#pragma once

#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stream channel for managing multiple concurrent outputs */

typedef enum {
  CHANNEL_STATUS_INACTIVE, /* Channel exists but not streaming */
  CHANNEL_STATUS_STARTING, /* Channel is starting streams */
  CHANNEL_STATUS_ACTIVE,   /* Channel is actively streaming */
  CHANNEL_STATUS_STOPPING, /* Channel is stopping streams */
  CHANNEL_STATUS_PREVIEW,  /* Channel is in test/preview mode */
  CHANNEL_STATUS_ERROR     /* Channel encountered an error */
} channel_status_t;

/* Per-output encoding settings */
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

/* Enhanced output with encoding settings */
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
  bool is_backup;             /* This is a backup output */
  size_t primary_index;       /* Index of primary (if this is backup) */
  size_t backup_index;        /* Index of backup (if this is primary) */
  bool failover_active;       /* Failover is currently active */
  time_t failover_start_time; /* When failover started */
} channel_output_t;

/* Stream channel structure */
typedef struct stream_channel {
  char *channel_name; /* User-friendly name */
  char *channel_id;   /* Unique identifier */

  /* Source configuration */
  stream_orientation_t
      source_orientation; /* Auto, Horizontal, Vertical, Square */
  bool auto_detect_orientation;
  uint32_t source_width;  /* Expected source width */
  uint32_t source_height; /* Expected source height */
  char *input_url;        /* RTMP input URL (rtmp://host/app/key) */

  /* Outputs */
  channel_output_t *outputs;
  size_t output_count;

  /* OBS output instance */
  obs_output_t *output;

  /* Status */
  channel_status_t status;
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
} stream_channel_t;

/* Output template for quick configuration */
typedef struct {
  char *template_name;              /* Template display name */
  char *template_id;                /* Unique identifier */
  streaming_service_t service;      /* Target service */
  stream_orientation_t orientation; /* Recommended orientation */
  encoding_settings_t encoding;     /* Recommended encoding */
  bool is_builtin;                  /* Built-in vs user-created */
} output_template_t;

/* Channel manager - manages all channels */
typedef struct {
  stream_channel_t **channels;
  size_t channel_count;
  restreamer_api_t *api; /* Shared API connection */

  /* Output templates */
  output_template_t **templates;
  size_t template_count;
} channel_manager_t;

/* Channel Manager Functions */

/* Create channel manager */
channel_manager_t *channel_manager_create(restreamer_api_t *api);

/* Destroy channel manager */
void channel_manager_destroy(channel_manager_t *manager);

/* Channel Management */

/* Create new channel */
stream_channel_t *channel_manager_create_channel(channel_manager_t *manager,
                                                 const char *name);

/* Delete channel */
bool channel_manager_delete_channel(channel_manager_t *manager,
                                    const char *channel_id);

/* Get channel by ID */
stream_channel_t *channel_manager_get_channel(channel_manager_t *manager,
                                              const char *channel_id);

/* Get channel by index */
stream_channel_t *channel_manager_get_channel_at(channel_manager_t *manager,
                                                 size_t index);

/* Get channel count */
size_t channel_manager_get_count(channel_manager_t *manager);

/* Channel Operations */

/* Add output to channel */
bool channel_add_output(stream_channel_t *channel,
                        streaming_service_t service,
                        const char *stream_key,
                        stream_orientation_t target_orientation,
                        encoding_settings_t *encoding);

/* Remove output from channel */
bool channel_remove_output(stream_channel_t *channel, size_t index);

/* Update output encoding settings */
bool channel_update_output_encoding(stream_channel_t *channel,
                                    size_t index,
                                    encoding_settings_t *encoding);

/* Update output encoding settings during active streaming */
bool channel_update_output_encoding_live(stream_channel_t *channel,
                                         restreamer_api_t *api,
                                         size_t index,
                                         encoding_settings_t *encoding);

/* Enable/disable output */
bool channel_set_output_enabled(stream_channel_t *channel, size_t index,
                                bool enabled);

/* Channel Streaming Control */

/* Start streaming for channel */
bool channel_start(channel_manager_t *manager, const char *channel_id);

/* Stop streaming for channel */
bool channel_stop(channel_manager_t *manager, const char *channel_id);

/* Restart streaming for channel */
bool channel_restart(channel_manager_t *manager, const char *channel_id);

/* Start all channels */
bool channel_manager_start_all(channel_manager_t *manager);

/* Stop all channels */
bool channel_manager_stop_all(channel_manager_t *manager);

/* Get active channel count */
size_t channel_manager_get_active_count(channel_manager_t *manager);

/* ========================================================================
 * Preview/Test Mode
 * ======================================================================== */

/* Start channel in preview mode */
bool channel_start_preview(channel_manager_t *manager,
                           const char *channel_id,
                           uint32_t duration_sec);

/* Stop preview and go live */
bool channel_preview_to_live(channel_manager_t *manager,
                              const char *channel_id);

/* Cancel preview mode */
bool channel_cancel_preview(channel_manager_t *manager,
                            const char *channel_id);

/* Check if preview time has elapsed */
bool channel_check_preview_timeout(stream_channel_t *channel);

/* ========================================================================
 * Health Monitoring & Auto-Recovery
 * ======================================================================== */

/* Check health of channel outputs */
bool channel_check_health(stream_channel_t *channel, restreamer_api_t *api);

/* Attempt to reconnect failed output */
bool channel_reconnect_output(stream_channel_t *channel,
                              restreamer_api_t *api, size_t output_index);

/* Enable/disable health monitoring for channel */
void channel_set_health_monitoring(stream_channel_t *channel, bool enabled);

/* Configuration Persistence */

/* Load channels from OBS settings */
void channel_manager_load_from_settings(channel_manager_t *manager,
                                        obs_data_t *settings);

/* Save channels to OBS settings */
void channel_manager_save_to_settings(channel_manager_t *manager,
                                      obs_data_t *settings);

/* Load single channel from settings */
stream_channel_t *channel_load_from_settings(obs_data_t *settings);

/* Save single channel to settings */
void channel_save_to_settings(stream_channel_t *channel, obs_data_t *settings);

/* Utility Functions */

/* Get default encoding settings */
encoding_settings_t channel_get_default_encoding(void);

/* Generate unique channel ID */
char *channel_generate_id(void);

/* Duplicate channel */
stream_channel_t *channel_duplicate(stream_channel_t *source,
                                    const char *new_name);

/* Update channel stats from restreamer */
bool channel_update_stats(stream_channel_t *channel, restreamer_api_t *api);

/* ========================================================================
 * Output Templates/Presets
 * ======================================================================== */

/* Load built-in templates */
void channel_manager_load_builtin_templates(channel_manager_t *manager);

/* Create custom template from output */
output_template_t *channel_manager_create_template(
    channel_manager_t *manager, const char *name, streaming_service_t service,
    stream_orientation_t orientation, encoding_settings_t *encoding);

/* Delete template */
bool channel_manager_delete_template(channel_manager_t *manager,
                                     const char *template_id);

/* Get template by ID */
output_template_t *channel_manager_get_template(channel_manager_t *manager,
                                                const char *template_id);

/* Get template by index */
output_template_t *
channel_manager_get_template_at(channel_manager_t *manager, size_t index);

/* Apply template to channel (add output) */
bool channel_apply_template(stream_channel_t *channel,
                            output_template_t *tmpl,
                            const char *stream_key);

/* Save custom templates to settings */
void channel_manager_save_templates(channel_manager_t *manager,
                                    obs_data_t *settings);

/* Load custom templates from settings */
void channel_manager_load_templates(channel_manager_t *manager,
                                    obs_data_t *settings);

/* ========================================================================
 * Backup/Failover Output Support
 * ======================================================================== */

/* Set output as backup for primary */
bool channel_set_output_backup(stream_channel_t *channel,
                               size_t primary_index, size_t backup_index);

/* Remove backup relationship */
bool channel_remove_output_backup(stream_channel_t *channel,
                                  size_t primary_index);

/* Manually trigger failover to backup */
bool channel_trigger_failover(stream_channel_t *channel, restreamer_api_t *api,
                              size_t primary_index);

/* Restore primary output after failover */
bool channel_restore_primary(stream_channel_t *channel, restreamer_api_t *api,
                             size_t primary_index);

/* Check and auto-failover if primary fails */
bool channel_check_failover(stream_channel_t *channel, restreamer_api_t *api);

/* ========================================================================
 * Bulk Output Operations
 * ======================================================================== */

/* Enable/disable multiple outputs at once */
bool channel_bulk_enable_outputs(stream_channel_t *channel,
                                 restreamer_api_t *api, size_t *indices,
                                 size_t count, bool enabled);

/* Delete multiple outputs at once */
bool channel_bulk_delete_outputs(stream_channel_t *channel,
                                 size_t *indices, size_t count);

/* Apply encoding settings to multiple outputs */
bool channel_bulk_update_encoding(stream_channel_t *channel,
                                  restreamer_api_t *api, size_t *indices,
                                  size_t count, encoding_settings_t *encoding);

/* Start streaming to multiple outputs */
bool channel_bulk_start_outputs(stream_channel_t *channel,
                                restreamer_api_t *api, size_t *indices,
                                size_t count);

/* Stop streaming to multiple outputs */
bool channel_bulk_stop_outputs(stream_channel_t *channel,
                               restreamer_api_t *api, size_t *indices,
                               size_t count);

#ifdef __cplusplus
}
#endif
