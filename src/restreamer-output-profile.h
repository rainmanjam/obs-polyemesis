#pragma once

#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Output profile for managing multiple concurrent streams */

typedef enum {
	PROFILE_STATUS_INACTIVE,  /* Profile exists but not streaming */
	PROFILE_STATUS_STARTING,  /* Profile is starting streams */
	PROFILE_STATUS_ACTIVE,    /* Profile is actively streaming */
	PROFILE_STATUS_STOPPING,  /* Profile is stopping streams */
	PROFILE_STATUS_ERROR      /* Profile encountered an error */
} profile_status_t;

/* Per-destination encoding settings */
typedef struct {
	/* Video settings */
	uint32_t width;              /* Output width (0 = use source) */
	uint32_t height;             /* Output height (0 = use source) */
	uint32_t bitrate;            /* Video bitrate in kbps (0 = use default) */
	uint32_t fps_num;            /* FPS numerator (0 = use source) */
	uint32_t fps_den;            /* FPS denominator (0 = use source) */

	/* Audio settings */
	uint32_t audio_bitrate;      /* Audio bitrate in kbps (0 = use default) */
	uint32_t audio_track;        /* OBS audio track index (1-6, 0 = default) */

	/* Network settings */
	uint32_t max_bandwidth;      /* Max bandwidth in kbps (0 = unlimited) */
	bool low_latency;            /* Enable low latency mode */
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
} profile_destination_t;

/* Output profile structure */
typedef struct output_profile {
	char *profile_name;                  /* User-friendly name */
	char *profile_id;                    /* Unique identifier */

	/* Source configuration */
	stream_orientation_t source_orientation; /* Auto, Horizontal, Vertical, Square */
	bool auto_detect_orientation;
	uint32_t source_width;               /* Expected source width */
	uint32_t source_height;              /* Expected source height */
	char *input_url;                     /* RTMP input URL (rtmp://host/app/key) */

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
	bool auto_start;                     /* Auto-start with OBS streaming */
	bool auto_reconnect;                 /* Auto-reconnect on disconnect */
	uint32_t reconnect_delay_sec;        /* Delay before reconnect */
} output_profile_t;

/* Profile manager - manages all profiles */
typedef struct {
	output_profile_t **profiles;
	size_t profile_count;
	restreamer_api_t *api;               /* Shared API connection */
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

/* Enable/disable destination */
bool profile_set_destination_enabled(output_profile_t *profile,
                                     size_t index, bool enabled);

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

#ifdef __cplusplus
}
#endif
