#include "restreamer-multistream.h"
#include <math.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

multistream_config_t *restreamer_multistream_create(void) {
  multistream_config_t *config = bzalloc(sizeof(multistream_config_t));
  config->auto_detect_orientation = true;
  config->source_orientation = ORIENTATION_AUTO;
  return config;
}

void restreamer_multistream_destroy(multistream_config_t *config) {
  if (!config) {
    return;
  }

  for (size_t i = 0; i < config->destination_count; i++) {
    bfree(config->destinations[i].service_name);
    bfree(config->destinations[i].stream_key);
    bfree(config->destinations[i].rtmp_url);
    bfree(config->destinations[i].output_id);
  }

  bfree(config->destinations);
  bfree(config->process_reference);
  bfree(config);
}

// cppcheck-suppress staticFunction
const char *
restreamer_multistream_get_service_url(streaming_service_t service,
                                       stream_orientation_t orientation) {
  /* Treat AUTO as HORIZONTAL (landscape) for URL selection */
  if (orientation == ORIENTATION_AUTO) {
    orientation = ORIENTATION_HORIZONTAL;
  }

  /* Return RTMP ingest URLs for various services */
  switch (service) {
  case SERVICE_TWITCH:
    return "rtmp://live.twitch.tv/app";

  case SERVICE_YOUTUBE:
    return "rtmp://a.rtmp.youtube.com/live2";

  case SERVICE_FACEBOOK:
    return "rtmps://live-api-s.facebook.com:443/rtmp";

  case SERVICE_KICK:
    return "rtmp://stream.kick.com/app";

  case SERVICE_TIKTOK:
    /* TikTok uses different endpoints for orientation */
    if (orientation == ORIENTATION_VERTICAL) {
      return "rtmp://live.tiktok.com/live";
    }
    return "rtmp://live.tiktok.com/live/horizontal";

  case SERVICE_INSTAGRAM:
    return "rtmps://live-upload.instagram.com:443/rtmp";

  case SERVICE_X_TWITTER:
    return "rtmp://ingest.pscp.tv:80/x";

  case SERVICE_CUSTOM:
  default:
    return "";
  }
}

// cppcheck-suppress staticFunction
const char *
restreamer_multistream_get_service_name(streaming_service_t service) {
  switch (service) {
  case SERVICE_TWITCH:
    return "Twitch";
  case SERVICE_YOUTUBE:
    return "YouTube";
  case SERVICE_FACEBOOK:
    return "Facebook";
  case SERVICE_KICK:
    return "Kick";
  case SERVICE_TIKTOK:
    return "TikTok";
  case SERVICE_INSTAGRAM:
    return "Instagram";
  case SERVICE_X_TWITTER:
    return "X (Twitter)";
  case SERVICE_CUSTOM:
  default:
    return "Custom";
  }
}

// cppcheck-suppress staticFunction
bool restreamer_multistream_add_destination(multistream_config_t *config,
                                            streaming_service_t service,
                                            const char *stream_key,
                                            stream_orientation_t orientation) {
  if (!config || !stream_key || stream_key[0] == '\0') {
    return false;
  }

  /* Expand destinations array */
  size_t new_count = config->destination_count + 1;
  config->destinations =
      brealloc(config->destinations, sizeof(stream_destination_t) * new_count);

  stream_destination_t *dest = &config->destinations[config->destination_count];
  memset(dest, 0, sizeof(stream_destination_t));

  dest->service = service;
  dest->service_name =
      bstrdup(restreamer_multistream_get_service_name(service));
  dest->stream_key = bstrdup(stream_key);
  dest->rtmp_url =
      bstrdup(restreamer_multistream_get_service_url(service, orientation));
  dest->supported_orientation = orientation;
  dest->enabled = true;

  config->destination_count = new_count;

  return true;
}

void restreamer_multistream_remove_destination(multistream_config_t *config,
                                               size_t index) {
  if (!config || index >= config->destination_count) {
    return;
  }

  /* Free the destination */
  bfree(config->destinations[index].service_name);
  bfree(config->destinations[index].stream_key);
  bfree(config->destinations[index].rtmp_url);
  bfree(config->destinations[index].output_id);

  /* Shift remaining destinations */
  if (index < config->destination_count - 1) {
    memmove(&config->destinations[index], &config->destinations[index + 1],
            sizeof(stream_destination_t) *
                (config->destination_count - index - 1));
  }

  config->destination_count--;

  if (config->destination_count == 0) {
    bfree(config->destinations);
    config->destinations = NULL;
  }
}

stream_orientation_t
restreamer_multistream_detect_orientation(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) {
    return ORIENTATION_AUTO;
  }

  float aspect_ratio = (float)width / (float)height;

  /* Check if square (within 5% tolerance) */
  if (fabs(aspect_ratio - 1.0f) < 0.05f) {
    return ORIENTATION_SQUARE;
  }

  /* Vertical (portrait) */
  if (aspect_ratio < 1.0f) {
    return ORIENTATION_VERTICAL;
  }

  /* Horizontal (landscape) */
  return ORIENTATION_HORIZONTAL;
}

char *restreamer_multistream_build_video_filter(stream_orientation_t source,
                                                stream_orientation_t target) {
  struct dstr filter;
  dstr_init(&filter);

  /* If same orientation, no filter needed */
  if (source == target) {
    dstr_free(&filter);  // FIX: Free before returning
    return NULL;
  }

  /* Build FFmpeg video filter to convert orientation */
  if (source == ORIENTATION_HORIZONTAL && target == ORIENTATION_VERTICAL) {
    /* Landscape to Portrait: crop and scale */
    dstr_cat(&filter, "crop=ih*9/16:ih,scale=1080:1920");
  } else if (source == ORIENTATION_VERTICAL &&
             target == ORIENTATION_HORIZONTAL) {
    /* Portrait to Landscape: crop and scale */
    dstr_cat(&filter, "crop=iw:iw*9/16,scale=1920:1080");
  } else if (source == ORIENTATION_SQUARE && target == ORIENTATION_HORIZONTAL) {
    /* Square to Landscape */
    dstr_cat(&filter, "scale=1920:1080,setsar=1");
  } else if (source == ORIENTATION_SQUARE && target == ORIENTATION_VERTICAL) {
    /* Square to Portrait */
    dstr_cat(&filter, "scale=1080:1920,setsar=1");
  } else if (target == ORIENTATION_SQUARE) {
    /* Any to Square */
    dstr_cat(&filter, "scale=1080:1080,setsar=1");
  }

  char *result = bstrdup(filter.array);
  dstr_free(&filter);

  return result;
}

bool restreamer_multistream_start(restreamer_api_t *api,
                                  multistream_config_t *config,
                                  const char *input_url) {
  if (!api || !config || !input_url) {
    return false;
  }

  if (config->destination_count == 0) {
    obs_log(LOG_WARNING, "No destinations configured for multistreaming");
    return false;
  }

  /* Build output URLs array */
  struct dstr *output_urls =
      bzalloc(sizeof(struct dstr) * config->destination_count);
  size_t active_count = 0;

  for (size_t i = 0; i < config->destination_count; i++) {
    stream_destination_t *dest = &config->destinations[i];

    if (!dest->enabled) {
      continue;
    }

    dstr_init(&output_urls[active_count]);

    /* Build complete RTMP URL with stream key */
    dstr_copy(&output_urls[active_count], dest->rtmp_url);

    if (dest->service == SERVICE_CUSTOM) {
      /* Custom URL might already include everything */
      if (dest->stream_key && strlen(dest->stream_key) > 0) {
        dstr_cat(&output_urls[active_count], "/");
        dstr_cat(&output_urls[active_count], dest->stream_key);
      }
    } else {
      /* Standard services need stream key appended */
      dstr_cat(&output_urls[active_count], "/");
      dstr_cat(&output_urls[active_count], dest->stream_key);
    }

    active_count++;
  }

  if (active_count == 0) {
    bfree(output_urls);
    obs_log(LOG_WARNING, "No enabled destinations for multistreaming");
    return false;
  }

  /* Build array of URL strings for API */
  const char **url_strings = bzalloc(sizeof(char *) * active_count);
  for (size_t i = 0; i < active_count; i++) {
    url_strings[i] = output_urls[i].array;
  }

  /* Create process reference */
  if (!config->process_reference) {
    struct dstr ref;
    dstr_init(&ref);
    dstr_printf(&ref, "obs_multistream_%lld", (long long)os_gettime_ns());
    config->process_reference = bstrdup(ref.array);
    dstr_free(&ref);
  }

  /* Build video filter if needed */
  char *video_filter = NULL;
  /* For now, we'll apply filters per-destination in a more complex setup
     This is a simplified version */

  /* Create the multistream process */
  bool result =
      restreamer_api_create_process(api, config->process_reference, input_url,
                                    url_strings, active_count, video_filter);

  /* Clean up */
  for (size_t i = 0; i < active_count; i++) {
    dstr_free(&output_urls[i]);
  }
  bfree(output_urls);
  bfree(url_strings);
  bfree(video_filter);

  if (result) {
    obs_log(LOG_INFO, "Multistream started with %d destinations",
            (int)active_count);
  } else {
    obs_log(LOG_ERROR, "Failed to start multistream: %s",
            restreamer_api_get_error(api));
  }

  return result;
}

bool restreamer_multistream_stop(restreamer_api_t *api,
                                 const char *process_reference) {
  if (!api || !process_reference) {
    return false;
  }

  /* Find the process by reference and stop it */
  restreamer_process_list_t list = {0};
  if (!restreamer_api_get_processes(api, &list)) {
    restreamer_api_free_process_list(&list);  // FIX: Ensure cleanup on failure
    return false;
  }

  bool found = false;
  char *process_id = NULL;

  for (size_t i = 0; i < list.count; i++) {
    if (list.processes[i].reference &&
        strcmp(list.processes[i].reference, process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }

  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_WARNING, "Process not found: %s", process_reference);
    return false;
  }

  bool result = restreamer_api_stop_process(api, process_id);
  bfree(process_id);

  return result;
}

void restreamer_multistream_load_from_settings(multistream_config_t *config,
                                               obs_data_t *settings) {
  if (!config || !settings) {
    return;
  }

  config->auto_detect_orientation =
      obs_data_get_bool(settings, "auto_detect_orientation");

  int source_orientation_int = (int)obs_data_get_int(settings, "source_orientation");
  if (source_orientation_int < ORIENTATION_AUTO || source_orientation_int > ORIENTATION_SQUARE) {
    config->source_orientation = ORIENTATION_AUTO;
  } else {
    config->source_orientation = (stream_orientation_t)source_orientation_int;
  }

  /* Load destinations */
  obs_data_array_t *destinations_array =
      obs_data_get_array(settings, "destinations");
  if (destinations_array) {
    size_t count = obs_data_array_count(destinations_array);

    for (size_t i = 0; i < count; i++) {
      obs_data_t *dest_data = obs_data_array_item(destinations_array, i);

      streaming_service_t service =
          (streaming_service_t)obs_data_get_int(dest_data, "service");
      const char *stream_key = obs_data_get_string(dest_data, "stream_key");

      int orientation_int = (int)obs_data_get_int(dest_data, "orientation");
      stream_orientation_t orientation;
      if (orientation_int < ORIENTATION_AUTO || orientation_int > ORIENTATION_SQUARE) {
        orientation = ORIENTATION_AUTO;
      } else {
        orientation = (stream_orientation_t)orientation_int;
      }

      bool enabled = obs_data_get_bool(dest_data, "enabled");

      if (stream_key && strlen(stream_key) > 0) {
        restreamer_multistream_add_destination(config, service, stream_key,
                                               orientation);

        if (config->destination_count > 0) {
          config->destinations[config->destination_count - 1].enabled = enabled;
        }
      }

      obs_data_release(dest_data);
    }

    obs_data_array_release(destinations_array);
  }
}

void restreamer_multistream_save_to_settings(multistream_config_t *config,
                                             obs_data_t *settings) {
  if (!config || !settings) {
    return;
  }

  obs_data_set_bool(settings, "auto_detect_orientation",
                    config->auto_detect_orientation);
  obs_data_set_int(settings, "source_orientation", config->source_orientation);

  /* Save destinations */
  obs_data_array_t *destinations_array = obs_data_array_create();

  for (size_t i = 0; i < config->destination_count; i++) {
    obs_data_t *dest_data = obs_data_create();
    stream_destination_t *dest = &config->destinations[i];

    obs_data_set_int(dest_data, "service", dest->service);
    obs_data_set_string(dest_data, "stream_key",
                        dest->stream_key ? dest->stream_key : "");
    obs_data_set_int(dest_data, "orientation", dest->supported_orientation);
    obs_data_set_bool(dest_data, "enabled", dest->enabled);

    obs_data_array_push_back(destinations_array, dest_data);
    obs_data_release(dest_data);
  }

  obs_data_set_array(settings, "destinations", destinations_array);
  obs_data_array_release(destinations_array);
}

/* ========================================================================
 * Dynamic Multistream Management Implementation
 * ======================================================================== */

bool restreamer_multistream_add_destination_live(restreamer_api_t *api,
                                                 multistream_config_t *config,
                                                 size_t dest_index) {
  if (!api || !config || dest_index >= config->destination_count) {
    return false;
  }

  if (!config->process_reference) {
    obs_log(LOG_ERROR, "Cannot add destination: multistream not active");
    return false;
  }

  stream_destination_t *dest = &config->destinations[dest_index];

  /* Build output ID and URL */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", dest->service_name, dest_index);

  struct dstr output_url;
  dstr_init(&output_url);
  dstr_copy(&output_url, dest->rtmp_url);
  dstr_cat(&output_url, "/");
  dstr_cat(&output_url, dest->stream_key);

  /* Build video filter if needed */
  char *video_filter = restreamer_multistream_build_video_filter(
      config->source_orientation, dest->supported_orientation);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (!restreamer_api_get_processes(api, &list)) {
    dstr_free(&output_id);
    dstr_free(&output_url);
    bfree(video_filter);
    return false;
  }

  for (size_t i = 0; i < list.count; i++) {
    if (list.processes[i].reference &&
        strcmp(list.processes[i].reference, config->process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }
  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", config->process_reference);
    dstr_free(&output_id);
    dstr_free(&output_url);
    bfree(video_filter);
    return false;
  }

  /* Add output to running process */
  bool result = restreamer_api_add_process_output(
      api, process_id, output_id.array, output_url.array, video_filter);

  bfree(process_id);
  dstr_free(&output_id);
  dstr_free(&output_url);
  bfree(video_filter);

  if (result) {
    dest->enabled = true;
    obs_log(LOG_INFO, "Successfully added destination %s to active multistream",
            dest->service_name);
  }

  return result;
}

bool restreamer_multistream_remove_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index) {
  if (!api || !config || dest_index >= config->destination_count) {
    return false;
  }

  if (!config->process_reference) {
    obs_log(LOG_ERROR, "Cannot remove destination: multistream not active");
    return false;
  }

  stream_destination_t *dest = &config->destinations[dest_index];

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", dest->service_name, dest_index);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (!restreamer_api_get_processes(api, &list)) {
    dstr_free(&output_id);
    return false;
  }

  for (size_t i = 0; i < list.count; i++) {
    if (list.processes[i].reference &&
        strcmp(list.processes[i].reference, config->process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }
  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", config->process_reference);
    dstr_free(&output_id);
    return false;
  }

  /* Remove output from running process */
  bool result =
      restreamer_api_remove_process_output(api, process_id, output_id.array);

  bfree(process_id);
  dstr_free(&output_id);

  if (result) {
    dest->enabled = false;
    obs_log(LOG_INFO,
            "Successfully removed destination %s from active multistream",
            dest->service_name);
  }

  return result;
}

bool restreamer_multistream_enable_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index,
    bool enabled) {
  if (!api || !config || dest_index >= config->destination_count) {
    return false;
  }

  stream_destination_t *dest = &config->destinations[dest_index];

  /* If already in desired state, nothing to do */
  if (dest->enabled == enabled) {
    return true;
  }

  /* Add or remove based on enabled flag */
  if (enabled) {
    return restreamer_multistream_add_destination_live(api, config, dest_index);
  } else {
    return restreamer_multistream_remove_destination_live(api, config,
                                                          dest_index);
  }
}

bool restreamer_multistream_update_destination_live(
    restreamer_api_t *api, multistream_config_t *config, size_t dest_index,
    const char *stream_key) {
  if (!api || !config || dest_index >= config->destination_count ||
      !stream_key) {
    return false;
  }

  if (!config->process_reference) {
    obs_log(LOG_ERROR, "Cannot update destination: multistream not active");
    return false;
  }

  stream_destination_t *dest = &config->destinations[dest_index];

  /* Update stream key */
  bfree(dest->stream_key);
  dest->stream_key = bstrdup(stream_key);

  /* Build new output URL */
  struct dstr output_url;
  dstr_init(&output_url);
  dstr_copy(&output_url, dest->rtmp_url);
  dstr_cat(&output_url, "/");
  dstr_cat(&output_url, stream_key);

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", dest->service_name, dest_index);

  /* Build video filter if needed */
  char *video_filter = restreamer_multistream_build_video_filter(
      config->source_orientation, dest->supported_orientation);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (!restreamer_api_get_processes(api, &list)) {
    dstr_free(&output_id);
    dstr_free(&output_url);
    bfree(video_filter);
    return false;
  }

  for (size_t i = 0; i < list.count; i++) {
    if (list.processes[i].reference &&
        strcmp(list.processes[i].reference, config->process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }
  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", config->process_reference);
    dstr_free(&output_id);
    dstr_free(&output_url);
    bfree(video_filter);
    return false;
  }

  /* Update output in running process */
  bool result = restreamer_api_update_process_output(
      api, process_id, output_id.array, output_url.array, video_filter);

  bfree(process_id);
  dstr_free(&output_id);
  dstr_free(&output_url);
  bfree(video_filter);

  if (result) {
    obs_log(LOG_INFO,
            "Successfully updated destination %s in active multistream",
            dest->service_name);
  }

  return result;
}

bool restreamer_multistream_is_active(restreamer_api_t *api,
                                      multistream_config_t *config) {
  if (!api || !config || !config->process_reference) {
    return false;
  }

  /* Check if process with this reference exists and is running */
  restreamer_process_list_t list = {0};
  bool is_active = false;

  if (restreamer_api_get_processes(api, &list)) {
    for (size_t i = 0; i < list.count; i++) {
      if (list.processes[i].reference &&
          strcmp(list.processes[i].reference, config->process_reference) == 0) {
        /* Check if state is "running" */
        if (list.processes[i].state &&
            strcmp(list.processes[i].state, "running") == 0) {
          is_active = true;
        }
        break;
      }
    }
    restreamer_api_free_process_list(&list);
  }

  return is_active;
}
