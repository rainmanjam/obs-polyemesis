#include "restreamer-output-profile.h"
#include <obs-module.h>
#include <plugin-support.h>
#include <time.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

/* Profile Manager Implementation */

profile_manager_t *profile_manager_create(restreamer_api_t *api) {
  profile_manager_t *manager = bzalloc(sizeof(profile_manager_t));
  manager->api = api;
  manager->profiles = NULL;
  manager->profile_count = 0;
  manager->templates = NULL;
  manager->template_count = 0;

  /* Load built-in templates */
  profile_manager_load_builtin_templates(manager);

  obs_log(LOG_INFO, "Profile manager created");
  return manager;
}

void profile_manager_destroy(profile_manager_t *manager) {
  if (!manager) {
    return;
  }

  /* Stop and destroy all profiles */
  for (size_t i = 0; i < manager->profile_count; i++) {
    output_profile_t *profile = manager->profiles[i];

    /* Stop if active */
    if (profile->status == PROFILE_STATUS_ACTIVE) {
      output_profile_stop(manager, profile->profile_id);
    }

    /* Destroy destinations */
    for (size_t j = 0; j < profile->destination_count; j++) {
      bfree(profile->destinations[j].service_name);
      bfree(profile->destinations[j].stream_key);
      bfree(profile->destinations[j].rtmp_url);
    }
    bfree(profile->destinations);

    /* Destroy profile */
    bfree(profile->profile_name);
    bfree(profile->profile_id);
    bfree(profile->last_error);
    bfree(profile->process_reference);
    bfree(profile->input_url);
    bfree(profile);
  }

  bfree(manager->profiles);

  /* Destroy all templates */
  for (size_t i = 0; i < manager->template_count; i++) {
    destination_template_t *tmpl = manager->templates[i];
    bfree(tmpl->template_name);
    bfree(tmpl->template_id);
    bfree(tmpl);
  }
  bfree(manager->templates);

  bfree(manager);

  obs_log(LOG_INFO, "Profile manager destroyed");
}

char *profile_generate_id(void) {
  struct dstr id = {0};
  dstr_init(&id);

  /* Use timestamp + random component */
  uint64_t timestamp = (uint64_t)time(NULL);
  uint32_t random = (uint32_t)rand();

  dstr_printf(&id, "profile_%llu_%u", (unsigned long long)timestamp, random);

  char *result = bstrdup(id.array);
  dstr_free(&id);

  return result;
}

output_profile_t *profile_manager_create_profile(profile_manager_t *manager,
                                                 const char *name) {
  if (!manager || !name) {
    return NULL;
  }

  /* Allocate new profile */
  output_profile_t *profile = bzalloc(sizeof(output_profile_t));

  /* Set basic properties */
  profile->profile_name = bstrdup(name);
  profile->profile_id = profile_generate_id();
  profile->source_orientation = ORIENTATION_AUTO;
  profile->auto_detect_orientation = true;
  profile->status = PROFILE_STATUS_INACTIVE;
  profile->auto_reconnect = true;
  profile->reconnect_delay_sec = 5;

  /* Set default input URL */
  profile->input_url = bstrdup("rtmp://localhost/live/obs_input");

  /* Add to manager */
  size_t new_count = manager->profile_count + 1;
  manager->profiles =
      brealloc(manager->profiles, sizeof(output_profile_t *) * new_count);
  manager->profiles[manager->profile_count] = profile;
  manager->profile_count = new_count;

  obs_log(LOG_INFO, "Created profile: %s (ID: %s)", name, profile->profile_id);

  return profile;
}

bool profile_manager_delete_profile(profile_manager_t *manager,
                                    const char *profile_id) {
  if (!manager || !profile_id) {
    return false;
  }

  /* Find profile */
  for (size_t i = 0; i < manager->profile_count; i++) {
    output_profile_t *profile = manager->profiles[i];
    if (strcmp(profile->profile_id, profile_id) == 0) {
      /* Stop if active */
      if (profile->status == PROFILE_STATUS_ACTIVE) {
        output_profile_stop(manager, profile_id);
      }

      /* Free destinations */
      for (size_t j = 0; j < profile->destination_count; j++) {
        bfree(profile->destinations[j].service_name);
        bfree(profile->destinations[j].stream_key);
        bfree(profile->destinations[j].rtmp_url);
      }
      bfree(profile->destinations);

      /* Free profile */
      bfree(profile->profile_name);
      bfree(profile->profile_id);
      bfree(profile->last_error);
      bfree(profile->process_reference);
      bfree(profile);

      /* Shift remaining profiles */
      if (i < manager->profile_count - 1) {
        memmove(&manager->profiles[i], &manager->profiles[i + 1],
                sizeof(output_profile_t *) * (manager->profile_count - i - 1));
      }

      manager->profile_count--;

      if (manager->profile_count == 0) {
        bfree(manager->profiles);
        manager->profiles = NULL;
      }

      obs_log(LOG_INFO, "Deleted profile: %s", profile_id);
      return true;
    }
  }

  return false;
}

output_profile_t *profile_manager_get_profile(profile_manager_t *manager,
                                              const char *profile_id) {
  if (!manager || !profile_id) {
    return NULL;
  }

  for (size_t i = 0; i < manager->profile_count; i++) {
    if (strcmp(manager->profiles[i]->profile_id, profile_id) == 0) {
      return manager->profiles[i];
    }
  }

  return NULL;
}

output_profile_t *profile_manager_get_profile_at(profile_manager_t *manager,
                                                 size_t index) {
  if (!manager || index >= manager->profile_count) {
    return NULL;
  }

  return manager->profiles[index];
}

size_t profile_manager_get_count(profile_manager_t *manager) {
  return manager ? manager->profile_count : 0;
}

/* Profile Operations */

encoding_settings_t profile_get_default_encoding(void) {
  encoding_settings_t settings = {0};

  /* Default: Use source settings */
  settings.width = 0;
  settings.height = 0;
  settings.bitrate = 0;
  settings.fps_num = 0;
  settings.fps_den = 0;
  settings.audio_bitrate = 0;
  settings.audio_track = 0;
  settings.max_bandwidth = 0;
  settings.low_latency = false;

  return settings;
}

bool profile_add_destination(output_profile_t *profile,
                             streaming_service_t service,
                             const char *stream_key,
                             stream_orientation_t target_orientation,
                             encoding_settings_t *encoding) {
  if (!profile || !stream_key) {
    return false;
  }

  /* Expand destinations array */
  size_t new_count = profile->destination_count + 1;
  profile->destinations = brealloc(profile->destinations,
                                   sizeof(profile_destination_t) * new_count);

  profile_destination_t *dest =
      &profile->destinations[profile->destination_count];
  memset(dest, 0, sizeof(profile_destination_t));

  /* Set basic properties */
  dest->service = service;
  dest->service_name =
      bstrdup(restreamer_multistream_get_service_name(service));
  dest->stream_key = bstrdup(stream_key);
  dest->rtmp_url = bstrdup(
      restreamer_multistream_get_service_url(service, target_orientation));
  dest->target_orientation = target_orientation;
  dest->enabled = true;

  /* Set encoding settings */
  if (encoding) {
    dest->encoding = *encoding;
  } else {
    dest->encoding = profile_get_default_encoding();
  }

  /* Initialize backup/failover fields */
  dest->is_backup = false;
  dest->primary_index = (size_t)-1;
  dest->backup_index = (size_t)-1;
  dest->failover_active = false;
  dest->failover_start_time = 0;

  profile->destination_count = new_count;

  obs_log(LOG_INFO, "Added destination %s to profile %s", dest->service_name,
          profile->profile_name);

  return true;
}

bool profile_remove_destination(output_profile_t *profile, size_t index) {
  if (!profile || index >= profile->destination_count) {
    return false;
  }

  /* Free destination */
  bfree(profile->destinations[index].service_name);
  bfree(profile->destinations[index].stream_key);
  bfree(profile->destinations[index].rtmp_url);

  /* Shift remaining destinations */
  if (index < profile->destination_count - 1) {
    memmove(&profile->destinations[index], &profile->destinations[index + 1],
            sizeof(profile_destination_t) *
                (profile->destination_count - index - 1));
  }

  profile->destination_count--;

  if (profile->destination_count == 0) {
    bfree(profile->destinations);
    profile->destinations = NULL;
  }

  return true;
}

bool profile_update_destination_encoding(output_profile_t *profile,
                                         size_t index,
                                         encoding_settings_t *encoding) {
  if (!profile || !encoding || index >= profile->destination_count) {
    return false;
  }

  profile->destinations[index].encoding = *encoding;
  return true;
}

bool profile_update_destination_encoding_live(output_profile_t *profile,
                                              restreamer_api_t *api,
                                              size_t index,
                                              encoding_settings_t *encoding) {
  if (!profile || !api || !encoding || index >= profile->destination_count) {
    return false;
  }

  /* Check if profile is active */
  if (profile->status != PROFILE_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot update encoding live: profile '%s' is not active",
            profile->profile_name);
    return false;
  }

  if (!profile->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active profile '%s'",
            profile->profile_name);
    return false;
  }

  profile_destination_t *dest = &profile->destinations[index];

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", dest->service_name, index);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (restreamer_api_get_processes(api, &list)) {
    for (size_t i = 0; i < list.count; i++) {
      if (list.processes[i].reference &&
          strcmp(list.processes[i].reference, profile->process_reference) ==
              0) {
        process_id = bstrdup(list.processes[i].id);
        found = true;
        break;
      }
    }
    restreamer_api_free_process_list(&list);
  }

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", profile->process_reference);
    dstr_free(&output_id);
    return false;
  }

  /* Convert profile encoding settings to API encoding params */
  encoding_params_t params = {0};
  params.video_bitrate_kbps = encoding->bitrate;
  params.audio_bitrate_kbps = encoding->audio_bitrate;
  params.width = encoding->width;
  params.height = encoding->height;
  params.fps_num = encoding->fps_num;
  params.fps_den = encoding->fps_den;
  /* Note: preset and profile not stored in encoding_settings_t */
  params.preset = NULL;
  params.profile = NULL;

  /* Update encoding via API */
  bool result = restreamer_api_update_output_encoding(api, process_id,
                                                      output_id.array, &params);

  bfree(process_id);
  dstr_free(&output_id);

  if (result) {
    /* Update local copy */
    dest->encoding = *encoding;
    obs_log(LOG_INFO,
            "Successfully updated encoding for destination %s in profile %s",
            dest->service_name, profile->profile_name);
  }

  return result;
}

bool profile_set_destination_enabled(output_profile_t *profile, size_t index,
                                     bool enabled) {
  if (!profile || index >= profile->destination_count) {
    return false;
  }

  profile->destinations[index].enabled = enabled;
  return true;
}

/* Streaming Control */

bool output_profile_start(profile_manager_t *manager, const char *profile_id) {
  if (!manager || !profile_id) {
    return false;
  }

  output_profile_t *profile = profile_manager_get_profile(manager, profile_id);
  if (!profile) {
    obs_log(LOG_ERROR, "Profile not found: %s", profile_id);
    return false;
  }

  if (profile->status == PROFILE_STATUS_ACTIVE) {
    obs_log(LOG_WARNING, "Profile already active: %s", profile->profile_name);
    return true;
  }

  /* Count enabled destinations */
  size_t enabled_count = 0;
  for (size_t i = 0; i < profile->destination_count; i++) {
    if (profile->destinations[i].enabled) {
      enabled_count++;
    }
  }

  if (enabled_count == 0) {
    obs_log(LOG_ERROR, "No enabled destinations in profile: %s",
            profile->profile_name);
    bfree(profile->last_error);
    profile->last_error = bstrdup("No enabled destinations configured");
    profile->status = PROFILE_STATUS_ERROR;
    return false;
  }

  profile->status = PROFILE_STATUS_STARTING;

  /* Check if API is available */
  if (!manager->api) {
    obs_log(LOG_ERROR, "No Restreamer API connection available for profile: %s",
            profile->profile_name);
    bfree(profile->last_error);
    profile->last_error = bstrdup("No Restreamer API connection");
    profile->status = PROFILE_STATUS_ERROR;
    return false;
  }

  /* Create temporary multistream config from profile destinations */
  multistream_config_t *config = restreamer_multistream_create();
  if (!config) {
    obs_log(LOG_ERROR, "Failed to create multistream config");
    profile->status = PROFILE_STATUS_ERROR;
    return false;
  }

  /* Set source orientation */
  config->source_orientation = profile->source_orientation;
  config->auto_detect_orientation = false;

  /* Set process reference to profile ID for tracking */
  config->process_reference = bstrdup(profile->profile_id);

  /* Copy enabled destinations */
  for (size_t i = 0; i < profile->destination_count; i++) {
    profile_destination_t *pdest = &profile->destinations[i];
    if (!pdest->enabled) {
      continue;
    }

    /* Add destination to multistream config */
    if (!restreamer_multistream_add_destination(config, pdest->service,
                                                pdest->stream_key,
                                                pdest->target_orientation)) {
      obs_log(LOG_WARNING, "Failed to add destination %s to profile %s",
              pdest->service_name, profile->profile_name);
    }
  }

  /* Use configured input URL */
  const char *input_url = profile->input_url;
  if (!input_url || strlen(input_url) == 0) {
    obs_log(LOG_ERROR, "No input URL configured for profile: %s",
            profile->profile_name);
    bfree(profile->last_error);
    profile->last_error = bstrdup("No input URL configured");
    restreamer_multistream_destroy(config);
    profile->status = PROFILE_STATUS_ERROR;
    return false;
  }

  obs_log(LOG_INFO, "Starting profile: %s with %zu destinations (input: %s)",
          profile->profile_name, enabled_count, input_url);

  /* Start multistream */
  if (!restreamer_multistream_start(manager->api, config, input_url)) {
    obs_log(LOG_ERROR, "Failed to start multistream for profile: %s",
            profile->profile_name);
    bfree(profile->last_error);
    profile->last_error = bstrdup(restreamer_api_get_error(manager->api));
    restreamer_multistream_destroy(config);
    profile->status = PROFILE_STATUS_ERROR;
    return false;
  }

  /* Store process reference for stopping later */
  bfree(profile->process_reference);
  profile->process_reference = bstrdup(config->process_reference);

  /* Clean up temporary config */
  restreamer_multistream_destroy(config);

  /* Clear last_error on successful start */
  bfree(profile->last_error);
  profile->last_error = NULL;

  profile->status = PROFILE_STATUS_ACTIVE;
  obs_log(LOG_INFO,
          "Profile %s started successfully with process reference: %s",
          profile->profile_name, profile->process_reference);

  return true;
}

bool output_profile_stop(profile_manager_t *manager, const char *profile_id) {
  if (!manager || !profile_id) {
    return false;
  }

  output_profile_t *profile = profile_manager_get_profile(manager, profile_id);
  if (!profile) {
    return false;
  }

  if (profile->status == PROFILE_STATUS_INACTIVE) {
    return true;
  }

  profile->status = PROFILE_STATUS_STOPPING;

  /* Stop the Restreamer process if we have a reference */
  if (profile->process_reference && manager->api) {
    obs_log(LOG_INFO,
            "Stopping Restreamer process for profile: %s (reference: %s)",
            profile->profile_name, profile->process_reference);

    if (!restreamer_multistream_stop(manager->api,
                                     profile->process_reference)) {
      obs_log(LOG_WARNING,
              "Failed to stop Restreamer process for profile: %s: %s",
              profile->profile_name, restreamer_api_get_error(manager->api));
      /* Continue anyway to update status */
    }

    /* Clear process reference */
    bfree(profile->process_reference);
    profile->process_reference = NULL;
  }

  obs_log(LOG_INFO, "Stopped profile: %s", profile->profile_name);

  /* Clear last_error on successful stop */
  bfree(profile->last_error);
  profile->last_error = NULL;

  profile->status = PROFILE_STATUS_INACTIVE;
  return true;
}

bool profile_restart(profile_manager_t *manager, const char *profile_id) {
  output_profile_stop(manager, profile_id);
  return output_profile_start(manager, profile_id);
}

bool profile_manager_start_all(profile_manager_t *manager) {
  if (!manager) {
    return false;
  }

  obs_log(LOG_INFO, "Starting all profiles (%zu total)",
          manager->profile_count);

  bool all_success = true;
  for (size_t i = 0; i < manager->profile_count; i++) {
    output_profile_t *profile = manager->profiles[i];
    if (profile->auto_start) {
      if (!output_profile_start(manager, profile->profile_id)) {
        all_success = false;
      }
    }
  }

  return all_success;
}

bool profile_manager_stop_all(profile_manager_t *manager) {
  if (!manager) {
    return false;
  }

  obs_log(LOG_INFO, "Stopping all profiles");

  bool all_success = true;
  for (size_t i = 0; i < manager->profile_count; i++) {
    if (!output_profile_stop(manager, manager->profiles[i]->profile_id)) {
      all_success = false;
    }
  }

  return all_success;
}

size_t profile_manager_get_active_count(profile_manager_t *manager) {
  if (!manager) {
    return 0;
  }

  size_t active_count = 0;
  for (size_t i = 0; i < manager->profile_count; i++) {
    if (manager->profiles[i]->status == PROFILE_STATUS_ACTIVE) {
      active_count++;
    }
  }

  return active_count;
}

/* ========================================================================
 * Preview/Test Mode Implementation
 * ======================================================================== */

bool output_profile_start_preview(profile_manager_t *manager,
                                  const char *profile_id,
                                  uint32_t duration_sec) {
  if (!manager || !profile_id) {
    return false;
  }

  output_profile_t *profile = profile_manager_get_profile(manager, profile_id);
  if (!profile) {
    obs_log(LOG_ERROR, "Profile not found: %s", profile_id);
    return false;
  }

  if (profile->status != PROFILE_STATUS_INACTIVE) {
    obs_log(LOG_WARNING, "Profile '%s' is not inactive, cannot start preview",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO, "Starting preview mode for profile: %s (duration: %u sec)",
          profile->profile_name, duration_sec);

  /* Enable preview mode */
  profile->preview_mode_enabled = true;
  profile->preview_duration_sec = duration_sec;
  profile->preview_start_time = time(NULL);

  /* Start the profile normally */
  if (!output_profile_start(manager, profile_id)) {
    profile->preview_mode_enabled = false;
    profile->preview_duration_sec = 0;
    profile->preview_start_time = 0;
    return false;
  }

  /* Update status to preview */
  profile->status = PROFILE_STATUS_PREVIEW;

  obs_log(LOG_INFO, "Preview mode started successfully for profile: %s",
          profile->profile_name);

  return true;
}

bool output_profile_preview_to_live(profile_manager_t *manager,
                                    const char *profile_id) {
  if (!manager || !profile_id) {
    return false;
  }

  output_profile_t *profile = profile_manager_get_profile(manager, profile_id);
  if (!profile) {
    obs_log(LOG_ERROR, "Profile not found: %s", profile_id);
    return false;
  }

  if (profile->status != PROFILE_STATUS_PREVIEW) {
    obs_log(LOG_WARNING, "Profile '%s' is not in preview mode, cannot go live",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO, "Converting preview to live for profile: %s",
          profile->profile_name);

  /* Disable preview mode */
  profile->preview_mode_enabled = false;
  profile->preview_duration_sec = 0;
  profile->preview_start_time = 0;

  /* Update status to active */
  /* Clear last_error on successful preview to live transition */
  bfree(profile->last_error);
  profile->last_error = NULL;

  profile->status = PROFILE_STATUS_ACTIVE;

  obs_log(LOG_INFO, "Profile %s is now live", profile->profile_name);

  return true;
}

bool output_profile_cancel_preview(profile_manager_t *manager,
                                   const char *profile_id) {
  if (!manager || !profile_id) {
    return false;
  }

  output_profile_t *profile = profile_manager_get_profile(manager, profile_id);
  if (!profile) {
    obs_log(LOG_ERROR, "Profile not found: %s", profile_id);
    return false;
  }

  if (profile->status != PROFILE_STATUS_PREVIEW) {
    obs_log(LOG_WARNING, "Profile '%s' is not in preview mode, cannot cancel",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO, "Canceling preview mode for profile: %s",
          profile->profile_name);

  /* Disable preview mode */
  profile->preview_mode_enabled = false;
  profile->preview_duration_sec = 0;
  profile->preview_start_time = 0;

  /* Stop the profile */
  bool result = output_profile_stop(manager, profile_id);

  obs_log(LOG_INFO, "Preview mode canceled for profile: %s",
          profile->profile_name);

  return result;
}

bool output_profile_check_preview_timeout(output_profile_t *profile) {
  if (!profile || !profile->preview_mode_enabled) {
    return false;
  }

  /* If duration is 0, preview mode is unlimited */
  if (profile->preview_duration_sec == 0) {
    return false;
  }

  /* Check if preview time has elapsed */
  time_t current_time = time(NULL);
  time_t elapsed = current_time - profile->preview_start_time;

  if (elapsed >= (time_t)profile->preview_duration_sec) {
    obs_log(LOG_INFO,
            "Preview timeout reached for profile: %s (elapsed: %ld sec)",
            profile->profile_name, (long)elapsed);
    return true;
  }

  return false;
}

/* Configuration Persistence */

void profile_manager_load_from_settings(profile_manager_t *manager,
                                        obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *profiles_array =
      obs_data_get_array(settings, "output_profiles");
  if (!profiles_array) {
    return;
  }

  size_t count = obs_data_array_count(profiles_array);
  for (size_t i = 0; i < count; i++) {
    obs_data_t *profile_data = obs_data_array_item(profiles_array, i);
    output_profile_t *profile = profile_load_from_settings(profile_data);

    if (profile) {
      /* Add to manager */
      size_t new_count = manager->profile_count + 1;
      manager->profiles =
          brealloc(manager->profiles, sizeof(output_profile_t *) * new_count);
      manager->profiles[manager->profile_count] = profile;
      manager->profile_count = new_count;
    }

    obs_data_release(profile_data);
  }

  obs_data_array_release(profiles_array);

  obs_log(LOG_INFO, "Loaded %zu profiles from settings", count);
}

void profile_manager_save_to_settings(profile_manager_t *manager,
                                      obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *profiles_array = obs_data_array_create();

  for (size_t i = 0; i < manager->profile_count; i++) {
    obs_data_t *profile_data = obs_data_create();
    profile_save_to_settings(manager->profiles[i], profile_data);
    obs_data_array_push_back(profiles_array, profile_data);
    obs_data_release(profile_data);
  }

  obs_data_set_array(settings, "output_profiles", profiles_array);
  obs_data_array_release(profiles_array);

  obs_log(LOG_INFO, "Saved %zu profiles to settings", manager->profile_count);
}

output_profile_t *profile_load_from_settings(obs_data_t *settings) {
  if (!settings) {
    return NULL;
  }

  output_profile_t *profile = bzalloc(sizeof(output_profile_t));

  /* Load basic properties */
  profile->profile_name = bstrdup(obs_data_get_string(settings, "name"));
  profile->profile_id = bstrdup(obs_data_get_string(settings, "id"));
  profile->source_orientation =
      (stream_orientation_t)obs_data_get_int(settings, "source_orientation");
  profile->auto_detect_orientation =
      obs_data_get_bool(settings, "auto_detect_orientation");
  profile->source_width = (uint32_t)obs_data_get_int(settings, "source_width");
  profile->source_height =
      (uint32_t)obs_data_get_int(settings, "source_height");

  /* Load input URL with default fallback */
  const char *input_url = obs_data_get_string(settings, "input_url");
  if (input_url && strlen(input_url) > 0) {
    profile->input_url = bstrdup(input_url);
  } else {
    profile->input_url = bstrdup("rtmp://localhost/live/obs_input");
  }

  profile->auto_start = obs_data_get_bool(settings, "auto_start");
  profile->auto_reconnect = obs_data_get_bool(settings, "auto_reconnect");
  profile->reconnect_delay_sec =
      (uint32_t)obs_data_get_int(settings, "reconnect_delay_sec");

  /* Load destinations */
  obs_data_array_t *dests_array = obs_data_get_array(settings, "destinations");
  if (dests_array) {
    size_t count = obs_data_array_count(dests_array);
    for (size_t i = 0; i < count; i++) {
      obs_data_t *dest_data = obs_data_array_item(dests_array, i);

      encoding_settings_t enc = profile_get_default_encoding();
      enc.width = (uint32_t)obs_data_get_int(dest_data, "width");
      enc.height = (uint32_t)obs_data_get_int(dest_data, "height");
      enc.bitrate = (uint32_t)obs_data_get_int(dest_data, "bitrate");
      enc.audio_bitrate =
          (uint32_t)obs_data_get_int(dest_data, "audio_bitrate");
      enc.audio_track = (uint32_t)obs_data_get_int(dest_data, "audio_track");

      profile_add_destination(
          profile, (streaming_service_t)obs_data_get_int(dest_data, "service"),
          obs_data_get_string(dest_data, "stream_key"),
          (stream_orientation_t)obs_data_get_int(dest_data,
                                                 "target_orientation"),
          &enc);

      profile->destinations[i].enabled =
          obs_data_get_bool(dest_data, "enabled");

      obs_data_release(dest_data);
    }

    obs_data_array_release(dests_array);
  }

  profile->status = PROFILE_STATUS_INACTIVE;

  return profile;
}

void profile_save_to_settings(output_profile_t *profile, obs_data_t *settings) {
  if (!profile || !settings) {
    return;
  }

  /* Save basic properties */
  obs_data_set_string(settings, "name", profile->profile_name);
  obs_data_set_string(settings, "id", profile->profile_id);
  obs_data_set_int(settings, "source_orientation", profile->source_orientation);
  obs_data_set_bool(settings, "auto_detect_orientation",
                    profile->auto_detect_orientation);
  obs_data_set_int(settings, "source_width", profile->source_width);
  obs_data_set_int(settings, "source_height", profile->source_height);
  obs_data_set_string(settings, "input_url",
                      profile->input_url ? profile->input_url : "");
  obs_data_set_bool(settings, "auto_start", profile->auto_start);
  obs_data_set_bool(settings, "auto_reconnect", profile->auto_reconnect);
  obs_data_set_int(settings, "reconnect_delay_sec",
                   profile->reconnect_delay_sec);

  /* Save destinations */
  obs_data_array_t *dests_array = obs_data_array_create();

  for (size_t i = 0; i < profile->destination_count; i++) {
    profile_destination_t *dest = &profile->destinations[i];
    obs_data_t *dest_data = obs_data_create();

    obs_data_set_int(dest_data, "service", dest->service);
    obs_data_set_string(dest_data, "stream_key", dest->stream_key);
    obs_data_set_int(dest_data, "target_orientation", dest->target_orientation);
    obs_data_set_bool(dest_data, "enabled", dest->enabled);

    /* Encoding settings */
    obs_data_set_int(dest_data, "width", dest->encoding.width);
    obs_data_set_int(dest_data, "height", dest->encoding.height);
    obs_data_set_int(dest_data, "bitrate", dest->encoding.bitrate);
    obs_data_set_int(dest_data, "audio_bitrate", dest->encoding.audio_bitrate);
    obs_data_set_int(dest_data, "audio_track", dest->encoding.audio_track);

    obs_data_array_push_back(dests_array, dest_data);
    obs_data_release(dest_data);
  }

  obs_data_set_array(settings, "destinations", dests_array);
  obs_data_array_release(dests_array);
}

output_profile_t *profile_duplicate(output_profile_t *source,
                                    const char *new_name) {
  if (!source || !new_name) {
    return NULL;
  }

  output_profile_t *duplicate = bzalloc(sizeof(output_profile_t));

  /* Copy basic properties */
  duplicate->profile_name = bstrdup(new_name);
  duplicate->profile_id = profile_generate_id();
  duplicate->source_orientation = source->source_orientation;
  duplicate->auto_detect_orientation = source->auto_detect_orientation;
  duplicate->source_width = source->source_width;
  duplicate->source_height = source->source_height;
  duplicate->auto_start = source->auto_start;
  duplicate->auto_reconnect = source->auto_reconnect;
  duplicate->reconnect_delay_sec = source->reconnect_delay_sec;
  duplicate->status = PROFILE_STATUS_INACTIVE;

  /* Copy destinations */
  for (size_t i = 0; i < source->destination_count; i++) {
    profile_add_destination(duplicate, source->destinations[i].service,
                            source->destinations[i].stream_key,
                            source->destinations[i].target_orientation,
                            &source->destinations[i].encoding);

    duplicate->destinations[i].enabled = source->destinations[i].enabled;
  }

  return duplicate;
}

bool profile_update_stats(output_profile_t *profile, restreamer_api_t *api) {
  if (!profile || !api || !profile->process_reference) {
    return false;
  }

  /* TODO: Query restreamer API for process stats and update destination stats
   */
  /* This will be implemented when we integrate with actual OBS outputs */

  return true;
}

/* ========================================================================
 * Health Monitoring & Auto-Recovery Implementation
 * ======================================================================== */

bool profile_check_health(output_profile_t *profile, restreamer_api_t *api) {
  if (!profile || !api) {
    return false;
  }

  /* Only check health if profile is active and monitoring enabled */
  if (profile->status != PROFILE_STATUS_ACTIVE ||
      !profile->health_monitoring_enabled) {
    return true;
  }

  if (!profile->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active profile '%s'",
            profile->profile_name);
    return false;
  }

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (!restreamer_api_get_processes(api, &list)) {
    obs_log(LOG_WARNING, "Failed to get process list for health check");
    return false;
  }

  for (size_t i = 0; i < list.count; i++) {
    if (list.processes[i].reference &&
        strcmp(list.processes[i].reference, profile->process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }
  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_WARNING, "Process not found during health check: %s",
            profile->process_reference);
    return false;
  }

  /* Get detailed process info */
  restreamer_process_t process = {0};
  bool got_info = restreamer_api_get_process(api, process_id, &process);

  if (!got_info) {
    obs_log(LOG_WARNING, "Failed to get process info for health check: %s",
            process_id);
    bfree(process_id);
    return false;
  }

  /* Get list of outputs for this process */
  char **output_ids = NULL;
  size_t output_count = 0;
  bool got_outputs = restreamer_api_get_process_outputs(
      api, process_id, &output_ids, &output_count);

  bfree(process_id);

  /* Update destination health based on process state */
  bool all_healthy = true;
  time_t current_time = time(NULL);

  for (size_t i = 0; i < profile->destination_count; i++) {
    profile_destination_t *dest = &profile->destinations[i];
    if (!dest->enabled) {
      continue;
    }

    /* Update last health check time */
    dest->last_health_check = current_time;

    /* Build expected output ID */
    struct dstr expected_id;
    dstr_init(&expected_id);
    dstr_printf(&expected_id, "%s_%zu", dest->service_name, i);

    /* Check if this destination is in the output list */
    bool dest_found = false;
    if (got_outputs && output_ids) {
      for (size_t j = 0; j < output_count; j++) {
        if (strcmp(output_ids[j], expected_id.array) == 0) {
          dest_found = true;
          break;
        }
      }
    }

    /* Check health based on process state and output presence */
    bool dest_healthy = false;
    if (strcmp(process.state, "running") == 0 && dest_found) {
      dest_healthy = true;
      dest->connected = true;
      dest->consecutive_failures = 0;
    } else {
      dest_healthy = false;
      dest->connected = false;
      dest->consecutive_failures++;
    }

    dstr_free(&expected_id);

    if (!dest_healthy) {
      all_healthy = false;
      obs_log(LOG_WARNING,
              "Destination %s in profile %s is unhealthy (failures: %u, "
              "process state: %s, output found: %s)",
              dest->service_name, profile->profile_name,
              dest->consecutive_failures, process.state,
              dest_found ? "yes" : "no");

      /* Check if we should attempt reconnection */
      if (dest->auto_reconnect_enabled &&
          dest->consecutive_failures >= profile->failure_threshold) {
        obs_log(LOG_INFO, "Attempting auto-reconnect for destination %s",
                dest->service_name);
        profile_reconnect_destination(profile, api, i);
      }
    }
  }

  /* Free output IDs */
  if (output_ids) {
    for (size_t i = 0; i < output_count; i++) {
      bfree(output_ids[i]);
    }
    bfree(output_ids);
  }

  /* Free process fields */
  bfree(process.id);
  bfree(process.reference);
  bfree(process.state);
  bfree(process.command);

  /* Check for failover opportunities if health monitoring enabled */
  if (profile->health_monitoring_enabled && !all_healthy) {
    profile_check_failover(profile, api);
  }

  return all_healthy;
}

bool profile_reconnect_destination(output_profile_t *profile,
                                   restreamer_api_t *api, size_t dest_index) {
  if (!profile || !api || dest_index >= profile->destination_count) {
    return false;
  }

  profile_destination_t *dest = &profile->destinations[dest_index];

  /* Check if profile is active */
  if (profile->status != PROFILE_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot reconnect destination: profile '%s' is not active",
            profile->profile_name);
    return false;
  }

  if (!profile->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active profile '%s'",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO,
          "Attempting to reconnect destination %s in profile %s (attempt %u)",
          dest->service_name, profile->profile_name,
          dest->consecutive_failures);

  /* Check if max reconnect attempts exceeded */
  if (profile->max_reconnect_attempts > 0 &&
      dest->consecutive_failures >= profile->max_reconnect_attempts) {
    obs_log(LOG_ERROR,
            "Max reconnect attempts (%u) exceeded for destination %s",
            profile->max_reconnect_attempts, dest->service_name);
    dest->enabled = false;
    return false;
  }

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", dest->service_name, dest_index);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (restreamer_api_get_processes(api, &list)) {
    for (size_t i = 0; i < list.count; i++) {
      if (list.processes[i].reference &&
          strcmp(list.processes[i].reference, profile->process_reference) ==
              0) {
        process_id = bstrdup(list.processes[i].id);
        found = true;
        break;
      }
    }
    restreamer_api_free_process_list(&list);
  }

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", profile->process_reference);
    dstr_free(&output_id);
    return false;
  }

  /* Try to remove the failed output first */
  restreamer_api_remove_process_output(api, process_id, output_id.array);

  /* Wait a moment before re-adding */
  os_sleep_ms(profile->reconnect_delay_sec * 1000);

  /* Build output URL */
  struct dstr output_url;
  dstr_init(&output_url);
  dstr_copy(&output_url, dest->rtmp_url);
  dstr_cat(&output_url, "/");
  dstr_cat(&output_url, dest->stream_key);

  /* Build video filter if needed */
  const char *video_filter = NULL;
  struct dstr filter_str;
  dstr_init(&filter_str);

  if (dest->target_orientation != ORIENTATION_AUTO &&
      dest->target_orientation != profile->source_orientation) {
    /* TODO: Build appropriate filter based on orientation */
    video_filter = filter_str.array;
  }

  /* Re-add the output */
  bool result = restreamer_api_add_process_output(
      api, process_id, output_id.array, output_url.array, video_filter);

  bfree(process_id);
  dstr_free(&output_id);
  dstr_free(&output_url);
  dstr_free(&filter_str);

  if (result) {
    dest->connected = true;
    dest->consecutive_failures = 0;
    obs_log(LOG_INFO, "Successfully reconnected destination %s in profile %s",
            dest->service_name, profile->profile_name);
  } else {
    obs_log(LOG_ERROR, "Failed to reconnect destination %s in profile %s",
            dest->service_name, profile->profile_name);
  }

  return result;
}

void profile_set_health_monitoring(output_profile_t *profile, bool enabled) {
  if (!profile) {
    return;
  }

  profile->health_monitoring_enabled = enabled;

  /* Set default values if enabling for first time */
  if (enabled && profile->health_check_interval_sec == 0) {
    profile->health_check_interval_sec = 30; /* Check every 30 seconds */
    profile->failure_threshold = 3;          /* Reconnect after 3 failures */
    profile->max_reconnect_attempts = 5;     /* Max 5 reconnect attempts */
  }

  /* Enable auto-reconnect for all destinations */
  for (size_t i = 0; i < profile->destination_count; i++) {
    profile->destinations[i].auto_reconnect_enabled = enabled;
  }

  obs_log(LOG_INFO, "Health monitoring %s for profile %s",
          enabled ? "enabled" : "disabled", profile->profile_name);
}

/* ========================================================================
 * Destination Templates/Presets Implementation
 * ======================================================================== */

static destination_template_t *
create_builtin_template(const char *name, const char *id,
                        streaming_service_t service,
                        stream_orientation_t orientation, uint32_t bitrate,
                        uint32_t width, uint32_t height) {
  destination_template_t *tmpl = bzalloc(sizeof(destination_template_t));

  tmpl->template_name = bstrdup(name);
  tmpl->template_id = bstrdup(id);
  tmpl->service = service;
  tmpl->orientation = orientation;
  tmpl->is_builtin = true;

  /* Set encoding settings */
  tmpl->encoding = profile_get_default_encoding();
  tmpl->encoding.bitrate = bitrate;
  tmpl->encoding.width = width;
  tmpl->encoding.height = height;
  tmpl->encoding.audio_bitrate = 128; /* Default audio bitrate */

  return tmpl;
}

void profile_manager_load_builtin_templates(profile_manager_t *manager) {
  if (!manager) {
    return;
  }

  obs_log(LOG_INFO, "Loading built-in destination templates");

  /* YouTube templates */
  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "YouTube 1080p60", "builtin_youtube_1080p60", SERVICE_YOUTUBE,
      ORIENTATION_HORIZONTAL, 6000, 1920, 1080);

  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "YouTube 720p60", "builtin_youtube_720p60", SERVICE_YOUTUBE,
      ORIENTATION_HORIZONTAL, 4500, 1280, 720);

  /* Twitch templates */
  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "Twitch 1080p60", "builtin_twitch_1080p60", SERVICE_TWITCH,
      ORIENTATION_HORIZONTAL, 6000, 1920, 1080);

  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "Twitch 720p60", "builtin_twitch_720p60", SERVICE_TWITCH,
      ORIENTATION_HORIZONTAL, 4500, 1280, 720);

  /* Facebook templates */
  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "Facebook 1080p", "builtin_facebook_1080p", SERVICE_FACEBOOK,
      ORIENTATION_HORIZONTAL, 4000, 1920, 1080);

  /* TikTok vertical template */
  manager->templates =
      brealloc(manager->templates, sizeof(destination_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = create_builtin_template(
      "TikTok Vertical", "builtin_tiktok_vertical", SERVICE_TIKTOK,
      ORIENTATION_VERTICAL, 3000, 1080, 1920);

  obs_log(LOG_INFO, "Loaded %zu built-in templates", manager->template_count);
}

destination_template_t *profile_manager_create_template(
    profile_manager_t *manager, const char *name, streaming_service_t service,
    stream_orientation_t orientation, encoding_settings_t *encoding) {
  if (!manager || !name || !encoding) {
    return NULL;
  }

  destination_template_t *tmpl = bzalloc(sizeof(destination_template_t));

  tmpl->template_name = bstrdup(name);
  tmpl->template_id = profile_generate_id(); /* Reuse ID generator */
  tmpl->service = service;
  tmpl->orientation = orientation;
  tmpl->encoding = *encoding;
  tmpl->is_builtin = false;

  /* Add to manager */
  size_t new_count = manager->template_count + 1;
  manager->templates = brealloc(manager->templates,
                                sizeof(destination_template_t *) * new_count);
  manager->templates[manager->template_count] = tmpl;
  manager->template_count = new_count;

  obs_log(LOG_INFO, "Created custom template: %s", name);

  return tmpl;
}

bool profile_manager_delete_template(profile_manager_t *manager,
                                     const char *template_id) {
  if (!manager || !template_id) {
    return false;
  }

  for (size_t i = 0; i < manager->template_count; i++) {
    destination_template_t *tmpl = manager->templates[i];
    if (strcmp(tmpl->template_id, template_id) == 0) {
      /* Don't allow deleting built-in templates */
      if (tmpl->is_builtin) {
        obs_log(LOG_WARNING, "Cannot delete built-in template: %s",
                tmpl->template_name);
        return false;
      }

      /* Free template */
      bfree(tmpl->template_name);
      bfree(tmpl->template_id);
      bfree(tmpl);

      /* Shift remaining templates */
      if (i < manager->template_count - 1) {
        memmove(&manager->templates[i], &manager->templates[i + 1],
                sizeof(destination_template_t *) *
                    (manager->template_count - i - 1));
      }

      manager->template_count--;

      if (manager->template_count == 0) {
        bfree(manager->templates);
        manager->templates = NULL;
      }

      obs_log(LOG_INFO, "Deleted template: %s", template_id);
      return true;
    }
  }

  return false;
}

destination_template_t *profile_manager_get_template(profile_manager_t *manager,
                                                     const char *template_id) {
  if (!manager || !template_id) {
    return NULL;
  }

  for (size_t i = 0; i < manager->template_count; i++) {
    if (strcmp(manager->templates[i]->template_id, template_id) == 0) {
      return manager->templates[i];
    }
  }

  return NULL;
}

destination_template_t *
profile_manager_get_template_at(profile_manager_t *manager, size_t index) {
  if (!manager || index >= manager->template_count) {
    return NULL;
  }

  return manager->templates[index];
}

bool profile_apply_template(output_profile_t *profile,
                            destination_template_t *tmpl,
                            const char *stream_key) {
  if (!profile || !tmpl || !stream_key) {
    return false;
  }

  /* Add destination using template settings */
  bool result = profile_add_destination(profile, tmpl->service, stream_key,
                                        tmpl->orientation, &tmpl->encoding);

  if (result) {
    obs_log(LOG_INFO, "Applied template '%s' to profile '%s' with stream key",
            tmpl->template_name, profile->profile_name);
  }

  return result;
}

void profile_manager_save_templates(profile_manager_t *manager,
                                    obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *templates_array = obs_data_array_create();

  /* Only save custom (non-builtin) templates */
  for (size_t i = 0; i < manager->template_count; i++) {
    destination_template_t *tmpl = manager->templates[i];
    if (tmpl->is_builtin) {
      continue;
    }

    obs_data_t *tmpl_data = obs_data_create();

    obs_data_set_string(tmpl_data, "name", tmpl->template_name);
    obs_data_set_string(tmpl_data, "id", tmpl->template_id);
    obs_data_set_int(tmpl_data, "service", tmpl->service);
    obs_data_set_int(tmpl_data, "orientation", tmpl->orientation);

    /* Encoding settings */
    obs_data_set_int(tmpl_data, "bitrate", tmpl->encoding.bitrate);
    obs_data_set_int(tmpl_data, "width", tmpl->encoding.width);
    obs_data_set_int(tmpl_data, "height", tmpl->encoding.height);
    obs_data_set_int(tmpl_data, "audio_bitrate", tmpl->encoding.audio_bitrate);

    obs_data_array_push_back(templates_array, tmpl_data);
    obs_data_release(tmpl_data);
  }

  obs_data_set_array(settings, "destination_templates", templates_array);
  obs_data_array_release(templates_array);

  obs_log(LOG_INFO, "Saved custom templates to settings");
}

void profile_manager_load_templates(profile_manager_t *manager,
                                    obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *templates_array =
      obs_data_get_array(settings, "destination_templates");
  if (!templates_array) {
    return;
  }

  size_t count = obs_data_array_count(templates_array);
  for (size_t i = 0; i < count; i++) {
    obs_data_t *tmpl_data = obs_data_array_item(templates_array, i);

    encoding_settings_t enc = profile_get_default_encoding();
    enc.bitrate = (uint32_t)obs_data_get_int(tmpl_data, "bitrate");
    enc.width = (uint32_t)obs_data_get_int(tmpl_data, "width");
    enc.height = (uint32_t)obs_data_get_int(tmpl_data, "height");
    enc.audio_bitrate = (uint32_t)obs_data_get_int(tmpl_data, "audio_bitrate");

    profile_manager_create_template(
        manager, obs_data_get_string(tmpl_data, "name"),
        (streaming_service_t)obs_data_get_int(tmpl_data, "service"),
        (stream_orientation_t)obs_data_get_int(tmpl_data, "orientation"), &enc);

    obs_data_release(tmpl_data);
  }

  obs_data_array_release(templates_array);

  obs_log(LOG_INFO, "Loaded %zu custom templates from settings", count);
}

/* ========================================================================
 * Backup/Failover Destination Support Implementation
 * ======================================================================== */

bool profile_set_destination_backup(output_profile_t *profile,
                                    size_t primary_index, size_t backup_index) {
  if (!profile || primary_index >= profile->destination_count ||
      backup_index >= profile->destination_count) {
    return false;
  }

  if (primary_index == backup_index) {
    obs_log(LOG_ERROR, "Cannot set destination as backup for itself");
    return false;
  }

  profile_destination_t *primary = &profile->destinations[primary_index];
  profile_destination_t *backup = &profile->destinations[backup_index];

  /* Check if primary already has a backup */
  if (primary->backup_index != (size_t)-1 &&
      primary->backup_index != backup_index) {
    obs_log(LOG_WARNING,
            "Primary destination %s already has a backup, replacing",
            primary->service_name);
    /* Clear old backup relationship */
    profile->destinations[primary->backup_index].is_backup = false;
    profile->destinations[primary->backup_index].primary_index = (size_t)-1;
  }

  /* Set backup relationship */
  primary->backup_index = backup_index;
  backup->is_backup = true;
  backup->primary_index = primary_index;
  backup->enabled = false; /* Backup starts disabled */

  obs_log(LOG_INFO, "Set %s as backup for %s in profile %s",
          backup->service_name, primary->service_name, profile->profile_name);

  return true;
}

bool profile_remove_destination_backup(output_profile_t *profile,
                                       size_t primary_index) {
  if (!profile || primary_index >= profile->destination_count) {
    return false;
  }

  profile_destination_t *primary = &profile->destinations[primary_index];

  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_WARNING, "Primary destination has no backup to remove");
    return false;
  }

  /* Clear backup relationship */
  profile_destination_t *backup = &profile->destinations[primary->backup_index];
  backup->is_backup = false;
  backup->primary_index = (size_t)-1;
  primary->backup_index = (size_t)-1;

  obs_log(LOG_INFO, "Removed backup relationship for %s in profile %s",
          primary->service_name, profile->profile_name);

  return true;
}

bool profile_trigger_failover(output_profile_t *profile, restreamer_api_t *api,
                              size_t primary_index) {
  if (!profile || !api || primary_index >= profile->destination_count) {
    return false;
  }

  profile_destination_t *primary = &profile->destinations[primary_index];

  /* Check if primary has a backup */
  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_ERROR, "Cannot failover: primary destination %s has no backup",
            primary->service_name);
    return false;
  }

  profile_destination_t *backup = &profile->destinations[primary->backup_index];

  /* Check if already failed over */
  if (primary->failover_active) {
    obs_log(LOG_WARNING, "Failover already active for %s",
            primary->service_name);
    return true;
  }

  obs_log(LOG_INFO, "Triggering failover from %s to %s in profile %s",
          primary->service_name, backup->service_name, profile->profile_name);

  /* Only failover if profile is active */
  if (profile->status == PROFILE_STATUS_ACTIVE) {
    /* Disable primary if it's running */
    if (primary->enabled) {
      bool removed = restreamer_multistream_enable_destination_live(
          api, NULL, primary_index, false);
      if (!removed) {
        obs_log(LOG_WARNING, "Failed to disable primary during failover");
      }
      primary->enabled = false;
    }

    /* Enable backup */
    bool added = restreamer_multistream_add_destination_live(
        api, NULL, backup->backup_index);
    if (!added) {
      obs_log(LOG_ERROR, "Failed to enable backup destination");
      return false;
    }
    backup->enabled = true;
  }

  /* Mark failover as active */
  primary->failover_active = true;
  backup->failover_active = true;
  primary->failover_start_time = time(NULL);
  backup->failover_start_time = time(NULL);

  obs_log(LOG_INFO, "Failover complete: %s -> %s", primary->service_name,
          backup->service_name);

  return true;
}

bool profile_restore_primary(output_profile_t *profile, restreamer_api_t *api,
                             size_t primary_index) {
  if (!profile || !api || primary_index >= profile->destination_count) {
    return false;
  }

  profile_destination_t *primary = &profile->destinations[primary_index];

  /* Check if primary has a backup */
  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_ERROR, "Primary destination has no backup");
    return false;
  }

  profile_destination_t *backup = &profile->destinations[primary->backup_index];

  /* Check if failover is active */
  if (!primary->failover_active) {
    obs_log(LOG_WARNING, "No active failover to restore from");
    return true;
  }

  obs_log(LOG_INFO,
          "Restoring primary destination %s from backup %s in profile %s",
          primary->service_name, backup->service_name, profile->profile_name);

  /* Only restore if profile is active */
  if (profile->status == PROFILE_STATUS_ACTIVE) {
    /* Re-enable primary */
    bool added =
        restreamer_multistream_add_destination_live(api, NULL, primary_index);
    if (!added) {
      obs_log(LOG_ERROR, "Failed to re-enable primary destination");
      return false;
    }
    primary->enabled = true;

    /* Disable backup */
    bool removed = restreamer_multistream_enable_destination_live(
        api, NULL, backup->backup_index, false);
    if (!removed) {
      obs_log(LOG_WARNING, "Failed to disable backup during restore");
    }
    backup->enabled = false;
  }

  /* Clear failover state */
  primary->failover_active = false;
  backup->failover_active = false;
  primary->consecutive_failures = 0;

  time_t duration = time(NULL) - primary->failover_start_time;
  obs_log(LOG_INFO, "Primary restored: %s (failover duration: %ld seconds)",
          primary->service_name, (long)duration);

  return true;
}

bool profile_check_failover(output_profile_t *profile, restreamer_api_t *api) {
  if (!profile || !api) {
    return false;
  }

  /* Only check failover if profile is active */
  if (profile->status != PROFILE_STATUS_ACTIVE) {
    return true;
  }

  bool any_failover = false;

  for (size_t i = 0; i < profile->destination_count; i++) {
    profile_destination_t *dest = &profile->destinations[i];

    /* Skip backup destinations */
    if (dest->is_backup) {
      continue;
    }

    /* Skip destinations without backups */
    if (dest->backup_index == (size_t)-1) {
      continue;
    }

    /* Check if primary is unhealthy and should failover */
    if (!dest->failover_active && !dest->connected &&
        dest->consecutive_failures >= profile->failure_threshold) {
      obs_log(LOG_WARNING,
              "Primary destination %s has failed %u times, triggering failover",
              dest->service_name, dest->consecutive_failures);

      if (profile_trigger_failover(profile, api, i)) {
        any_failover = true;
      }
    }

    /* Check if primary has recovered and should be restored */
    if (dest->failover_active && dest->connected &&
        dest->consecutive_failures == 0) {
      obs_log(LOG_INFO,
              "Primary destination %s has recovered, restoring from backup",
              dest->service_name);

      profile_restore_primary(profile, api, i);
    }
  }

  return any_failover;
}

/* ========================================================================
 * Bulk Destination Operations Implementation
 * ======================================================================== */

bool profile_bulk_enable_destinations(output_profile_t *profile,
                                      restreamer_api_t *api, size_t *indices,
                                      size_t count, bool enabled) {
  if (!profile || !indices || count == 0) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk %s %zu destinations in profile %s",
          enabled ? "enabling" : "disabling", count, profile->profile_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= profile->destination_count) {
      obs_log(LOG_WARNING, "Invalid destination index: %zu", idx);
      fail_count++;
      continue;
    }

    /* Skip backup destinations */
    if (profile->destinations[idx].is_backup) {
      obs_log(LOG_WARNING,
              "Cannot directly enable/disable backup destination %s",
              profile->destinations[idx].service_name);
      fail_count++;
      continue;
    }

    bool result = profile_set_destination_enabled(profile, idx, enabled);
    if (result) {
      success_count++;

      /* If profile is active, apply change live */
      if (profile->status == PROFILE_STATUS_ACTIVE && api) {
        restreamer_multistream_enable_destination_live(api, NULL, idx, enabled);
      }
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk enable/disable complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}

bool profile_bulk_delete_destinations(output_profile_t *profile,
                                      size_t *indices, size_t count) {
  if (!profile || !indices || count == 0) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk deleting %zu destinations from profile %s", count,
          profile->profile_name);

  /* Sort indices in descending order to avoid index shifts */
  for (size_t i = 0; i < count - 1; i++) {
    for (size_t j = i + 1; j < count; j++) {
      if (indices[i] < indices[j]) {
        size_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
      }
    }
  }

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= profile->destination_count) {
      obs_log(LOG_WARNING, "Invalid destination index: %zu", idx);
      fail_count++;
      continue;
    }

    /* Remove backup relationships before deleting */
    profile_destination_t *dest = &profile->destinations[idx];
    if (dest->backup_index != (size_t)-1) {
      profile_remove_destination_backup(profile, idx);
    }
    if (dest->is_backup) {
      profile_remove_destination_backup(profile, dest->primary_index);
    }

    bool result = profile_remove_destination(profile, idx);
    if (result) {
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk delete complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}

bool profile_bulk_update_encoding(output_profile_t *profile,
                                  restreamer_api_t *api, size_t *indices,
                                  size_t count, encoding_settings_t *encoding) {
  if (!profile || !indices || count == 0 || !encoding) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk updating encoding for %zu destinations in profile %s",
          count, profile->profile_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  bool is_active = (profile->status == PROFILE_STATUS_ACTIVE);

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= profile->destination_count) {
      obs_log(LOG_WARNING, "Invalid destination index: %zu", idx);
      fail_count++;
      continue;
    }

    bool result;
    if (is_active && api) {
      /* Update encoding live */
      result =
          profile_update_destination_encoding_live(profile, api, idx, encoding);
    } else {
      /* Update encoding settings only */
      result = profile_update_destination_encoding(profile, idx, encoding);
    }

    if (result) {
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk encoding update complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}

bool profile_bulk_start_destinations(output_profile_t *profile,
                                     restreamer_api_t *api, size_t *indices,
                                     size_t count) {
  if (!profile || !api || !indices || count == 0) {
    return false;
  }

  /* Only start if profile is active */
  if (profile->status != PROFILE_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot bulk start destinations: profile %s is not active",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO, "Bulk starting %zu destinations in profile %s", count,
          profile->profile_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= profile->destination_count) {
      obs_log(LOG_WARNING, "Invalid destination index: %zu", idx);
      fail_count++;
      continue;
    }

    profile_destination_t *dest = &profile->destinations[idx];

    /* Skip if already enabled */
    if (dest->enabled) {
      obs_log(LOG_DEBUG, "Destination %s already enabled", dest->service_name);
      success_count++;
      continue;
    }

    /* Skip backup destinations */
    if (dest->is_backup) {
      obs_log(LOG_WARNING, "Cannot directly start backup destination %s",
              dest->service_name);
      fail_count++;
      continue;
    }

    /* Add destination to active stream */
    bool result = restreamer_multistream_add_destination_live(api, NULL, idx);
    if (result) {
      dest->enabled = true;
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk start complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}

bool profile_bulk_stop_destinations(output_profile_t *profile,
                                    restreamer_api_t *api, size_t *indices,
                                    size_t count) {
  if (!profile || !api || !indices || count == 0) {
    return false;
  }

  /* Only stop if profile is active */
  if (profile->status != PROFILE_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot bulk stop destinations: profile %s is not active",
            profile->profile_name);
    return false;
  }

  obs_log(LOG_INFO, "Bulk stopping %zu destinations in profile %s", count,
          profile->profile_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= profile->destination_count) {
      obs_log(LOG_WARNING, "Invalid destination index: %zu", idx);
      fail_count++;
      continue;
    }

    profile_destination_t *dest = &profile->destinations[idx];

    /* Skip if already disabled */
    if (!dest->enabled) {
      obs_log(LOG_DEBUG, "Destination %s already disabled", dest->service_name);
      success_count++;
      continue;
    }

    /* Remove destination from active stream */
    bool result =
        restreamer_multistream_enable_destination_live(api, NULL, idx, false);
    if (result) {
      dest->enabled = false;
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk stop complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}
