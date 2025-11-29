/**
 * Unit Tests for Health Monitoring Functions
 * Tests channel_check_health, channel_reconnect_output, and channel_set_health_monitoring
 */

#include "test_framework.h"
#include "../src/restreamer-channel.h"
#include "../src/restreamer-api.h"
#include <obs.h>
#include <time.h>

/* Mock API state for testing */
typedef struct {
	bool get_processes_should_succeed;
	bool get_process_should_succeed;
	bool get_outputs_should_succeed;
	bool add_output_should_succeed;
	bool remove_output_should_succeed;
	char *process_state;
	size_t output_count;
	char **output_ids;
	char *process_id;
	char *process_reference;
} mock_api_state_t;

static mock_api_state_t g_mock_state = {0};

/* Helper function to create a mock API */
static restreamer_api_t *create_mock_api(void)
{
	/* Initialize mock state with defaults */
	g_mock_state.get_processes_should_succeed = true;
	g_mock_state.get_process_should_succeed = true;
	g_mock_state.get_outputs_should_succeed = true;
	g_mock_state.add_output_should_succeed = true;
	g_mock_state.remove_output_should_succeed = true;
	g_mock_state.process_state = bstrdup("running");
	g_mock_state.output_count = 0;
	g_mock_state.output_ids = NULL;
	g_mock_state.process_id = bstrdup("test-process-id");
	g_mock_state.process_reference = bstrdup("test-process-ref");

	/* Return a dummy pointer (we'll mock the API functions) */
	return (restreamer_api_t *)0x1;
}

/* Helper function to clean up mock API */
static void destroy_mock_api(void)
{
	bfree(g_mock_state.process_state);
	bfree(g_mock_state.process_id);
	bfree(g_mock_state.process_reference);

	if (g_mock_state.output_ids) {
		for (size_t i = 0; i < g_mock_state.output_count; i++) {
			bfree(g_mock_state.output_ids[i]);
		}
		bfree(g_mock_state.output_ids);
	}

	memset(&g_mock_state, 0, sizeof(g_mock_state));
}

/* Helper function to create a test channel with outputs */
static stream_channel_t *create_test_channel(const char *name,
					      bool add_outputs)
{
	stream_channel_t *channel = bzalloc(sizeof(stream_channel_t));
	channel->channel_name = bstrdup(name);
	channel->channel_id = bstrdup("test-channel-id");
	channel->status = CHANNEL_STATUS_INACTIVE;
	channel->source_orientation = ORIENTATION_HORIZONTAL;
	channel->health_monitoring_enabled = false;
	channel->health_check_interval_sec = 0;
	channel->failure_threshold = 0;
	channel->max_reconnect_attempts = 0;
	channel->reconnect_delay_sec = 1; // Short delay for testing

	if (add_outputs) {
		/* Add test outputs */
		encoding_settings_t encoding = channel_get_default_encoding();
		channel_add_output(channel, SERVICE_YOUTUBE, "youtube-key",
				   ORIENTATION_HORIZONTAL, &encoding);
		channel_add_output(channel, SERVICE_TWITCH, "twitch-key",
				   ORIENTATION_HORIZONTAL, &encoding);

		/* Set outputs as enabled */
		channel->outputs[0].enabled = true;
		channel->outputs[1].enabled = true;
	}

	return channel;
}

/* Helper function to destroy test channel */
static void destroy_test_channel(stream_channel_t *channel)
{
	if (!channel)
		return;

	bfree(channel->channel_name);
	bfree(channel->channel_id);
	bfree(channel->process_reference);
	bfree(channel->last_error);

	for (size_t i = 0; i < channel->output_count; i++) {
		bfree(channel->outputs[i].service_name);
		bfree(channel->outputs[i].stream_key);
		bfree(channel->outputs[i].rtmp_url);
	}
	bfree(channel->outputs);
	bfree(channel);
}

/* Mock implementations of restreamer_api functions */

bool restreamer_api_get_processes(restreamer_api_t *api,
				   restreamer_process_list_t *list)
{
	(void)api;

	if (!g_mock_state.get_processes_should_succeed) {
		return false;
	}

	list->count = 1;
	list->processes = bzalloc(sizeof(restreamer_process_t));
	list->processes[0].id = bstrdup(g_mock_state.process_id);
	list->processes[0].reference =
		bstrdup(g_mock_state.process_reference);
	list->processes[0].state = bstrdup(g_mock_state.process_state);
	list->processes[0].command = bstrdup("ffmpeg ...");

	return true;
}

bool restreamer_api_get_process(restreamer_api_t *api,
				 const char *process_id,
				 restreamer_process_t *process)
{
	(void)api;
	(void)process_id;

	if (!g_mock_state.get_process_should_succeed) {
		return false;
	}

	process->id = bstrdup(g_mock_state.process_id);
	process->reference = bstrdup(g_mock_state.process_reference);
	process->state = bstrdup(g_mock_state.process_state);
	process->command = bstrdup("ffmpeg ...");

	return true;
}

bool restreamer_api_get_process_outputs(restreamer_api_t *api,
					 const char *process_id,
					 char ***output_ids,
					 size_t *output_count)
{
	(void)api;
	(void)process_id;

	if (!g_mock_state.get_outputs_should_succeed) {
		return false;
	}

	*output_count = g_mock_state.output_count;

	if (g_mock_state.output_count > 0) {
		*output_ids = bzalloc(sizeof(char *) *
				      g_mock_state.output_count);
		for (size_t i = 0; i < g_mock_state.output_count; i++) {
			(*output_ids)[i] = bstrdup(g_mock_state.output_ids[i]);
		}
	} else {
		*output_ids = NULL;
	}

	return true;
}

bool restreamer_api_add_process_output(restreamer_api_t *api,
					const char *process_id,
					const char *output_id,
					const char *output_url,
					const char *video_filter)
{
	(void)api;
	(void)process_id;
	(void)output_id;
	(void)output_url;
	(void)video_filter;

	return g_mock_state.add_output_should_succeed;
}

bool restreamer_api_remove_process_output(restreamer_api_t *api,
					   const char *process_id,
					   const char *output_id)
{
	(void)api;
	(void)process_id;
	(void)output_id;

	return g_mock_state.remove_output_should_succeed;
}

void restreamer_api_free_process_list(restreamer_process_list_t *list)
{
	if (!list)
		return;

	for (size_t i = 0; i < list->count; i++) {
		bfree(list->processes[i].id);
		bfree(list->processes[i].reference);
		bfree(list->processes[i].state);
		bfree(list->processes[i].command);
	}
	bfree(list->processes);
	memset(list, 0, sizeof(*list));
}

/* Stub for channel_check_failover (called by channel_check_health) */
bool channel_check_failover(stream_channel_t *channel, restreamer_api_t *api)
{
	(void)channel;
	(void)api;
	return true;
}

/* ========================================================================
 * Test Cases
 * ======================================================================== */

/* Test 1: Return true when channel not active */
static bool test_check_health_not_active(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as inactive */
	channel->status = CHANNEL_STATUS_INACTIVE;
	channel->health_monitoring_enabled = true;

	/* Health check should return true for inactive channel */
	bool result = channel_check_health(channel, api);
	ASSERT_TRUE(result, "Health check should return true for inactive channel");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 2: Return true when monitoring disabled */
static bool test_check_health_monitoring_disabled(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active but monitoring disabled */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = false;

	/* Health check should return true when monitoring disabled */
	bool result = channel_check_health(channel, api);
	ASSERT_TRUE(result, "Health check should return true when monitoring disabled");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 3: Return false when no process reference */
static bool test_check_health_no_process_reference(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with monitoring enabled but no process reference */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = true;
	channel->process_reference = NULL;

	/* Health check should return false */
	bool result = channel_check_health(channel, api);
	ASSERT_FALSE(result, "Health check should return false with no process reference");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 4: Return false when process not found in list */
static bool test_check_health_process_not_found(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with monitoring enabled */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = true;
	channel->process_reference =
		bstrdup("non-existent-process-ref");

	/* Mock will return a process with different reference */
	g_mock_state.process_reference = bstrdup("different-ref");

	/* Health check should return false */
	bool result = channel_check_health(channel, api);
	ASSERT_FALSE(result, "Health check should return false when process not found");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 5: Return true when all outputs healthy */
static bool test_check_health_all_outputs_healthy(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with monitoring enabled */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = true;
	channel->process_reference =
		bstrdup(g_mock_state.process_reference);

	/* Mock outputs as healthy (running process with matching output IDs) */
	g_mock_state.process_state = bstrdup("running");
	g_mock_state.output_count = 2;
	g_mock_state.output_ids = bzalloc(sizeof(char *) * 2);
	g_mock_state.output_ids[0] = bstrdup("YouTube_0");
	g_mock_state.output_ids[1] = bstrdup("Twitch_1");

	/* Health check should return true */
	bool result = channel_check_health(channel, api);
	ASSERT_TRUE(result, "Health check should return true when all outputs healthy");

	/* Verify outputs marked as connected */
	ASSERT_TRUE(channel->outputs[0].connected, "Output 0 should be connected");
	ASSERT_TRUE(channel->outputs[1].connected, "Output 1 should be connected");
	ASSERT_EQ(channel->outputs[0].consecutive_failures, 0,
		  "Output 0 should have no failures");
	ASSERT_EQ(channel->outputs[1].consecutive_failures, 0,
		  "Output 1 should have no failures");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 6: Detect unhealthy output */
static bool test_check_health_output_unhealthy(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with monitoring enabled */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = true;
	channel->process_reference =
		bstrdup(g_mock_state.process_reference);
	channel->failure_threshold = 5; // High threshold to prevent auto-reconnect

	/* Mock only one output as healthy */
	g_mock_state.process_state = bstrdup("running");
	g_mock_state.output_count = 1;
	g_mock_state.output_ids = bzalloc(sizeof(char *) * 1);
	g_mock_state.output_ids[0] = bstrdup("YouTube_0");

	/* Health check should return false */
	bool result = channel_check_health(channel, api);
	ASSERT_FALSE(result, "Health check should return false when output unhealthy");

	/* Verify first output is healthy, second is not */
	ASSERT_TRUE(channel->outputs[0].connected, "Output 0 should be connected");
	ASSERT_FALSE(channel->outputs[1].connected, "Output 1 should not be connected");
	ASSERT_EQ(channel->outputs[0].consecutive_failures, 0,
		  "Output 0 should have no failures");
	ASSERT_EQ(channel->outputs[1].consecutive_failures, 1,
		  "Output 1 should have 1 failure");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 7: Auto-reconnect when threshold reached */
static bool test_check_health_triggers_auto_reconnect(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with monitoring enabled */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->health_monitoring_enabled = true;
	channel->process_reference =
		bstrdup(g_mock_state.process_reference);
	channel->failure_threshold = 3;
	channel->max_reconnect_attempts = 5;
	channel->reconnect_delay_sec = 0; // No delay for testing

	/* Enable auto-reconnect on outputs */
	channel->outputs[0].auto_reconnect_enabled = true;
	channel->outputs[1].auto_reconnect_enabled = true;

	/* Set output 1 to have failures at threshold */
	channel->outputs[1].consecutive_failures = 2;

	/* Mock only one output as healthy */
	g_mock_state.process_state = bstrdup("running");
	g_mock_state.output_count = 1;
	g_mock_state.output_ids = bzalloc(sizeof(char *) * 1);
	g_mock_state.output_ids[0] = bstrdup("YouTube_0");
	g_mock_state.add_output_should_succeed = true;

	/* Health check should trigger auto-reconnect */
	bool result = channel_check_health(channel, api);
	ASSERT_FALSE(result, "Health check should return false");

	/* Verify output 1 had consecutive_failures incremented to 3 (threshold) */
	ASSERT_EQ(channel->outputs[1].consecutive_failures, 0,
		  "Output 1 failures should be reset after reconnect");
	ASSERT_TRUE(channel->outputs[1].connected,
		    "Output 1 should be reconnected");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 8: Fail when channel not active */
static bool test_reconnect_output_channel_not_active(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as inactive */
	channel->status = CHANNEL_STATUS_INACTIVE;

	/* Reconnect should fail */
	bool result = channel_reconnect_output(channel, api, 0);
	ASSERT_FALSE(result, "Reconnect should fail for inactive channel");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 9: Disable output after max attempts exceeded */
static bool test_reconnect_output_max_attempts_exceeded(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active with max reconnect attempts */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->process_reference =
		bstrdup(g_mock_state.process_reference);
	channel->max_reconnect_attempts = 3;
	channel->reconnect_delay_sec = 0;

	/* Set output to have exceeded max attempts */
	channel->outputs[0].consecutive_failures = 3;
	channel->outputs[0].enabled = true;

	/* Reconnect should fail and disable output */
	bool result = channel_reconnect_output(channel, api, 0);
	ASSERT_FALSE(result, "Reconnect should fail when max attempts exceeded");
	ASSERT_FALSE(channel->outputs[0].enabled,
		     "Output should be disabled after max attempts");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 10: Successfully reconnect output */
static bool test_reconnect_output_success(void)
{
	restreamer_api_t *api = create_mock_api();
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set channel as active */
	channel->status = CHANNEL_STATUS_ACTIVE;
	channel->process_reference =
		bstrdup(g_mock_state.process_reference);
	channel->max_reconnect_attempts = 5;
	channel->reconnect_delay_sec = 0;

	/* Set output to have some failures */
	channel->outputs[0].consecutive_failures = 2;
	channel->outputs[0].connected = false;
	channel->outputs[0].enabled = true;

	/* Mock successful reconnect */
	g_mock_state.add_output_should_succeed = true;

	/* Reconnect should succeed */
	bool result = channel_reconnect_output(channel, api, 0);
	ASSERT_TRUE(result, "Reconnect should succeed");
	ASSERT_TRUE(channel->outputs[0].connected,
		    "Output should be marked as connected");
	ASSERT_EQ(channel->outputs[0].consecutive_failures, 0,
		  "Failures should be reset");

	destroy_test_channel(channel);
	destroy_mock_api();
	return true;
}

/* Test 11: Enable monitoring and set defaults */
static bool test_set_health_monitoring_enable(void)
{
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Initially monitoring disabled with no defaults */
	ASSERT_FALSE(channel->health_monitoring_enabled,
		     "Monitoring should be disabled initially");
	ASSERT_EQ(channel->health_check_interval_sec, 0,
		  "Health check interval should be 0 initially");
	ASSERT_EQ(channel->failure_threshold, 0,
		  "Failure threshold should be 0 initially");
	ASSERT_EQ(channel->max_reconnect_attempts, 0,
		  "Max reconnect attempts should be 0 initially");

	/* Enable monitoring */
	channel_set_health_monitoring(channel, true);

	/* Verify monitoring enabled and defaults set */
	ASSERT_TRUE(channel->health_monitoring_enabled,
		    "Monitoring should be enabled");
	ASSERT_EQ(channel->health_check_interval_sec, 30,
		  "Health check interval should be 30");
	ASSERT_EQ(channel->failure_threshold, 3,
		  "Failure threshold should be 3");
	ASSERT_EQ(channel->max_reconnect_attempts, 5,
		  "Max reconnect attempts should be 5");

	/* Verify auto-reconnect enabled for all outputs */
	ASSERT_TRUE(channel->outputs[0].auto_reconnect_enabled,
		    "Auto-reconnect should be enabled for output 0");
	ASSERT_TRUE(channel->outputs[1].auto_reconnect_enabled,
		    "Auto-reconnect should be enabled for output 1");

	destroy_test_channel(channel);
	return true;
}

/* Test 12: Disable monitoring for all outputs */
static bool test_set_health_monitoring_disable(void)
{
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Enable monitoring first */
	channel_set_health_monitoring(channel, true);
	ASSERT_TRUE(channel->health_monitoring_enabled,
		    "Monitoring should be enabled");
	ASSERT_TRUE(channel->outputs[0].auto_reconnect_enabled,
		    "Auto-reconnect should be enabled");

	/* Disable monitoring */
	channel_set_health_monitoring(channel, false);

	/* Verify monitoring disabled */
	ASSERT_FALSE(channel->health_monitoring_enabled,
		     "Monitoring should be disabled");

	/* Verify auto-reconnect disabled for all outputs */
	ASSERT_FALSE(channel->outputs[0].auto_reconnect_enabled,
		     "Auto-reconnect should be disabled for output 0");
	ASSERT_FALSE(channel->outputs[1].auto_reconnect_enabled,
		     "Auto-reconnect should be disabled for output 1");

	destroy_test_channel(channel);
	return true;
}

/* Test 13: Don't override existing settings when enabling */
static bool test_set_health_monitoring_preserves_custom_settings(void)
{
	stream_channel_t *channel = create_test_channel("Test", true);

	/* Set custom values */
	channel->health_check_interval_sec = 60;
	channel->failure_threshold = 5;
	channel->max_reconnect_attempts = 10;

	/* Enable monitoring */
	channel_set_health_monitoring(channel, true);

	/* Verify custom values preserved */
	ASSERT_EQ(channel->health_check_interval_sec, 60,
		  "Custom health check interval should be preserved");
	ASSERT_EQ(channel->failure_threshold, 5,
		  "Custom failure threshold should be preserved");
	ASSERT_EQ(channel->max_reconnect_attempts, 10,
		  "Custom max reconnect attempts should be preserved");

	destroy_test_channel(channel);
	return true;
}

/* ========================================================================
 * Test Suite
 * ======================================================================== */

bool run_channel_health_tests(void) {
    bool all_passed = true;

    printf("\n");
    printf("========================================================================\n");
    printf("Channel Health Monitoring Tests\n");
    printf("========================================================================\n");

    RUN_TEST(test_check_health_not_active,
             "Health check returns true when channel not active");
    RUN_TEST(test_check_health_monitoring_disabled,
             "Health check returns true when monitoring disabled");
    RUN_TEST(test_check_health_no_process_reference,
             "Health check returns false when no process reference");
    RUN_TEST(test_check_health_process_not_found,
             "Health check returns false when process not found");
    RUN_TEST(test_check_health_all_outputs_healthy,
             "Health check returns true when all outputs healthy");
    RUN_TEST(test_check_health_output_unhealthy,
             "Health check detects unhealthy output");
    RUN_TEST(test_check_health_triggers_auto_reconnect,
             "Health check triggers auto-reconnect when threshold reached");
    RUN_TEST(test_reconnect_output_channel_not_active,
             "Reconnect fails when channel not active");
    RUN_TEST(test_reconnect_output_max_attempts_exceeded,
             "Reconnect disables output after max attempts exceeded");
    RUN_TEST(test_reconnect_output_success,
             "Reconnect successfully restores output");
    RUN_TEST(test_set_health_monitoring_enable,
             "Enable monitoring sets default values");
    RUN_TEST(test_set_health_monitoring_disable,
             "Disable monitoring turns off auto-reconnect");
    RUN_TEST(test_set_health_monitoring_preserves_custom_settings,
             "Enable monitoring preserves custom settings");

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
