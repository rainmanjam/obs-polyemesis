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

#ifdef __cplusplus
}
#endif
