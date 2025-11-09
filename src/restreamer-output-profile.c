#include "restreamer-output-profile.h"
#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <time.h>

/* Profile Manager Implementation */

profile_manager_t *profile_manager_create(restreamer_api_t *api)
{
	profile_manager_t *manager = bzalloc(sizeof(profile_manager_t));
	manager->api = api;
	manager->profiles = NULL;
	manager->profile_count = 0;

	obs_log(LOG_INFO, "Profile manager created");
	return manager;
}

void profile_manager_destroy(profile_manager_t *manager)
{
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
	bfree(manager);

	obs_log(LOG_INFO, "Profile manager destroyed");
}

char *profile_generate_id(void)
{
	struct dstr id = {0};
	dstr_init(&id);

	/* Use timestamp + random component */
	uint64_t timestamp = (uint64_t)time(NULL);
	uint32_t random = (uint32_t)rand();

	dstr_printf(&id, "profile_%llu_%u", (unsigned long long)timestamp,
		    random);

	char *result = bstrdup(id.array);
	dstr_free(&id);

	return result;
}

output_profile_t *profile_manager_create_profile(profile_manager_t *manager,
						  const char *name)
{
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
		brealloc(manager->profiles,
			 sizeof(output_profile_t *) * new_count);
	manager->profiles[manager->profile_count] = profile;
	manager->profile_count = new_count;

	obs_log(LOG_INFO, "Created profile: %s (ID: %s)", name,
		profile->profile_id);

	return profile;
}

bool profile_manager_delete_profile(profile_manager_t *manager,
				    const char *profile_id)
{
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
			for (size_t j = 0; j < profile->destination_count;
			     j++) {
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
				memmove(&manager->profiles[i],
					&manager->profiles[i + 1],
					sizeof(output_profile_t *) *
						(manager->profile_count - i -
						 1));
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
					       const char *profile_id)
{
	if (!manager || !profile_id) {
		return NULL;
	}

	for (size_t i = 0; i < manager->profile_count; i++) {
		if (strcmp(manager->profiles[i]->profile_id, profile_id) ==
		    0) {
			return manager->profiles[i];
		}
	}

	return NULL;
}

output_profile_t *profile_manager_get_profile_at(profile_manager_t *manager,
						  size_t index)
{
	if (!manager || index >= manager->profile_count) {
		return NULL;
	}

	return manager->profiles[index];
}

size_t profile_manager_get_count(profile_manager_t *manager)
{
	return manager ? manager->profile_count : 0;
}

/* Profile Operations */

encoding_settings_t profile_get_default_encoding(void)
{
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
			     encoding_settings_t *encoding)
{
	if (!profile || !stream_key) {
		return false;
	}

	/* Expand destinations array */
	size_t new_count = profile->destination_count + 1;
	profile->destinations =
		brealloc(profile->destinations,
			 sizeof(profile_destination_t) * new_count);

	profile_destination_t *dest =
		&profile->destinations[profile->destination_count];
	memset(dest, 0, sizeof(profile_destination_t));

	/* Set basic properties */
	dest->service = service;
	dest->service_name =
		bstrdup(restreamer_multistream_get_service_name(service));
	dest->stream_key = bstrdup(stream_key);
	dest->rtmp_url = bstrdup(restreamer_multistream_get_service_url(
		service, target_orientation));
	dest->target_orientation = target_orientation;
	dest->enabled = true;

	/* Set encoding settings */
	if (encoding) {
		dest->encoding = *encoding;
	} else {
		dest->encoding = profile_get_default_encoding();
	}

	profile->destination_count = new_count;

	obs_log(LOG_INFO, "Added destination %s to profile %s",
		dest->service_name, profile->profile_name);

	return true;
}

bool profile_remove_destination(output_profile_t *profile, size_t index)
{
	if (!profile || index >= profile->destination_count) {
		return false;
	}

	/* Free destination */
	bfree(profile->destinations[index].service_name);
	bfree(profile->destinations[index].stream_key);
	bfree(profile->destinations[index].rtmp_url);

	/* Shift remaining destinations */
	if (index < profile->destination_count - 1) {
		memmove(&profile->destinations[index],
			&profile->destinations[index + 1],
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
					 encoding_settings_t *encoding)
{
	if (!profile || !encoding || index >= profile->destination_count) {
		return false;
	}

	profile->destinations[index].encoding = *encoding;
	return true;
}

bool profile_set_destination_enabled(output_profile_t *profile, size_t index,
				     bool enabled)
{
	if (!profile || index >= profile->destination_count) {
		return false;
	}

	profile->destinations[index].enabled = enabled;
	return true;
}

/* Streaming Control */

bool output_profile_start(profile_manager_t *manager, const char *profile_id)
{
	if (!manager || !profile_id) {
		return false;
	}

	output_profile_t *profile =
		profile_manager_get_profile(manager, profile_id);
	if (!profile) {
		obs_log(LOG_ERROR, "Profile not found: %s", profile_id);
		return false;
	}

	if (profile->status == PROFILE_STATUS_ACTIVE) {
		obs_log(LOG_WARNING, "Profile already active: %s",
			profile->profile_name);
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
		obs_log(LOG_ERROR,
			"No enabled destinations in profile: %s",
			profile->profile_name);
		bfree(profile->last_error);
		profile->last_error =
			bstrdup("No enabled destinations configured");
		profile->status = PROFILE_STATUS_ERROR;
		return false;
	}

	profile->status = PROFILE_STATUS_STARTING;

	/* Check if API is available */
	if (!manager->api) {
		obs_log(LOG_ERROR,
			"No Restreamer API connection available for profile: %s",
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
		if (!restreamer_multistream_add_destination(
			    config, pdest->service, pdest->stream_key,
			    pdest->target_orientation)) {
			obs_log(LOG_WARNING,
				"Failed to add destination %s to profile %s",
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
		profile->last_error =
			bstrdup(restreamer_api_get_error(manager->api));
		restreamer_multistream_destroy(config);
		profile->status = PROFILE_STATUS_ERROR;
		return false;
	}

	/* Store process reference for stopping later */
	bfree(profile->process_reference);
	profile->process_reference = bstrdup(config->process_reference);

	/* Clean up temporary config */
	restreamer_multistream_destroy(config);

	profile->status = PROFILE_STATUS_ACTIVE;
	obs_log(LOG_INFO, "Profile %s started successfully with process reference: %s",
		profile->profile_name, profile->process_reference);

	return true;
}

bool output_profile_stop(profile_manager_t *manager, const char *profile_id)
{
	if (!manager || !profile_id) {
		return false;
	}

	output_profile_t *profile =
		profile_manager_get_profile(manager, profile_id);
	if (!profile) {
		return false;
	}

	if (profile->status == PROFILE_STATUS_INACTIVE) {
		return true;
	}

	profile->status = PROFILE_STATUS_STOPPING;

	/* Stop the Restreamer process if we have a reference */
	if (profile->process_reference && manager->api) {
		obs_log(LOG_INFO, "Stopping Restreamer process for profile: %s (reference: %s)",
			profile->profile_name, profile->process_reference);

		if (!restreamer_multistream_stop(manager->api,
						 profile->process_reference)) {
			obs_log(LOG_WARNING,
				"Failed to stop Restreamer process for profile: %s: %s",
				profile->profile_name,
				restreamer_api_get_error(manager->api));
			/* Continue anyway to update status */
		}

		/* Clear process reference */
		bfree(profile->process_reference);
		profile->process_reference = NULL;
	}

	obs_log(LOG_INFO, "Stopped profile: %s", profile->profile_name);

	profile->status = PROFILE_STATUS_INACTIVE;
	return true;
}

bool profile_restart(profile_manager_t *manager, const char *profile_id)
{
	output_profile_stop(manager, profile_id);
	return output_profile_start(manager, profile_id);
}

bool profile_manager_start_all(profile_manager_t *manager)
{
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

bool profile_manager_stop_all(profile_manager_t *manager)
{
	if (!manager) {
		return false;
	}

	obs_log(LOG_INFO, "Stopping all profiles");

	bool all_success = true;
	for (size_t i = 0; i < manager->profile_count; i++) {
		if (!output_profile_stop(manager,
				  manager->profiles[i]->profile_id)) {
			all_success = false;
		}
	}

	return all_success;
}

size_t profile_manager_get_active_count(profile_manager_t *manager)
{
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

/* Configuration Persistence */

void profile_manager_load_from_settings(profile_manager_t *manager,
					obs_data_t *settings)
{
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
		output_profile_t *profile =
			profile_load_from_settings(profile_data);

		if (profile) {
			/* Add to manager */
			size_t new_count = manager->profile_count + 1;
			manager->profiles = brealloc(
				manager->profiles,
				sizeof(output_profile_t *) * new_count);
			manager->profiles[manager->profile_count] = profile;
			manager->profile_count = new_count;
		}

		obs_data_release(profile_data);
	}

	obs_data_array_release(profiles_array);

	obs_log(LOG_INFO, "Loaded %zu profiles from settings", count);
}

void profile_manager_save_to_settings(profile_manager_t *manager,
				      obs_data_t *settings)
{
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

	obs_log(LOG_INFO, "Saved %zu profiles to settings",
		manager->profile_count);
}

output_profile_t *profile_load_from_settings(obs_data_t *settings)
{
	if (!settings) {
		return NULL;
	}

	output_profile_t *profile = bzalloc(sizeof(output_profile_t));

	/* Load basic properties */
	profile->profile_name =
		bstrdup(obs_data_get_string(settings, "name"));
	profile->profile_id = bstrdup(obs_data_get_string(settings, "id"));
	profile->source_orientation =
		(stream_orientation_t)obs_data_get_int(settings,
						       "source_orientation");
	profile->auto_detect_orientation =
		obs_data_get_bool(settings, "auto_detect_orientation");
	profile->source_width =
		(uint32_t)obs_data_get_int(settings, "source_width");
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
	profile->auto_reconnect =
		obs_data_get_bool(settings, "auto_reconnect");
	profile->reconnect_delay_sec =
		(uint32_t)obs_data_get_int(settings, "reconnect_delay_sec");

	/* Load destinations */
	obs_data_array_t *dests_array =
		obs_data_get_array(settings, "destinations");
	if (dests_array) {
		size_t count = obs_data_array_count(dests_array);
		for (size_t i = 0; i < count; i++) {
			obs_data_t *dest_data =
				obs_data_array_item(dests_array, i);

			encoding_settings_t enc = profile_get_default_encoding();
			enc.width = (uint32_t)obs_data_get_int(dest_data,
							       "width");
			enc.height = (uint32_t)obs_data_get_int(dest_data,
								"height");
			enc.bitrate = (uint32_t)obs_data_get_int(dest_data,
								 "bitrate");
			enc.audio_bitrate = (uint32_t)obs_data_get_int(
				dest_data, "audio_bitrate");
			enc.audio_track = (uint32_t)obs_data_get_int(
				dest_data, "audio_track");

			profile_add_destination(
				profile,
				(streaming_service_t)obs_data_get_int(
					dest_data, "service"),
				obs_data_get_string(dest_data, "stream_key"),
				(stream_orientation_t)obs_data_get_int(
					dest_data, "target_orientation"),
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

void profile_save_to_settings(output_profile_t *profile, obs_data_t *settings)
{
	if (!profile || !settings) {
		return;
	}

	/* Save basic properties */
	obs_data_set_string(settings, "name", profile->profile_name);
	obs_data_set_string(settings, "id", profile->profile_id);
	obs_data_set_int(settings, "source_orientation",
			 profile->source_orientation);
	obs_data_set_bool(settings, "auto_detect_orientation",
			  profile->auto_detect_orientation);
	obs_data_set_int(settings, "source_width", profile->source_width);
	obs_data_set_int(settings, "source_height", profile->source_height);
	obs_data_set_string(settings, "input_url",
			    profile->input_url ? profile->input_url : "");
	obs_data_set_bool(settings, "auto_start", profile->auto_start);
	obs_data_set_bool(settings, "auto_reconnect",
			  profile->auto_reconnect);
	obs_data_set_int(settings, "reconnect_delay_sec",
			 profile->reconnect_delay_sec);

	/* Save destinations */
	obs_data_array_t *dests_array = obs_data_array_create();

	for (size_t i = 0; i < profile->destination_count; i++) {
		profile_destination_t *dest = &profile->destinations[i];
		obs_data_t *dest_data = obs_data_create();

		obs_data_set_int(dest_data, "service", dest->service);
		obs_data_set_string(dest_data, "stream_key",
				    dest->stream_key);
		obs_data_set_int(dest_data, "target_orientation",
				 dest->target_orientation);
		obs_data_set_bool(dest_data, "enabled", dest->enabled);

		/* Encoding settings */
		obs_data_set_int(dest_data, "width", dest->encoding.width);
		obs_data_set_int(dest_data, "height", dest->encoding.height);
		obs_data_set_int(dest_data, "bitrate", dest->encoding.bitrate);
		obs_data_set_int(dest_data, "audio_bitrate",
				 dest->encoding.audio_bitrate);
		obs_data_set_int(dest_data, "audio_track",
				 dest->encoding.audio_track);

		obs_data_array_push_back(dests_array, dest_data);
		obs_data_release(dest_data);
	}

	obs_data_set_array(settings, "destinations", dests_array);
	obs_data_array_release(dests_array);
}

output_profile_t *profile_duplicate(output_profile_t *source,
				    const char *new_name)
{
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
		profile_add_destination(
			duplicate, source->destinations[i].service,
			source->destinations[i].stream_key,
			source->destinations[i].target_orientation,
			&source->destinations[i].encoding);

		duplicate->destinations[i].enabled =
			source->destinations[i].enabled;
	}

	return duplicate;
}

bool profile_update_stats(output_profile_t *profile, restreamer_api_t *api)
{
	if (!profile || !api || !profile->process_reference) {
		return false;
	}

	/* TODO: Query restreamer API for process stats and update destination stats */
	/* This will be implemented when we integrate with actual OBS outputs */

	return true;
}
