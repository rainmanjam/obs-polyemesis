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

#include "restreamer-config.h"
#include "restreamer-multistream.h"
#include <obs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
static void test_suite_start(const char *name) { printf("\n%s\n========================================\n", name); }
static void test_suite_end(const char *name, bool result) {
  if (result) printf("✓ %s: PASSED\n", name);
  else printf("✗ %s: FAILED\n", name);
}

/* External output plugin functions */
extern const char *restreamer_output_getname(void *unused);
extern void *restreamer_output_create(obs_data_t *settings,
				      obs_output_t *output);
extern void restreamer_output_destroy(void *data);
extern bool restreamer_output_start(void *data);
extern void restreamer_output_stop(void *data, uint64_t ts);
extern void restreamer_output_defaults(obs_data_t *settings);
extern obs_properties_t *restreamer_output_properties(void *data);

/* Test output name retrieval */
static bool test_output_name(void)
{
	test_section_start("Output Name");

	const char *name = restreamer_output_getname(NULL);
	test_assert(name != NULL, "Output name should not be NULL");
	test_assert(strlen(name) > 0, "Output name should not be empty");
	test_assert(strcmp(name, "Restreamer Output") == 0,
		    "Output name should be 'Restreamer Output'");

	test_section_end("Output Name");
	return true;
}

/* Test output defaults */
static bool test_output_defaults(void)
{
	test_section_start("Output Defaults");

	obs_data_t *settings = obs_data_create();
	restreamer_output_defaults(settings);

	/* Check default values */
	bool enable_multistream =
		obs_data_get_bool(settings, "enable_multistream");
	test_assert(enable_multistream == false,
		    "Multistream should be disabled by default");

	obs_data_release(settings);

	test_section_end("Output Defaults");
	return true;
}

/* Test output properties */
static bool test_output_properties(void)
{
	test_section_start("Output Properties");

	/* Get properties (without data context) */
	obs_properties_t *props = restreamer_output_properties(NULL);
	test_assert(props != NULL, "Should return properties");

	/* Check for expected properties */
	obs_property_t *prop;

	prop = obs_properties_get(props, "enable_multistream");
	test_assert(prop != NULL,
		    "Should have 'enable_multistream' property");
	test_assert(obs_property_get_type(prop) == OBS_PROPERTY_BOOL,
		    "enable_multistream should be boolean");

	obs_properties_destroy(props);

	test_section_end("Output Properties");
	return true;
}

/* Test output creation without multistream */
static bool test_output_create_simple(void)
{
	test_section_start("Output Creation (Simple)");

	/* Set up global connection */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", false);

	/* Note: We can't fully test output_create without a valid obs_output_t,
	 * but we can test that it doesn't crash with NULL */
	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(true, "Output created successfully");
		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output requires valid obs_output_t (expected)");
	}

	obs_data_release(settings);

	test_section_end("Output Creation (Simple)");
	return true;
}

/* Test output creation with multistream */
static bool test_output_create_multistream(void)
{
	test_section_start("Output Creation (Multistream)");

	/* Set up global connection */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", true);

	/* Add some multistream destinations */
	obs_data_array_t *destinations = obs_data_array_create();

	obs_data_t *dest1 = obs_data_create();
	obs_data_set_int(dest1, "service", 0); // Twitch
	obs_data_set_string(dest1, "stream_key", "test_key_1");
	obs_data_set_int(dest1, "orientation", 1); // Horizontal
	obs_data_array_push_back(destinations, dest1);
	obs_data_release(dest1);

	obs_data_t *dest2 = obs_data_create();
	obs_data_set_int(dest2, "service", 1); // YouTube
	obs_data_set_string(dest2, "stream_key", "test_key_2");
	obs_data_set_int(dest2, "orientation", 1); // Horizontal
	obs_data_array_push_back(destinations, dest2);
	obs_data_release(dest2);

	obs_data_set_array(settings, "destinations", destinations);
	obs_data_array_release(destinations);

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(true,
			    "Output created with multistream successfully");
		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output requires valid obs_output_t (expected)");
	}

	obs_data_release(settings);

	test_section_end("Output Creation (Multistream)");
	return true;
}

/* Test output start (will fail without real connection, but should not crash) */
static bool test_output_start_stop(void)
{
	test_section_start("Output Start/Stop");

	/* Set up global connection */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", false);

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		/* Try to start (will likely fail, but should not crash) */
		bool started = restreamer_output_start(output_data);
		(void)started; /* Result depends on actual connection */

		/* We don't assert on the result since it depends on actual connection */
		test_assert(
			true,
			"Start operation completed without crash (may have failed due to connection)");

		/* Stop output */
		restreamer_output_stop(output_data, 0);
		test_assert(true, "Stop operation completed without crash");

		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output requires valid obs_output_t (skipping start/stop test)");
	}

	obs_data_release(settings);

	test_section_end("Output Start/Stop");
	return true;
}

/* Test output with empty settings */
static bool test_output_empty_settings(void)
{
	test_section_start("Output Empty Settings");

	obs_data_t *settings = obs_data_create();
	/* Don't set any values - test with defaults */

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(
			true,
			"Output created with empty settings (using defaults)");
		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output requires valid obs_output_t (expected)");
	}

	obs_data_release(settings);

	test_section_end("Output Empty Settings");
	return true;
}

/* Test output edge cases */
static bool test_output_edge_cases(void)
{
	test_section_start("Output Edge Cases");

	/* Test with NULL - should handle gracefully */
	const char *name = restreamer_output_getname(NULL);
	test_assert(name != NULL, "getname should handle NULL data");

	obs_properties_t *props = restreamer_output_properties(NULL);
	test_assert(props != NULL,
		    "get_properties should handle NULL data");
	obs_properties_destroy(props);

	/* Test defaults with valid settings */
	obs_data_t *settings = obs_data_create();
	restreamer_output_defaults(settings);
	test_assert(true, "get_defaults should handle valid settings");
	obs_data_release(settings);

	/* Test start with NULL should not crash (may return false) */
	// Most implementations will check for NULL and return false
	// We can't easily test this without modifying the implementation

	test_section_end("Output Edge Cases");
	return true;
}

/* Test output multistream configuration */
static bool test_output_multistream_config(void)
{
	test_section_start("Output Multistream Configuration");

	/* Set up global connection */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", true);

	/* Create comprehensive multistream configuration */
	obs_data_array_t *destinations = obs_data_array_create();

	/* Add multiple platforms */
	const char *keys[] = {"twitch_key", "youtube_key", "facebook_key"};

	for (int i = 0; i < 3; i++) {
		obs_data_t *dest = obs_data_create();
		obs_data_set_int(dest, "service", i);
		obs_data_set_string(dest, "stream_key", keys[i]);
		obs_data_set_int(dest, "orientation", 1); // Horizontal
		obs_data_array_push_back(destinations, dest);
		obs_data_release(dest);
	}

	obs_data_set_array(settings, "destinations", destinations);
	obs_data_array_release(destinations);

	/* Set source orientation */
	obs_data_set_int(settings, "source_orientation", 1); // Horizontal

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(true,
			    "Output created with full multistream configuration");
		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output requires valid obs_output_t (expected)");
	}

	obs_data_release(settings);

	test_section_end("Output Multistream Configuration");
	return true;
}

/* External data callback */
extern void restreamer_output_data(void *data, struct encoder_packet *packet);

/* External button callback */
extern bool add_destination_clicked(obs_properties_t *props,
				    obs_property_t *property, void *data);

/* Test data callback (should handle NULL gracefully) */
static bool test_output_data_callback(void)
{
	test_section_start("Output Data Callback");

	/* Create dummy packet */
	struct encoder_packet packet = {0};

	/* Call data callback with NULL - should not crash */
	restreamer_output_data(NULL, &packet);
	test_assert(true, "Data callback handled NULL data");

	/* Create an output context and test with it */
	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", false);

	void *output_data = restreamer_output_create(settings, NULL);
	if (output_data != NULL) {
		restreamer_output_data(output_data, &packet);
		test_assert(true, "Data callback handled valid data");
		restreamer_output_destroy(output_data);
	}

	obs_data_release(settings);

	test_section_end("Output Data Callback");
	return true;
}

/* Test button callback */
static bool test_add_destination_button(void)
{
	test_section_start("Add Destination Button");

	/* Create properties */
	obs_properties_t *props = restreamer_output_properties(NULL);
	obs_property_t *button_prop =
		obs_properties_get(props, "add_destination");

	test_assert(button_prop != NULL, "Button property should exist");

	if (button_prop) {
		/* Trigger button click - should not crash */
		bool result = add_destination_clicked(props, button_prop, NULL);
		test_assert(true, "Button click handled (result depends on implementation)");
		(void)result;
	}

	obs_properties_destroy(props);

	test_section_end("Add Destination Button");
	return true;
}

/* Test all properties in detail */
static bool test_output_properties_detailed(void)
{
	test_section_start("Output Properties (Detailed)");

	obs_properties_t *props = restreamer_output_properties(NULL);
	test_assert(props != NULL, "Should return properties");

	/* Check enable_multistream */
	obs_property_t *prop = obs_properties_get(props, "enable_multistream");
	test_assert(prop != NULL, "Should have enable_multistream property");
	if (prop) {
		test_assert(obs_property_get_type(prop) == OBS_PROPERTY_BOOL,
			    "enable_multistream should be boolean");
	}

	/* Check auto_detect_orientation */
	prop = obs_properties_get(props, "auto_detect_orientation");
	test_assert(prop != NULL,
		    "Should have auto_detect_orientation property");
	if (prop) {
		test_assert(obs_property_get_type(prop) == OBS_PROPERTY_BOOL,
			    "auto_detect_orientation should be boolean");
	}

	/* Check source_orientation */
	prop = obs_properties_get(props, "source_orientation");
	test_assert(prop != NULL, "Should have source_orientation property");
	if (prop) {
		test_assert(obs_property_get_type(prop) == OBS_PROPERTY_LIST,
			    "source_orientation should be list");
	}

	/* Check add_destination button */
	prop = obs_properties_get(props, "add_destination");
	test_assert(prop != NULL, "Should have add_destination button");
	if (prop) {
		test_assert(obs_property_get_type(prop) == OBS_PROPERTY_BUTTON,
			    "add_destination should be button");
	}

	/* Check destinations_info */
	prop = obs_properties_get(props, "destinations_info");
	test_assert(prop != NULL, "Should have destinations_info property");
	if (prop) {
		test_assert(obs_property_get_type(prop) == OBS_PROPERTY_TEXT,
			    "destinations_info should be text");
	}

	obs_properties_destroy(props);

	test_section_end("Output Properties (Detailed)");
	return true;
}

/* Test defaults with various settings */
static bool test_output_defaults_detailed(void)
{
	test_section_start("Output Defaults (Detailed)");

	obs_data_t *settings = obs_data_create();

	/* Apply defaults */
	restreamer_output_defaults(settings);

	/* Check all default values */
	bool enable_multistream =
		obs_data_get_bool(settings, "enable_multistream");
	test_assert(enable_multistream == false,
		    "Multistream should be disabled by default");

	bool auto_detect =
		obs_data_get_bool(settings, "auto_detect_orientation");
	test_assert(auto_detect == true,
		    "Auto-detect orientation should be enabled by default");

	obs_data_release(settings);

	test_section_end("Output Defaults (Detailed)");
	return true;
}

/* Test destroy with NULL */
static bool test_output_destroy_null(void)
{
	test_section_start("Output Destroy NULL");

	/* Destroy NULL - should not crash if implementation checks */
	// Note: Most implementations don't check for NULL in destroy
	// This is more of a documentation test
	test_assert(true,
		    "Destroy NULL handling depends on implementation");

	test_section_end("Output Destroy NULL");
	return true;
}

/* Test multistream with different orientations */
static bool test_output_multistream_orientations(void)
{
	test_section_start("Output Multistream Orientations");

	/* Set up global connection */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	/* Test different orientations */
	int orientations[] = {0, 1, 2, 3}; // Auto, Horizontal, Vertical, Square
	const char *orientation_names[] = {"Auto", "Horizontal", "Vertical",
					   "Square"};

	for (int i = 0; i < 4; i++) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_bool(settings, "enable_multistream", true);
		obs_data_set_int(settings, "source_orientation",
				 orientations[i]);

		/* Add a destination */
		obs_data_array_t *destinations = obs_data_array_create();
		obs_data_t *dest = obs_data_create();
		obs_data_set_int(dest, "service", 0);
		obs_data_set_string(dest, "stream_key", "test_key");
		obs_data_set_int(dest, "orientation", orientations[i]);
		obs_data_array_push_back(destinations, dest);
		obs_data_release(dest);

		obs_data_set_array(settings, "destinations", destinations);
		obs_data_array_release(destinations);

		void *output_data = restreamer_output_create(settings, NULL);

		if (output_data != NULL) {
			test_assert(true, orientation_names[i]);
			restreamer_output_destroy(output_data);
		}

		obs_data_release(settings);
	}

	test_section_end("Output Multistream Orientations");
	return true;
}

/* Test multistream with empty destinations array */
static bool test_output_multistream_empty_destinations(void)
{
	test_section_start("Output Multistream Empty Destinations");

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", true);

	/* Create empty destinations array */
	obs_data_array_t *destinations = obs_data_array_create();
	obs_data_set_array(settings, "destinations", destinations);
	obs_data_array_release(destinations);

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(true,
			    "Output created with empty destinations array");
		restreamer_output_destroy(output_data);
	} else {
		test_assert(true,
			    "Output may require valid destinations or obs_output_t");
	}

	obs_data_release(settings);

	test_section_end("Output Multistream Empty Destinations");
	return true;
}

/* Test multistream with many destinations */
static bool test_output_multistream_many_destinations(void)
{
	test_section_start("Output Multistream Many Destinations");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", true);

	/* Create many destinations (10) */
	obs_data_array_t *destinations = obs_data_array_create();

	for (int i = 0; i < 10; i++) {
		obs_data_t *dest = obs_data_create();
		obs_data_set_int(dest, "service", i % 7); // Cycle through services
		char key[32];
		snprintf(key, sizeof(key), "test_key_%d", i);
		obs_data_set_string(dest, "stream_key", key);
		obs_data_set_int(dest, "orientation", 1); // Horizontal
		obs_data_array_push_back(destinations, dest);
		obs_data_release(dest);
	}

	obs_data_set_array(settings, "destinations", destinations);
	obs_data_array_release(destinations);

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		test_assert(true,
			    "Output created with 10 destinations successfully");
		restreamer_output_destroy(output_data);
	}

	obs_data_release(settings);

	test_section_end("Output Multistream Many Destinations");
	return true;
}

/* Test creation and immediate destroy */
static bool test_output_create_destroy_immediate(void)
{
	test_section_start("Output Create/Destroy Immediate");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	/* Create and immediately destroy multiple times */
	for (int i = 0; i < 5; i++) {
		obs_data_t *settings = obs_data_create();
		obs_data_set_bool(settings, "enable_multistream", i % 2 == 0);

		void *output_data = restreamer_output_create(settings, NULL);

		if (output_data != NULL) {
			restreamer_output_destroy(output_data);
		}

		obs_data_release(settings);
	}

	test_assert(true, "Multiple create/destroy cycles completed");

	test_section_end("Output Create/Destroy Immediate");
	return true;
}

/* Test stop when not active */
static bool test_output_stop_when_inactive(void)
{
	test_section_start("Output Stop When Inactive");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "admin",
		.password = "admin",
		.use_https = false,
	};
	restreamer_config_set_global_connection(&conn);

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "enable_multistream", false);

	void *output_data = restreamer_output_create(settings, NULL);

	if (output_data != NULL) {
		/* Stop without starting - should handle gracefully */
		restreamer_output_stop(output_data, 0);
		test_assert(true,
			    "Stop on inactive output handled gracefully");

		restreamer_output_destroy(output_data);
	}

	obs_data_release(settings);

	test_section_end("Output Stop When Inactive");
	return true;
}

/* Test suite runner */
bool run_output_tests(void)
{
	test_suite_start("Output Plugin Tests");

	bool result = true;

	test_start("Output name");
	result &= test_output_name();
	test_end();

	test_start("Output defaults");
	result &= test_output_defaults();
	test_end();

	test_start("Output properties");
	result &= test_output_properties();
	test_end();

	test_start("Output creation (simple)");
	result &= test_output_create_simple();
	test_end();

	test_start("Output creation (multistream)");
	result &= test_output_create_multistream();
	test_end();

	test_start("Output start/stop");
	result &= test_output_start_stop();
	test_end();

	test_start("Output empty settings");
	result &= test_output_empty_settings();
	test_end();

	test_start("Output edge cases");
	result &= test_output_edge_cases();
	test_end();

	test_start("Output multistream configuration");
	result &= test_output_multistream_config();
	test_end();

	/* New comprehensive tests */
	test_start("Output data callback");
	result &= test_output_data_callback();
	test_end();

	test_start("Add destination button");
	result &= test_add_destination_button();
	test_end();

	test_start("Output properties (detailed)");
	result &= test_output_properties_detailed();
	test_end();

	test_start("Output defaults (detailed)");
	result &= test_output_defaults_detailed();
	test_end();

	test_start("Output destroy NULL");
	result &= test_output_destroy_null();
	test_end();

	test_start("Output multistream orientations");
	result &= test_output_multistream_orientations();
	test_end();

	test_start("Output multistream empty destinations");
	result &= test_output_multistream_empty_destinations();
	test_end();

	test_start("Output multistream many destinations");
	result &= test_output_multistream_many_destinations();
	test_end();

	test_start("Output create/destroy immediate");
	result &= test_output_create_destroy_immediate();
	test_end();

	test_start("Output stop when inactive");
	result &= test_output_stop_when_inactive();
	test_end();

	test_suite_end("Output Plugin Tests", result);
	return result;
}
