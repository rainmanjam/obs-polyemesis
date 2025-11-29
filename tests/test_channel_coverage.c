/*
obs-polyemesis
Copyright (C) 2025 rainmanjam

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

/**
 * Additional coverage tests for restreamer-output-profile.c
 * Tests uncovered functions and edge cases to reach 80% code coverage
 */

#include "restreamer-channel.h"
#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include "mock_restreamer.h"
#include <obs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <util/bmem.h>

/* Test macros from test framework */
#define test_assert(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
      return false;                                                            \
    }                                                                          \
  } while (0)

static void test_section_start(const char *name) { (void)name; }
static void test_section_end(const char *name) { (void)name; }
static void test_start(const char *name) { printf("  Testing %s...\n", name); }
static void test_end(void) {}
static void test_suite_start(const char *name) {
  printf("\n%s\n========================================\n", name);
}
static void test_suite_end(const char *name, bool result) {
  if (result) printf("✓ %s: PASSED\n", name);
  else printf("✗ %s: FAILED\n", name);
}

/* Helper to create API connection */
static restreamer_api_t *create_test_api(void) {
  restreamer_connection_t conn = {
    .host = "localhost",
    .port = 8080,
    .username = "test",
    .password = "test",
    .use_https = false,
  };
  return restreamer_api_create(&conn);
}

/* Test: channel_manager_destroy with active channels (lines 26-71) */
static bool test_channel_manager_destroy_with_active_profiles(void)
{
  test_section_start("Manager Destroy with Active Profiles");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Create channel with outputs */
  stream_channel_t *channel = channel_manager_create_channel(manager, "Active Profile");
  encoding_settings_t enc = channel_get_default_encoding();
  enc.bitrate = 5000;

  channel_add_output(channel, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);

  /* Mark profile as active to test stop path in destroy */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->process_reference = bstrdup("test_process_ref");

  test_assert(manager->channel_count == 1, "Manager should have 1 channel");
  test_assert(channel->output_count == 2, "Channel should have 2 outputs");

  /* Destroy manager - should stop active channel and free all resources */
  channel_manager_destroy(manager);

  /* Test NULL manager doesn't crash */
  channel_manager_destroy(NULL);

  restreamer_api_destroy(api);

  test_section_end("Manager Destroy with Active Profiles");
  return true;
}

/* Test: channel_manager_delete_channel with active channel (lines 122-171) */
static bool test_channel_manager_delete_active_profile(void)
{
  test_section_start("Delete Active Profile");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Create channel and set it to active */
  stream_channel_t *channel = channel_manager_create_channel(manager, "To Delete");
  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->process_reference = bstrdup("delete_test_ref");

  char *channel_id = bstrdup(channel->channel_id);

  /* Delete active channel - should stop it first */
  bool deleted = channel_manager_delete_channel(manager, channel_id);
  test_assert(deleted, "Should delete active channel");
  test_assert(manager->channel_count == 0, "Manager should have 0 channels");
  test_assert(manager->channels == NULL, "Profiles array should be NULL after deleting last profile");

  bfree(channel_id);

  /* Test NULL parameters */
  deleted = channel_manager_delete_channel(NULL, "id");
  test_assert(!deleted, "NULL manager should fail");

  deleted = channel_manager_delete_channel(manager, NULL);
  test_assert(!deleted, "NULL channel_id should fail");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Delete Active Profile");
  return true;
}

/* Test: channel_update_output_encoding_live (lines 308-389) */
static bool test_channel_update_output_encoding_live(void)
{
  test_section_start("Update Output Encoding Live");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);
  stream_channel_t *channel = channel_manager_create_channel(manager, "Live Update Test");

  encoding_settings_t enc = channel_get_default_encoding();
  enc.bitrate = 5000;
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Test with inactive channel - should fail */
  encoding_settings_t new_enc = enc;
  new_enc.bitrate = 8000;

  bool updated = channel_update_output_encoding_live(channel, api, 0, &new_enc);
  test_assert(!updated, "Should fail when profile is not active");

  /* Test with active channel but no process reference - should fail */
  channel->status = CHANNEL_STATUS_ACTIVE;
  updated = channel_update_output_encoding_live(channel, api, 0, &new_enc);
  test_assert(!updated, "Should fail when no process reference");

  /* Test with process reference but process not found */
  channel->process_reference = bstrdup("nonexistent_process");
  updated = channel_update_output_encoding_live(channel, api, 0, &new_enc);
  test_assert(!updated, "Should fail when process not found");

  /* Test NULL parameters */
  updated = channel_update_output_encoding_live(NULL, api, 0, &new_enc);
  test_assert(!updated, "NULL channel should fail");

  updated = channel_update_output_encoding_live(channel, NULL, 0, &new_enc);
  test_assert(!updated, "NULL api should fail");

  updated = channel_update_output_encoding_live(channel, api, 0, NULL);
  test_assert(!updated, "NULL encoding should fail");

  updated = channel_update_output_encoding_live(channel, api, 999, &new_enc);
  test_assert(!updated, "Invalid index should fail");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Update Output Encoding Live");
  return true;
}

/* Test: stream_channel_start error paths (lines 403-522) */
static bool test_stream_channel_start_error_paths(void)
{
  test_section_start("Stream Channel Start Error Paths");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test NULL parameters */
  bool started = channel_start(NULL, "id");
  test_assert(!started, "NULL manager should fail");

  started = channel_start(manager, NULL);
  test_assert(!started, "NULL channel_id should fail");

  /* Test non-existent channel */
  started = channel_start(manager, "nonexistent");
  test_assert(!started, "Non-existent channel should fail");

  /* Create channel and test already active */
  stream_channel_t *channel = channel_manager_create_channel(manager, "Start Test");
  channel->status = CHANNEL_STATUS_ACTIVE;

  started = channel_start(manager, channel->channel_id);
  test_assert(started, "Already active channel should return true (no-op)");

  /* Test no enabled outputs */
  channel->status = CHANNEL_STATUS_INACTIVE;
  started = channel_start(manager, channel->channel_id);
  test_assert(!started, "No enabled outputs should fail");
  test_assert(channel->status == CHANNEL_STATUS_ERROR, "Channel should be in error state");
  test_assert(channel->last_error != NULL, "Should have error message");
  test_assert(strstr(channel->last_error, "No enabled outputs") != NULL,
              "Error message should mention outputs");

  /* Test with outputs but no input URL */
  channel->status = CHANNEL_STATUS_INACTIVE;
  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  bfree(channel->input_url);
  channel->input_url = bstrdup("");

  started = channel_start(manager, channel->channel_id);
  test_assert(!started, "Empty input URL should fail");
  test_assert(channel->status == CHANNEL_STATUS_ERROR, "Should be in error state");
  test_assert(channel->last_error != NULL, "Should have error message");

  /* Test with no API connection */
  channel_manager_t *manager_no_api = channel_manager_create(NULL);
  stream_channel_t *channel2 = channel_manager_create_channel(manager_no_api, "No API Test");
  channel_add_output(channel2, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  started = channel_start(manager_no_api, channel2->channel_id);
  test_assert(!started, "No API connection should fail");
  test_assert(channel2->status == CHANNEL_STATUS_ERROR, "Should be in error state");

  channel_manager_destroy(manager_no_api);
  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Stream Channel Start Error Paths");
  return true;
}

/* Test: stream_channel_stop with process reference (lines 524-567) */
static bool test_stream_channel_stop_with_process(void)
{
  test_section_start("Stream Channel Stop with Process");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);
  stream_channel_t *channel = channel_manager_create_channel(manager, "Stop Test");

  /* Test NULL parameters */
  bool stopped = channel_stop(NULL, "id");
  test_assert(!stopped, "NULL manager should fail");

  stopped = channel_stop(manager, NULL);
  test_assert(!stopped, "NULL channel_id should fail");

  /* Test non-existent channel */
  stopped = channel_stop(manager, "nonexistent");
  test_assert(!stopped, "Non-existent channel should fail");

  /* Test already inactive channel */
  channel->status = CHANNEL_STATUS_INACTIVE;
  stopped = channel_stop(manager, channel->channel_id);
  test_assert(stopped, "Already inactive should succeed (no-op)");

  /* Test stopping with process reference */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->process_reference = bstrdup("test_process_ref");

  stopped = channel_stop(manager, channel->channel_id);
  test_assert(stopped, "Should stop profile");
  test_assert(channel->status == CHANNEL_STATUS_INACTIVE, "Should be inactive");
  test_assert(channel->process_reference == NULL, "Process reference should be cleared");
  test_assert(channel->last_error == NULL, "Error should be cleared");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Stream Channel Stop with Process");
  return true;
}

/* Test: channel_restart (lines 569-572) */
static bool test_channel_restart(void)
{
  test_section_start("Channel Restart");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test NULL parameters */
  bool restarted = channel_restart(NULL, "id");
  test_assert(!restarted, "NULL manager should fail");

  restarted = channel_restart(manager, NULL);
  test_assert(!restarted, "NULL channel_id should fail");

  /* Create channel */
  stream_channel_t *channel = channel_manager_create_channel(manager, "Restart Test");
  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Set as active with process reference */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->process_reference = bstrdup("restart_ref");

  /* Restart should stop then start */
  restarted = channel_restart(manager, channel->channel_id);
  test_assert(!restarted, "Restart should fail on start (no actual API)");
  test_assert(channel->status == CHANNEL_STATUS_ERROR, "Should be in error state after failed restart");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Channel Restart");
  return true;
}

/* Test: channel_manager_start_all and stop_all (lines 574-610) */
static bool test_channel_manager_bulk_start_stop(void)
{
  test_section_start("Channel Manager Bulk Start/Stop");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test NULL manager */
  bool result = channel_manager_start_all(NULL);
  test_assert(!result, "NULL manager should fail start_all");

  result = channel_manager_stop_all(NULL);
  test_assert(!result, "NULL manager should fail stop_all");

  /* Test with empty manager */
  result = channel_manager_start_all(manager);
  test_assert(result, "Empty manager start_all should succeed");

  result = channel_manager_stop_all(manager);
  test_assert(result, "Empty manager stop_all should succeed");

  /* Create channels */
  stream_channel_t *channel1 = channel_manager_create_channel(manager, "Channel 1");
  stream_channel_t *channel2 = channel_manager_create_channel(manager, "Channel 2");
  stream_channel_t *channel3 = channel_manager_create_channel(manager, "Channel 3");

  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel1, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel2, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel3, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);

  /* Set auto_start flags */
  channel1->auto_start = true;
  channel2->auto_start = false;  /* This one should not start */
  channel3->auto_start = true;

  /* Start all - should attempt to start profiles with auto_start */
  result = channel_manager_start_all(manager);
  test_assert(!result, "start_all should fail (no real API)");

  /* Set profiles to active for testing stop_all */
  channel1->status = CHANNEL_STATUS_ACTIVE;
  channel1->process_reference = bstrdup("proc1");
  channel2->status = CHANNEL_STATUS_ACTIVE;
  channel2->process_reference = bstrdup("proc2");
  channel3->status = CHANNEL_STATUS_INACTIVE;

  /* Stop all */
  result = channel_manager_stop_all(manager);
  test_assert(result, "stop_all should succeed");
  test_assert(channel1->status == CHANNEL_STATUS_INACTIVE, "Channel 1 should be stopped");
  test_assert(channel2->status == CHANNEL_STATUS_INACTIVE, "Channel 2 should be stopped");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Channel Manager Bulk Start/Stop");
  return true;
}

/* Test: Preview mode functions (lines 631-746) */
static bool test_preview_mode_functions(void)
{
  test_section_start("Preview Mode Functions");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test NULL parameters for start_preview */
  bool result = channel_start_preview(NULL, "id", 60);
  test_assert(!result, "NULL manager should fail");

  result = channel_start_preview(manager, NULL, 60);
  test_assert(!result, "NULL channel_id should fail");

  /* Test non-existent channel */
  result = channel_start_preview(manager, "nonexistent", 60);
  test_assert(!result, "Non-existent channel should fail");

  /* Create channel */
  stream_channel_t *channel = channel_manager_create_channel(manager, "Preview Test");
  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Test starting preview on non-inactive channel */
  channel->status = CHANNEL_STATUS_ACTIVE;
  result = channel_start_preview(manager, channel->channel_id, 120);
  test_assert(!result, "Should fail when profile not inactive");

  /* Test starting preview on inactive channel */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_start_preview(manager, channel->channel_id, 180);
  test_assert(!result, "Should fail (no real API)");
  test_assert(channel->preview_mode_enabled == false, "Preview mode should be disabled after failure");

  /* Manually set preview mode for further testing */
  channel->status = CHANNEL_STATUS_PREVIEW;
  channel->preview_mode_enabled = true;
  channel->preview_duration_sec = 60;
  channel->preview_start_time = time(NULL);

  /* Test preview_to_live */
  result = channel_preview_to_live(NULL, "id");
  test_assert(!result, "NULL manager should fail");

  result = channel_preview_to_live(manager, NULL);
  test_assert(!result, "NULL channel_id should fail");

  result = channel_preview_to_live(manager, "nonexistent");
  test_assert(!result, "Non-existent channel should fail");

  /* Test preview_to_live with wrong status */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_preview_to_live(manager, channel->channel_id);
  test_assert(!result, "Should fail when not in preview mode");

  /* Test successful preview_to_live */
  channel->status = CHANNEL_STATUS_PREVIEW;
  result = channel_preview_to_live(manager, channel->channel_id);
  test_assert(result, "Should succeed");
  test_assert(channel->status == CHANNEL_STATUS_ACTIVE, "Should be active");
  test_assert(channel->preview_mode_enabled == false, "Preview mode should be disabled");
  test_assert(channel->preview_duration_sec == 0, "Duration should be cleared");
  test_assert(channel->last_error == NULL, "Error should be cleared");

  /* Test cancel_preview */
  channel->status = CHANNEL_STATUS_PREVIEW;
  channel->preview_mode_enabled = true;
  channel->preview_duration_sec = 60;
  channel->preview_start_time = time(NULL);

  result = channel_cancel_preview(NULL, "id");
  test_assert(!result, "NULL manager should fail");

  result = channel_cancel_preview(manager, NULL);
  test_assert(!result, "NULL channel_id should fail");

  /* Test cancel with wrong status */
  channel->status = CHANNEL_STATUS_ACTIVE;
  result = channel_cancel_preview(manager, channel->channel_id);
  test_assert(!result, "Should fail when not in preview mode");

  /* Test successful cancel */
  channel->status = CHANNEL_STATUS_PREVIEW;
  result = channel_cancel_preview(manager, channel->channel_id);
  test_assert(result, "Should succeed");
  test_assert(channel->preview_mode_enabled == false, "Preview mode should be disabled");

  /* Test preview timeout check */
  channel->preview_mode_enabled = false;
  bool timeout = channel_check_preview_timeout(channel);
  test_assert(!timeout, "Should not timeout when disabled");

  timeout = channel_check_preview_timeout(NULL);
  test_assert(!timeout, "NULL channel should not timeout");

  /* Test with unlimited duration */
  channel->preview_mode_enabled = true;
  channel->preview_duration_sec = 0;
  timeout = channel_check_preview_timeout(channel);
  test_assert(!timeout, "Should not timeout with 0 duration");

  /* Test with elapsed time */
  channel->preview_duration_sec = 1;
  channel->preview_start_time = time(NULL) - 2;
  timeout = channel_check_preview_timeout(channel);
  test_assert(timeout, "Should timeout when time elapsed");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Preview Mode Functions");
  return true;
}

/* Test: channel_duplicate (lines 943-974) */
static bool test_channel_duplicate(void)
{
  test_section_start("Channel Duplicate");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test NULL parameters */
  stream_channel_t *dup = channel_duplicate(NULL, "New Name");
  test_assert(dup == NULL, "NULL source should fail");

  stream_channel_t *channel = channel_manager_create_channel(manager, "Original");
  dup = channel_duplicate(channel, NULL);
  test_assert(dup == NULL, "NULL new_name should fail");

  /* Add outputs and settings to original */
  encoding_settings_t enc = channel_get_default_encoding();
  enc.bitrate = 5000;
  channel_add_output(channel, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_YOUTUBE, "key2", ORIENTATION_VERTICAL, &enc);

  channel->source_orientation = ORIENTATION_HORIZONTAL;
  channel->auto_detect_orientation = false;
  channel->source_width = 1920;
  channel->source_height = 1080;
  channel->auto_start = true;
  channel->auto_reconnect = true;
  channel->reconnect_delay_sec = 15;

  /* Duplicate profile */
  dup = channel_duplicate(channel, "Duplicate");
  test_assert(dup != NULL, "Should duplicate profile");
  test_assert(strcmp(dup->channel_name, "Duplicate") == 0, "Name should match");
  test_assert(strcmp(dup->channel_id, channel->channel_id) != 0, "ID should be different");
  test_assert(dup->output_count == 2, "Should copy outputs");
  test_assert(dup->source_orientation == channel->source_orientation, "Should copy orientation");
  test_assert(dup->source_width == 1920, "Should copy dimensions");
  test_assert(dup->source_height == 1080, "Should copy dimensions");
  test_assert(dup->auto_start == true, "Should copy auto_start");
  test_assert(dup->auto_reconnect == true, "Should copy auto_reconnect");
  test_assert(dup->reconnect_delay_sec == 15, "Should copy reconnect delay");
  test_assert(dup->status == CHANNEL_STATUS_INACTIVE, "Duplicate should be inactive");

  /* Verify outputs were copied */
  test_assert(dup->outputs[0].service == SERVICE_TWITCH, "First output service should match");
  test_assert(strcmp(dup->outputs[0].stream_key, "key1") == 0, "Stream key should be copied");
  test_assert(dup->outputs[0].encoding.bitrate == 5000, "Encoding should be copied");
  test_assert(dup->outputs[0].enabled == channel->outputs[0].enabled, "Enabled state should match");

  /* Clean up duplicate (not managed by manager) */
  bfree(dup->channel_name);
  bfree(dup->channel_id);
  for (size_t i = 0; i < dup->output_count; i++) {
    bfree(dup->outputs[i].service_name);
    bfree(dup->outputs[i].stream_key);
    bfree(dup->outputs[i].rtmp_url);
  }
  bfree(dup->outputs);
  bfree(dup->input_url);
  bfree(dup);

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Channel Duplicate");
  return true;
}

/* Test: Health monitoring functions (lines 992-1248) */
static bool test_health_monitoring_functions(void)
{
  test_section_start("Health Monitoring Functions");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);
  stream_channel_t *channel = channel_manager_create_channel(manager, "Health Test");

  /* Test NULL parameters for channel_check_health */
  bool result = channel_check_health(NULL, api);
  test_assert(!result, "NULL channel should fail");

  result = channel_check_health(channel, NULL);
  test_assert(!result, "NULL api should fail");

  /* Test when profile not active - should return true */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_check_health(channel, api);
  test_assert(result, "Inactive channel should return true");

  /* Test when health monitoring disabled - should return true */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->health_monitoring_enabled = false;
  result = channel_check_health(channel, api);
  test_assert(result, "Disabled monitoring should return true");

  /* Test when no process reference */
  channel->health_monitoring_enabled = true;
  channel->process_reference = NULL;
  result = channel_check_health(channel, api);
  test_assert(!result, "No process reference should fail");

  /* Test channel_reconnect_output NULL parameters */
  result = channel_reconnect_output(NULL, api, 0);
  test_assert(!result, "NULL channel should fail");

  result = channel_reconnect_output(channel, NULL, 0);
  test_assert(!result, "NULL api should fail");

  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  result = channel_reconnect_output(channel, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when profile not active */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_reconnect_output(channel, api, 0);
  test_assert(!result, "Inactive channel should fail");

  /* Test when no process reference */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->process_reference = NULL;
  result = channel_reconnect_output(channel, api, 0);
  test_assert(!result, "No process reference should fail");

  /* Test channel_set_health_monitoring NULL safety */
  channel_set_health_monitoring(NULL, true);  /* Should not crash */

  /* Test enabling health monitoring */
  channel->health_monitoring_enabled = false;
  channel->health_check_interval_sec = 0;
  channel_set_health_monitoring(channel, true);

  test_assert(channel->health_monitoring_enabled == true, "Should be enabled");
  test_assert(channel->health_check_interval_sec == 30, "Should set default interval");
  test_assert(channel->failure_threshold == 3, "Should set default threshold");
  test_assert(channel->max_reconnect_attempts == 5, "Should set default max attempts");
  test_assert(channel->outputs[0].auto_reconnect_enabled == true, "Output should have auto-reconnect");

  /* Test disabling health monitoring */
  channel_set_health_monitoring(channel, false);
  test_assert(channel->health_monitoring_enabled == false, "Should be disabled");
  test_assert(channel->outputs[0].auto_reconnect_enabled == false, "Output auto-reconnect should be disabled");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Health Monitoring Functions");
  return true;
}

/* Test: Failover functions (lines 1610-1778) */
static bool test_failover_functions(void)
{
  test_section_start("Failover Functions");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);
  stream_channel_t *channel = channel_manager_create_channel(manager, "Failover Test");

  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "primary", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_TWITCH, "backup", ORIENTATION_HORIZONTAL, &enc);

  /* Set backup relationship */
  channel_set_output_backup(channel, 0, 1);

  /* Test channel_trigger_failover NULL parameters */
  bool result = channel_trigger_failover(NULL, api, 0);
  test_assert(!result, "NULL channel should fail");

  result = channel_trigger_failover(channel, NULL, 0);
  test_assert(!result, "NULL api should fail");

  result = channel_trigger_failover(channel, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when output has no backup */
  channel_add_output(channel, SERVICE_YOUTUBE, "no_backup", ORIENTATION_HORIZONTAL, &enc);
  result = channel_trigger_failover(channel, api, 2);
  test_assert(!result, "No backup should fail");

  /* Test when already failed over */
  channel->outputs[0].failover_active = true;
  result = channel_trigger_failover(channel, api, 0);
  test_assert(result, "Already active failover should return true");

  /* Test triggering failover when inactive */
  channel->outputs[0].failover_active = false;
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_trigger_failover(channel, api, 0);
  test_assert(result, "Should succeed but not modify outputs when inactive");
  test_assert(channel->outputs[0].failover_active == true, "Failover should be marked active");
  test_assert(channel->outputs[1].failover_active == true, "Backup failover should be marked active");

  /* Test channel_restore_primary NULL parameters */
  result = channel_restore_primary(NULL, api, 0);
  test_assert(!result, "NULL channel should fail");

  result = channel_restore_primary(channel, NULL, 0);
  test_assert(!result, "NULL api should fail");

  result = channel_restore_primary(channel, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when no backup configured */
  result = channel_restore_primary(channel, api, 2);
  test_assert(!result, "No backup should fail");

  /* Test when no failover active */
  channel->outputs[0].failover_active = false;
  channel->outputs[1].failover_active = false;
  result = channel_restore_primary(channel, api, 0);
  test_assert(result, "No active failover should return true (no-op)");

  /* Test successful restore when inactive */
  channel->outputs[0].failover_active = true;
  channel->outputs[1].failover_active = true;
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_restore_primary(channel, api, 0);
  test_assert(result, "Should succeed");
  test_assert(channel->outputs[0].failover_active == false, "Primary failover should be cleared");
  test_assert(channel->outputs[1].failover_active == false, "Backup failover should be cleared");
  test_assert(channel->outputs[0].consecutive_failures == 0, "Failures should be reset");

  /* Test channel_check_failover NULL parameters */
  result = channel_check_failover(NULL, api);
  test_assert(!result, "NULL channel should fail");

  result = channel_check_failover(channel, NULL);
  test_assert(!result, "NULL api should fail");

  /* Test when profile not active */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_check_failover(channel, api);
  test_assert(result, "Inactive channel should return true");

  /* Test with active channel - failover triggers but API calls fail in test env */
  channel->status = CHANNEL_STATUS_ACTIVE;
  channel->outputs[0].failover_active = false;
  channel->outputs[0].connected = false;
  channel->outputs[0].consecutive_failures = 5;
  channel->failure_threshold = 3;

  result = channel_check_failover(channel, api);
  /* Returns false because channel_trigger_failover's API calls fail without a real server */
  test_assert(!result, "Active profile failover fails without real API connection");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Failover Functions");
  return true;
}

/* Test: Bulk operations (lines 1784-2048) */
static bool test_bulk_operations(void)
{
  test_section_start("Bulk Operations");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);
  stream_channel_t *channel = channel_manager_create_channel(manager, "Bulk Test");

  encoding_settings_t enc = channel_get_default_encoding();
  channel_add_output(channel, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);
  channel_add_output(channel, SERVICE_CUSTOM, "key4", ORIENTATION_HORIZONTAL, &enc);

  /* Set one as backup to test skipping */
  channel_set_output_backup(channel, 0, 1);

  size_t indices[] = {0, 2};

  /* Test channel_bulk_enable_outputs NULL parameters */
  bool result = channel_bulk_enable_outputs(NULL, api, indices, 2, true);
  test_assert(!result, "NULL channel should fail");

  result = channel_bulk_enable_outputs(channel, api, NULL, 2, true);
  test_assert(!result, "NULL indices should fail");

  result = channel_bulk_enable_outputs(channel, api, indices, 0, true);
  test_assert(!result, "Zero count should fail");

  /* Test with invalid index */
  size_t invalid_indices[] = {0, 999};
  result = channel_bulk_enable_outputs(channel, api, invalid_indices, 2, false);
  test_assert(!result, "Invalid index should cause failure");

  /* Test trying to enable backup output */
  size_t backup_indices[] = {1};
  result = channel_bulk_enable_outputs(channel, api, backup_indices, 1, true);
  test_assert(!result, "Cannot directly enable backup output");

  /* Test successful bulk enable/disable */
  size_t valid_indices[] = {0, 2};
  result = channel_bulk_enable_outputs(channel, NULL, valid_indices, 2, false);
  test_assert(result, "Should succeed");
  test_assert(channel->outputs[0].enabled == false, "Dest 0 should be disabled");
  test_assert(channel->outputs[2].enabled == false, "Dest 2 should be disabled");

  /* Test channel_bulk_delete_outputs */
  result = channel_bulk_delete_outputs(NULL, indices, 2);
  test_assert(!result, "NULL channel should fail");

  result = channel_bulk_delete_outputs(channel, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = channel_bulk_delete_outputs(channel, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test deleting with backup relationships */
  size_t delete_indices[] = {3};  /* Delete output without backup */
  result = channel_bulk_delete_outputs(channel, delete_indices, 1);
  test_assert(result, "Should succeed");
  test_assert(channel->output_count == 3, "Should have 3 outputs");

  /* Test channel_bulk_update_encoding */
  encoding_settings_t new_enc = channel_get_default_encoding();
  new_enc.bitrate = 8000;

  result = channel_bulk_update_encoding(NULL, api, indices, 2, &new_enc);
  test_assert(!result, "NULL channel should fail");

  result = channel_bulk_update_encoding(channel, api, NULL, 2, &new_enc);
  test_assert(!result, "NULL indices should fail");

  result = channel_bulk_update_encoding(channel, api, indices, 0, &new_enc);
  test_assert(!result, "Zero count should fail");

  result = channel_bulk_update_encoding(channel, api, indices, 2, NULL);
  test_assert(!result, "NULL encoding should fail");

  size_t update_indices[] = {0, 2};
  result = channel_bulk_update_encoding(channel, NULL, update_indices, 2, &new_enc);
  test_assert(result, "Should succeed when inactive");

  /* Test channel_bulk_start_outputs */
  result = channel_bulk_start_outputs(NULL, api, indices, 2);
  test_assert(!result, "NULL channel should fail");

  result = channel_bulk_start_outputs(channel, NULL, indices, 2);
  test_assert(!result, "NULL api should fail");

  result = channel_bulk_start_outputs(channel, api, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = channel_bulk_start_outputs(channel, api, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test when profile not active */
  channel->status = CHANNEL_STATUS_INACTIVE;
  result = channel_bulk_start_outputs(channel, api, indices, 2);
  test_assert(!result, "Should fail when profile not active");

  /* Test channel_bulk_stop_outputs */
  result = channel_bulk_stop_outputs(NULL, api, indices, 2);
  test_assert(!result, "NULL channel should fail");

  result = channel_bulk_stop_outputs(channel, NULL, indices, 2);
  test_assert(!result, "NULL api should fail");

  result = channel_bulk_stop_outputs(channel, api, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = channel_bulk_stop_outputs(channel, api, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test when profile not active */
  result = channel_bulk_stop_outputs(channel, api, indices, 2);
  test_assert(!result, "Should fail when profile not active");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Bulk Operations");
  return true;
}

/* Test: Edge cases and additional NULL checks */
static bool test_additional_edge_cases(void)
{
  test_section_start("Additional Edge Cases");

  restreamer_api_t *api = create_test_api();
  channel_manager_t *manager = channel_manager_create(api);

  /* Test channel_update_stats with NULL process reference */
  stream_channel_t *channel = channel_manager_create_channel(manager, "Stats Test");
  bool result = channel_update_stats(channel, api);
  test_assert(!result, "No process reference should fail");

  channel->process_reference = bstrdup("test_ref");
  result = channel_update_stats(channel, api);
  test_assert(result, "Should succeed (no-op in current implementation)");

  /* Test channel_get_default_encoding */
  encoding_settings_t enc = channel_get_default_encoding();
  test_assert(enc.width == 0, "Default width should be 0");
  test_assert(enc.height == 0, "Default height should be 0");
  test_assert(enc.bitrate == 0, "Default bitrate should be 0");
  test_assert(enc.fps_num == 0, "Default fps_num should be 0");
  test_assert(enc.fps_den == 0, "Default fps_den should be 0");
  test_assert(enc.audio_bitrate == 0, "Default audio_bitrate should be 0");
  test_assert(enc.audio_track == 0, "Default audio_track should be 0");
  test_assert(enc.max_bandwidth == 0, "Default max_bandwidth should be 0");
  test_assert(enc.low_latency == false, "Default low_latency should be false");

  /* Test channel_generate_id uniqueness */
  char *id1 = channel_generate_id();
  char *id2 = channel_generate_id();
  char *id3 = channel_generate_id();

  test_assert(id1 != NULL, "ID should be generated");
  test_assert(id2 != NULL, "ID should be generated");
  test_assert(id3 != NULL, "ID should be generated");
  test_assert(strcmp(id1, id2) != 0, "IDs should be unique");
  test_assert(strcmp(id2, id3) != 0, "IDs should be unique");

  bfree(id1);
  bfree(id2);
  bfree(id3);

  /* Test channel_manager_get_active_count */
  size_t count = channel_manager_get_active_count(NULL);
  test_assert(count == 0, "NULL manager should return 0");

  count = channel_manager_get_active_count(manager);
  test_assert(count == 0, "No active channels should return 0");

  channel->status = CHANNEL_STATUS_ACTIVE;
  count = channel_manager_get_active_count(manager);
  test_assert(count == 1, "Should count active channel");

  /* Test channel_add_output with NULL encoding */
  stream_channel_t *channel2 = channel_manager_create_channel(manager, "Null Encoding Test");
  result = channel_add_output(channel2, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, NULL);
  test_assert(result, "Should succeed with NULL encoding (uses default)");
  test_assert(channel2->output_count == 1, "Should have 1 output");
  test_assert(channel2->outputs[0].encoding.bitrate == 0, "Should use default encoding");

  channel_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Additional Edge Cases");
  return true;
}

/* Test suite runner */
bool run_channel_coverage_tests(void)
{
  test_suite_start("Channel Coverage Tests");

  bool result = true;

  test_start("Channel manager destroy with active channels");
  result &= test_channel_manager_destroy_with_active_profiles();
  test_end();

  test_start("Channel manager delete active channel");
  result &= test_channel_manager_delete_active_profile();
  test_end();

  test_start("Channel update output encoding live");
  result &= test_channel_update_output_encoding_live();
  test_end();

  test_start("Output profile start error paths");
  result &= test_stream_channel_start_error_paths();
  test_end();

  test_start("Output profile stop with process reference");
  result &= test_stream_channel_stop_with_process();
  test_end();

  test_start("Channel restart");
  result &= test_channel_restart();
  test_end();

  test_start("Channel manager bulk start/stop");
  result &= test_channel_manager_bulk_start_stop();
  test_end();

  test_start("Preview mode functions");
  result &= test_preview_mode_functions();
  test_end();

  test_start("Channel duplicate");
  result &= test_channel_duplicate();
  test_end();

  test_start("Health monitoring functions");
  result &= test_health_monitoring_functions();
  test_end();

  test_start("Failover functions");
  result &= test_failover_functions();
  test_end();

  test_start("Bulk operations");
  result &= test_bulk_operations();
  test_end();

  test_start("Additional edge cases");
  result &= test_additional_edge_cases();
  test_end();

  test_suite_end("Channel Coverage Tests", result);
  return result;
}
