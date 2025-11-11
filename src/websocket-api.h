#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file websocket-api.h
 * @brief OBS WebSocket Vendor API for remote plugin control and test automation
 *
 * This module registers "polyemesis" as a WebSocket vendor, exposing plugin
 * functionality for automated testing and remote control. Compatible with
 * obs-websocket clients like Python obswebsocket library.
 *
 * Vendor Requests:
 * - Profile Management: CreateProfile, DeleteProfile, DuplicateProfile, GetProfiles
 * - Destination Management: AddDestination, RemoveDestination, EditDestination
 * - Stream Control: StartProfile, StopProfile, StartAllProfiles, StopAllProfiles
 * - State Queries: GetPluginState, GetProfileState, GetConnectionStatus
 * - UI State: GetButtonStates, IsProfileSelected
 *
 * Events Emitted:
 * - ProfileCreated, ProfileDeleted, ProfileStateChanged
 * - ConnectionStatusChanged, ButtonStatesChanged, ErrorOccurred
 */

/**
 * @brief Initialize WebSocket vendor API
 * @return true on success, false on failure
 */
bool websocket_api_init(void);

/**
 * @brief Shutdown WebSocket vendor API and unregister vendor
 */
void websocket_api_shutdown(void);

/**
 * @brief Emit a WebSocket event to connected clients
 * @param event_name Name of the event
 * @param event_data JSON data for the event (obs_data_t will be released by caller)
 */
void websocket_api_emit_event(const char *event_name, obs_data_t *event_data);

#ifdef __cplusplus
}
#endif
