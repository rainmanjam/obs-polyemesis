#include "websocket-api.h"
#include "plugin-main.h"
#include "restreamer-dock.h"
#include <obs-websocket-api.h>
#include <util/platform.h>

/* WebSocket vendor handle */
static obs_websocket_vendor vendor = nullptr;

/* Forward declarations for request handlers */
static void handle_create_profile(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_delete_profile(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_duplicate_profile(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_get_profiles(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_add_destination(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_remove_destination(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_edit_destination(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_start_profile(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_stop_profile(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_start_all_profiles(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_stop_all_profiles(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_get_plugin_state(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_get_profile_state(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_get_connection_status(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_get_button_states(obs_data_t *request, obs_data_t *response, void *priv);
static void handle_connect_to_server(obs_data_t *request, obs_data_t *response, void *priv);

bool websocket_api_init(void)
{
	blog(LOG_INFO, "[obs-polyemesis] Initializing WebSocket Vendor API");

	/* Register vendor */
	vendor = obs_websocket_register_vendor("polyemesis");
	if (!vendor) {
		blog(LOG_ERROR, "[obs-polyemesis] Failed to register WebSocket vendor");
		return false;
	}

	/* Register all request handlers */
	obs_websocket_vendor_register_request(vendor, "CreateProfile", handle_create_profile, nullptr);
	obs_websocket_vendor_register_request(vendor, "DeleteProfile", handle_delete_profile, nullptr);
	obs_websocket_vendor_register_request(vendor, "DuplicateProfile", handle_duplicate_profile, nullptr);
	obs_websocket_vendor_register_request(vendor, "GetProfiles", handle_get_profiles, nullptr);

	obs_websocket_vendor_register_request(vendor, "AddDestination", handle_add_destination, nullptr);
	obs_websocket_vendor_register_request(vendor, "RemoveDestination", handle_remove_destination, nullptr);
	obs_websocket_vendor_register_request(vendor, "EditDestination", handle_edit_destination, nullptr);

	obs_websocket_vendor_register_request(vendor, "StartProfile", handle_start_profile, nullptr);
	obs_websocket_vendor_register_request(vendor, "StopProfile", handle_stop_profile, nullptr);
	obs_websocket_vendor_register_request(vendor, "StartAllProfiles", handle_start_all_profiles, nullptr);
	obs_websocket_vendor_register_request(vendor, "StopAllProfiles", handle_stop_all_profiles, nullptr);

	obs_websocket_vendor_register_request(vendor, "GetPluginState", handle_get_plugin_state, nullptr);
	obs_websocket_vendor_register_request(vendor, "GetProfileState", handle_get_profile_state, nullptr);
	obs_websocket_vendor_register_request(vendor, "GetConnectionStatus", handle_get_connection_status, nullptr);
	obs_websocket_vendor_register_request(vendor, "GetButtonStates", handle_get_button_states, nullptr);
	obs_websocket_vendor_register_request(vendor, "ConnectToServer", handle_connect_to_server, nullptr);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket Vendor API initialized with 16 requests");
	return true;
}

void websocket_api_shutdown(void)
{
	if (vendor) {
		blog(LOG_INFO, "[obs-polyemesis] Shutting down WebSocket Vendor API");
		obs_websocket_vendor_unregister_request(vendor, "CreateProfile");
		obs_websocket_vendor_unregister_request(vendor, "DeleteProfile");
		obs_websocket_vendor_unregister_request(vendor, "DuplicateProfile");
		obs_websocket_vendor_unregister_request(vendor, "GetProfiles");
		obs_websocket_vendor_unregister_request(vendor, "AddDestination");
		obs_websocket_vendor_unregister_request(vendor, "RemoveDestination");
		obs_websocket_vendor_unregister_request(vendor, "EditDestination");
		obs_websocket_vendor_unregister_request(vendor, "StartProfile");
		obs_websocket_vendor_unregister_request(vendor, "StopProfile");
		obs_websocket_vendor_unregister_request(vendor, "StartAllProfiles");
		obs_websocket_vendor_unregister_request(vendor, "StopAllProfiles");
		obs_websocket_vendor_unregister_request(vendor, "GetPluginState");
		obs_websocket_vendor_unregister_request(vendor, "GetProfileState");
		obs_websocket_vendor_unregister_request(vendor, "GetConnectionStatus");
		obs_websocket_vendor_unregister_request(vendor, "GetButtonStates");
		obs_websocket_vendor_unregister_request(vendor, "ConnectToServer");
		vendor = nullptr;
	}
}

void websocket_api_emit_event(const char *event_name, obs_data_t *event_data)
{
	if (vendor && event_name && event_data) {
		obs_websocket_vendor_emit_event(vendor, event_name, event_data);
	}
}

/* ========================================================================= */
/* Request Handler Implementations                                           */
/* ========================================================================= */

/**
 * CreateProfile - Create a new streaming profile
 * Request: {"profileName": "My Profile"}
 * Response: {"profileId": "uuid", "success": true}
 */
static void handle_create_profile(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_name = obs_data_get_string(request, "profileName");
	if (!profile_name || strlen(profile_name) == 0) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileName is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	output_profile_t *profile = profile_manager_create_profile(pm, profile_name);
	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Failed to create profile");
		return;
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_string(response, "profileId", profile->profile_id);
	obs_data_set_string(response, "profileName", profile->profile_name);

	/* Emit event */
	obs_data_t *event_data = obs_data_create();
	obs_data_set_string(event_data, "profileId", profile->profile_id);
	obs_data_set_string(event_data, "profileName", profile->profile_name);
	websocket_api_emit_event("ProfileCreated", event_data);
	obs_data_release(event_data);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket: Created profile '%s' (ID: %s)",
	     profile_name, profile->profile_id);
}

/**
 * DeleteProfile - Delete an existing profile
 * Request: {"profileId": "uuid"}
 * Response: {"success": true}
 */
static void handle_delete_profile(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	if (!profile_id || strlen(profile_id) == 0) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);
	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	/* Check if profile is active */
	if (profile->status != PROFILE_INACTIVE) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Cannot delete active profile - stop it first");
		return;
	}

	profile_manager_delete_profile(pm, profile_id);
	obs_data_set_bool(response, "success", true);

	/* Emit event */
	obs_data_t *event_data = obs_data_create();
	obs_data_set_string(event_data, "profileId", profile_id);
	websocket_api_emit_event("ProfileDeleted", event_data);
	obs_data_release(event_data);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket: Deleted profile ID: %s", profile_id);
}

/**
 * DuplicateProfile - Duplicate an existing profile
 * Request: {"profileId": "uuid", "newName": "Copy of Profile"}
 * Response: {"success": true, "newProfileId": "uuid"}
 */
static void handle_duplicate_profile(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	const char *new_name = obs_data_get_string(request, "newName");

	if (!profile_id || strlen(profile_id) == 0) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	output_profile_t *new_profile = profile_manager_duplicate_profile(pm, profile_id, new_name);
	if (!new_profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Failed to duplicate profile");
		return;
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_string(response, "newProfileId", new_profile->profile_id);
	obs_data_set_string(response, "newProfileName", new_profile->profile_name);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket: Duplicated profile %s to %s",
	     profile_id, new_profile->profile_id);
}

/**
 * GetProfiles - Get list of all profiles
 * Request: {}
 * Response: {"profiles": [{"id": "uuid", "name": "Profile", "status": 0, "destinationCount": 2}, ...]}
 */
static void handle_get_profiles(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(request);
	UNUSED_PARAMETER(priv);

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	obs_data_array_t *profiles_array = obs_data_array_create();

	for (size_t i = 0; i < pm->profile_count; i++) {
		output_profile_t *profile = pm->profiles[i];
		obs_data_t *profile_obj = obs_data_create();

		obs_data_set_string(profile_obj, "id", profile->profile_id);
		obs_data_set_string(profile_obj, "name", profile->profile_name);
		obs_data_set_int(profile_obj, "status", profile->status);
		obs_data_set_int(profile_obj, "destinationCount", profile->destination_count);
		obs_data_set_string(profile_obj, "processId", profile->process_id ? profile->process_id : "");

		obs_data_array_push_back(profiles_array, profile_obj);
		obs_data_release(profile_obj);
	}

	obs_data_set_array(response, "profiles", profiles_array);
	obs_data_set_bool(response, "success", true);
	obs_data_array_release(profiles_array);

	blog(LOG_DEBUG, "[obs-polyemesis] WebSocket: Retrieved %zu profiles", pm->profile_count);
}

/**
 * AddDestination - Add streaming destination to profile
 * Request: {"profileId": "uuid", "name": "Twitch", "url": "rtmp://...", "streamKey": "key"}
 * Response: {"success": true, "destinationIndex": 0}
 */
static void handle_add_destination(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	const char *name = obs_data_get_string(request, "name");
	const char *url = obs_data_get_string(request, "url");
	const char *stream_key = obs_data_get_string(request, "streamKey");

	if (!profile_id || !name || !url || !stream_key) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Missing required fields: profileId, name, url, streamKey");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	stream_destination_t dest;
	dest.name = bstrdup(name);
	dest.url = bstrdup(url);
	dest.stream_key = bstrdup(stream_key);

	bool success = profile_manager_add_destination(pm, profile_id, &dest);

	bfree((void *)dest.name);
	bfree((void *)dest.url);
	bfree((void *)dest.stream_key);

	if (success) {
		obs_data_set_bool(response, "success", true);
		obs_data_set_int(response, "destinationIndex", profile->destination_count - 1);
		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Added destination '%s' to profile %s",
		     name, profile_id);
	} else {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Failed to add destination");
	}
}

/**
 * RemoveDestination - Remove destination from profile
 * Request: {"profileId": "uuid", "destinationIndex": 0}
 * Response: {"success": true}
 */
static void handle_remove_destination(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	int index = (int)obs_data_get_int(request, "destinationIndex");

	if (!profile_id) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	if (index < 0 || index >= (int)profile->destination_count) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Invalid destination index");
		return;
	}

	bool success = profile_manager_remove_destination(pm, profile_id, index);
	obs_data_set_bool(response, "success", success);

	if (success) {
		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Removed destination %d from profile %s",
		     index, profile_id);
	}
}

/**
 * EditDestination - Edit existing destination
 * Request: {"profileId": "uuid", "destinationIndex": 0, "name": "New Name", "url": "...", "streamKey": "..."}
 * Response: {"success": true}
 */
static void handle_edit_destination(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	int index = (int)obs_data_get_int(request, "destinationIndex");
	const char *name = obs_data_get_string(request, "name");
	const char *url = obs_data_get_string(request, "url");
	const char *stream_key = obs_data_get_string(request, "streamKey");

	if (!profile_id || !name || !url || !stream_key) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Missing required fields");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile || index < 0 || index >= (int)profile->destination_count) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Invalid profile or destination index");
		return;
	}

	stream_destination_t dest;
	dest.name = bstrdup(name);
	dest.url = bstrdup(url);
	dest.stream_key = bstrdup(stream_key);

	bool success = profile_manager_edit_destination(pm, profile_id, index, &dest);

	bfree((void *)dest.name);
	bfree((void *)dest.url);
	bfree((void *)dest.stream_key);

	obs_data_set_bool(response, "success", success);

	if (success) {
		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Edited destination %d in profile %s",
		     index, profile_id);
	}
}

/**
 * StartProfile - Start streaming for a profile
 * Request: {"profileId": "uuid"}
 * Response: {"success": true}
 */
static void handle_start_profile(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");

	if (!profile_id) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	bool success = profile_manager_start_profile(pm, profile_id);
	obs_data_set_bool(response, "success", success);

	if (success) {
		/* Emit state change event */
		obs_data_t *event_data = obs_data_create();
		obs_data_set_string(event_data, "profileId", profile_id);
		obs_data_set_int(event_data, "status", profile->status);
		websocket_api_emit_event("ProfileStateChanged", event_data);
		obs_data_release(event_data);

		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Started profile %s", profile_id);
	} else {
		obs_data_set_string(response, "error", "Failed to start profile");
	}
}

/**
 * StopProfile - Stop streaming for a profile
 * Request: {"profileId": "uuid"}
 * Response: {"success": true}
 */
static void handle_stop_profile(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");

	if (!profile_id) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	bool success = profile_manager_stop_profile(pm, profile_id);
	obs_data_set_bool(response, "success", success);

	if (success) {
		/* Emit state change event */
		obs_data_t *event_data = obs_data_create();
		obs_data_set_string(event_data, "profileId", profile_id);
		obs_data_set_int(event_data, "status", profile->status);
		websocket_api_emit_event("ProfileStateChanged", event_data);
		obs_data_release(event_data);

		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Stopped profile %s", profile_id);
	} else {
		obs_data_set_string(response, "error", "Failed to stop profile");
	}
}

/**
 * StartAllProfiles - Start all inactive profiles
 * Request: {}
 * Response: {"success": true, "startedCount": 3}
 */
static void handle_start_all_profiles(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(request);
	UNUSED_PARAMETER(priv);

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	int started_count = 0;
	for (size_t i = 0; i < pm->profile_count; i++) {
		output_profile_t *profile = pm->profiles[i];
		if (profile->status == PROFILE_INACTIVE && profile->destination_count > 0) {
			if (profile_manager_start_profile(pm, profile->profile_id)) {
				started_count++;
			}
		}
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_int(response, "startedCount", started_count);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket: Started %d profiles", started_count);
}

/**
 * StopAllProfiles - Stop all active profiles
 * Request: {}
 * Response: {"success": true, "stoppedCount": 3}
 */
static void handle_stop_all_profiles(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(request);
	UNUSED_PARAMETER(priv);

	profile_manager_t *pm = plugin_get_profile_manager();
	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	int stopped_count = 0;
	for (size_t i = 0; i < pm->profile_count; i++) {
		output_profile_t *profile = pm->profiles[i];
		if (profile->status == PROFILE_ACTIVE || profile->status == PROFILE_STARTING) {
			if (profile_manager_stop_profile(pm, profile->profile_id)) {
				stopped_count++;
			}
		}
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_int(response, "stoppedCount", stopped_count);

	blog(LOG_INFO, "[obs-polyemesis] WebSocket: Stopped %d profiles", stopped_count);
}

/**
 * GetPluginState - Get overall plugin state
 * Request: {}
 * Response: {"connected": true, "serverUrl": "...", "profileCount": 5, "activeProfileCount": 2}
 */
static void handle_get_plugin_state(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(request);
	UNUSED_PARAMETER(priv);

	profile_manager_t *pm = plugin_get_profile_manager();
	restreamer_api_t *client = plugin_get_api_client();

	if (!pm || !client) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Plugin not initialized");
		return;
	}

	int active_count = 0;
	for (size_t i = 0; i < pm->profile_count; i++) {
		if (pm->profiles[i]->status == PROFILE_ACTIVE ||
		    pm->profiles[i]->status == PROFILE_STARTING) {
			active_count++;
		}
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_bool(response, "connected", client->connected);
	obs_data_set_string(response, "serverUrl", client->base_url ? client->base_url : "");
	obs_data_set_int(response, "profileCount", pm->profile_count);
	obs_data_set_int(response, "activeProfileCount", active_count);
}

/**
 * GetProfileState - Get detailed state of a specific profile
 * Request: {"profileId": "uuid"}
 * Response: {"success": true, "status": 2, "processId": "...", "uptime": 3600, ...}
 */
static void handle_get_profile_state(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");

	if (!profile_id) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "profileId is required");
		return;
	}

	profile_manager_t *pm = plugin_get_profile_manager();
	output_profile_t *profile = profile_manager_get_profile_by_id(pm, profile_id);

	if (!profile) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile not found");
		return;
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_string(response, "profileId", profile->profile_id);
	obs_data_set_string(response, "profileName", profile->profile_name);
	obs_data_set_int(response, "status", profile->status);
	obs_data_set_string(response, "processId", profile->process_id ? profile->process_id : "");
	obs_data_set_int(response, "destinationCount", profile->destination_count);

	if (profile->metrics) {
		obs_data_t *metrics = obs_data_create();
		obs_data_set_int(metrics, "fps", profile->metrics->fps);
		obs_data_set_int(metrics, "bitrate", profile->metrics->bitrate);
		obs_data_set_int(metrics, "uptime", profile->metrics->uptime);
		obs_data_set_array(response, "metrics", obs_data_get_array(metrics, ""));
		obs_data_release(metrics);
	}
}

/**
 * GetConnectionStatus - Get Restreamer connection status
 * Request: {}
 * Response: {"connected": true, "serverUrl": "..."}
 */
static void handle_get_connection_status(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(request);
	UNUSED_PARAMETER(priv);

	restreamer_api_t *client = plugin_get_api_client();

	if (!client) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "API client not initialized");
		return;
	}

	obs_data_set_bool(response, "success", true);
	obs_data_set_bool(response, "connected", client->connected);
	obs_data_set_string(response, "serverUrl", client->base_url ? client->base_url : "");
}

/**
 * GetButtonStates - Get UI button states for testing
 * Request: {"profileId": "uuid"} (optional - if provided, checks buttons for that profile)
 * Response: {"canEdit": true, "canDuplicate": true, "canStart": false, "canStop": true, ...}
 */
static void handle_get_button_states(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *profile_id = obs_data_get_string(request, "profileId");
	profile_manager_t *pm = plugin_get_profile_manager();

	if (!pm) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Profile manager not initialized");
		return;
	}

	bool has_selection = (profile_id && strlen(profile_id) > 0);
	output_profile_t *profile = has_selection ? profile_manager_get_profile_by_id(pm, profile_id) : nullptr;

	bool is_active = profile && (profile->status == PROFILE_ACTIVE || profile->status == PROFILE_STARTING);
	bool is_inactive = profile && profile->status == PROFILE_INACTIVE;
	bool has_destinations = profile && profile->destination_count > 0;

	obs_data_set_bool(response, "success", true);
	obs_data_set_bool(response, "profileSelected", has_selection);
	obs_data_set_bool(response, "canCreateProfile", true);
	obs_data_set_bool(response, "canEditProfile", has_selection && is_inactive);
	obs_data_set_bool(response, "canDuplicateProfile", has_selection);
	obs_data_set_bool(response, "canDeleteProfile", has_selection && is_inactive);
	obs_data_set_bool(response, "canStartProfile", has_selection && is_inactive && has_destinations);
	obs_data_set_bool(response, "canStopProfile", has_selection && is_active);
	obs_data_set_bool(response, "canStartAll", pm->profile_count > 0);
	obs_data_set_bool(response, "canStopAll", pm->profile_count > 0);
}

/**
 * ConnectToServer - Connect to Restreamer server
 * Request: {"serverUrl": "http://localhost:8080", "username": "admin", "password": "admin"}
 * Response: {"success": true}
 */
static void handle_connect_to_server(obs_data_t *request, obs_data_t *response, void *priv)
{
	UNUSED_PARAMETER(priv);

	const char *server_url = obs_data_get_string(request, "serverUrl");
	const char *username = obs_data_get_string(request, "username");
	const char *password = obs_data_get_string(request, "password");

	if (!server_url || !username || !password) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "Missing required fields: serverUrl, username, password");
		return;
	}

	restreamer_api_t *client = plugin_get_api_client();
	if (!client) {
		obs_data_set_bool(response, "success", false);
		obs_data_set_string(response, "error", "API client not initialized");
		return;
	}

	/* Set base URL */
	if (client->base_url) {
		bfree((void *)client->base_url);
	}
	client->base_url = bstrdup(server_url);

	/* Set credentials */
	if (client->username) {
		bfree((void *)client->username);
	}
	if (client->password) {
		bfree((void *)client->password);
	}
	client->username = bstrdup(username);
	client->password = bstrdup(password);

	/* Test connection by getting /api/v3/about */
	char *about_json = restreamer_api_get(client, "/api/v3/about");
	bool success = (about_json != NULL);

	if (about_json) {
		bfree(about_json);
	}

	obs_data_set_bool(response, "success", success);
	client->connected = success;

	if (success) {
		/* Emit connection event */
		obs_data_t *event_data = obs_data_create();
		obs_data_set_bool(event_data, "connected", true);
		obs_data_set_string(event_data, "serverUrl", server_url);
		websocket_api_emit_event("ConnectionStatusChanged", event_data);
		obs_data_release(event_data);

		blog(LOG_INFO, "[obs-polyemesis] WebSocket: Connected to %s", server_url);
	} else {
		obs_data_set_string(response, "error", "Connection test failed");
	}
}
