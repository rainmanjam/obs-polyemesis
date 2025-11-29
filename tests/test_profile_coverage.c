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

#include "restreamer-output-profile.h"
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

/* Test: profile_manager_destroy with active profiles (lines 26-71) */
static bool test_profile_manager_destroy_with_active_profiles(void)
{
  test_section_start("Manager Destroy with Active Profiles");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Create profile with destinations */
  output_profile_t *profile = profile_manager_create_profile(manager, "Active Profile");
  encoding_settings_t enc = profile_get_default_encoding();
  enc.bitrate = 5000;

  profile_add_destination(profile, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);

  /* Mark profile as active to test stop path in destroy */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->process_reference = bstrdup("test_process_ref");

  test_assert(manager->profile_count == 1, "Manager should have 1 profile");
  test_assert(profile->destination_count == 2, "Profile should have 2 destinations");

  /* Destroy manager - should stop active profile and free all resources */
  profile_manager_destroy(manager);

  /* Test NULL manager doesn't crash */
  profile_manager_destroy(NULL);

  restreamer_api_destroy(api);

  test_section_end("Manager Destroy with Active Profiles");
  return true;
}

/* Test: profile_manager_delete_profile with active profile (lines 122-171) */
static bool test_profile_manager_delete_active_profile(void)
{
  test_section_start("Delete Active Profile");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Create profile and set it to active */
  output_profile_t *profile = profile_manager_create_profile(manager, "To Delete");
  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  profile->status = PROFILE_STATUS_ACTIVE;
  profile->process_reference = bstrdup("delete_test_ref");

  char *profile_id = bstrdup(profile->profile_id);

  /* Delete active profile - should stop it first */
  bool deleted = profile_manager_delete_profile(manager, profile_id);
  test_assert(deleted, "Should delete active profile");
  test_assert(manager->profile_count == 0, "Manager should have 0 profiles");
  test_assert(manager->profiles == NULL, "Profiles array should be NULL after deleting last profile");

  bfree(profile_id);

  /* Test NULL parameters */
  deleted = profile_manager_delete_profile(NULL, "id");
  test_assert(!deleted, "NULL manager should fail");

  deleted = profile_manager_delete_profile(manager, NULL);
  test_assert(!deleted, "NULL profile_id should fail");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Delete Active Profile");
  return true;
}

/* Test: profile_update_destination_encoding_live (lines 308-389) */
static bool test_profile_update_destination_encoding_live(void)
{
  test_section_start("Update Destination Encoding Live");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);
  output_profile_t *profile = profile_manager_create_profile(manager, "Live Update Test");

  encoding_settings_t enc = profile_get_default_encoding();
  enc.bitrate = 5000;
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Test with inactive profile - should fail */
  encoding_settings_t new_enc = enc;
  new_enc.bitrate = 8000;

  bool updated = profile_update_destination_encoding_live(profile, api, 0, &new_enc);
  test_assert(!updated, "Should fail when profile is not active");

  /* Test with active profile but no process reference - should fail */
  profile->status = PROFILE_STATUS_ACTIVE;
  updated = profile_update_destination_encoding_live(profile, api, 0, &new_enc);
  test_assert(!updated, "Should fail when no process reference");

  /* Test with process reference but process not found */
  profile->process_reference = bstrdup("nonexistent_process");
  updated = profile_update_destination_encoding_live(profile, api, 0, &new_enc);
  test_assert(!updated, "Should fail when process not found");

  /* Test NULL parameters */
  updated = profile_update_destination_encoding_live(NULL, api, 0, &new_enc);
  test_assert(!updated, "NULL profile should fail");

  updated = profile_update_destination_encoding_live(profile, NULL, 0, &new_enc);
  test_assert(!updated, "NULL api should fail");

  updated = profile_update_destination_encoding_live(profile, api, 0, NULL);
  test_assert(!updated, "NULL encoding should fail");

  updated = profile_update_destination_encoding_live(profile, api, 999, &new_enc);
  test_assert(!updated, "Invalid index should fail");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Update Destination Encoding Live");
  return true;
}

/* Test: output_profile_start error paths (lines 403-522) */
static bool test_output_profile_start_error_paths(void)
{
  test_section_start("Output Profile Start Error Paths");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test NULL parameters */
  bool started = output_profile_start(NULL, "id");
  test_assert(!started, "NULL manager should fail");

  started = output_profile_start(manager, NULL);
  test_assert(!started, "NULL profile_id should fail");

  /* Test non-existent profile */
  started = output_profile_start(manager, "nonexistent");
  test_assert(!started, "Non-existent profile should fail");

  /* Create profile and test already active */
  output_profile_t *profile = profile_manager_create_profile(manager, "Start Test");
  profile->status = PROFILE_STATUS_ACTIVE;

  started = output_profile_start(manager, profile->profile_id);
  test_assert(started, "Already active profile should return true (no-op)");

  /* Test no enabled destinations */
  profile->status = PROFILE_STATUS_INACTIVE;
  started = output_profile_start(manager, profile->profile_id);
  test_assert(!started, "No enabled destinations should fail");
  test_assert(profile->status == PROFILE_STATUS_ERROR, "Profile should be in error state");
  test_assert(profile->last_error != NULL, "Should have error message");
  test_assert(strstr(profile->last_error, "No enabled destinations") != NULL,
              "Error message should mention destinations");

  /* Test with destinations but no input URL */
  profile->status = PROFILE_STATUS_INACTIVE;
  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  bfree(profile->input_url);
  profile->input_url = bstrdup("");

  started = output_profile_start(manager, profile->profile_id);
  test_assert(!started, "Empty input URL should fail");
  test_assert(profile->status == PROFILE_STATUS_ERROR, "Should be in error state");
  test_assert(profile->last_error != NULL, "Should have error message");

  /* Test with no API connection */
  profile_manager_t *manager_no_api = profile_manager_create(NULL);
  output_profile_t *profile2 = profile_manager_create_profile(manager_no_api, "No API Test");
  profile_add_destination(profile2, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  started = output_profile_start(manager_no_api, profile2->profile_id);
  test_assert(!started, "No API connection should fail");
  test_assert(profile2->status == PROFILE_STATUS_ERROR, "Should be in error state");

  profile_manager_destroy(manager_no_api);
  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Output Profile Start Error Paths");
  return true;
}

/* Test: output_profile_stop with process reference (lines 524-567) */
static bool test_output_profile_stop_with_process(void)
{
  test_section_start("Output Profile Stop with Process");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);
  output_profile_t *profile = profile_manager_create_profile(manager, "Stop Test");

  /* Test NULL parameters */
  bool stopped = output_profile_stop(NULL, "id");
  test_assert(!stopped, "NULL manager should fail");

  stopped = output_profile_stop(manager, NULL);
  test_assert(!stopped, "NULL profile_id should fail");

  /* Test non-existent profile */
  stopped = output_profile_stop(manager, "nonexistent");
  test_assert(!stopped, "Non-existent profile should fail");

  /* Test already inactive profile */
  profile->status = PROFILE_STATUS_INACTIVE;
  stopped = output_profile_stop(manager, profile->profile_id);
  test_assert(stopped, "Already inactive should succeed (no-op)");

  /* Test stopping with process reference */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->process_reference = bstrdup("test_process_ref");

  stopped = output_profile_stop(manager, profile->profile_id);
  test_assert(stopped, "Should stop profile");
  test_assert(profile->status == PROFILE_STATUS_INACTIVE, "Should be inactive");
  test_assert(profile->process_reference == NULL, "Process reference should be cleared");
  test_assert(profile->last_error == NULL, "Error should be cleared");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Output Profile Stop with Process");
  return true;
}

/* Test: profile_restart (lines 569-572) */
static bool test_profile_restart(void)
{
  test_section_start("Profile Restart");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test NULL parameters */
  bool restarted = profile_restart(NULL, "id");
  test_assert(!restarted, "NULL manager should fail");

  restarted = profile_restart(manager, NULL);
  test_assert(!restarted, "NULL profile_id should fail");

  /* Create profile */
  output_profile_t *profile = profile_manager_create_profile(manager, "Restart Test");
  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Set as active with process reference */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->process_reference = bstrdup("restart_ref");

  /* Restart should stop then start */
  restarted = profile_restart(manager, profile->profile_id);
  test_assert(!restarted, "Restart should fail on start (no actual API)");
  test_assert(profile->status == PROFILE_STATUS_ERROR, "Should be in error state after failed restart");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Profile Restart");
  return true;
}

/* Test: profile_manager_start_all and stop_all (lines 574-610) */
static bool test_profile_manager_bulk_start_stop(void)
{
  test_section_start("Profile Manager Bulk Start/Stop");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test NULL manager */
  bool result = profile_manager_start_all(NULL);
  test_assert(!result, "NULL manager should fail start_all");

  result = profile_manager_stop_all(NULL);
  test_assert(!result, "NULL manager should fail stop_all");

  /* Test with empty manager */
  result = profile_manager_start_all(manager);
  test_assert(result, "Empty manager start_all should succeed");

  result = profile_manager_stop_all(manager);
  test_assert(result, "Empty manager stop_all should succeed");

  /* Create profiles */
  output_profile_t *profile1 = profile_manager_create_profile(manager, "Profile 1");
  output_profile_t *profile2 = profile_manager_create_profile(manager, "Profile 2");
  output_profile_t *profile3 = profile_manager_create_profile(manager, "Profile 3");

  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile1, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile2, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile3, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);

  /* Set auto_start flags */
  profile1->auto_start = true;
  profile2->auto_start = false;  /* This one should not start */
  profile3->auto_start = true;

  /* Start all - should attempt to start profiles with auto_start */
  result = profile_manager_start_all(manager);
  test_assert(!result, "start_all should fail (no real API)");

  /* Set profiles to active for testing stop_all */
  profile1->status = PROFILE_STATUS_ACTIVE;
  profile1->process_reference = bstrdup("proc1");
  profile2->status = PROFILE_STATUS_ACTIVE;
  profile2->process_reference = bstrdup("proc2");
  profile3->status = PROFILE_STATUS_INACTIVE;

  /* Stop all */
  result = profile_manager_stop_all(manager);
  test_assert(result, "stop_all should succeed");
  test_assert(profile1->status == PROFILE_STATUS_INACTIVE, "Profile 1 should be stopped");
  test_assert(profile2->status == PROFILE_STATUS_INACTIVE, "Profile 2 should be stopped");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Profile Manager Bulk Start/Stop");
  return true;
}

/* Test: Preview mode functions (lines 631-746) */
static bool test_preview_mode_functions(void)
{
  test_section_start("Preview Mode Functions");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test NULL parameters for start_preview */
  bool result = output_profile_start_preview(NULL, "id", 60);
  test_assert(!result, "NULL manager should fail");

  result = output_profile_start_preview(manager, NULL, 60);
  test_assert(!result, "NULL profile_id should fail");

  /* Test non-existent profile */
  result = output_profile_start_preview(manager, "nonexistent", 60);
  test_assert(!result, "Non-existent profile should fail");

  /* Create profile */
  output_profile_t *profile = profile_manager_create_profile(manager, "Preview Test");
  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  /* Test starting preview on non-inactive profile */
  profile->status = PROFILE_STATUS_ACTIVE;
  result = output_profile_start_preview(manager, profile->profile_id, 120);
  test_assert(!result, "Should fail when profile not inactive");

  /* Test starting preview on inactive profile */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = output_profile_start_preview(manager, profile->profile_id, 180);
  test_assert(!result, "Should fail (no real API)");
  test_assert(profile->preview_mode_enabled == false, "Preview mode should be disabled after failure");

  /* Manually set preview mode for further testing */
  profile->status = PROFILE_STATUS_PREVIEW;
  profile->preview_mode_enabled = true;
  profile->preview_duration_sec = 60;
  profile->preview_start_time = time(NULL);

  /* Test preview_to_live */
  result = output_profile_preview_to_live(NULL, "id");
  test_assert(!result, "NULL manager should fail");

  result = output_profile_preview_to_live(manager, NULL);
  test_assert(!result, "NULL profile_id should fail");

  result = output_profile_preview_to_live(manager, "nonexistent");
  test_assert(!result, "Non-existent profile should fail");

  /* Test preview_to_live with wrong status */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = output_profile_preview_to_live(manager, profile->profile_id);
  test_assert(!result, "Should fail when not in preview mode");

  /* Test successful preview_to_live */
  profile->status = PROFILE_STATUS_PREVIEW;
  result = output_profile_preview_to_live(manager, profile->profile_id);
  test_assert(result, "Should succeed");
  test_assert(profile->status == PROFILE_STATUS_ACTIVE, "Should be active");
  test_assert(profile->preview_mode_enabled == false, "Preview mode should be disabled");
  test_assert(profile->preview_duration_sec == 0, "Duration should be cleared");
  test_assert(profile->last_error == NULL, "Error should be cleared");

  /* Test cancel_preview */
  profile->status = PROFILE_STATUS_PREVIEW;
  profile->preview_mode_enabled = true;
  profile->preview_duration_sec = 60;
  profile->preview_start_time = time(NULL);

  result = output_profile_cancel_preview(NULL, "id");
  test_assert(!result, "NULL manager should fail");

  result = output_profile_cancel_preview(manager, NULL);
  test_assert(!result, "NULL profile_id should fail");

  /* Test cancel with wrong status */
  profile->status = PROFILE_STATUS_ACTIVE;
  result = output_profile_cancel_preview(manager, profile->profile_id);
  test_assert(!result, "Should fail when not in preview mode");

  /* Test successful cancel */
  profile->status = PROFILE_STATUS_PREVIEW;
  result = output_profile_cancel_preview(manager, profile->profile_id);
  test_assert(result, "Should succeed");
  test_assert(profile->preview_mode_enabled == false, "Preview mode should be disabled");

  /* Test preview timeout check */
  profile->preview_mode_enabled = false;
  bool timeout = output_profile_check_preview_timeout(profile);
  test_assert(!timeout, "Should not timeout when disabled");

  timeout = output_profile_check_preview_timeout(NULL);
  test_assert(!timeout, "NULL profile should not timeout");

  /* Test with unlimited duration */
  profile->preview_mode_enabled = true;
  profile->preview_duration_sec = 0;
  timeout = output_profile_check_preview_timeout(profile);
  test_assert(!timeout, "Should not timeout with 0 duration");

  /* Test with elapsed time */
  profile->preview_duration_sec = 1;
  profile->preview_start_time = time(NULL) - 2;
  timeout = output_profile_check_preview_timeout(profile);
  test_assert(timeout, "Should timeout when time elapsed");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Preview Mode Functions");
  return true;
}

/* Test: profile_duplicate (lines 943-974) */
static bool test_profile_duplicate(void)
{
  test_section_start("Profile Duplicate");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test NULL parameters */
  output_profile_t *dup = profile_duplicate(NULL, "New Name");
  test_assert(dup == NULL, "NULL source should fail");

  output_profile_t *profile = profile_manager_create_profile(manager, "Original");
  dup = profile_duplicate(profile, NULL);
  test_assert(dup == NULL, "NULL new_name should fail");

  /* Add destinations and settings to original */
  encoding_settings_t enc = profile_get_default_encoding();
  enc.bitrate = 5000;
  profile_add_destination(profile, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_YOUTUBE, "key2", ORIENTATION_VERTICAL, &enc);

  profile->source_orientation = ORIENTATION_HORIZONTAL;
  profile->auto_detect_orientation = false;
  profile->source_width = 1920;
  profile->source_height = 1080;
  profile->auto_start = true;
  profile->auto_reconnect = true;
  profile->reconnect_delay_sec = 15;

  /* Duplicate profile */
  dup = profile_duplicate(profile, "Duplicate");
  test_assert(dup != NULL, "Should duplicate profile");
  test_assert(strcmp(dup->profile_name, "Duplicate") == 0, "Name should match");
  test_assert(strcmp(dup->profile_id, profile->profile_id) != 0, "ID should be different");
  test_assert(dup->destination_count == 2, "Should copy destinations");
  test_assert(dup->source_orientation == profile->source_orientation, "Should copy orientation");
  test_assert(dup->source_width == 1920, "Should copy dimensions");
  test_assert(dup->source_height == 1080, "Should copy dimensions");
  test_assert(dup->auto_start == true, "Should copy auto_start");
  test_assert(dup->auto_reconnect == true, "Should copy auto_reconnect");
  test_assert(dup->reconnect_delay_sec == 15, "Should copy reconnect delay");
  test_assert(dup->status == PROFILE_STATUS_INACTIVE, "Duplicate should be inactive");

  /* Verify destinations were copied */
  test_assert(dup->destinations[0].service == SERVICE_TWITCH, "First destination service should match");
  test_assert(strcmp(dup->destinations[0].stream_key, "key1") == 0, "Stream key should be copied");
  test_assert(dup->destinations[0].encoding.bitrate == 5000, "Encoding should be copied");
  test_assert(dup->destinations[0].enabled == profile->destinations[0].enabled, "Enabled state should match");

  /* Clean up duplicate (not managed by manager) */
  bfree(dup->profile_name);
  bfree(dup->profile_id);
  for (size_t i = 0; i < dup->destination_count; i++) {
    bfree(dup->destinations[i].service_name);
    bfree(dup->destinations[i].stream_key);
    bfree(dup->destinations[i].rtmp_url);
  }
  bfree(dup->destinations);
  bfree(dup->input_url);
  bfree(dup);

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Profile Duplicate");
  return true;
}

/* Test: Health monitoring functions (lines 992-1248) */
static bool test_health_monitoring_functions(void)
{
  test_section_start("Health Monitoring Functions");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);
  output_profile_t *profile = profile_manager_create_profile(manager, "Health Test");

  /* Test NULL parameters for profile_check_health */
  bool result = profile_check_health(NULL, api);
  test_assert(!result, "NULL profile should fail");

  result = profile_check_health(profile, NULL);
  test_assert(!result, "NULL api should fail");

  /* Test when profile not active - should return true */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_check_health(profile, api);
  test_assert(result, "Inactive profile should return true");

  /* Test when health monitoring disabled - should return true */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->health_monitoring_enabled = false;
  result = profile_check_health(profile, api);
  test_assert(result, "Disabled monitoring should return true");

  /* Test when no process reference */
  profile->health_monitoring_enabled = true;
  profile->process_reference = NULL;
  result = profile_check_health(profile, api);
  test_assert(!result, "No process reference should fail");

  /* Test profile_reconnect_destination NULL parameters */
  result = profile_reconnect_destination(NULL, api, 0);
  test_assert(!result, "NULL profile should fail");

  result = profile_reconnect_destination(profile, NULL, 0);
  test_assert(!result, "NULL api should fail");

  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, &enc);

  result = profile_reconnect_destination(profile, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when profile not active */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_reconnect_destination(profile, api, 0);
  test_assert(!result, "Inactive profile should fail");

  /* Test when no process reference */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->process_reference = NULL;
  result = profile_reconnect_destination(profile, api, 0);
  test_assert(!result, "No process reference should fail");

  /* Test profile_set_health_monitoring NULL safety */
  profile_set_health_monitoring(NULL, true);  /* Should not crash */

  /* Test enabling health monitoring */
  profile->health_monitoring_enabled = false;
  profile->health_check_interval_sec = 0;
  profile_set_health_monitoring(profile, true);

  test_assert(profile->health_monitoring_enabled == true, "Should be enabled");
  test_assert(profile->health_check_interval_sec == 30, "Should set default interval");
  test_assert(profile->failure_threshold == 3, "Should set default threshold");
  test_assert(profile->max_reconnect_attempts == 5, "Should set default max attempts");
  test_assert(profile->destinations[0].auto_reconnect_enabled == true, "Destination should have auto-reconnect");

  /* Test disabling health monitoring */
  profile_set_health_monitoring(profile, false);
  test_assert(profile->health_monitoring_enabled == false, "Should be disabled");
  test_assert(profile->destinations[0].auto_reconnect_enabled == false, "Destination auto-reconnect should be disabled");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Health Monitoring Functions");
  return true;
}

/* Test: Failover functions (lines 1610-1778) */
static bool test_failover_functions(void)
{
  test_section_start("Failover Functions");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);
  output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "primary", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_TWITCH, "backup", ORIENTATION_HORIZONTAL, &enc);

  /* Set backup relationship */
  profile_set_destination_backup(profile, 0, 1);

  /* Test profile_trigger_failover NULL parameters */
  bool result = profile_trigger_failover(NULL, api, 0);
  test_assert(!result, "NULL profile should fail");

  result = profile_trigger_failover(profile, NULL, 0);
  test_assert(!result, "NULL api should fail");

  result = profile_trigger_failover(profile, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when destination has no backup */
  profile_add_destination(profile, SERVICE_YOUTUBE, "no_backup", ORIENTATION_HORIZONTAL, &enc);
  result = profile_trigger_failover(profile, api, 2);
  test_assert(!result, "No backup should fail");

  /* Test when already failed over */
  profile->destinations[0].failover_active = true;
  result = profile_trigger_failover(profile, api, 0);
  test_assert(result, "Already active failover should return true");

  /* Test triggering failover when inactive */
  profile->destinations[0].failover_active = false;
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_trigger_failover(profile, api, 0);
  test_assert(result, "Should succeed but not modify outputs when inactive");
  test_assert(profile->destinations[0].failover_active == true, "Failover should be marked active");
  test_assert(profile->destinations[1].failover_active == true, "Backup failover should be marked active");

  /* Test profile_restore_primary NULL parameters */
  result = profile_restore_primary(NULL, api, 0);
  test_assert(!result, "NULL profile should fail");

  result = profile_restore_primary(profile, NULL, 0);
  test_assert(!result, "NULL api should fail");

  result = profile_restore_primary(profile, api, 999);
  test_assert(!result, "Invalid index should fail");

  /* Test when no backup configured */
  result = profile_restore_primary(profile, api, 2);
  test_assert(!result, "No backup should fail");

  /* Test when no failover active */
  profile->destinations[0].failover_active = false;
  profile->destinations[1].failover_active = false;
  result = profile_restore_primary(profile, api, 0);
  test_assert(result, "No active failover should return true (no-op)");

  /* Test successful restore when inactive */
  profile->destinations[0].failover_active = true;
  profile->destinations[1].failover_active = true;
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_restore_primary(profile, api, 0);
  test_assert(result, "Should succeed");
  test_assert(profile->destinations[0].failover_active == false, "Primary failover should be cleared");
  test_assert(profile->destinations[1].failover_active == false, "Backup failover should be cleared");
  test_assert(profile->destinations[0].consecutive_failures == 0, "Failures should be reset");

  /* Test profile_check_failover NULL parameters */
  result = profile_check_failover(NULL, api);
  test_assert(!result, "NULL profile should fail");

  result = profile_check_failover(profile, NULL);
  test_assert(!result, "NULL api should fail");

  /* Test when profile not active */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_check_failover(profile, api);
  test_assert(result, "Inactive profile should return true");

  /* Test with active profile - failover triggers but API calls fail in test env */
  profile->status = PROFILE_STATUS_ACTIVE;
  profile->destinations[0].failover_active = false;
  profile->destinations[0].connected = false;
  profile->destinations[0].consecutive_failures = 5;
  profile->failure_threshold = 3;

  result = profile_check_failover(profile, api);
  /* Returns false because profile_trigger_failover's API calls fail without a real server */
  test_assert(!result, "Active profile failover fails without real API connection");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Failover Functions");
  return true;
}

/* Test: Bulk operations (lines 1784-2048) */
static bool test_bulk_operations(void)
{
  test_section_start("Bulk Operations");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);
  output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

  encoding_settings_t enc = profile_get_default_encoding();
  profile_add_destination(profile, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);
  profile_add_destination(profile, SERVICE_CUSTOM, "key4", ORIENTATION_HORIZONTAL, &enc);

  /* Set one as backup to test skipping */
  profile_set_destination_backup(profile, 0, 1);

  size_t indices[] = {0, 2};

  /* Test profile_bulk_enable_destinations NULL parameters */
  bool result = profile_bulk_enable_destinations(NULL, api, indices, 2, true);
  test_assert(!result, "NULL profile should fail");

  result = profile_bulk_enable_destinations(profile, api, NULL, 2, true);
  test_assert(!result, "NULL indices should fail");

  result = profile_bulk_enable_destinations(profile, api, indices, 0, true);
  test_assert(!result, "Zero count should fail");

  /* Test with invalid index */
  size_t invalid_indices[] = {0, 999};
  result = profile_bulk_enable_destinations(profile, api, invalid_indices, 2, false);
  test_assert(!result, "Invalid index should cause failure");

  /* Test trying to enable backup destination */
  size_t backup_indices[] = {1};
  result = profile_bulk_enable_destinations(profile, api, backup_indices, 1, true);
  test_assert(!result, "Cannot directly enable backup destination");

  /* Test successful bulk enable/disable */
  size_t valid_indices[] = {0, 2};
  result = profile_bulk_enable_destinations(profile, NULL, valid_indices, 2, false);
  test_assert(result, "Should succeed");
  test_assert(profile->destinations[0].enabled == false, "Dest 0 should be disabled");
  test_assert(profile->destinations[2].enabled == false, "Dest 2 should be disabled");

  /* Test profile_bulk_delete_destinations */
  result = profile_bulk_delete_destinations(NULL, indices, 2);
  test_assert(!result, "NULL profile should fail");

  result = profile_bulk_delete_destinations(profile, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = profile_bulk_delete_destinations(profile, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test deleting with backup relationships */
  size_t delete_indices[] = {3};  /* Delete destination without backup */
  result = profile_bulk_delete_destinations(profile, delete_indices, 1);
  test_assert(result, "Should succeed");
  test_assert(profile->destination_count == 3, "Should have 3 destinations");

  /* Test profile_bulk_update_encoding */
  encoding_settings_t new_enc = profile_get_default_encoding();
  new_enc.bitrate = 8000;

  result = profile_bulk_update_encoding(NULL, api, indices, 2, &new_enc);
  test_assert(!result, "NULL profile should fail");

  result = profile_bulk_update_encoding(profile, api, NULL, 2, &new_enc);
  test_assert(!result, "NULL indices should fail");

  result = profile_bulk_update_encoding(profile, api, indices, 0, &new_enc);
  test_assert(!result, "Zero count should fail");

  result = profile_bulk_update_encoding(profile, api, indices, 2, NULL);
  test_assert(!result, "NULL encoding should fail");

  size_t update_indices[] = {0, 2};
  result = profile_bulk_update_encoding(profile, NULL, update_indices, 2, &new_enc);
  test_assert(result, "Should succeed when inactive");

  /* Test profile_bulk_start_destinations */
  result = profile_bulk_start_destinations(NULL, api, indices, 2);
  test_assert(!result, "NULL profile should fail");

  result = profile_bulk_start_destinations(profile, NULL, indices, 2);
  test_assert(!result, "NULL api should fail");

  result = profile_bulk_start_destinations(profile, api, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = profile_bulk_start_destinations(profile, api, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test when profile not active */
  profile->status = PROFILE_STATUS_INACTIVE;
  result = profile_bulk_start_destinations(profile, api, indices, 2);
  test_assert(!result, "Should fail when profile not active");

  /* Test profile_bulk_stop_destinations */
  result = profile_bulk_stop_destinations(NULL, api, indices, 2);
  test_assert(!result, "NULL profile should fail");

  result = profile_bulk_stop_destinations(profile, NULL, indices, 2);
  test_assert(!result, "NULL api should fail");

  result = profile_bulk_stop_destinations(profile, api, NULL, 2);
  test_assert(!result, "NULL indices should fail");

  result = profile_bulk_stop_destinations(profile, api, indices, 0);
  test_assert(!result, "Zero count should fail");

  /* Test when profile not active */
  result = profile_bulk_stop_destinations(profile, api, indices, 2);
  test_assert(!result, "Should fail when profile not active");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Bulk Operations");
  return true;
}

/* Test: Edge cases and additional NULL checks */
static bool test_additional_edge_cases(void)
{
  test_section_start("Additional Edge Cases");

  restreamer_api_t *api = create_test_api();
  profile_manager_t *manager = profile_manager_create(api);

  /* Test profile_update_stats with NULL process reference */
  output_profile_t *profile = profile_manager_create_profile(manager, "Stats Test");
  bool result = profile_update_stats(profile, api);
  test_assert(!result, "No process reference should fail");

  profile->process_reference = bstrdup("test_ref");
  result = profile_update_stats(profile, api);
  test_assert(result, "Should succeed (no-op in current implementation)");

  /* Test profile_get_default_encoding */
  encoding_settings_t enc = profile_get_default_encoding();
  test_assert(enc.width == 0, "Default width should be 0");
  test_assert(enc.height == 0, "Default height should be 0");
  test_assert(enc.bitrate == 0, "Default bitrate should be 0");
  test_assert(enc.fps_num == 0, "Default fps_num should be 0");
  test_assert(enc.fps_den == 0, "Default fps_den should be 0");
  test_assert(enc.audio_bitrate == 0, "Default audio_bitrate should be 0");
  test_assert(enc.audio_track == 0, "Default audio_track should be 0");
  test_assert(enc.max_bandwidth == 0, "Default max_bandwidth should be 0");
  test_assert(enc.low_latency == false, "Default low_latency should be false");

  /* Test profile_generate_id uniqueness */
  char *id1 = profile_generate_id();
  char *id2 = profile_generate_id();
  char *id3 = profile_generate_id();

  test_assert(id1 != NULL, "ID should be generated");
  test_assert(id2 != NULL, "ID should be generated");
  test_assert(id3 != NULL, "ID should be generated");
  test_assert(strcmp(id1, id2) != 0, "IDs should be unique");
  test_assert(strcmp(id2, id3) != 0, "IDs should be unique");

  bfree(id1);
  bfree(id2);
  bfree(id3);

  /* Test profile_manager_get_active_count */
  size_t count = profile_manager_get_active_count(NULL);
  test_assert(count == 0, "NULL manager should return 0");

  count = profile_manager_get_active_count(manager);
  test_assert(count == 0, "No active profiles should return 0");

  profile->status = PROFILE_STATUS_ACTIVE;
  count = profile_manager_get_active_count(manager);
  test_assert(count == 1, "Should count active profile");

  /* Test profile_add_destination with NULL encoding */
  output_profile_t *profile2 = profile_manager_create_profile(manager, "Null Encoding Test");
  result = profile_add_destination(profile2, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, NULL);
  test_assert(result, "Should succeed with NULL encoding (uses default)");
  test_assert(profile2->destination_count == 1, "Should have 1 destination");
  test_assert(profile2->destinations[0].encoding.bitrate == 0, "Should use default encoding");

  profile_manager_destroy(manager);
  restreamer_api_destroy(api);

  test_section_end("Additional Edge Cases");
  return true;
}

/* Test suite runner */
bool run_profile_coverage_tests(void)
{
  test_suite_start("Profile Coverage Tests");

  bool result = true;

  test_start("Profile manager destroy with active profiles");
  result &= test_profile_manager_destroy_with_active_profiles();
  test_end();

  test_start("Profile manager delete active profile");
  result &= test_profile_manager_delete_active_profile();
  test_end();

  test_start("Profile update destination encoding live");
  result &= test_profile_update_destination_encoding_live();
  test_end();

  test_start("Output profile start error paths");
  result &= test_output_profile_start_error_paths();
  test_end();

  test_start("Output profile stop with process reference");
  result &= test_output_profile_stop_with_process();
  test_end();

  test_start("Profile restart");
  result &= test_profile_restart();
  test_end();

  test_start("Profile manager bulk start/stop");
  result &= test_profile_manager_bulk_start_stop();
  test_end();

  test_start("Preview mode functions");
  result &= test_preview_mode_functions();
  test_end();

  test_start("Profile duplicate");
  result &= test_profile_duplicate();
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

  test_suite_end("Profile Coverage Tests", result);
  return result;
}
