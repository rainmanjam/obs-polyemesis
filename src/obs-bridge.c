/*
OBS Polyemesis - OBS Output Bridge Implementation
Copyright (C) 2024
*/

#include "obs-bridge.h"
#include "restreamer-api.h"
#include "restreamer-channel.h"
#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

struct obs_bridge {
  /* Configuration */
  obs_bridge_config_t config;

  /* OBS outputs */
  obs_output_t *horizontal_output;
  obs_output_t *vertical_output;

  /* OBS encoders */
  obs_encoder_t *horizontal_video_encoder;
  obs_encoder_t *horizontal_audio_encoder;
  obs_encoder_t *vertical_video_encoder;
  obs_encoder_t *vertical_audio_encoder;

  /* Status */
  obs_bridge_status_t status;
  obs_bridge_status_callback_t status_callback;
  void *status_callback_data;

  /* Integration */
  restreamer_api_t *api_client;
  channel_manager_t *channel_manager;

  /* State tracking */
  bool obs_streaming;
  bool vertical_canvas_available;
  video_t *vertical_video;
};

/* Helper: Get OBS video settings */
static bool get_obs_video_settings(struct obs_video_info *ovi) {
  return obs_get_video_info(ovi);
}

/* Helper: Get OBS audio settings */
static bool get_obs_audio_settings(struct obs_audio_info *oai) {
  return obs_get_audio_info(oai);
}

/* Helper: Create RTMP service for Restreamer */
static obs_service_t *create_rtmp_service(const char *url) {
  if (!url || !*url)
    return NULL;

  obs_data_t *settings = obs_data_create();
  obs_data_set_string(settings, "server", url);
  obs_data_set_string(settings, "key", ""); /* No key needed for Restreamer */

  obs_service_t *service = obs_service_create(
      "rtmp_custom", "polyemesis_rtmp_service", settings, NULL);
  obs_data_release(settings);

  return service;
}

/* Helper: Create video encoder using OBS's stream encoder settings */
static obs_encoder_t *create_video_encoder(const char *name) {
  /* Get OBS's current video encoder settings from Settings > Output */
  obs_data_t *encoder_settings = obs_data_create();

  /* Use x264 encoder with reasonable defaults for RTMP to localhost */
  const char *encoder_id =
      "obs_x264"; /* Use software encoder for compatibility */

  /* Get video info */
  struct obs_video_info ovi;
  if (!get_obs_video_settings(&ovi)) {
    obs_log(LOG_ERROR, "[OBS Bridge] Failed to get video settings");
    obs_data_release(encoder_settings);
    return NULL;
  }

  /* Set encoder settings optimized for local RTMP */
  obs_data_set_string(encoder_settings, "rate_control", "CBR");
  obs_data_set_int(encoder_settings, "bitrate",
                   6000); /* 6 Mbps - good for 1080p */
  obs_data_set_string(encoder_settings, "preset",
                      "veryfast"); /* Low CPU usage */
  obs_data_set_string(encoder_settings, "profile", "high");
  obs_data_set_string(encoder_settings, "tune", "zerolatency");
  obs_data_set_int(encoder_settings, "keyint_sec", 2);
  obs_data_set_bool(encoder_settings, "repeat_headers", true);

  obs_encoder_t *encoder =
      obs_video_encoder_create(encoder_id, name, encoder_settings, NULL);
  obs_data_release(encoder_settings);

  if (!encoder) {
    obs_log(LOG_ERROR, "[OBS Bridge] Failed to create video encoder: %s", name);
    return NULL;
  }

  /* Connect to OBS video */
  obs_encoder_set_video(encoder, obs_get_video());

  return encoder;
}

/* Helper: Create audio encoder */
static obs_encoder_t *create_audio_encoder(const char *name) {
  obs_data_t *encoder_settings = obs_data_create();

  /* Use FFmpeg AAC encoder */
  const char *encoder_id = "ffmpeg_aac";

  /* Set audio settings */
  obs_data_set_int(encoder_settings, "bitrate", 160); /* 160 kbps */

  obs_encoder_t *encoder =
      obs_audio_encoder_create(encoder_id, name, encoder_settings, 0, NULL);
  obs_data_release(encoder_settings);

  if (!encoder) {
    obs_log(LOG_ERROR, "[OBS Bridge] Failed to create audio encoder: %s", name);
    return NULL;
  }

  /* Connect to OBS audio */
  obs_encoder_set_audio(encoder, obs_get_audio());

  return encoder;
}

/* Helper: Check for vertical canvas via proc_handler */
static bool check_vertical_canvas_available(obs_bridge_t *bridge) {
  if (!bridge)
    return false;

  proc_handler_t *ph = obs_get_proc_handler();
  if (!ph)
    return false;

  struct calldata cd;
  calldata_init(&cd);

  /* Try to call vertical canvas's proc handler */
  bool available = proc_handler_call(ph, "aitum_vertical_get_video", &cd);

  if (available) {
    /* Get the vertical video context */
    bridge->vertical_video = (video_t *)calldata_ptr(&cd, "video");
    obs_log(LOG_INFO, "[OBS Bridge] Vertical canvas detected");
  }

  calldata_free(&cd);
  return available;
}

/* Create bridge */
obs_bridge_t *obs_bridge_create(const obs_bridge_config_t *config) {
  obs_bridge_t *bridge = bzalloc(sizeof(obs_bridge_t));

  if (config) {
    obs_bridge_set_config(bridge, config);
  } else {
    /* Default configuration */
    /* Security: HTTP is used here for local development default only.
     * Users should configure HTTPS for production deployments via Settings. */
    bridge->config.restreamer_url = bstrdup("http://localhost:8080");
    bridge->config.rtmp_horizontal_url =
        bstrdup("rtmp://localhost/live/obs_horizontal");
    bridge->config.rtmp_vertical_url =
        bstrdup("rtmp://localhost/live/obs_vertical");
    bridge->config.auto_start_enabled = true;
    bridge->config.show_vertical_notification = true;
    bridge->config.show_preflight_check = true;
  }

  bridge->status = BRIDGE_STATUS_IDLE;

  obs_log(LOG_INFO, "[OBS Bridge] Created with auto-start: %s",
          bridge->config.auto_start_enabled ? "enabled" : "disabled");

  return bridge;
}

/* Destroy bridge */
void obs_bridge_destroy(obs_bridge_t *bridge) {
  if (!bridge)
    return;

  /* Stop all outputs first */
  obs_bridge_stop_all(bridge);

  /* Release encoders */
  if (bridge->horizontal_video_encoder) {
    obs_encoder_release(bridge->horizontal_video_encoder);
    bridge->horizontal_video_encoder = NULL;
  }
  if (bridge->horizontal_audio_encoder) {
    obs_encoder_release(bridge->horizontal_audio_encoder);
    bridge->horizontal_audio_encoder = NULL;
  }
  if (bridge->vertical_video_encoder) {
    obs_encoder_release(bridge->vertical_video_encoder);
    bridge->vertical_video_encoder = NULL;
  }
  if (bridge->vertical_audio_encoder) {
    obs_encoder_release(bridge->vertical_audio_encoder);
    bridge->vertical_audio_encoder = NULL;
  }

  /* Release outputs */
  if (bridge->horizontal_output) {
    obs_output_release(bridge->horizontal_output);
    bridge->horizontal_output = NULL;
  }
  if (bridge->vertical_output) {
    obs_output_release(bridge->vertical_output);
    bridge->vertical_output = NULL;
  }

  /* Free config strings */
  bfree(bridge->config.restreamer_url);
  bfree(bridge->config.rtmp_horizontal_url);
  bfree(bridge->config.rtmp_vertical_url);

  bfree(bridge);
  obs_log(LOG_INFO, "[OBS Bridge] Destroyed");
}

/* Set configuration */
void obs_bridge_set_config(obs_bridge_t *bridge,
                           const obs_bridge_config_t *config) {
  if (!bridge || !config)
    return;

  /* Free old config strings */
  bfree(bridge->config.restreamer_url);
  bfree(bridge->config.rtmp_horizontal_url);
  bfree(bridge->config.rtmp_vertical_url);

  /* Copy new config */
  bridge->config.restreamer_url =
      config->restreamer_url ? bstrdup(config->restreamer_url) : NULL;
  bridge->config.rtmp_horizontal_url =
      config->rtmp_horizontal_url ? bstrdup(config->rtmp_horizontal_url) : NULL;
  bridge->config.rtmp_vertical_url =
      config->rtmp_vertical_url ? bstrdup(config->rtmp_vertical_url) : NULL;
  bridge->config.auto_start_enabled = config->auto_start_enabled;
  bridge->config.show_vertical_notification =
      config->show_vertical_notification;
  bridge->config.show_preflight_check = config->show_preflight_check;

  obs_log(LOG_INFO, "[OBS Bridge] Configuration updated");
}

/* Get configuration */
void obs_bridge_get_config(obs_bridge_t *bridge, obs_bridge_config_t *config) {
  if (!bridge || !config)
    return;

  config->restreamer_url = bridge->config.restreamer_url
                               ? bstrdup(bridge->config.restreamer_url)
                               : NULL;
  config->rtmp_horizontal_url =
      bridge->config.rtmp_horizontal_url
          ? bstrdup(bridge->config.rtmp_horizontal_url)
          : NULL;
  config->rtmp_vertical_url = bridge->config.rtmp_vertical_url
                                  ? bstrdup(bridge->config.rtmp_vertical_url)
                                  : NULL;
  config->auto_start_enabled = bridge->config.auto_start_enabled;
  config->show_vertical_notification =
      bridge->config.show_vertical_notification;
  config->show_preflight_check = bridge->config.show_preflight_check;
}

/* Set API client */
void obs_bridge_set_api_client(obs_bridge_t *bridge, restreamer_api_t *api) {
  if (bridge)
    bridge->api_client = api;
}

/* Set channel manager */
void obs_bridge_set_channel_manager(obs_bridge_t *bridge,
                                    channel_manager_t *cm) {
  if (bridge)
    bridge->channel_manager = cm;
}

/* Get status */
obs_bridge_status_t obs_bridge_get_status(obs_bridge_t *bridge) {
  return bridge ? bridge->status : BRIDGE_STATUS_ERROR;
}

/* Set status callback */
void obs_bridge_set_status_callback(obs_bridge_t *bridge,
                                    obs_bridge_status_callback_t callback,
                                    void *user_data) {
  if (bridge) {
    bridge->status_callback = callback;
    bridge->status_callback_data = user_data;
  }
}

/* Check if outputs are active */
bool obs_bridge_is_horizontal_active(obs_bridge_t *bridge) {
  if (!bridge || !bridge->horizontal_output)
    return false;
  return obs_output_active(bridge->horizontal_output);
}

bool obs_bridge_is_vertical_active(obs_bridge_t *bridge) {
  if (!bridge || !bridge->vertical_output)
    return false;
  return obs_output_active(bridge->vertical_output);
}

/* Start horizontal output */
bool obs_bridge_start_horizontal(obs_bridge_t *bridge) {
  if (!bridge || !bridge->config.rtmp_horizontal_url) {
    obs_log(LOG_ERROR,
            "[OBS Bridge] Cannot start horizontal: invalid configuration");
    return false;
  }

  /* Create encoder if needed */
  if (!bridge->horizontal_video_encoder) {
    bridge->horizontal_video_encoder =
        create_video_encoder("polyemesis_horizontal_video");
    if (!bridge->horizontal_video_encoder) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create horizontal video encoder");
      return false;
    }
  }

  if (!bridge->horizontal_audio_encoder) {
    bridge->horizontal_audio_encoder =
        create_audio_encoder("polyemesis_horizontal_audio");
    if (!bridge->horizontal_audio_encoder) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create horizontal audio encoder");
      return false;
    }
  }

  /* Create output if needed */
  if (!bridge->horizontal_output) {
    /* Create RTMP service */
    obs_service_t *service =
        create_rtmp_service(bridge->config.rtmp_horizontal_url);
    if (!service) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create RTMP service for horizontal");
      return false;
    }

    /* Create output */
    bridge->horizontal_output = obs_output_create(
        "rtmp_output", "polyemesis_horizontal_output", NULL, NULL);
    if (!bridge->horizontal_output) {
      obs_log(LOG_ERROR, "[OBS Bridge] Failed to create horizontal output");
      obs_service_release(service);
      return false;
    }

    /* Configure output */
    obs_output_set_service(bridge->horizontal_output, service);
    obs_output_set_video_encoder(bridge->horizontal_output,
                                 bridge->horizontal_video_encoder);
    obs_output_set_audio_encoder(bridge->horizontal_output,
                                 bridge->horizontal_audio_encoder, 0);

    obs_service_release(service);
  }

  /* Start output */
  if (!obs_output_start(bridge->horizontal_output)) {
    const char *error = obs_output_get_last_error(bridge->horizontal_output);
    obs_log(LOG_ERROR, "[OBS Bridge] Failed to start horizontal output: %s",
            error ? error : "unknown");
    return false;
  }

  obs_log(LOG_INFO, "[OBS Bridge] Horizontal output started -> %s",
          bridge->config.rtmp_horizontal_url);
  return true;
}

/* Start vertical output */
bool obs_bridge_start_vertical(obs_bridge_t *bridge) {
  if (!bridge || !bridge->config.rtmp_vertical_url) {
    obs_log(LOG_WARNING,
            "[OBS Bridge] Cannot start vertical: invalid configuration");
    return false;
  }

  /* Check if vertical canvas is available */
  if (!check_vertical_canvas_available(bridge)) {
    obs_log(LOG_INFO, "[OBS Bridge] Vertical canvas not available, skipping");
    return false;
  }

  /* Create encoders for vertical if needed */
  if (!bridge->vertical_video_encoder) {
    bridge->vertical_video_encoder =
        create_video_encoder("polyemesis_vertical_video");
    if (!bridge->vertical_video_encoder) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create vertical video encoder");
      return false;
    }

    /* Connect to vertical canvas video */
    if (bridge->vertical_video) {
      obs_encoder_set_video(bridge->vertical_video_encoder,
                            bridge->vertical_video);
    }
  }

  if (!bridge->vertical_audio_encoder) {
    bridge->vertical_audio_encoder =
        create_audio_encoder("polyemesis_vertical_audio");
    if (!bridge->vertical_audio_encoder) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create vertical audio encoder");
      return false;
    }
  }

  /* Create output if needed */
  if (!bridge->vertical_output) {
    obs_service_t *service =
        create_rtmp_service(bridge->config.rtmp_vertical_url);
    if (!service) {
      obs_log(LOG_ERROR,
              "[OBS Bridge] Failed to create RTMP service for vertical");
      return false;
    }

    bridge->vertical_output = obs_output_create(
        "rtmp_output", "polyemesis_vertical_output", NULL, NULL);
    if (!bridge->vertical_output) {
      obs_log(LOG_ERROR, "[OBS Bridge] Failed to create vertical output");
      obs_service_release(service);
      return false;
    }

    obs_output_set_service(bridge->vertical_output, service);
    obs_output_set_video_encoder(bridge->vertical_output,
                                 bridge->vertical_video_encoder);
    obs_output_set_audio_encoder(bridge->vertical_output,
                                 bridge->vertical_audio_encoder, 0);

    obs_service_release(service);
  }

  /* Start output */
  if (!obs_output_start(bridge->vertical_output)) {
    const char *error = obs_output_get_last_error(bridge->vertical_output);
    obs_log(LOG_ERROR, "[OBS Bridge] Failed to start vertical output: %s",
            error ? error : "unknown");
    return false;
  }

  obs_log(LOG_INFO, "[OBS Bridge] Vertical output started -> %s",
          bridge->config.rtmp_vertical_url);
  return true;
}

/* Stop horizontal output */
void obs_bridge_stop_horizontal(obs_bridge_t *bridge) {
  if (!bridge || !bridge->horizontal_output)
    return;

  if (obs_output_active(bridge->horizontal_output)) {
    obs_output_stop(bridge->horizontal_output);
    obs_log(LOG_INFO, "[OBS Bridge] Horizontal output stopped");
  }
}

/* Stop vertical output */
void obs_bridge_stop_vertical(obs_bridge_t *bridge) {
  if (!bridge || !bridge->vertical_output)
    return;

  if (obs_output_active(bridge->vertical_output)) {
    obs_output_stop(bridge->vertical_output);
    obs_log(LOG_INFO, "[OBS Bridge] Vertical output stopped");
  }
}

/* Stop all outputs */
void obs_bridge_stop_all(obs_bridge_t *bridge) {
  if (!bridge)
    return;

  obs_bridge_stop_horizontal(bridge);
  obs_bridge_stop_vertical(bridge);
}

/* Handle OBS frontend events */
void obs_bridge_handle_frontend_event(obs_bridge_t *bridge,
                                      enum obs_frontend_event event) {
  if (!bridge)
    return;

  switch (event) {
  case OBS_FRONTEND_EVENT_STREAMING_STARTING:
    obs_log(LOG_INFO, "[OBS Bridge] OBS streaming starting...");
    bridge->obs_streaming = true;
    bridge->status = BRIDGE_STATUS_STARTING;

    if (bridge->config.auto_start_enabled) {
      obs_log(LOG_INFO,
              "[OBS Bridge] Auto-start enabled, creating RTMP outputs");

      /* Start horizontal output */
      if (!obs_bridge_start_horizontal(bridge)) {
        obs_log(LOG_ERROR, "[OBS Bridge] Failed to start horizontal output");
      }

      /* Try to start vertical output */
      obs_bridge_start_vertical(bridge); /* Non-fatal if it fails */

      bridge->status = BRIDGE_STATUS_ACTIVE;

      /* Call status callback */
      if (bridge->status_callback) {
        bridge->status_callback(bridge->status, bridge->status_callback_data);
      }
    }
    break;

  case OBS_FRONTEND_EVENT_STREAMING_STARTED:
    obs_log(LOG_INFO, "[OBS Bridge] OBS streaming started");
    break;

  case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
    obs_log(LOG_INFO, "[OBS Bridge] OBS streaming stopping...");
    bridge->status = BRIDGE_STATUS_STOPPING;
    break;

  case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
    obs_log(LOG_INFO, "[OBS Bridge] OBS streaming stopped");
    bridge->obs_streaming = false;

    if (bridge->config.auto_start_enabled) {
      /* Stop all RTMP outputs */
      obs_bridge_stop_all(bridge);
    }

    bridge->status = BRIDGE_STATUS_IDLE;

    /* Call status callback */
    if (bridge->status_callback) {
      bridge->status_callback(bridge->status, bridge->status_callback_data);
    }
    break;

  default:
    break;
  }
}
