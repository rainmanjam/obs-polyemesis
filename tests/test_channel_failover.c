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
 * Unit Tests for Channel Failover Logic
 * Tests backup/failover functionality for channel outputs
 *
 * Target functions (src/restreamer-channel.c lines 1543-1778):
 * - channel_set_output_backup
 * - channel_remove_output_backup
 * - channel_trigger_failover
 * - channel_restore_primary
 * - channel_check_failover
 */

#include "test_framework.h"
#include "../src/restreamer-channel.h"
#include "../src/restreamer-api.h"
#include "../src/restreamer-multistream.h"
#include <time.h>

/* ========================================================================
 * Test Fixtures and Helper Functions
 * ======================================================================== */

/**
 * Create a test channel manager with mock API
 */
static channel_manager_t *create_test_manager(void) {
    restreamer_connection_t conn = {
        .host = "localhost",
        .port = 8080,
        .username = "test",
        .password = "test",
        .use_https = false,
    };

    restreamer_api_t *api = restreamer_api_create(&conn);
    if (!api) {
        return NULL;
    }

    channel_manager_t *manager = channel_manager_create(api);
    return manager;
}

/**
 * Destroy test manager and API
 */
static void destroy_test_manager(channel_manager_t *manager) {
    if (!manager) {
        return;
    }

    restreamer_api_t *api = manager->api;
    channel_manager_destroy(manager);
    restreamer_api_destroy(api);
}

/**
 * Create a test channel with two outputs (primary and backup)
 */
static stream_channel_t *create_channel_with_outputs(channel_manager_t *manager) {
    stream_channel_t *channel = channel_manager_create_channel(manager, "Failover Test");
    if (!channel) {
        return NULL;
    }

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Add primary output */
    channel_add_output(channel, SERVICE_YOUTUBE, "primary-key",
                      ORIENTATION_HORIZONTAL, &encoding);

    /* Add backup output */
    channel_add_output(channel, SERVICE_YOUTUBE, "backup-key",
                      ORIENTATION_HORIZONTAL, &encoding);

    return channel;
}

/* ========================================================================
 * Test Cases: channel_set_output_backup
 * ======================================================================== */

/**
 * Test: Successfully set backup relationship
 */
static bool test_set_output_backup_success(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");
    ASSERT_EQ(channel->output_count, 2, "Should have 2 outputs");

    /* Set output 1 as backup for output 0 */
    bool result = channel_set_output_backup(channel, 0, 1);
    ASSERT_TRUE(result, "Set backup should succeed");

    /* Verify primary output configuration */
    ASSERT_EQ(channel->outputs[0].backup_index, 1,
              "Primary should reference backup at index 1");
    ASSERT_FALSE(channel->outputs[0].is_backup,
                 "Primary should not be marked as backup");
    ASSERT_EQ(channel->outputs[0].primary_index, (size_t)-1,
              "Primary should not have a primary_index");

    /* Verify backup output configuration */
    ASSERT_TRUE(channel->outputs[1].is_backup,
                "Output 1 should be marked as backup");
    ASSERT_EQ(channel->outputs[1].primary_index, 0,
              "Backup should reference primary at index 0");
    ASSERT_FALSE(channel->outputs[1].enabled,
                 "Backup should start disabled");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Cannot set output as its own backup
 */
static bool test_set_output_backup_same_index_fails(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Try to set output as its own backup */
    bool result = channel_set_output_backup(channel, 0, 0);
    ASSERT_FALSE(result, "Should fail to set output as its own backup");

    /* Verify no backup relationship was created */
    ASSERT_EQ(channel->outputs[0].backup_index, (size_t)-1,
              "Primary should not have backup");
    ASSERT_FALSE(channel->outputs[0].is_backup,
                 "Output should not be marked as backup");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Invalid indices should fail
 */
static bool test_set_output_backup_invalid_indices(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Test invalid backup index */
    bool result = channel_set_output_backup(channel, 0, 999);
    ASSERT_FALSE(result, "Should fail with invalid backup index");

    /* Test invalid primary index */
    result = channel_set_output_backup(channel, 999, 0);
    ASSERT_FALSE(result, "Should fail with invalid primary index");

    /* Test NULL channel */
    result = channel_set_output_backup(NULL, 0, 1);
    ASSERT_FALSE(result, "Should fail with NULL channel");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Replacing existing backup relationship
 */
static bool test_set_output_backup_replaces_existing(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = channel_manager_create_channel(manager, "Failover Test");
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Add primary and two backup candidates */
    channel_add_output(channel, SERVICE_YOUTUBE, "primary-key",
                      ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(channel, SERVICE_YOUTUBE, "backup1-key",
                      ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(channel, SERVICE_YOUTUBE, "backup2-key",
                      ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(channel->output_count, 3, "Should have 3 outputs");

    /* Set first backup */
    bool result = channel_set_output_backup(channel, 0, 1);
    ASSERT_TRUE(result, "First backup assignment should succeed");
    ASSERT_EQ(channel->outputs[0].backup_index, 1, "Should have backup1");
    ASSERT_TRUE(channel->outputs[1].is_backup, "Backup1 should be marked");

    /* Replace with second backup */
    result = channel_set_output_backup(channel, 0, 2);
    ASSERT_TRUE(result, "Backup replacement should succeed");
    ASSERT_EQ(channel->outputs[0].backup_index, 2, "Should now have backup2");

    /* Verify old backup is cleared */
    ASSERT_FALSE(channel->outputs[1].is_backup,
                 "Backup1 should no longer be marked as backup");
    ASSERT_EQ(channel->outputs[1].primary_index, (size_t)-1,
              "Backup1 should no longer reference primary");

    /* Verify new backup is set */
    ASSERT_TRUE(channel->outputs[2].is_backup, "Backup2 should be marked");
    ASSERT_EQ(channel->outputs[2].primary_index, 0,
              "Backup2 should reference primary");

    destroy_test_manager(manager);
    return true;
}

/* ========================================================================
 * Test Cases: channel_remove_output_backup
 * ======================================================================== */

/**
 * Test: Successfully remove backup relationship
 */
static bool test_remove_output_backup_success(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup first */
    channel_set_output_backup(channel, 0, 1);
    ASSERT_EQ(channel->outputs[0].backup_index, 1, "Backup should be set");

    /* Remove backup relationship */
    bool result = channel_remove_output_backup(channel, 0);
    ASSERT_TRUE(result, "Remove backup should succeed");

    /* Verify primary output is cleared */
    ASSERT_EQ(channel->outputs[0].backup_index, (size_t)-1,
              "Primary should no longer reference backup");

    /* Verify backup output is cleared */
    ASSERT_FALSE(channel->outputs[1].is_backup,
                 "Output should no longer be marked as backup");
    ASSERT_EQ(channel->outputs[1].primary_index, (size_t)-1,
              "Backup should no longer reference primary");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Handle case when no backup exists
 */
static bool test_remove_output_backup_no_backup(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Try to remove backup when none is set */
    bool result = channel_remove_output_backup(channel, 0);
    ASSERT_FALSE(result, "Should fail when no backup exists");

    /* Test with NULL channel */
    result = channel_remove_output_backup(NULL, 0);
    ASSERT_FALSE(result, "Should fail with NULL channel");

    /* Test with invalid index */
    result = channel_remove_output_backup(channel, 999);
    ASSERT_FALSE(result, "Should fail with invalid index");

    destroy_test_manager(manager);
    return true;
}

/* ========================================================================
 * Test Cases: channel_trigger_failover
 * ======================================================================== */

/**
 * Test: Successfully trigger failover to backup
 */
static bool test_trigger_failover_success(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup relationship */
    channel_set_output_backup(channel, 0, 1);

    /* Set channel to active status (required for failover to actually switch outputs) */
    channel->status = CHANNEL_STATUS_ACTIVE;

    /* Trigger failover */
    bool result = channel_trigger_failover(channel, manager->api, 0);
    ASSERT_TRUE(result, "Failover should succeed");

    /* Verify failover state on primary */
    ASSERT_TRUE(channel->outputs[0].failover_active,
                "Primary failover should be marked active");
    ASSERT_NE(channel->outputs[0].failover_start_time, 0,
              "Primary failover start time should be set");

    /* Verify failover state on backup */
    ASSERT_TRUE(channel->outputs[1].failover_active,
                "Backup failover should be marked active");
    ASSERT_NE(channel->outputs[1].failover_start_time, 0,
              "Backup failover start time should be set");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Fail when no backup configured
 */
static bool test_trigger_failover_no_backup(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Do NOT set backup relationship */

    /* Try to trigger failover without backup */
    bool result = channel_trigger_failover(channel, manager->api, 0);
    ASSERT_FALSE(result, "Failover should fail when no backup is configured");

    /* Verify no failover state was set */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should not be active");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Handle already active failover
 */
static bool test_trigger_failover_already_active(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup relationship */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;

    /* Trigger failover first time */
    bool result = channel_trigger_failover(channel, manager->api, 0);
    ASSERT_TRUE(result, "First failover should succeed");

    time_t first_start_time = channel->outputs[0].failover_start_time;

    /* Try to trigger failover again */
    result = channel_trigger_failover(channel, manager->api, 0);
    ASSERT_TRUE(result, "Should return true when failover already active");

    /* Verify failover state hasn't changed */
    ASSERT_TRUE(channel->outputs[0].failover_active,
                "Failover should still be active");
    ASSERT_EQ(channel->outputs[0].failover_start_time, first_start_time,
              "Start time should not change");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Invalid parameters for trigger_failover
 */
static bool test_trigger_failover_invalid_params(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    channel_set_output_backup(channel, 0, 1);

    /* Test NULL channel */
    bool result = channel_trigger_failover(NULL, manager->api, 0);
    ASSERT_FALSE(result, "Should fail with NULL channel");

    /* Test NULL API */
    result = channel_trigger_failover(channel, NULL, 0);
    ASSERT_FALSE(result, "Should fail with NULL API");

    /* Test invalid index */
    result = channel_trigger_failover(channel, manager->api, 999);
    ASSERT_FALSE(result, "Should fail with invalid index");

    destroy_test_manager(manager);
    return true;
}

/* ========================================================================
 * Test Cases: channel_restore_primary
 * ======================================================================== */

/**
 * Test: Successfully restore from backup to primary
 */
static bool test_restore_primary_success(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup and trigger failover */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel_trigger_failover(channel, manager->api, 0);

    ASSERT_TRUE(channel->outputs[0].failover_active,
                "Failover should be active before restore");

    /* Restore primary */
    bool result = channel_restore_primary(channel, manager->api, 0);
    ASSERT_TRUE(result, "Restore should succeed");

    /* Verify failover state cleared on primary */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Primary failover should be cleared");
    ASSERT_EQ(channel->outputs[0].consecutive_failures, 0,
              "Primary consecutive failures should be reset");

    /* Verify failover state cleared on backup */
    ASSERT_FALSE(channel->outputs[1].failover_active,
                 "Backup failover should be cleared");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Restore when no failover is active
 */
static bool test_restore_primary_no_active_failover(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup but do NOT trigger failover */
    channel_set_output_backup(channel, 0, 1);

    /* Try to restore when no failover is active */
    bool result = channel_restore_primary(channel, manager->api, 0);
    ASSERT_TRUE(result, "Should return true when no failover is active");

    /* State should remain unchanged */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should remain inactive");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Restore fails without backup configured
 */
static bool test_restore_primary_no_backup(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Do NOT set backup relationship */

    /* Try to restore without backup */
    bool result = channel_restore_primary(channel, manager->api, 0);
    ASSERT_FALSE(result, "Should fail when no backup is configured");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Invalid parameters for restore_primary
 */
static bool test_restore_primary_invalid_params(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    channel_set_output_backup(channel, 0, 1);

    /* Test NULL channel */
    bool result = channel_restore_primary(NULL, manager->api, 0);
    ASSERT_FALSE(result, "Should fail with NULL channel");

    /* Test NULL API */
    result = channel_restore_primary(channel, NULL, 0);
    ASSERT_FALSE(result, "Should fail with NULL API");

    /* Test invalid index */
    result = channel_restore_primary(channel, manager->api, 999);
    ASSERT_FALSE(result, "Should fail with invalid index");

    destroy_test_manager(manager);
    return true;
}

/* ========================================================================
 * Test Cases: channel_check_failover
 * ======================================================================== */

/**
 * Test: Auto-failover when failure threshold reached
 */
static bool test_check_failover_triggers_on_failure_threshold(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Configure channel for auto-failover */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel->failure_threshold = 3;

    /* Simulate failure threshold reached */
    channel->outputs[0].connected = false;
    channel->outputs[0].consecutive_failures = 3;
    channel->outputs[0].failover_active = false;

    /* Check failover - should trigger */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_TRUE(result, "Check failover should detect and trigger failover");

    /* Verify failover was triggered */
    ASSERT_TRUE(channel->outputs[0].failover_active,
                "Failover should be active after check");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Auto-restore when primary recovers
 */
static bool test_check_failover_restores_on_recovery(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set up failover state */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel->failure_threshold = 3;

    /* Trigger failover */
    channel->outputs[0].failover_active = false;
    channel->outputs[0].connected = false;
    channel->outputs[0].consecutive_failures = 3;
    channel_trigger_failover(channel, manager->api, 0);

    ASSERT_TRUE(channel->outputs[0].failover_active,
                "Failover should be active");

    /* Simulate primary recovery */
    channel->outputs[0].connected = true;
    channel->outputs[0].consecutive_failures = 0;

    /* Check failover - should restore */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_FALSE(result, "Should return false (no new failovers triggered)");

    /* Verify restoration happened */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should be cleared after restoration");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: No failover when threshold not reached
 */
static bool test_check_failover_no_trigger_below_threshold(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Configure channel */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel->failure_threshold = 3;

    /* Simulate failures below threshold */
    channel->outputs[0].connected = false;
    channel->outputs[0].consecutive_failures = 2; // Below threshold
    channel->outputs[0].failover_active = false;

    /* Check failover - should NOT trigger */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_FALSE(result, "Should not trigger failover below threshold");

    /* Verify failover was not triggered */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should not be active");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Skip outputs without backups
 */
static bool test_check_failover_skips_outputs_without_backup(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Do NOT set backup for output 0 */
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel->failure_threshold = 3;

    /* Simulate failures */
    channel->outputs[0].connected = false;
    channel->outputs[0].consecutive_failures = 5; // Above threshold
    channel->outputs[0].failover_active = false;

    /* Check failover - should NOT trigger (no backup configured) */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_FALSE(result, "Should not trigger failover without backup");

    /* Verify failover was not triggered */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should not be active without backup");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Skip backup outputs themselves
 */
static bool test_check_failover_skips_backup_outputs(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Set backup relationship */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_ACTIVE;
    channel->failure_threshold = 3;

    /* Simulate failures on the BACKUP output (index 1) */
    channel->outputs[1].connected = false;
    channel->outputs[1].consecutive_failures = 5; // Above threshold

    /* Check failover - should skip backup output */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_FALSE(result, "Should not process backup outputs");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Only check when channel is active
 */
static bool test_check_failover_only_when_active(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Configure for failover */
    channel_set_output_backup(channel, 0, 1);
    channel->status = CHANNEL_STATUS_INACTIVE; // Not active
    channel->failure_threshold = 3;

    /* Simulate failures */
    channel->outputs[0].connected = false;
    channel->outputs[0].consecutive_failures = 5;

    /* Check failover - should skip (channel not active) */
    bool result = channel_check_failover(channel, manager->api);
    ASSERT_FALSE(result, "Should not trigger failover when channel inactive");

    /* Verify no failover triggered */
    ASSERT_FALSE(channel->outputs[0].failover_active,
                 "Failover should not be active");

    destroy_test_manager(manager);
    return true;
}

/**
 * Test: Invalid parameters for check_failover
 */
static bool test_check_failover_invalid_params(void) {
    channel_manager_t *manager = create_test_manager();
    ASSERT_NOT_NULL(manager, "Manager creation should succeed");

    stream_channel_t *channel = create_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel creation should succeed");

    /* Test NULL channel */
    bool result = channel_check_failover(NULL, manager->api);
    ASSERT_FALSE(result, "Should fail with NULL channel");

    /* Test NULL API */
    result = channel_check_failover(channel, NULL);
    ASSERT_FALSE(result, "Should fail with NULL API");

    destroy_test_manager(manager);
    return true;
}

/* ========================================================================
 * Test Suite Runner
 * ======================================================================== */

bool run_channel_failover_tests(void) {
    bool all_passed = true;

    printf("\n");
    printf("========================================================================\n");
    printf("Channel Failover Logic Tests\n");
    printf("========================================================================\n");

    /* channel_set_output_backup tests */
    RUN_TEST(test_set_output_backup_success,
             "Set backup output - Success");
    RUN_TEST(test_set_output_backup_same_index_fails,
             "Set backup output - Same index fails");
    RUN_TEST(test_set_output_backup_invalid_indices,
             "Set backup output - Invalid indices");
    RUN_TEST(test_set_output_backup_replaces_existing,
             "Set backup output - Replace existing");

    /* channel_remove_output_backup tests */
    RUN_TEST(test_remove_output_backup_success,
             "Remove backup - Success");
    RUN_TEST(test_remove_output_backup_no_backup,
             "Remove backup - No backup exists");

    /* channel_trigger_failover tests */
    RUN_TEST(test_trigger_failover_success,
             "Trigger failover - Success");
    RUN_TEST(test_trigger_failover_no_backup,
             "Trigger failover - No backup configured");
    RUN_TEST(test_trigger_failover_already_active,
             "Trigger failover - Already active");
    RUN_TEST(test_trigger_failover_invalid_params,
             "Trigger failover - Invalid parameters");

    /* channel_restore_primary tests */
    RUN_TEST(test_restore_primary_success,
             "Restore primary - Success");
    RUN_TEST(test_restore_primary_no_active_failover,
             "Restore primary - No active failover");
    RUN_TEST(test_restore_primary_no_backup,
             "Restore primary - No backup configured");
    RUN_TEST(test_restore_primary_invalid_params,
             "Restore primary - Invalid parameters");

    /* channel_check_failover tests */
    RUN_TEST(test_check_failover_triggers_on_failure_threshold,
             "Check failover - Trigger on threshold");
    RUN_TEST(test_check_failover_restores_on_recovery,
             "Check failover - Restore on recovery");
    RUN_TEST(test_check_failover_no_trigger_below_threshold,
             "Check failover - No trigger below threshold");
    RUN_TEST(test_check_failover_skips_outputs_without_backup,
             "Check failover - Skip outputs without backup");
    RUN_TEST(test_check_failover_skips_backup_outputs,
             "Check failover - Skip backup outputs");
    RUN_TEST(test_check_failover_only_when_active,
             "Check failover - Only when channel active");
    RUN_TEST(test_check_failover_invalid_params,
             "Check failover - Invalid parameters");

    print_test_summary();

    all_passed = (global_stats.failed == 0 && global_stats.crashed == 0);

    /* Reset stats for next test suite */
    global_stats.total = 0;
    global_stats.passed = 0;
    global_stats.failed = 0;
    global_stats.crashed = 0;
    global_stats.skipped = 0;

    return all_passed;
}
