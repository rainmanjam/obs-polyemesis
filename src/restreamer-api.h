#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Restreamer API client for OBS Plugin */

typedef struct restreamer_api restreamer_api_t;

/* Connection settings */
typedef struct {
  char *host;
  uint16_t port;
  char *username;
  char *password;
  bool use_https;
} restreamer_connection_t;

/* Process information */
typedef struct {
  char *id;
  char *reference;
  char *state; // running, finished, failed, etc.
  uint64_t uptime_seconds;
  double cpu_usage;
  uint64_t memory_bytes;
  char *command;
} restreamer_process_t;

/* Process list */
typedef struct {
  restreamer_process_t *processes;
  size_t count;
} restreamer_process_list_t;

/* Session information */
typedef struct {
  char *session_id;
  char *reference;
  uint64_t bytes_sent;
  uint64_t bytes_received;
  char *remote_addr;
} restreamer_session_t;

/* Session list */
typedef struct {
  restreamer_session_t *sessions;
  size_t count;
} restreamer_session_list_t;

/* Log entry */
typedef struct {
  char *timestamp;
  char *message;
  char *level; // info, warning, error, debug
} restreamer_log_entry_t;

/* Log list */
typedef struct {
  restreamer_log_entry_t *entries;
  size_t count;
} restreamer_log_list_t;

/* API Functions */

/* Create a new API client */
restreamer_api_t *restreamer_api_create(restreamer_connection_t *connection);

/* Destroy API client */
void restreamer_api_destroy(restreamer_api_t *api);

/* Test connection */
bool restreamer_api_test_connection(restreamer_api_t *api);

/* Check if API is connected (has valid access token) */
bool restreamer_api_is_connected(restreamer_api_t *api);

/* Get list of processes */
bool restreamer_api_get_processes(restreamer_api_t *api,
                                  restreamer_process_list_t *list);

/* Start a process */
bool restreamer_api_start_process(restreamer_api_t *api,
                                  const char *process_id);

/* Stop a process */
bool restreamer_api_stop_process(restreamer_api_t *api, const char *process_id);

/* Restart a process */
bool restreamer_api_restart_process(restreamer_api_t *api,
                                    const char *process_id);

/* Get process details */
bool restreamer_api_get_process(restreamer_api_t *api, const char *process_id,
                                restreamer_process_t *process);

/* Get process logs */
bool restreamer_api_get_process_logs(restreamer_api_t *api,
                                     const char *process_id,
                                     restreamer_log_list_t *logs);

/* Get active sessions */
bool restreamer_api_get_sessions(restreamer_api_t *api,
                                 restreamer_session_list_t *sessions);

/* Create a new process for multistreaming */
bool restreamer_api_create_process(restreamer_api_t *api, const char *reference,
                                   const char *input_url,
                                   const char **output_urls,
                                   size_t output_count,
                                   const char *video_filter);

/* Delete a process */
bool restreamer_api_delete_process(restreamer_api_t *api,
                                   const char *process_id);

/* ========================================================================
 * Dynamic Process Modification API
 * ======================================================================== */

/* Add output to running process */
bool restreamer_api_add_process_output(restreamer_api_t *api,
                                       const char *process_id,
                                       const char *output_id,
                                       const char *output_url,
                                       const char *video_filter);

/* Remove output from running process */
bool restreamer_api_remove_process_output(restreamer_api_t *api,
                                          const char *process_id,
                                          const char *output_id);

/* Update process output settings */
bool restreamer_api_update_process_output(restreamer_api_t *api,
                                          const char *process_id,
                                          const char *output_id,
                                          const char *output_url,
                                          const char *video_filter);

/* Get process outputs list */
bool restreamer_api_get_process_outputs(restreamer_api_t *api,
                                        const char *process_id,
                                        char ***output_ids,
                                        size_t *output_count);

/* Free outputs list */
void restreamer_api_free_outputs_list(char **output_ids, size_t count);

/* ========================================================================
 * Live Encoding Settings API
 * ======================================================================== */

/* Encoding parameters for dynamic adjustment */
typedef struct {
  int video_bitrate_kbps; /* Video bitrate in kbps (0 = no change) */
  int audio_bitrate_kbps; /* Audio bitrate in kbps (0 = no change) */
  int width;              /* Video width (0 = no change) */
  int height;             /* Video height (0 = no change) */
  int fps_num;            /* FPS numerator (0 = no change) */
  int fps_den;            /* FPS denominator (0 = no change) */
  char *preset;           /* Encoder preset (NULL = no change) */
  char *profile;          /* Encoder profile (NULL = no change) */
} encoding_params_t;

/* Update encoding settings on running output */
bool restreamer_api_update_output_encoding(restreamer_api_t *api,
                                           const char *process_id,
                                           const char *output_id,
                                           encoding_params_t *params);

/* Get current encoding settings */
bool restreamer_api_get_output_encoding(restreamer_api_t *api,
                                        const char *process_id,
                                        const char *output_id,
                                        encoding_params_t *params);

/* Free encoding params */
void restreamer_api_free_encoding_params(encoding_params_t *params);

/* Free process list */
void restreamer_api_free_process_list(restreamer_process_list_t *list);

/* Free session list */
void restreamer_api_free_session_list(restreamer_session_list_t *list);

/* Free log list */
void restreamer_api_free_log_list(restreamer_log_list_t *list);

/* Free process */
void restreamer_api_free_process(restreamer_process_t *process);

/* Get last error message */
const char *restreamer_api_get_error(restreamer_api_t *api);

/* ========================================================================
 * Extended API - Process State & Monitoring
 * ======================================================================== */

/* Process state information (from /api/v3/process/{id}/state) */
typedef struct {
  char *order;              /* Current processing order/state */
  uint64_t frames;          /* Total frames processed */
  uint64_t dropped_frames;  /* Number of dropped frames */
  uint32_t current_bitrate; /* Current bitrate in kbps */
  double fps;               /* Current frames per second */
  uint64_t bytes_written;   /* Total bytes written */
  uint64_t packets_sent;    /* Total packets sent */
  double progress;          /* Progress percentage (0.0-100.0) */
  bool is_running;          /* Is process currently running */
} restreamer_process_state_t;

/* Get detailed process state */
bool restreamer_api_get_process_state(restreamer_api_t *api,
                                      const char *process_id,
                                      restreamer_process_state_t *state);

/* Free process state */
void restreamer_api_free_process_state(restreamer_process_state_t *state);

/* ========================================================================
 * Extended API - Input Probe
 * ======================================================================== */

/* Stream information from probe */
typedef struct {
  char *codec_name;      /* Codec name (h264, aac, etc.) */
  char *codec_long_name; /* Full codec name */
  char *codec_type;      /* video, audio, subtitle */
  uint32_t width;        /* Video width */
  uint32_t height;       /* Video height */
  uint32_t fps_num;      /* FPS numerator */
  uint32_t fps_den;      /* FPS denominator */
  uint32_t bitrate;      /* Bitrate in bps */
  uint32_t sample_rate;  /* Audio sample rate */
  uint32_t channels;     /* Audio channels */
  char *pix_fmt;         /* Pixel format */
  char *profile;         /* Codec profile */
  int64_t duration;      /* Duration in microseconds */
} restreamer_stream_info_t;

/* Input probe results */
typedef struct {
  char *format_name;                 /* Container format */
  char *format_long_name;            /* Full format name */
  int64_t duration;                  /* Total duration */
  uint64_t size;                     /* File/stream size */
  uint32_t bitrate;                  /* Overall bitrate */
  restreamer_stream_info_t *streams; /* Array of streams */
  size_t stream_count;               /* Number of streams */
} restreamer_probe_info_t;

/* Probe input stream */
bool restreamer_api_probe_input(restreamer_api_t *api, const char *process_id,
                                restreamer_probe_info_t *info);

/* Free probe info */
void restreamer_api_free_probe_info(restreamer_probe_info_t *info);

/* ========================================================================
 * Extended API - Configuration Management
 * ======================================================================== */

/* Get Restreamer configuration as JSON string */
bool restreamer_api_get_config(restreamer_api_t *api, char **config_json);

/* Set Restreamer configuration from JSON string */
bool restreamer_api_set_config(restreamer_api_t *api, const char *config_json);

/* Reload configuration */
bool restreamer_api_reload_config(restreamer_api_t *api);

/* ========================================================================
 * Extended API - Metrics & Monitoring
 * ======================================================================== */

/* Metric value */
typedef struct {
  char *name;        /* Metric name */
  double value;      /* Metric value */
  char *labels;      /* Labels (JSON) */
  int64_t timestamp; /* Timestamp */
} restreamer_metric_t;

/* Metrics list */
typedef struct {
  restreamer_metric_t *metrics;
  size_t count;
} restreamer_metrics_t;

/* Get available metrics descriptions */
bool restreamer_api_get_metrics_list(restreamer_api_t *api,
                                     char **metrics_json);

/* Query metrics with custom query */
bool restreamer_api_query_metrics(restreamer_api_t *api, const char *query_json,
                                  char **result_json);

/* Get Prometheus-formatted metrics */
bool restreamer_api_get_prometheus_metrics(restreamer_api_t *api,
                                           char **prometheus_text);

/* Free metrics */
void restreamer_api_free_metrics(restreamer_metrics_t *metrics);

/* ========================================================================
 * Extended API - Metadata Storage
 * ======================================================================== */

/* Get global metadata */
bool restreamer_api_get_metadata(restreamer_api_t *api, const char *key,
                                 char **value);

/* Set global metadata */
bool restreamer_api_set_metadata(restreamer_api_t *api, const char *key,
                                 const char *value);

/* Get process-specific metadata */
bool restreamer_api_get_process_metadata(restreamer_api_t *api,
                                         const char *process_id,
                                         const char *key, char **value);

/* Set process-specific metadata */
bool restreamer_api_set_process_metadata(restreamer_api_t *api,
                                         const char *process_id,
                                         const char *key, const char *value);

/* ========================================================================
 * Extended API - Playout Management
 * ======================================================================== */

/* Playout status */
typedef struct {
  char *input_id;          /* Input identifier */
  char *url;               /* Current stream URL */
  bool is_connected;       /* Connection status */
  uint64_t bytes_received; /* Bytes received */
  uint32_t bitrate;        /* Current bitrate */
  char *state;             /* Playout state */
} restreamer_playout_status_t;

/* Get playout status */
bool restreamer_api_get_playout_status(restreamer_api_t *api,
                                       const char *process_id,
                                       const char *input_id,
                                       restreamer_playout_status_t *status);

/* Switch input stream URL */
bool restreamer_api_switch_input_stream(restreamer_api_t *api,
                                        const char *process_id,
                                        const char *input_id,
                                        const char *new_url);

/* Reopen input connection */
bool restreamer_api_reopen_input(restreamer_api_t *api, const char *process_id,
                                 const char *input_id);

/* Get last keyframe */
bool restreamer_api_get_keyframe(restreamer_api_t *api, const char *process_id,
                                 const char *input_id, const char *name,
                                 unsigned char **data, size_t *size);

/* Free playout status */
void restreamer_api_free_playout_status(restreamer_playout_status_t *status);

/* ========================================================================
 * Extended API - Authentication & Token Management
 * ======================================================================== */

/* Refresh access token using refresh token */
bool restreamer_api_refresh_token(restreamer_api_t *api);

/* Force re-login (invalidates current tokens) */
bool restreamer_api_force_login(restreamer_api_t *api);

/* ========================================================================
 * Extended API - File System Access
 * ======================================================================== */

/* File system entry */
typedef struct {
  char *name;        /* File/directory name */
  char *path;        /* Full path */
  uint64_t size;     /* File size in bytes */
  int64_t modified;  /* Last modified timestamp */
  bool is_directory; /* Is this a directory */
} restreamer_fs_entry_t;

/* File system list */
typedef struct {
  restreamer_fs_entry_t *entries;
  size_t count;
} restreamer_fs_list_t;

/* List available filesystems */
bool restreamer_api_list_filesystems(restreamer_api_t *api,
                                     char **filesystems_json);

/* List files in filesystem */
bool restreamer_api_list_files(restreamer_api_t *api, const char *storage,
                               const char *glob_pattern,
                               restreamer_fs_list_t *files);

/* Download file from filesystem */
bool restreamer_api_download_file(restreamer_api_t *api, const char *storage,
                                  const char *filepath, unsigned char **data,
                                  size_t *size);

/* Upload file to filesystem */
bool restreamer_api_upload_file(restreamer_api_t *api, const char *storage,
                                const char *filepath, const unsigned char *data,
                                size_t size);

/* Delete file from filesystem */
bool restreamer_api_delete_file(restreamer_api_t *api, const char *storage,
                                const char *filepath);

/* Free filesystem list */
void restreamer_api_free_fs_list(restreamer_fs_list_t *list);

/* ========================================================================
 * Extended API - Protocol Monitoring
 * ======================================================================== */

/* Get active RTMP streams */
bool restreamer_api_get_rtmp_streams(restreamer_api_t *api,
                                     char **streams_json);

/* Get active SRT streams */
bool restreamer_api_get_srt_streams(restreamer_api_t *api, char **streams_json);

/* ========================================================================
 * Extended API - FFmpeg Capabilities
 * ======================================================================== */

/* Get FFmpeg capabilities (codecs, formats, etc.) */
bool restreamer_api_get_skills(restreamer_api_t *api, char **skills_json);

/* Reload FFmpeg capabilities */
bool restreamer_api_reload_skills(restreamer_api_t *api);

/* ========================================================================
 * Extended API - Server Info & Diagnostics
 * ======================================================================== */

/* Check server liveliness - returns true if server responds with "pong" */
bool restreamer_api_ping(restreamer_api_t *api);

/* API version information */
typedef struct {
  char *name;       /* API name */
  char *version;    /* Version string */
  char *build_date; /* Build date */
  char *commit;     /* Git commit hash */
} restreamer_api_info_t;

/* Get API version info */
bool restreamer_api_get_info(restreamer_api_t *api,
                             restreamer_api_info_t *info);

/* Free API info */
void restreamer_api_free_info(restreamer_api_info_t *info);

/* Get application logs */
bool restreamer_api_get_logs(restreamer_api_t *api, char **logs_text);

/* Active session summary */
typedef struct {
  size_t session_count;    /* Number of active sessions */
  uint64_t total_rx_bytes; /* Total bytes received */
  uint64_t total_tx_bytes; /* Total bytes transmitted */
} restreamer_active_sessions_t;

/* Get active session summary */
bool restreamer_api_get_active_sessions(restreamer_api_t *api,
                                        restreamer_active_sessions_t *sessions);

/* Get process configuration as JSON string */
bool restreamer_api_get_process_config(restreamer_api_t *api,
                                       const char *process_id,
                                       char **config_json);

#ifdef __cplusplus
}
#endif
