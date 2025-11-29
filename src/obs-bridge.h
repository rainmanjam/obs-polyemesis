/*
OBS Polyemesis - OBS Output Bridge
Copyright (C) 2024

Handles automatic RTMP output creation and management for bridging
OBS video/audio to Restreamer server.
*/

#pragma once

#include "restreamer-channel.h"
#include <obs-frontend-api.h>
#include <obs.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct obs_bridge obs_bridge_t;
typedef struct restreamer_api restreamer_api_t;

/* Bridge configuration */
typedef struct {
  char *restreamer_url;      /* e.g., "http://localhost:8080" */
  char *rtmp_horizontal_url; /* e.g., "rtmp://localhost/live/obs_horizontal" */
  char *rtmp_vertical_url;   /* e.g., "rtmp://localhost/live/obs_vertical" */
  bool auto_start_enabled;   /* Auto-start destinations when OBS streams */
  bool show_vertical_notification; /* Show notification when vertical canvas
                                      detected */
  bool show_preflight_check;       /* Show pre-flight check dialog */
} obs_bridge_config_t;

/* Bridge status */
typedef enum {
  BRIDGE_STATUS_IDLE,
  BRIDGE_STATUS_STARTING,
  BRIDGE_STATUS_ACTIVE,
  BRIDGE_STATUS_STOPPING,
  BRIDGE_STATUS_ERROR
} obs_bridge_status_t;

/* Callback for status changes */
typedef void (*obs_bridge_status_callback_t)(obs_bridge_status_t status,
                                             void *user_data);

/* Create/destroy bridge */
obs_bridge_t *obs_bridge_create(const obs_bridge_config_t *config);
void obs_bridge_destroy(obs_bridge_t *bridge);

/* Configuration */
void obs_bridge_set_config(obs_bridge_t *bridge,
                           const obs_bridge_config_t *config);
void obs_bridge_get_config(obs_bridge_t *bridge, obs_bridge_config_t *config);

/* Integration with Restreamer API and Channel Manager */
void obs_bridge_set_api_client(obs_bridge_t *bridge, restreamer_api_t *api);
void obs_bridge_set_channel_manager(obs_bridge_t *bridge,
                                    channel_manager_t *cm);

/* Status monitoring */
obs_bridge_status_t obs_bridge_get_status(obs_bridge_t *bridge);
void obs_bridge_set_status_callback(obs_bridge_t *bridge,
                                    obs_bridge_status_callback_t callback,
                                    void *user_data);

/* Check if outputs are active */
bool obs_bridge_is_horizontal_active(obs_bridge_t *bridge);
bool obs_bridge_is_vertical_active(obs_bridge_t *bridge);

/* Manual control (for testing or override) */
bool obs_bridge_start_horizontal(obs_bridge_t *bridge);
bool obs_bridge_start_vertical(obs_bridge_t *bridge);
void obs_bridge_stop_horizontal(obs_bridge_t *bridge);
void obs_bridge_stop_vertical(obs_bridge_t *bridge);
void obs_bridge_stop_all(obs_bridge_t *bridge);

/* OBS Frontend event handling - call this from plugin's frontend callback */
void obs_bridge_handle_frontend_event(obs_bridge_t *bridge,
                                      enum obs_frontend_event event);

#ifdef __cplusplus
}
#endif
