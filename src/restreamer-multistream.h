#pragma once

#include "restreamer-api.h"
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stream orientation types */
typedef enum {
  ORIENTATION_AUTO,       /* Detect from video dimensions */
  ORIENTATION_HORIZONTAL, /* Landscape/16:9 */
  ORIENTATION_VERTICAL,   /* Portrait/9:16 */
  ORIENTATION_SQUARE      /* 1:1 */
} stream_orientation_t;

/* Streaming service types */
typedef enum {
  SERVICE_CUSTOM,
  SERVICE_TWITCH,
  SERVICE_YOUTUBE,
  SERVICE_FACEBOOK,
  SERVICE_KICK,
  SERVICE_TIKTOK,
  SERVICE_INSTAGRAM,
  SERVICE_X_TWITTER
} streaming_service_t;

/* Output destination */
typedef struct {
  streaming_service_t service;
  char *service_name;
  char *stream_key;
  char *rtmp_url;
  char *output_id;  /* Actual output ID used in restreamer process */
  stream_orientation_t supported_orientation;
  bool enabled;
} stream_destination_t;

/* Multistream configuration */
typedef struct {
  stream_destination_t *destinations;
  size_t destination_count;
  stream_orientation_t source_orientation;
  bool auto_detect_orientation;
  char *process_reference;
} multistream_config_t;

/* Initialize multistream config */
multistream_config_t *restreamer_multistream_create(void);

/* Destroy multistream config */
void restreamer_multistream_destroy(multistream_config_t *config);

/* Add destination */
bool restreamer_multistream_add_destination(multistream_config_t *config,
                                            streaming_service_t service,
                                            const char *stream_key,
                                            stream_orientation_t orientation);

/* Remove destination */
void restreamer_multistream_remove_destination(multistream_config_t *config,
                                               size_t index);

/* Get service RTMP URL */
const char *
restreamer_multistream_get_service_url(streaming_service_t service,
                                       stream_orientation_t orientation);

/* Get service name */
const char *
restreamer_multistream_get_service_name(streaming_service_t service);

/* Detect orientation from video dimensions */
stream_orientation_t restreamer_multistream_detect_orientation(uint32_t width,
                                                               uint32_t height);

/* Build video filter for orientation conversion */
char *restreamer_multistream_build_video_filter(stream_orientation_t source,
                                                stream_orientation_t target);

/* Start multistreaming */
bool restreamer_multistream_start(restreamer_api_t *api,
                                  multistream_config_t *config,
                                  const char *input_url);

/* Stop multistreaming */
bool restreamer_multistream_stop(restreamer_api_t *api,
                                 const char *process_reference);

/* ========================================================================
 * Dynamic Multistream Management
 * ======================================================================== */

/* Add destination to active multistream (hot-add) */
bool restreamer_multistream_add_destination_live(restreamer_api_t *api,
                                                 multistream_config_t *config,
                                                 size_t dest_index);

/* Remove destination from active multistream (hot-remove) */
bool restreamer_multistream_remove_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index);

/* Enable destination in active multistream */
bool restreamer_multistream_enable_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index,
    bool enabled);

/* Update destination settings in active multistream */
bool restreamer_multistream_update_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index,
    const char *stream_key);

/* Check if multistream is currently active */
bool restreamer_multistream_is_active(restreamer_api_t *api,
                                      multistream_config_t *config);

/* Load multistream config from settings */
void restreamer_multistream_load_from_settings(multistream_config_t *config,
                                               obs_data_t *settings);

/* Save multistream config to settings */
void restreamer_multistream_save_to_settings(multistream_config_t *config,
                                             obs_data_t *settings);

#ifdef __cplusplus
}
#endif
