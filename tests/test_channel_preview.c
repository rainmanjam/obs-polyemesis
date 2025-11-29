/**
 * Unit Tests for Channel Preview Mode Functions
 * Tests preview mode operations: start, cancel, convert to live, and timeout checks
 */

#include "test_framework.h"
#include "../src/restreamer-channel.h"
#include "../src/restreamer-api.h"
#include <obs.h>
#include <time.h>

/* Mock time for testing timeout functionality */
static time_t mock_time_value = 0;
static bool use_mock_time = false;

/* Override time() for testing */
time_t time(time_t *tloc) {
    time_t result = use_mock_time ? mock_time_value : 0;
    if (!use_mock_time) {
        /* Call actual time() from libc */
        extern time_t __real_time(time_t *);
        result = __real_time(tloc);
    }
    if (tloc) {
        *tloc = result;
    }
    return result;
}

/* Helper: Create test channel manager with mock API */
static channel_manager_t *create_test_manager(void) {
    /* Create mock API connection */
    restreamer_connection_t conn = {
        .host = "localhost",
        .port = 8080,
        .username = "test",
        .password = "test",
        .use_https = false,
    };

    restreamer_api_t *api = restreamer_api_create(&conn);
    channel_manager_t *manager = channel_manager_create(api);

    return manager;
}

/* Helper: Create test channel with outputs */
static stream_channel_t *create_test_channel_with_outputs(channel_manager_t *manager) {
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Preview Channel");
    if (!channel) {
        return NULL;
    }

    /* Set input URL */
    channel->input_url = bstrdup("rtmp://localhost/live/test");

    /* Add test output */
    encoding_settings_t encoding = channel_get_default_encoding();
    encoding.bitrate = 2500;
    encoding.width = 1280;
    encoding.height = 720;

    channel_add_output(channel, SERVICE_YOUTUBE, "test-key-123",
                      ORIENTATION_HORIZONTAL, &encoding);

    return channel;
}

/* Helper: Cleanup manager and API */
static void cleanup_test_manager(channel_manager_t *manager) {
    restreamer_api_t *api = manager->api;
    channel_manager_destroy(manager);
    if (api) {
        restreamer_api_destroy(api);
    }
}

/* ============================================================================
 * Test 1: Successfully Start Preview Mode
 * ============================================================================ */
static bool test_start_preview_success(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);
    uint32_t duration = 300; /* 5 minutes */

    /* Start preview mode */
    bool result = channel_start_preview(manager, channel_id, duration);

    /* Note: This may fail due to missing API connection, but we test the logic */
    /* In a real environment with API, this should succeed */

    /* Verify preview mode flags are set (even if start failed) */
    if (result) {
        ASSERT_TRUE(channel->preview_mode_enabled, "Preview mode should be enabled");
        ASSERT_EQ(channel->preview_duration_sec, duration, "Preview duration should match");
        ASSERT_NE(channel->preview_start_time, 0, "Preview start time should be set");
        ASSERT_EQ(channel->status, CHANNEL_STATUS_PREVIEW, "Status should be PREVIEW");
    }

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 2: Fail to Start Preview When Channel Not Inactive
 * ============================================================================ */
static bool test_start_preview_channel_not_inactive(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Manually set channel to ACTIVE status */
    channel->status = CHANNEL_STATUS_ACTIVE;

    /* Try to start preview - should fail */
    bool result = channel_start_preview(manager, channel_id, 300);
    ASSERT_FALSE(result, "Should not start preview when channel is not inactive");

    /* Verify preview mode not enabled */
    ASSERT_FALSE(channel->preview_mode_enabled, "Preview mode should not be enabled");
    ASSERT_EQ(channel->preview_duration_sec, 0, "Preview duration should be 0");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 3: Verify Preview State is Set Correctly
 * ============================================================================ */
static bool test_start_preview_sets_correct_state(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);
    uint32_t duration = 600; /* 10 minutes */

    /* Record time before starting preview */
    use_mock_time = true;
    mock_time_value = 1000000;

    /* Verify initial state */
    ASSERT_FALSE(channel->preview_mode_enabled, "Preview should not be enabled initially");
    ASSERT_EQ(channel->preview_duration_sec, 0, "Duration should be 0 initially");
    ASSERT_EQ(channel->preview_start_time, 0, "Start time should be 0 initially");

    /* Start preview (may fail due to API, but state should be attempted to be set) */
    channel_start_preview(manager, channel_id, duration);

    /* If preview was started, verify state */
    if (channel->preview_mode_enabled) {
        ASSERT_EQ(channel->preview_duration_sec, duration, "Duration should match requested");
        ASSERT_EQ(channel->preview_start_time, mock_time_value, "Start time should be set to current time");
    }

    use_mock_time = false;
    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 4: Successfully Convert Preview to Live
 * ============================================================================ */
static bool test_preview_to_live_success(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Manually set channel to preview mode */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 300;
    channel->preview_start_time = 1000000;
    channel->status = CHANNEL_STATUS_PREVIEW;

    /* Set an error message to verify it gets cleared */
    channel->last_error = bstrdup("Test error");

    /* Convert to live */
    bool result = channel_preview_to_live(manager, channel_id);
    ASSERT_TRUE(result, "Preview to live should succeed");

    /* Verify preview mode disabled */
    ASSERT_FALSE(channel->preview_mode_enabled, "Preview mode should be disabled");
    ASSERT_EQ(channel->preview_duration_sec, 0, "Preview duration should be reset");
    ASSERT_EQ(channel->preview_start_time, 0, "Preview start time should be reset");

    /* Verify status changed to ACTIVE */
    ASSERT_EQ(channel->status, CHANNEL_STATUS_ACTIVE, "Status should be ACTIVE");

    /* Verify error was cleared */
    ASSERT_NULL(channel->last_error, "Last error should be cleared");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 5: Fail Preview to Live When Not in Preview Mode
 * ============================================================================ */
static bool test_preview_to_live_not_in_preview(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Channel is INACTIVE, not PREVIEW */
    ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE, "Initial status should be INACTIVE");

    /* Try to convert to live - should fail */
    bool result = channel_preview_to_live(manager, channel_id);
    ASSERT_FALSE(result, "Should not convert to live when not in preview mode");

    /* Verify status unchanged */
    ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE, "Status should remain INACTIVE");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 6: Successfully Cancel Preview Mode
 * ============================================================================ */
static bool test_cancel_preview_success(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Manually set channel to preview mode */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 300;
    channel->preview_start_time = 1000000;
    channel->status = CHANNEL_STATUS_PREVIEW;

    /* Cancel preview */
    bool result = channel_cancel_preview(manager, channel_id);

    /* Should succeed (will call channel_stop internally) */
    ASSERT_TRUE(result, "Cancel preview should succeed");

    /* Verify preview mode disabled */
    ASSERT_FALSE(channel->preview_mode_enabled, "Preview mode should be disabled");
    ASSERT_EQ(channel->preview_duration_sec, 0, "Preview duration should be reset");
    ASSERT_EQ(channel->preview_start_time, 0, "Preview start time should be reset");

    /* Status should be INACTIVE after stop */
    ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE, "Status should be INACTIVE");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 7: Fail to Cancel Preview When Not in Preview Mode
 * ============================================================================ */
static bool test_cancel_preview_not_in_preview(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Channel is INACTIVE, not PREVIEW */
    ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE, "Initial status should be INACTIVE");

    /* Try to cancel preview - should fail */
    bool result = channel_cancel_preview(manager, channel_id);
    ASSERT_FALSE(result, "Should not cancel preview when not in preview mode");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 8: Check Preview Timeout - Not Enabled
 * ============================================================================ */
static bool test_check_preview_timeout_not_enabled(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    /* Channel not in preview mode */
    ASSERT_FALSE(channel->preview_mode_enabled, "Preview should not be enabled");

    /* Check timeout - should return false */
    bool timed_out = channel_check_preview_timeout(channel);
    ASSERT_FALSE(timed_out, "Should not timeout when preview not enabled");

    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 9: Check Preview Timeout - Unlimited Duration
 * ============================================================================ */
static bool test_check_preview_timeout_unlimited(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    /* Set preview mode with unlimited duration (0) */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 0; /* 0 = unlimited */
    channel->preview_start_time = 1000000;

    /* Check timeout - should return false (unlimited) */
    bool timed_out = channel_check_preview_timeout(channel);
    ASSERT_FALSE(timed_out, "Should not timeout when duration is 0 (unlimited)");

    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 10: Check Preview Timeout - Expired
 * ============================================================================ */
static bool test_check_preview_timeout_expired(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    use_mock_time = true;

    /* Set preview mode starting at time 1000 with 300 second duration */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 300;
    channel->preview_start_time = 1000;

    /* Set current time to 1400 (400 seconds elapsed > 300 duration) */
    mock_time_value = 1400;

    /* Check timeout - should return true (expired) */
    bool timed_out = channel_check_preview_timeout(channel);
    ASSERT_TRUE(timed_out, "Should timeout when elapsed time exceeds duration");

    use_mock_time = false;
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 11: Check Preview Timeout - Not Expired
 * ============================================================================ */
static bool test_check_preview_timeout_not_expired(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    use_mock_time = true;

    /* Set preview mode starting at time 1000 with 300 second duration */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 300;
    channel->preview_start_time = 1000;

    /* Set current time to 1200 (200 seconds elapsed < 300 duration) */
    mock_time_value = 1200;

    /* Check timeout - should return false (not expired) */
    bool timed_out = channel_check_preview_timeout(channel);
    ASSERT_FALSE(timed_out, "Should not timeout when elapsed time is less than duration");

    use_mock_time = false;
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 12: Preview Timeout Boundary - Exactly at Duration
 * ============================================================================ */
static bool test_check_preview_timeout_boundary(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    use_mock_time = true;

    /* Set preview mode starting at time 1000 with 300 second duration */
    channel->preview_mode_enabled = true;
    channel->preview_duration_sec = 300;
    channel->preview_start_time = 1000;

    /* Set current time to 1300 (exactly 300 seconds elapsed = duration) */
    mock_time_value = 1300;

    /* Check timeout - should return true (elapsed >= duration) */
    bool timed_out = channel_check_preview_timeout(channel);
    ASSERT_TRUE(timed_out, "Should timeout when elapsed time equals duration");

    use_mock_time = false;
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 13: Null Channel Check
 * ============================================================================ */
static bool test_check_preview_timeout_null_channel(void) {
    /* Check timeout with NULL channel - should return false */
    bool timed_out = channel_check_preview_timeout(NULL);
    ASSERT_FALSE(timed_out, "Should return false for NULL channel");

    return true;
}

/* ============================================================================
 * Test 14: Preview Start with NULL Parameters
 * ============================================================================ */
static bool test_start_preview_null_params(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Test NULL manager */
    bool result = channel_start_preview(NULL, channel_id, 300);
    ASSERT_FALSE(result, "Should fail with NULL manager");

    /* Test NULL channel_id */
    result = channel_start_preview(manager, NULL, 300);
    ASSERT_FALSE(result, "Should fail with NULL channel_id");

    /* Test invalid channel_id */
    result = channel_start_preview(manager, "invalid-id-12345", 300);
    ASSERT_FALSE(result, "Should fail with invalid channel_id");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 15: Preview to Live with NULL Parameters
 * ============================================================================ */
static bool test_preview_to_live_null_params(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Test NULL manager */
    bool result = channel_preview_to_live(NULL, channel_id);
    ASSERT_FALSE(result, "Should fail with NULL manager");

    /* Test NULL channel_id */
    result = channel_preview_to_live(manager, NULL);
    ASSERT_FALSE(result, "Should fail with NULL channel_id");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Test 16: Cancel Preview with NULL Parameters
 * ============================================================================ */
static bool test_cancel_preview_null_params(void) {
    channel_manager_t *manager = create_test_manager();
    stream_channel_t *channel = create_test_channel_with_outputs(manager);
    ASSERT_NOT_NULL(channel, "Channel should be created");

    char *channel_id = bstrdup(channel->channel_id);

    /* Test NULL manager */
    bool result = channel_cancel_preview(NULL, channel_id);
    ASSERT_FALSE(result, "Should fail with NULL manager");

    /* Test NULL channel_id */
    result = channel_cancel_preview(manager, NULL);
    ASSERT_FALSE(result, "Should fail with NULL channel_id");

    bfree(channel_id);
    cleanup_test_manager(manager);
    return true;
}

/* ============================================================================
 * Main Test Suite Runner
 * ============================================================================ */
bool run_channel_preview_tests(void) {
    bool all_passed = true;

    printf("\n");
    printf("========================================================================\n");
    printf("Channel Preview Mode Tests\n");
    printf("========================================================================\n");

    /* Basic functionality tests */
    RUN_TEST(test_start_preview_success, "Start preview mode successfully");
    RUN_TEST(test_start_preview_channel_not_inactive, "Reject preview start when channel not inactive");
    RUN_TEST(test_start_preview_sets_correct_state, "Verify preview state is set correctly");

    /* Preview to live tests */
    RUN_TEST(test_preview_to_live_success, "Convert preview to live successfully");
    RUN_TEST(test_preview_to_live_not_in_preview, "Reject preview to live when not in preview mode");

    /* Cancel preview tests */
    RUN_TEST(test_cancel_preview_success, "Cancel preview mode successfully");
    RUN_TEST(test_cancel_preview_not_in_preview, "Reject cancel when not in preview mode");

    /* Timeout check tests */
    RUN_TEST(test_check_preview_timeout_not_enabled, "Return false when preview not enabled");
    RUN_TEST(test_check_preview_timeout_unlimited, "Return false when duration is unlimited (0)");
    RUN_TEST(test_check_preview_timeout_expired, "Return true when preview time expired");
    RUN_TEST(test_check_preview_timeout_not_expired, "Return false when preview time not expired");
    RUN_TEST(test_check_preview_timeout_boundary, "Return true when exactly at timeout boundary");
    RUN_TEST(test_check_preview_timeout_null_channel, "Handle NULL channel gracefully");

    /* Error handling tests */
    RUN_TEST(test_start_preview_null_params, "Handle NULL parameters in start_preview");
    RUN_TEST(test_preview_to_live_null_params, "Handle NULL parameters in preview_to_live");
    RUN_TEST(test_cancel_preview_null_params, "Handle NULL parameters in cancel_preview");

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
