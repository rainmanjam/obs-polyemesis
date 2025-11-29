#include "restreamer-channel.h"
#include <obs-module.h>
#include <plugin-support.h>
#include <time.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

/* Channel Manager Implementation */

channel_manager_t *channel_manager_create(restreamer_api_t *api) {
  channel_manager_t *manager = bzalloc(sizeof(channel_manager_t));
  manager->api = api;
  manager->channels = NULL;
  manager->channel_count = 0;
  manager->templates = NULL;
  manager->template_count = 0;

  /* Load built-in templates */
  channel_manager_load_builtin_templates(manager);

  obs_log(LOG_INFO, "Channel manager created");
  return manager;
}

void channel_manager_destroy(channel_manager_t *manager) {
  if (!manager) {
    return;
  }

  /* Stop and destroy all channels */
  for (size_t i = 0; i < manager->channel_count; i++) {
    stream_channel_t *channel = manager->channels[i];

    /* Stop if active */
    if (channel->status == CHANNEL_STATUS_ACTIVE) {
      channel_stop(manager, channel->channel_id);
    }

    /* Destroy outputs */
    for (size_t j = 0; j < channel->output_count; j++) {
      bfree(channel->outputs[j].service_name);
      bfree(channel->outputs[j].stream_key);
      bfree(channel->outputs[j].rtmp_url);
    }
    bfree(channel->outputs);

    /* Destroy channel */
    bfree(channel->channel_name);
    bfree(channel->channel_id);
    bfree(channel->last_error);
    bfree(channel->process_reference);
    bfree(channel->input_url);
    bfree(channel);
  }

  bfree(manager->channels);

  /* Destroy all templates */
  for (size_t i = 0; i < manager->template_count; i++) {
    output_template_t *tmpl = manager->templates[i];
    bfree(tmpl->template_name);
    bfree(tmpl->template_id);
    bfree(tmpl);
  }
  bfree(manager->templates);

  bfree(manager);

  obs_log(LOG_INFO, "Channel manager destroyed");
}

char *channel_generate_id(void) {
  struct dstr id = {0};
  dstr_init(&id);

  /* Use timestamp + random component */
  uint64_t timestamp = (uint64_t)time(NULL);
  uint32_t random = (uint32_t)rand();

  dstr_printf(&id, "channel_%llu_%u", (unsigned long long)timestamp, random);

  char *result = bstrdup(id.array);
  dstr_free(&id);

  return result;
}

stream_channel_t *channel_manager_create_channel(channel_manager_t *manager,
                                                 const char *name) {
  if (!manager || !name) {
    return NULL;
  }

  /* Allocate new channel */
  stream_channel_t *channel = bzalloc(sizeof(stream_channel_t));

  /* Set basic properties */
  channel->channel_name = bstrdup(name);
  channel->channel_id = channel_generate_id();
  channel->source_orientation = ORIENTATION_AUTO;
  channel->auto_detect_orientation = true;
  channel->status = CHANNEL_STATUS_INACTIVE;
  channel->auto_reconnect = true;
  channel->reconnect_delay_sec = 5;

  /* Set default input URL */
  channel->input_url = bstrdup("rtmp://localhost/live/obs_input");

  /* Add to manager */
  size_t new_count = manager->channel_count + 1;
  manager->channels =
      brealloc(manager->channels, sizeof(stream_channel_t *) * new_count);
  manager->channels[manager->channel_count] = channel;
  manager->channel_count = new_count;

  obs_log(LOG_INFO, "Created channel: %s (ID: %s)", name, channel->channel_id);

  return channel;
}

bool channel_manager_delete_channel(channel_manager_t *manager,
                                    const char *channel_id) {
  if (!manager || !channel_id) {
    return false;
  }

  /* Find channel */
  for (size_t i = 0; i < manager->channel_count; i++) {
    stream_channel_t *channel = manager->channels[i];
    if (strcmp(channel->channel_id, channel_id) == 0) {
      /* Stop if active */
      if (channel->status == CHANNEL_STATUS_ACTIVE) {
        channel_stop(manager, channel_id);
      }

      /* Free outputs */
      for (size_t j = 0; j < channel->output_count; j++) {
        bfree(channel->outputs[j].service_name);
        bfree(channel->outputs[j].stream_key);
        bfree(channel->outputs[j].rtmp_url);
      }
      bfree(channel->outputs);

      /* Free channel */
      bfree(channel->channel_name);
      bfree(channel->channel_id);
      bfree(channel->last_error);
      bfree(channel->process_reference);
      bfree(channel);

      /* Shift remaining channels */
      if (i < manager->channel_count - 1) {
        memmove(&manager->channels[i], &manager->channels[i + 1],
                sizeof(stream_channel_t *) * (manager->channel_count - i - 1));
      }

      manager->channel_count--;

      if (manager->channel_count == 0) {
        bfree(manager->channels);
        manager->channels = NULL;
      }

      obs_log(LOG_INFO, "Deleted channel: %s", channel_id);
      return true;
    }
  }

  return false;
}

stream_channel_t *channel_manager_get_channel(channel_manager_t *manager,
                                              const char *channel_id) {
  if (!manager || !channel_id) {
    return NULL;
  }

  for (size_t i = 0; i < manager->channel_count; i++) {
    if (strcmp(manager->channels[i]->channel_id, channel_id) == 0) {
      return manager->channels[i];
    }
  }

  return NULL;
}

stream_channel_t *channel_manager_get_channel_at(channel_manager_t *manager,
                                                 size_t index) {
  if (!manager || index >= manager->channel_count) {
    return NULL;
  }

  return manager->channels[index];
}

size_t channel_manager_get_count(channel_manager_t *manager) {
  return manager ? manager->channel_count : 0;
}

/* Channel Operations */

encoding_settings_t channel_get_default_encoding(void) {
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

bool channel_add_output(stream_channel_t *channel,
                        streaming_service_t service,
                        const char *stream_key,
                        stream_orientation_t target_orientation,
                        encoding_settings_t *encoding) {
  if (!channel || !stream_key) {
    return false;
  }

  /* Expand outputs array */
  size_t new_count = channel->output_count + 1;
  channel->outputs = brealloc(channel->outputs,
                              sizeof(channel_output_t) * new_count);

  channel_output_t *output =
      &channel->outputs[channel->output_count];
  memset(output, 0, sizeof(channel_output_t));

  /* Set basic properties */
  output->service = service;
  output->service_name =
      bstrdup(restreamer_multistream_get_service_name(service));
  output->stream_key = bstrdup(stream_key);
  output->rtmp_url = bstrdup(
      restreamer_multistream_get_service_url(service, target_orientation));
  output->target_orientation = target_orientation;
  output->enabled = true;

  /* Set encoding settings */
  if (encoding) {
    output->encoding = *encoding;
  } else {
    output->encoding = channel_get_default_encoding();
  }

  /* Initialize backup/failover fields */
  output->is_backup = false;
  output->primary_index = (size_t)-1;
  output->backup_index = (size_t)-1;
  output->failover_active = false;
  output->failover_start_time = 0;

  channel->output_count = new_count;

  obs_log(LOG_INFO, "Added output %s to channel %s", output->service_name,
          channel->channel_name);

  return true;
}

bool channel_remove_output(stream_channel_t *channel, size_t index) {
  if (!channel || index >= channel->output_count) {
    return false;
  }

  /* Free output */
  bfree(channel->outputs[index].service_name);
  bfree(channel->outputs[index].stream_key);
  bfree(channel->outputs[index].rtmp_url);

  /* Shift remaining outputs */
  if (index < channel->output_count - 1) {
    memmove(&channel->outputs[index], &channel->outputs[index + 1],
            sizeof(channel_output_t) *
                (channel->output_count - index - 1));
  }

  channel->output_count--;

  if (channel->output_count == 0) {
    bfree(channel->outputs);
    channel->outputs = NULL;
  }

  return true;
}

bool channel_update_output_encoding(stream_channel_t *channel,
                                    size_t index,
                                    encoding_settings_t *encoding) {
  if (!channel || !encoding || index >= channel->output_count) {
    return false;
  }

  channel->outputs[index].encoding = *encoding;
  return true;
}

bool channel_update_output_encoding_live(stream_channel_t *channel,
                                         restreamer_api_t *api,
                                         size_t index,
                                         encoding_settings_t *encoding) {
  if (!channel || !api || !encoding || index >= channel->output_count) {
    return false;
  }

  /* Check if channel is active */
  if (channel->status != CHANNEL_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot update encoding live: channel '%s' is not active",
            channel->channel_name);
    return false;
  }

  if (!channel->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active channel '%s'",
            channel->channel_name);
    return false;
  }

  channel_output_t *output = &channel->outputs[index];

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", output->service_name, index);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (restreamer_api_get_processes(api, &list)) {
    for (size_t i = 0; i < list.count; i++) {
      if (list.processes[i].reference &&
          strcmp(list.processes[i].reference, channel->process_reference) ==
              0) {
        process_id = bstrdup(list.processes[i].id);
        found = true;
        break;
      }
    }
    restreamer_api_free_process_list(&list);
  }

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", channel->process_reference);
    dstr_free(&output_id);
    return false;
  }

  /* Convert channel encoding settings to API encoding params */
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
    output->encoding = *encoding;
    obs_log(LOG_INFO,
            "Successfully updated encoding for output %s in channel %s",
            output->service_name, channel->channel_name);
  }

  return result;
}

bool channel_set_output_enabled(stream_channel_t *channel, size_t index,
                                bool enabled) {
  if (!channel || index >= channel->output_count) {
    return false;
  }

  channel->outputs[index].enabled = enabled;
  return true;
}

/* Streaming Control */

bool channel_start(channel_manager_t *manager, const char *channel_id) {
  if (!manager || !channel_id) {
    return false;
  }

  stream_channel_t *channel = channel_manager_get_channel(manager, channel_id);
  if (!channel) {
    obs_log(LOG_ERROR, "Channel not found: %s", channel_id);
    return false;
  }

  if (channel->status == CHANNEL_STATUS_ACTIVE) {
    obs_log(LOG_WARNING, "Channel already active: %s", channel->channel_name);
    return true;
  }

  /* Count enabled outputs */
  size_t enabled_count = 0;
  for (size_t i = 0; i < channel->output_count; i++) {
    if (channel->outputs[i].enabled) {
      enabled_count++;
    }
  }

  if (enabled_count == 0) {
    obs_log(LOG_ERROR, "No enabled outputs in channel: %s",
            channel->channel_name);
    bfree(channel->last_error);
    channel->last_error = bstrdup("No enabled outputs configured");
    channel->status = CHANNEL_STATUS_ERROR;
    return false;
  }

  channel->status = CHANNEL_STATUS_STARTING;

  /* Check if API is available */
  if (!manager->api) {
    obs_log(LOG_ERROR, "No Restreamer API connection available for channel: %s",
            channel->channel_name);
    bfree(channel->last_error);
    channel->last_error = bstrdup("No Restreamer API connection");
    channel->status = CHANNEL_STATUS_ERROR;
    return false;
  }

  /* Create temporary multistream config from channel outputs */
  multistream_config_t *config = restreamer_multistream_create();
  if (!config) {
    obs_log(LOG_ERROR, "Failed to create multistream config");
    channel->status = CHANNEL_STATUS_ERROR;
    return false;
  }

  /* Set source orientation */
  config->source_orientation = channel->source_orientation;
  config->auto_detect_orientation = false;

  /* Set process reference to channel ID for tracking */
  config->process_reference = bstrdup(channel->channel_id);

  /* Copy enabled outputs */
  for (size_t i = 0; i < channel->output_count; i++) {
    channel_output_t *output = &channel->outputs[i];
    if (!output->enabled) {
      continue;
    }

    /* Add output to multistream config */
    if (!restreamer_multistream_add_destination(config, output->service,
                                                output->stream_key,
                                                output->target_orientation)) {
      obs_log(LOG_WARNING, "Failed to add output %s to channel %s",
              output->service_name, channel->channel_name);
    }
  }

  /* Use configured input URL */
  const char *input_url = channel->input_url;
  if (!input_url || strlen(input_url) == 0) {
    obs_log(LOG_ERROR, "No input URL configured for channel: %s",
            channel->channel_name);
    bfree(channel->last_error);
    channel->last_error = bstrdup("No input URL configured");
    restreamer_multistream_destroy(config);
    channel->status = CHANNEL_STATUS_ERROR;
    return false;
  }

  obs_log(LOG_INFO, "Starting channel: %s with %zu outputs (input: %s)",
          channel->channel_name, enabled_count, input_url);

  /* Start multistream */
  if (!restreamer_multistream_start(manager->api, config, input_url)) {
    obs_log(LOG_ERROR, "Failed to start multistream for channel: %s",
            channel->channel_name);
    bfree(channel->last_error);
    channel->last_error = bstrdup(restreamer_api_get_error(manager->api));
    restreamer_multistream_destroy(config);
    channel->status = CHANNEL_STATUS_ERROR;
    return false;
  }

  /* Store process reference for stopping later */
  bfree(channel->process_reference);
  channel->process_reference = bstrdup(config->process_reference);

  /* Clean up temporary config */
  restreamer_multistream_destroy(config);

  /* Clear last_error on successful start */
  bfree(channel->last_error);
  channel->last_error = NULL;

  channel->status = CHANNEL_STATUS_ACTIVE;
  obs_log(LOG_INFO,
          "Channel %s started successfully with process reference: %s",
          channel->channel_name, channel->process_reference);

  return true;
}

bool channel_stop(channel_manager_t *manager, const char *channel_id) {
  if (!manager || !channel_id) {
    return false;
  }

  stream_channel_t *channel = channel_manager_get_channel(manager, channel_id);
  if (!channel) {
    return false;
  }

  if (channel->status == CHANNEL_STATUS_INACTIVE) {
    return true;
  }

  channel->status = CHANNEL_STATUS_STOPPING;

  /* Stop the Restreamer process if we have a reference */
  if (channel->process_reference && manager->api) {
    obs_log(LOG_INFO,
            "Stopping Restreamer process for channel: %s (reference: %s)",
            channel->channel_name, channel->process_reference);

    if (!restreamer_multistream_stop(manager->api,
                                     channel->process_reference)) {
      obs_log(LOG_WARNING,
              "Failed to stop Restreamer process for channel: %s: %s",
              channel->channel_name, restreamer_api_get_error(manager->api));
      /* Continue anyway to update status */
    }

    /* Clear process reference */
    bfree(channel->process_reference);
    channel->process_reference = NULL;
  }

  obs_log(LOG_INFO, "Stopped channel: %s", channel->channel_name);

  /* Clear last_error on successful stop */
  bfree(channel->last_error);
  channel->last_error = NULL;

  channel->status = CHANNEL_STATUS_INACTIVE;
  return true;
}

bool channel_restart(channel_manager_t *manager, const char *channel_id) {
  channel_stop(manager, channel_id);
  return channel_start(manager, channel_id);
}

bool channel_manager_start_all(channel_manager_t *manager) {
  if (!manager) {
    return false;
  }

  obs_log(LOG_INFO, "Starting all channels (%zu total)",
          manager->channel_count);

  bool all_success = true;
  for (size_t i = 0; i < manager->channel_count; i++) {
    stream_channel_t *channel = manager->channels[i];
    if (channel->auto_start) {
      if (!channel_start(manager, channel->channel_id)) {
        all_success = false;
      }
    }
  }

  return all_success;
}

bool channel_manager_stop_all(channel_manager_t *manager) {
  if (!manager) {
    return false;
  }

  obs_log(LOG_INFO, "Stopping all channels");

  bool all_success = true;
  for (size_t i = 0; i < manager->channel_count; i++) {
    if (!channel_stop(manager, manager->channels[i]->channel_id)) {
      all_success = false;
    }
  }

  return all_success;
}

size_t channel_manager_get_active_count(channel_manager_t *manager) {
  if (!manager) {
    return 0;
  }

  size_t active_count = 0;
  for (size_t i = 0; i < manager->channel_count; i++) {
    if (manager->channels[i]->status == CHANNEL_STATUS_ACTIVE) {
      active_count++;
    }
  }

  return active_count;
}

/* ========================================================================
 * Preview/Test Mode Implementation
 * ======================================================================== */

bool channel_start_preview(channel_manager_t *manager,
                           const char *channel_id,
                           uint32_t duration_sec) {
  if (!manager || !channel_id) {
    return false;
  }

  stream_channel_t *channel = channel_manager_get_channel(manager, channel_id);
  if (!channel) {
    obs_log(LOG_ERROR, "Channel not found: %s", channel_id);
    return false;
  }

  if (channel->status != CHANNEL_STATUS_INACTIVE) {
    obs_log(LOG_WARNING, "Channel '%s' is not inactive, cannot start preview",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO, "Starting preview mode for channel: %s (duration: %u sec)",
          channel->channel_name, duration_sec);

  /* Enable preview mode */
  channel->preview_mode_enabled = true;
  channel->preview_duration_sec = duration_sec;
  channel->preview_start_time = time(NULL);

  /* Start the channel normally */
  if (!channel_start(manager, channel_id)) {
    channel->preview_mode_enabled = false;
    channel->preview_duration_sec = 0;
    channel->preview_start_time = 0;
    return false;
  }

  /* Update status to preview */
  channel->status = CHANNEL_STATUS_PREVIEW;

  obs_log(LOG_INFO, "Preview mode started successfully for channel: %s",
          channel->channel_name);

  return true;
}

bool channel_preview_to_live(channel_manager_t *manager,
                              const char *channel_id) {
  if (!manager || !channel_id) {
    return false;
  }

  stream_channel_t *channel = channel_manager_get_channel(manager, channel_id);
  if (!channel) {
    obs_log(LOG_ERROR, "Channel not found: %s", channel_id);
    return false;
  }

  if (channel->status != CHANNEL_STATUS_PREVIEW) {
    obs_log(LOG_WARNING, "Channel '%s' is not in preview mode, cannot go live",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO, "Converting preview to live for channel: %s",
          channel->channel_name);

  /* Disable preview mode */
  channel->preview_mode_enabled = false;
  channel->preview_duration_sec = 0;
  channel->preview_start_time = 0;

  /* Update status to active */
  /* Clear last_error on successful preview to live transition */
  bfree(channel->last_error);
  channel->last_error = NULL;

  channel->status = CHANNEL_STATUS_ACTIVE;

  obs_log(LOG_INFO, "Channel %s is now live", channel->channel_name);

  return true;
}

bool channel_cancel_preview(channel_manager_t *manager,
                             const char *channel_id) {
  if (!manager || !channel_id) {
    return false;
  }

  stream_channel_t *channel = channel_manager_get_channel(manager, channel_id);
  if (!channel) {
    obs_log(LOG_ERROR, "Channel not found: %s", channel_id);
    return false;
  }

  if (channel->status != CHANNEL_STATUS_PREVIEW) {
    obs_log(LOG_WARNING, "Channel '%s' is not in preview mode, cannot cancel",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO, "Canceling preview mode for channel: %s",
          channel->channel_name);

  /* Disable preview mode */
  channel->preview_mode_enabled = false;
  channel->preview_duration_sec = 0;
  channel->preview_start_time = 0;

  /* Stop the channel */
  bool result = channel_stop(manager, channel_id);

  obs_log(LOG_INFO, "Preview mode canceled for channel: %s",
          channel->channel_name);

  return result;
}

bool channel_check_preview_timeout(stream_channel_t *channel) {
  if (!channel || !channel->preview_mode_enabled) {
    return false;
  }

  /* If duration is 0, preview mode is unlimited */
  if (channel->preview_duration_sec == 0) {
    return false;
  }

  /* Check if preview time has elapsed */
  time_t current_time = time(NULL);
  time_t elapsed = current_time - channel->preview_start_time;

  if (elapsed >= (time_t)channel->preview_duration_sec) {
    obs_log(LOG_INFO,
            "Preview timeout reached for channel: %s (elapsed: %ld sec)",
            channel->channel_name, (long)elapsed);
    return true;
  }

  return false;
}

/* Configuration Persistence */

void channel_manager_load_from_settings(channel_manager_t *manager,
                                        obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *channels_array =
      obs_data_get_array(settings, "stream_channels");
  if (!channels_array) {
    return;
  }

  size_t count = obs_data_array_count(channels_array);
  for (size_t i = 0; i < count; i++) {
    obs_data_t *channel_data = obs_data_array_item(channels_array, i);
    stream_channel_t *channel = channel_load_from_settings(channel_data);

    if (channel) {
      /* Add to manager */
      size_t new_count = manager->channel_count + 1;
      manager->channels =
          brealloc(manager->channels, sizeof(stream_channel_t *) * new_count);
      manager->channels[manager->channel_count] = channel;
      manager->channel_count = new_count;
    }

    obs_data_release(channel_data);
  }

  obs_data_array_release(channels_array);

  obs_log(LOG_INFO, "Loaded %zu channels from settings", count);
}

void channel_manager_save_to_settings(channel_manager_t *manager,
                                      obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *channels_array = obs_data_array_create();

  for (size_t i = 0; i < manager->channel_count; i++) {
    obs_data_t *channel_data = obs_data_create();
    channel_save_to_settings(manager->channels[i], channel_data);
    obs_data_array_push_back(channels_array, channel_data);
    obs_data_release(channel_data);
  }

  obs_data_set_array(settings, "stream_channels", channels_array);
  obs_data_array_release(channels_array);

  obs_log(LOG_INFO, "Saved %zu channels to settings", manager->channel_count);
}

stream_channel_t *channel_load_from_settings(obs_data_t *settings) {
  if (!settings) {
    return NULL;
  }

  stream_channel_t *channel = bzalloc(sizeof(stream_channel_t));

  /* Load basic properties */
  channel->channel_name = bstrdup(obs_data_get_string(settings, "name"));
  channel->channel_id = bstrdup(obs_data_get_string(settings, "id"));
  channel->source_orientation =
      (stream_orientation_t)obs_data_get_int(settings, "source_orientation");
  channel->auto_detect_orientation =
      obs_data_get_bool(settings, "auto_detect_orientation");
  channel->source_width = (uint32_t)obs_data_get_int(settings, "source_width");
  channel->source_height =
      (uint32_t)obs_data_get_int(settings, "source_height");

  /* Load input URL with default fallback */
  const char *input_url = obs_data_get_string(settings, "input_url");
  if (input_url && strlen(input_url) > 0) {
    channel->input_url = bstrdup(input_url);
  } else {
    channel->input_url = bstrdup("rtmp://localhost/live/obs_input");
  }

  channel->auto_start = obs_data_get_bool(settings, "auto_start");
  channel->auto_reconnect = obs_data_get_bool(settings, "auto_reconnect");
  channel->reconnect_delay_sec =
      (uint32_t)obs_data_get_int(settings, "reconnect_delay_sec");

  /* Load outputs */
  obs_data_array_t *outputs_array = obs_data_get_array(settings, "outputs");
  if (outputs_array) {
    size_t count = obs_data_array_count(outputs_array);
    for (size_t i = 0; i < count; i++) {
      obs_data_t *output_data = obs_data_array_item(outputs_array, i);

      encoding_settings_t enc = channel_get_default_encoding();
      enc.width = (uint32_t)obs_data_get_int(output_data, "width");
      enc.height = (uint32_t)obs_data_get_int(output_data, "height");
      enc.bitrate = (uint32_t)obs_data_get_int(output_data, "bitrate");
      enc.audio_bitrate =
          (uint32_t)obs_data_get_int(output_data, "audio_bitrate");
      enc.audio_track = (uint32_t)obs_data_get_int(output_data, "audio_track");

      channel_add_output(
          channel, (streaming_service_t)obs_data_get_int(output_data, "service"),
          obs_data_get_string(output_data, "stream_key"),
          (stream_orientation_t)obs_data_get_int(output_data,
                                                 "target_orientation"),
          &enc);

      channel->outputs[i].enabled =
          obs_data_get_bool(output_data, "enabled");

      obs_data_release(output_data);
    }

    obs_data_array_release(outputs_array);
  }

  channel->status = CHANNEL_STATUS_INACTIVE;

  return channel;
}

void channel_save_to_settings(stream_channel_t *channel, obs_data_t *settings) {
  if (!channel || !settings) {
    return;
  }

  /* Save basic properties */
  obs_data_set_string(settings, "name", channel->channel_name);
  obs_data_set_string(settings, "id", channel->channel_id);
  obs_data_set_int(settings, "source_orientation", channel->source_orientation);
  obs_data_set_bool(settings, "auto_detect_orientation",
                    channel->auto_detect_orientation);
  obs_data_set_int(settings, "source_width", channel->source_width);
  obs_data_set_int(settings, "source_height", channel->source_height);
  obs_data_set_string(settings, "input_url",
                      channel->input_url ? channel->input_url : "");
  obs_data_set_bool(settings, "auto_start", channel->auto_start);
  obs_data_set_bool(settings, "auto_reconnect", channel->auto_reconnect);
  obs_data_set_int(settings, "reconnect_delay_sec",
                   channel->reconnect_delay_sec);

  /* Save outputs */
  obs_data_array_t *outputs_array = obs_data_array_create();

  for (size_t i = 0; i < channel->output_count; i++) {
    channel_output_t *output = &channel->outputs[i];
    obs_data_t *output_data = obs_data_create();

    obs_data_set_int(output_data, "service", output->service);
    obs_data_set_string(output_data, "stream_key", output->stream_key);
    obs_data_set_int(output_data, "target_orientation", output->target_orientation);
    obs_data_set_bool(output_data, "enabled", output->enabled);

    /* Encoding settings */
    obs_data_set_int(output_data, "width", output->encoding.width);
    obs_data_set_int(output_data, "height", output->encoding.height);
    obs_data_set_int(output_data, "bitrate", output->encoding.bitrate);
    obs_data_set_int(output_data, "audio_bitrate", output->encoding.audio_bitrate);
    obs_data_set_int(output_data, "audio_track", output->encoding.audio_track);

    obs_data_array_push_back(outputs_array, output_data);
    obs_data_release(output_data);
  }

  obs_data_set_array(settings, "outputs", outputs_array);
  obs_data_array_release(outputs_array);
}

stream_channel_t *channel_duplicate(stream_channel_t *source,
                                    const char *new_name) {
  if (!source || !new_name) {
    return NULL;
  }

  stream_channel_t *duplicate = bzalloc(sizeof(stream_channel_t));

  /* Copy basic properties */
  duplicate->channel_name = bstrdup(new_name);
  duplicate->channel_id = channel_generate_id();
  duplicate->source_orientation = source->source_orientation;
  duplicate->auto_detect_orientation = source->auto_detect_orientation;
  duplicate->source_width = source->source_width;
  duplicate->source_height = source->source_height;
  duplicate->auto_start = source->auto_start;
  duplicate->auto_reconnect = source->auto_reconnect;
  duplicate->reconnect_delay_sec = source->reconnect_delay_sec;
  duplicate->status = CHANNEL_STATUS_INACTIVE;

  /* Copy outputs */
  for (size_t i = 0; i < source->output_count; i++) {
    channel_add_output(duplicate, source->outputs[i].service,
                       source->outputs[i].stream_key,
                       source->outputs[i].target_orientation,
                       &source->outputs[i].encoding);

    duplicate->outputs[i].enabled = source->outputs[i].enabled;
  }

  return duplicate;
}

bool channel_update_stats(stream_channel_t *channel, restreamer_api_t *api) {
  if (!channel || !api || !channel->process_reference) {
    return false;
  }

  /* TODO: Query restreamer API for process stats and update output stats
   */
  /* This will be implemented when we integrate with actual OBS outputs */

  return true;
}

/* ========================================================================
 * Health Monitoring & Auto-Recovery Implementation
 * ======================================================================== */

bool channel_check_health(stream_channel_t *channel, restreamer_api_t *api) {
  if (!channel || !api) {
    return false;
  }

  /* Only check health if channel is active and monitoring enabled */
  if (channel->status != CHANNEL_STATUS_ACTIVE ||
      !channel->health_monitoring_enabled) {
    return true;
  }

  if (!channel->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active channel '%s'",
            channel->channel_name);
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
        strcmp(list.processes[i].reference, channel->process_reference) == 0) {
      process_id = bstrdup(list.processes[i].id);
      found = true;
      break;
    }
  }
  restreamer_api_free_process_list(&list);

  if (!found) {
    obs_log(LOG_WARNING, "Process not found during health check: %s",
            channel->process_reference);
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

  /* Update output health based on process state */
  bool all_healthy = true;
  time_t current_time = time(NULL);

  for (size_t i = 0; i < channel->output_count; i++) {
    channel_output_t *output = &channel->outputs[i];
    if (!output->enabled) {
      continue;
    }

    /* Update last health check time */
    output->last_health_check = current_time;

    /* Build expected output ID */
    struct dstr expected_id;
    dstr_init(&expected_id);
    dstr_printf(&expected_id, "%s_%zu", output->service_name, i);

    /* Check if this output is in the output list */
    bool output_found = false;
    if (got_outputs && output_ids) {
      for (size_t j = 0; j < output_count; j++) {
        if (strcmp(output_ids[j], expected_id.array) == 0) {
          output_found = true;
          break;
        }
      }
    }

    /* Check health based on process state and output presence */
    bool output_healthy = false;
    if (strcmp(process.state, "running") == 0 && output_found) {
      output_healthy = true;
      output->connected = true;
      output->consecutive_failures = 0;
    } else {
      output_healthy = false;
      output->connected = false;
      output->consecutive_failures++;
    }

    dstr_free(&expected_id);

    if (!output_healthy) {
      all_healthy = false;
      obs_log(LOG_WARNING,
              "Output %s in channel %s is unhealthy (failures: %u, "
              "process state: %s, output found: %s)",
              output->service_name, channel->channel_name,
              output->consecutive_failures, process.state,
              output_found ? "yes" : "no");

      /* Check if we should attempt reconnection */
      if (output->auto_reconnect_enabled &&
          output->consecutive_failures >= channel->failure_threshold) {
        obs_log(LOG_INFO, "Attempting auto-reconnect for output %s",
                output->service_name);
        channel_reconnect_output(channel, api, i);
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
  if (channel->health_monitoring_enabled && !all_healthy) {
    channel_check_failover(channel, api);
  }

  return all_healthy;
}

bool channel_reconnect_output(stream_channel_t *channel,
                               restreamer_api_t *api, size_t output_index) {
  if (!channel || !api || output_index >= channel->output_count) {
    return false;
  }

  channel_output_t *output = &channel->outputs[output_index];

  /* Check if channel is active */
  if (channel->status != CHANNEL_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot reconnect output: channel '%s' is not active",
            channel->channel_name);
    return false;
  }

  if (!channel->process_reference) {
    obs_log(LOG_ERROR, "No process reference for active channel '%s'",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO,
          "Attempting to reconnect output %s in channel %s (attempt %u)",
          output->service_name, channel->channel_name,
          output->consecutive_failures);

  /* Check if max reconnect attempts exceeded */
  if (channel->max_reconnect_attempts > 0 &&
      output->consecutive_failures >= channel->max_reconnect_attempts) {
    obs_log(LOG_ERROR,
            "Max reconnect attempts (%u) exceeded for output %s",
            channel->max_reconnect_attempts, output->service_name);
    output->enabled = false;
    return false;
  }

  /* Build output ID */
  struct dstr output_id;
  dstr_init(&output_id);
  dstr_printf(&output_id, "%s_%zu", output->service_name, output_index);

  /* Find process ID from reference */
  restreamer_process_list_t list = {0};
  bool found = false;
  char *process_id = NULL;

  if (restreamer_api_get_processes(api, &list)) {
    for (size_t i = 0; i < list.count; i++) {
      if (list.processes[i].reference &&
          strcmp(list.processes[i].reference, channel->process_reference) ==
              0) {
        process_id = bstrdup(list.processes[i].id);
        found = true;
        break;
      }
    }
    restreamer_api_free_process_list(&list);
  }

  if (!found) {
    obs_log(LOG_ERROR, "Process not found: %s", channel->process_reference);
    dstr_free(&output_id);
    return false;
  }

  /* Try to remove the failed output first */
  restreamer_api_remove_process_output(api, process_id, output_id.array);

  /* Wait a moment before re-adding */
  os_sleep_ms(channel->reconnect_delay_sec * 1000);

  /* Build output URL */
  struct dstr output_url;
  dstr_init(&output_url);
  dstr_copy(&output_url, output->rtmp_url);
  dstr_cat(&output_url, "/");
  dstr_cat(&output_url, output->stream_key);

  /* Build video filter if needed */
  const char *video_filter = NULL;
  struct dstr filter_str;
  dstr_init(&filter_str);

  if (output->target_orientation != ORIENTATION_AUTO &&
      output->target_orientation != channel->source_orientation) {
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
    output->connected = true;
    output->consecutive_failures = 0;
    obs_log(LOG_INFO, "Successfully reconnected output %s in channel %s",
            output->service_name, channel->channel_name);
  } else {
    obs_log(LOG_ERROR, "Failed to reconnect output %s in channel %s",
            output->service_name, channel->channel_name);
  }

  return result;
}

void channel_set_health_monitoring(stream_channel_t *channel, bool enabled) {
  if (!channel) {
    return;
  }

  channel->health_monitoring_enabled = enabled;

  /* Set default values if enabling for first time */
  if (enabled && channel->health_check_interval_sec == 0) {
    channel->health_check_interval_sec = 30; /* Check every 30 seconds */
    channel->failure_threshold = 3;          /* Reconnect after 3 failures */
    channel->max_reconnect_attempts = 5;     /* Max 5 reconnect attempts */
  }

  /* Enable auto-reconnect for all outputs */
  for (size_t i = 0; i < channel->output_count; i++) {
    channel->outputs[i].auto_reconnect_enabled = enabled;
  }

  obs_log(LOG_INFO, "Health monitoring %s for channel %s",
          enabled ? "enabled" : "disabled", channel->channel_name);
}

/* ========================================================================
 * Output Templates/Presets Implementation
 * ======================================================================== */

static output_template_t *
create_builtin_template(const char *name, const char *id,
                        streaming_service_t service,
                        stream_orientation_t orientation, uint32_t bitrate,
                        uint32_t width, uint32_t height) {
  output_template_t *tmpl = bzalloc(sizeof(output_template_t));

  tmpl->template_name = bstrdup(name);
  tmpl->template_id = bstrdup(id);
  tmpl->service = service;
  tmpl->orientation = orientation;
  tmpl->is_builtin = true;

  /* Set encoding settings */
  tmpl->encoding = channel_get_default_encoding();
  tmpl->encoding.bitrate = bitrate;
  tmpl->encoding.width = width;
  tmpl->encoding.height = height;
  tmpl->encoding.audio_bitrate = 128; /* Default audio bitrate */

  return tmpl;
}

/* Helper function to add a builtin template to the manager's template array.
 * Handles memory allocation and array expansion internally. */
static output_template_t *
channel_manager_add_builtin_template(channel_manager_t *manager,
                                     const char *name, const char *id,
                                     streaming_service_t service,
                                     stream_orientation_t orientation,
                                     uint32_t bitrate, uint32_t width,
                                     uint32_t height) {
  if (!manager) {
    return NULL;
  }

  output_template_t *tmpl = create_builtin_template(name, id, service,
                                                     orientation, bitrate,
                                                     width, height);
  if (!tmpl) {
    return NULL;
  }

  /* Expand templates array */
  manager->templates =
      brealloc(manager->templates, sizeof(output_template_t *) *
                                       (manager->template_count + 1));
  manager->templates[manager->template_count++] = tmpl;

  return tmpl;
}

void channel_manager_load_builtin_templates(channel_manager_t *manager) {
  if (!manager) {
    return;
  }

  obs_log(LOG_INFO, "Loading built-in output templates");

  /* YouTube templates */
  channel_manager_add_builtin_template(manager, "YouTube 1080p60",
                                       "builtin_youtube_1080p60",
                                       SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL,
                                       6000, 1920, 1080);

  channel_manager_add_builtin_template(manager, "YouTube 720p60",
                                       "builtin_youtube_720p60",
                                       SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL,
                                       4500, 1280, 720);

  /* Twitch templates */
  channel_manager_add_builtin_template(manager, "Twitch 1080p60",
                                       "builtin_twitch_1080p60",
                                       SERVICE_TWITCH, ORIENTATION_HORIZONTAL,
                                       6000, 1920, 1080);

  channel_manager_add_builtin_template(manager, "Twitch 720p60",
                                       "builtin_twitch_720p60",
                                       SERVICE_TWITCH, ORIENTATION_HORIZONTAL,
                                       4500, 1280, 720);

  /* Facebook templates */
  channel_manager_add_builtin_template(manager, "Facebook 1080p",
                                       "builtin_facebook_1080p",
                                       SERVICE_FACEBOOK, ORIENTATION_HORIZONTAL,
                                       4000, 1920, 1080);

  /* TikTok vertical template */
  channel_manager_add_builtin_template(manager, "TikTok Vertical",
                                       "builtin_tiktok_vertical",
                                       SERVICE_TIKTOK, ORIENTATION_VERTICAL,
                                       3000, 1080, 1920);

  obs_log(LOG_INFO, "Loaded %zu built-in templates", manager->template_count);
}

output_template_t *channel_manager_create_template(
    channel_manager_t *manager, const char *name, streaming_service_t service,
    stream_orientation_t orientation, encoding_settings_t *encoding) {
  if (!manager || !name || !encoding) {
    return NULL;
  }

  output_template_t *tmpl = bzalloc(sizeof(output_template_t));

  tmpl->template_name = bstrdup(name);
  tmpl->template_id = channel_generate_id(); /* Reuse ID generator */
  tmpl->service = service;
  tmpl->orientation = orientation;
  tmpl->encoding = *encoding;
  tmpl->is_builtin = false;

  /* Add to manager */
  size_t new_count = manager->template_count + 1;
  manager->templates = brealloc(manager->templates,
                                sizeof(output_template_t *) * new_count);
  manager->templates[manager->template_count] = tmpl;
  manager->template_count = new_count;

  obs_log(LOG_INFO, "Created custom template: %s", name);

  return tmpl;
}

bool channel_manager_delete_template(channel_manager_t *manager,
                                     const char *template_id) {
  if (!manager || !template_id) {
    return false;
  }

  for (size_t i = 0; i < manager->template_count; i++) {
    output_template_t *tmpl = manager->templates[i];
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
                sizeof(output_template_t *) *
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

output_template_t *channel_manager_get_template(channel_manager_t *manager,
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

output_template_t *
channel_manager_get_template_at(channel_manager_t *manager, size_t index) {
  if (!manager || index >= manager->template_count) {
    return NULL;
  }

  return manager->templates[index];
}

bool channel_apply_template(stream_channel_t *channel,
                            output_template_t *tmpl,
                            const char *stream_key) {
  if (!channel || !tmpl || !stream_key) {
    return false;
  }

  /* Add output using template settings */
  bool result = channel_add_output(channel, tmpl->service, stream_key,
                                    tmpl->orientation, &tmpl->encoding);

  if (result) {
    obs_log(LOG_INFO, "Applied template '%s' to channel '%s' with stream key",
            tmpl->template_name, channel->channel_name);
  }

  return result;
}

void channel_manager_save_templates(channel_manager_t *manager,
                                    obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *templates_array = obs_data_array_create();

  /* Only save custom (non-builtin) templates */
  for (size_t i = 0; i < manager->template_count; i++) {
    output_template_t *tmpl = manager->templates[i];
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

  obs_data_set_array(settings, "output_templates", templates_array);
  obs_data_array_release(templates_array);

  obs_log(LOG_INFO, "Saved custom templates to settings");
}

void channel_manager_load_templates(channel_manager_t *manager,
                                    obs_data_t *settings) {
  if (!manager || !settings) {
    return;
  }

  obs_data_array_t *templates_array =
      obs_data_get_array(settings, "output_templates");
  if (!templates_array) {
    return;
  }

  size_t count = obs_data_array_count(templates_array);
  for (size_t i = 0; i < count; i++) {
    obs_data_t *tmpl_data = obs_data_array_item(templates_array, i);

    encoding_settings_t enc = channel_get_default_encoding();
    enc.bitrate = (uint32_t)obs_data_get_int(tmpl_data, "bitrate");
    enc.width = (uint32_t)obs_data_get_int(tmpl_data, "width");
    enc.height = (uint32_t)obs_data_get_int(tmpl_data, "height");
    enc.audio_bitrate = (uint32_t)obs_data_get_int(tmpl_data, "audio_bitrate");

    channel_manager_create_template(
        manager, obs_data_get_string(tmpl_data, "name"),
        (streaming_service_t)obs_data_get_int(tmpl_data, "service"),
        (stream_orientation_t)obs_data_get_int(tmpl_data, "orientation"), &enc);

    obs_data_release(tmpl_data);
  }

  obs_data_array_release(templates_array);

  obs_log(LOG_INFO, "Loaded %zu custom templates from settings", count);
}

/* ========================================================================
 * Backup/Failover Output Support Implementation
 * ======================================================================== */

bool channel_set_output_backup(stream_channel_t *channel,
                               size_t primary_index, size_t backup_index) {
  if (!channel || primary_index >= channel->output_count ||
      backup_index >= channel->output_count) {
    return false;
  }

  if (primary_index == backup_index) {
    obs_log(LOG_ERROR, "Cannot set output as backup for itself");
    return false;
  }

  channel_output_t *primary = &channel->outputs[primary_index];
  channel_output_t *backup = &channel->outputs[backup_index];

  /* Check if primary already has a backup */
  if (primary->backup_index != (size_t)-1 &&
      primary->backup_index != backup_index) {
    obs_log(LOG_WARNING,
            "Primary output %s already has a backup, replacing",
            primary->service_name);
    /* Clear old backup relationship */
    channel->outputs[primary->backup_index].is_backup = false;
    channel->outputs[primary->backup_index].primary_index = (size_t)-1;
  }

  /* Set backup relationship */
  primary->backup_index = backup_index;
  backup->is_backup = true;
  backup->primary_index = primary_index;
  backup->enabled = false; /* Backup starts disabled */

  obs_log(LOG_INFO, "Set %s as backup for %s in channel %s",
          backup->service_name, primary->service_name, channel->channel_name);

  return true;
}

bool channel_remove_output_backup(stream_channel_t *channel,
                                  size_t primary_index) {
  if (!channel || primary_index >= channel->output_count) {
    return false;
  }

  channel_output_t *primary = &channel->outputs[primary_index];

  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_WARNING, "Primary output has no backup to remove");
    return false;
  }

  /* Clear backup relationship */
  channel_output_t *backup = &channel->outputs[primary->backup_index];
  backup->is_backup = false;
  backup->primary_index = (size_t)-1;
  primary->backup_index = (size_t)-1;

  obs_log(LOG_INFO, "Removed backup relationship for %s in channel %s",
          primary->service_name, channel->channel_name);

  return true;
}

bool channel_trigger_failover(stream_channel_t *channel, restreamer_api_t *api,
                              size_t primary_index) {
  if (!channel || !api || primary_index >= channel->output_count) {
    return false;
  }

  channel_output_t *primary = &channel->outputs[primary_index];

  /* Check if primary has a backup */
  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_ERROR, "Cannot failover: primary output %s has no backup",
            primary->service_name);
    return false;
  }

  channel_output_t *backup = &channel->outputs[primary->backup_index];

  /* Check if already failed over */
  if (primary->failover_active) {
    obs_log(LOG_WARNING, "Failover already active for %s",
            primary->service_name);
    return true;
  }

  obs_log(LOG_INFO, "Triggering failover from %s to %s in channel %s",
          primary->service_name, backup->service_name, channel->channel_name);

  /* Only failover if channel is active */
  if (channel->status == CHANNEL_STATUS_ACTIVE) {
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
      obs_log(LOG_ERROR, "Failed to enable backup output");
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

bool channel_restore_primary(stream_channel_t *channel, restreamer_api_t *api,
                             size_t primary_index) {
  if (!channel || !api || primary_index >= channel->output_count) {
    return false;
  }

  channel_output_t *primary = &channel->outputs[primary_index];

  /* Check if primary has a backup */
  if (primary->backup_index == (size_t)-1) {
    obs_log(LOG_ERROR, "Primary output has no backup");
    return false;
  }

  channel_output_t *backup = &channel->outputs[primary->backup_index];

  /* Check if failover is active */
  if (!primary->failover_active) {
    obs_log(LOG_WARNING, "No active failover to restore from");
    return true;
  }

  obs_log(LOG_INFO,
          "Restoring primary output %s from backup %s in channel %s",
          primary->service_name, backup->service_name, channel->channel_name);

  /* Only restore if channel is active */
  if (channel->status == CHANNEL_STATUS_ACTIVE) {
    /* Re-enable primary */
    bool added =
        restreamer_multistream_add_destination_live(api, NULL, primary_index);
    if (!added) {
      obs_log(LOG_ERROR, "Failed to re-enable primary output");
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

bool channel_check_failover(stream_channel_t *channel, restreamer_api_t *api) {
  if (!channel || !api) {
    return false;
  }

  /* Only check failover if channel is active */
  if (channel->status != CHANNEL_STATUS_ACTIVE) {
    return true;
  }

  bool any_failover = false;

  for (size_t i = 0; i < channel->output_count; i++) {
    channel_output_t *output = &channel->outputs[i];

    /* Skip backup outputs */
    if (output->is_backup) {
      continue;
    }

    /* Skip outputs without backups */
    if (output->backup_index == (size_t)-1) {
      continue;
    }

    /* Check if primary is unhealthy and should failover */
    if (!output->failover_active && !output->connected &&
        output->consecutive_failures >= channel->failure_threshold) {
      obs_log(LOG_WARNING,
              "Primary output %s has failed %u times, triggering failover",
              output->service_name, output->consecutive_failures);

      if (channel_trigger_failover(channel, api, i)) {
        any_failover = true;
      }
    }

    /* Check if primary has recovered and should be restored */
    if (output->failover_active && output->connected &&
        output->consecutive_failures == 0) {
      obs_log(LOG_INFO,
              "Primary output %s has recovered, restoring from backup",
              output->service_name);

      channel_restore_primary(channel, api, i);
    }
  }

  return any_failover;
}

/* ========================================================================
 * Bulk Output Operations Implementation
 * ======================================================================== */

bool channel_bulk_enable_outputs(stream_channel_t *channel,
                                 restreamer_api_t *api, size_t *indices,
                                 size_t count, bool enabled) {
  if (!channel || !indices || count == 0) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk %s %zu outputs in channel %s",
          enabled ? "enabling" : "disabling", count, channel->channel_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= channel->output_count) {
      obs_log(LOG_WARNING, "Invalid output index: %zu", idx);
      fail_count++;
      continue;
    }

    /* Skip backup outputs */
    if (channel->outputs[idx].is_backup) {
      obs_log(LOG_WARNING,
              "Cannot directly enable/disable backup output %s",
              channel->outputs[idx].service_name);
      fail_count++;
      continue;
    }

    bool result = channel_set_output_enabled(channel, idx, enabled);
    if (result) {
      success_count++;

      /* If channel is active, apply change live */
      if (channel->status == CHANNEL_STATUS_ACTIVE && api) {
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

bool channel_bulk_delete_outputs(stream_channel_t *channel,
                                 size_t *indices, size_t count) {
  if (!channel || !indices || count == 0) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk deleting %zu outputs from channel %s", count,
          channel->channel_name);

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
    if (idx >= channel->output_count) {
      obs_log(LOG_WARNING, "Invalid output index: %zu", idx);
      fail_count++;
      continue;
    }

    /* Remove backup relationships before deleting */
    channel_output_t *output = &channel->outputs[idx];
    if (output->backup_index != (size_t)-1) {
      channel_remove_output_backup(channel, idx);
    }
    if (output->is_backup) {
      channel_remove_output_backup(channel, output->primary_index);
    }

    bool result = channel_remove_output(channel, idx);
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

bool channel_bulk_update_encoding(stream_channel_t *channel,
                                  restreamer_api_t *api, size_t *indices,
                                  size_t count, encoding_settings_t *encoding) {
  if (!channel || !indices || count == 0 || !encoding) {
    return false;
  }

  obs_log(LOG_INFO, "Bulk updating encoding for %zu outputs in channel %s",
          count, channel->channel_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  bool is_active = (channel->status == CHANNEL_STATUS_ACTIVE);

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= channel->output_count) {
      obs_log(LOG_WARNING, "Invalid output index: %zu", idx);
      fail_count++;
      continue;
    }

    bool result;
    if (is_active && api) {
      /* Update encoding live */
      result =
          channel_update_output_encoding_live(channel, api, idx, encoding);
    } else {
      /* Update encoding settings only */
      result = channel_update_output_encoding(channel, idx, encoding);
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

bool channel_bulk_start_outputs(stream_channel_t *channel,
                                restreamer_api_t *api, size_t *indices,
                                size_t count) {
  if (!channel || !api || !indices || count == 0) {
    return false;
  }

  /* Only start if channel is active */
  if (channel->status != CHANNEL_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot bulk start outputs: channel %s is not active",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO, "Bulk starting %zu outputs in channel %s", count,
          channel->channel_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= channel->output_count) {
      obs_log(LOG_WARNING, "Invalid output index: %zu", idx);
      fail_count++;
      continue;
    }

    channel_output_t *output = &channel->outputs[idx];

    /* Skip if already enabled */
    if (output->enabled) {
      obs_log(LOG_DEBUG, "Output %s already enabled", output->service_name);
      success_count++;
      continue;
    }

    /* Skip backup outputs */
    if (output->is_backup) {
      obs_log(LOG_WARNING, "Cannot directly start backup output %s",
              output->service_name);
      fail_count++;
      continue;
    }

    /* Add output to active stream */
    bool result = restreamer_multistream_add_destination_live(api, NULL, idx);
    if (result) {
      output->enabled = true;
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk start complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}

bool channel_bulk_stop_outputs(stream_channel_t *channel,
                               restreamer_api_t *api, size_t *indices,
                               size_t count) {
  if (!channel || !api || !indices || count == 0) {
    return false;
  }

  /* Only stop if channel is active */
  if (channel->status != CHANNEL_STATUS_ACTIVE) {
    obs_log(LOG_WARNING,
            "Cannot bulk stop outputs: channel %s is not active",
            channel->channel_name);
    return false;
  }

  obs_log(LOG_INFO, "Bulk stopping %zu outputs in channel %s", count,
          channel->channel_name);

  size_t success_count = 0;
  size_t fail_count = 0;

  for (size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if (idx >= channel->output_count) {
      obs_log(LOG_WARNING, "Invalid output index: %zu", idx);
      fail_count++;
      continue;
    }

    channel_output_t *output = &channel->outputs[idx];

    /* Skip if already disabled */
    if (!output->enabled) {
      obs_log(LOG_DEBUG, "Output %s already disabled", output->service_name);
      success_count++;
      continue;
    }

    /* Remove output from active stream */
    bool result =
        restreamer_multistream_enable_destination_live(api, NULL, idx, false);
    if (result) {
      output->enabled = false;
      success_count++;
    } else {
      fail_count++;
    }
  }

  obs_log(LOG_INFO, "Bulk stop complete: %zu succeeded, %zu failed",
          success_count, fail_count);

  return fail_count == 0;
}
