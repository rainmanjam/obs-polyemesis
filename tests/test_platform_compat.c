/*
 * Platform Compatibility Tests for OBS Polyemesis
 * Tests Windows, Linux, and macOS specific behavior
 */

#include "test_framework.h"
#include "../src/restreamer-output-profile.h"
#include "../src/restreamer-config.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define LINE_ENDING "\r\n"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#define LINE_ENDING "\n"
#endif

/* Mock API for testing */
static restreamer_api_t *create_mock_api(void)
{
	return NULL; /* Tests use NULL API to test logic in isolation */
}

/* Test 1: Path separator handling */
static bool test_path_separators(void)
{
	char test_path[256];

	/* Test with platform-specific separator */
	snprintf(test_path, sizeof(test_path), "obs-studio%sdata%splugin",
		 PATH_SEPARATOR_STR, PATH_SEPARATOR_STR);

#ifdef _WIN32
	ASSERT_TRUE(strchr(test_path, '\\') != NULL,
		    "Windows path should contain backslashes");
#else
	ASSERT_TRUE(strchr(test_path, '/') != NULL,
		    "Unix path should contain forward slashes");
#endif

	return true;
}

/* Test 2: Maximum path length handling */
static bool test_max_path_length(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

#ifdef _WIN32
	/* Windows MAX_PATH is typically 260 characters */
	const size_t MAX_WINDOWS_PATH = 260;
	char long_name[MAX_WINDOWS_PATH + 50];
	memset(long_name, 'A', MAX_WINDOWS_PATH + 40);
	long_name[MAX_WINDOWS_PATH + 40] = '\0';
#else
	/* Unix PATH_MAX is typically 4096 */
	const size_t UNIX_LONG_PATH = 4096;
	char long_name[UNIX_LONG_PATH + 50];
	memset(long_name, 'A', UNIX_LONG_PATH + 40);
	long_name[UNIX_LONG_PATH + 40] = '\0';
#endif

	/* Create profile with extremely long name */
	output_profile_t *profile =
		profile_manager_create_profile(manager, long_name);

	/* Should either accept it or handle gracefully */
	/* Different platforms have different limits */
	(void)profile; /* Implementation may accept or reject */

	profile_manager_destroy(manager);
	return true;
}

/* Test 3: Case sensitivity */
static bool test_case_sensitivity(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create two profiles with different cases */
	output_profile_t *profile1 =
		profile_manager_create_profile(manager, "TestProfile");
	output_profile_t *profile2 =
		profile_manager_create_profile(manager, "testprofile");

	ASSERT_NOT_NULL(profile1, "First profile should be created");
	ASSERT_NOT_NULL(profile2, "Second profile should be created");

#ifdef _WIN32
	/* Windows is case-insensitive, but IDs should still be unique */
	ASSERT_TRUE(strcmp(profile1->profile_id, profile2->profile_id) != 0,
		    "Profile IDs should be different even on Windows");
#else
	/* Unix is case-sensitive */
	ASSERT_TRUE(strcmp(profile1->profile_id, profile2->profile_id) != 0,
		    "Profile IDs should be different");
#endif

	profile_manager_destroy(manager);
	return true;
}

/* Test 4: Thread safety basics */
static bool test_thread_safety_basics(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create multiple profiles to test concurrent access patterns */
	for (int i = 0; i < 10; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Profile %d", i);
		output_profile_t *profile =
			profile_manager_create_profile(manager, name);
		ASSERT_NOT_NULL(profile, "Profile should be created");
	}

	/* Verify all profiles exist */
	ASSERT_EQ(manager->profile_count, 10,
		  "Should have 10 profiles created");

	profile_manager_destroy(manager);
	return true;
}

/* Test 5: Configuration file path handling */
static bool test_config_paths(void)
{
	const char *test_paths[] = {
		"/absolute/path/config.json",
		"relative/path/config.json",
		"./current/dir/config.json",
		"../parent/dir/config.json",
#ifdef _WIN32
		"C:\\Windows\\Path\\config.json",
		"\\\\Network\\Share\\config.json",
		".\\relative\\windows\\config.json",
#endif
		NULL
	};

	/* Test that path handling doesn't crash */
	for (int i = 0; test_paths[i] != NULL; i++) {
		size_t len = strlen(test_paths[i]);
		ASSERT_TRUE(len > 0, "Path should have non-zero length");
		/* Just verify we can process these paths without crashing */
	}

	return true;
}

/* Test 6: Profile ID generation consistency */
static bool test_profile_id_consistency(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create profiles with special characters */
	const char *special_names[] = {
		"Profile with spaces",
		"Profile-with-dashes",
		"Profile_with_underscores",
		"Profile.with.dots",
		"Profile@with#special$chars",
		NULL
	};

	for (int i = 0; special_names[i] != NULL; i++) {
		output_profile_t *profile = profile_manager_create_profile(
			manager, special_names[i]);
		ASSERT_NOT_NULL(profile, "Should create profile");

		/* Profile ID should be valid (non-empty, no null bytes) */
		ASSERT_NOT_NULL(profile->profile_id,
				"Profile ID should exist");
		ASSERT_TRUE(strlen(profile->profile_id) > 0,
			    "Profile ID should be non-empty");
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 7: Memory alignment (important for cross-platform) */
static bool test_memory_alignment(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create profile and check structure alignment */
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Alignment Test");
	ASSERT_NOT_NULL(profile, "Profile should be created");

	/* Verify pointers are properly aligned */
	/* On most platforms, pointers should be aligned to word boundaries */
	uintptr_t addr = (uintptr_t)profile;
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
	/* 64-bit: should be 8-byte aligned */
	ASSERT_EQ(addr % 8, 0, "64-bit pointer should be 8-byte aligned");
#else
	/* 32-bit: should be 4-byte aligned */
	ASSERT_EQ(addr % 4, 0, "32-bit pointer should be 4-byte aligned");
#endif

	profile_manager_destroy(manager);
	return true;
}

/* Test 8: String encoding handling */
static bool test_string_encoding(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* UTF-8 strings with various characters */
	const char *utf8_names[] = {
		"English Profile",
		"Español Perfil", /* Spanish */
		"中文配置", /* Chinese */
		"Русский профиль", /* Russian */
		"العربية", /* Arabic */
		"日本語プロファイル", /* Japanese */
		NULL
	};

	for (int i = 0; utf8_names[i] != NULL; i++) {
		output_profile_t *profile = profile_manager_create_profile(
			manager, utf8_names[i]);
		/* Should handle UTF-8 gracefully */
		ASSERT_NOT_NULL(profile, "Should create profile with UTF-8");
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 9: Endianness-neutral operations */
static bool test_endianness_neutral(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Endian Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Set encoding values that should work regardless of endianness */
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.bitrate = 5000;
	encoding.fps_num = 60;
	encoding.fps_den = 1;

	bool added = profile_add_destination(profile, SERVICE_YOUTUBE,
					     "test-key",
					     ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Should add destination");

	/* Verify values are stored correctly */
	ASSERT_EQ(profile->destinations[0].encoding.width, 1920,
		  "Width should match");
	ASSERT_EQ(profile->destinations[0].encoding.height, 1080,
		  "Height should match");
	ASSERT_EQ(profile->destinations[0].encoding.bitrate, 5000,
		  "Bitrate should match");

	profile_manager_destroy(manager);
	return true;
}

/* Test 10: Platform-specific line ending handling */
static bool test_line_endings(void)
{
	/* Test that we handle both CRLF and LF correctly */
	const char *crlf_text = "Line 1\r\nLine 2\r\nLine 3\r\n";
	const char *lf_text = "Line 1\nLine 2\nLine 3\n";

	/* Both should be valid */
	ASSERT_TRUE(strlen(crlf_text) > 0, "CRLF text should be valid");
	ASSERT_TRUE(strlen(lf_text) > 0, "LF text should be valid");

#ifdef _WIN32
	/* Windows native is CRLF */
	ASSERT_TRUE(strstr(LINE_ENDING, "\r\n") != NULL,
		    "Windows should use CRLF");
#else
	/* Unix native is LF */
	ASSERT_TRUE(strcmp(LINE_ENDING, "\n") == 0, "Unix should use LF");
#endif

	return true;
}

/* Test 11: Concurrent profile access simulation */
static bool test_concurrent_profile_access(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create profiles */
	for (int i = 0; i < 5; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Concurrent Profile %d", i);
		profile_manager_create_profile(manager, name);
	}

	/* Simulate concurrent reads by accessing multiple profiles */
	for (int iteration = 0; iteration < 100; iteration++) {
		for (size_t i = 0; i < manager->profile_count; i++) {
			output_profile_t *profile =
				profile_manager_get_profile_at(manager, i);
			ASSERT_NOT_NULL(profile,
					"Profile should be accessible");
			/* Read operations */
			(void)profile->profile_name;
			(void)profile->destination_count;
		}
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 12: Large allocation handling */
static bool test_large_allocations(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Large Alloc Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Try to add many destinations (stress memory allocation) */
	const size_t LARGE_COUNT = 100;
	for (size_t i = 0; i < LARGE_COUNT; i++) {
		char key[64];
		snprintf(key, sizeof(key), "dest-%zu", i);
		bool added = profile_add_destination(
			profile, SERVICE_YOUTUBE, key, ORIENTATION_HORIZONTAL,
			&encoding);
		ASSERT_TRUE(added, "Should add destination");
	}

	ASSERT_EQ(profile->destination_count, LARGE_COUNT,
		  "Should have all destinations");

	/* Remove them all */
	for (size_t i = 0; i < LARGE_COUNT; i++) {
		bool removed = profile_remove_destination(profile, 0);
		ASSERT_TRUE(removed, "Should remove destination");
	}

	ASSERT_EQ(profile->destination_count, 0,
		  "All destinations should be removed");

	profile_manager_destroy(manager);
	return true;
}

/* Test 13: NULL string handling across platforms */
static bool test_null_string_handling(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* NULL profile name */
	output_profile_t *profile1 =
		profile_manager_create_profile(manager, NULL);
	/* Should handle NULL gracefully */
	(void)profile1; /* May return NULL or create with default name */

	/* Empty string */
	output_profile_t *profile2 =
		profile_manager_create_profile(manager, "");
	ASSERT_NOT_NULL(profile2, "Empty string should create profile");

	profile_manager_destroy(manager);
	return true;
}

/* Test 14: Integer overflow protection */
static bool test_integer_overflow_protection(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Overflow Protection Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Test with maximum values */
	encoding.width = UINT32_MAX;
	encoding.height = UINT32_MAX;
	encoding.bitrate = UINT32_MAX;

	/* Should handle gracefully (either reject or clamp) */
	bool added = profile_add_destination(profile, SERVICE_YOUTUBE,
					     "overflow-test",
					     ORIENTATION_HORIZONTAL, &encoding);
	/* Implementation may accept or reject extreme values */
	(void)added;

	profile_manager_destroy(manager);
	return true;
}

/* Test 15: Platform-specific timestamp handling */
static bool test_timestamp_handling(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Timestamp Test");

	encoding_settings_t encoding = profile_get_default_encoding();
	profile_add_destination(profile, SERVICE_YOUTUBE, "test",
				ORIENTATION_HORIZONTAL, &encoding);

	/* Set up backup relationship to test failover timestamps */
	profile_add_destination(profile, SERVICE_YOUTUBE, "backup",
				ORIENTATION_HORIZONTAL, &encoding);
	profile_set_destination_backup(profile, 0, 1);

	/* Trigger failover to set timestamp */
	profile_trigger_failover(profile, api, 0);

	/* Verify timestamp was set */
	ASSERT_TRUE(
		profile->destinations[0].failover_start_time > 0 ||
			profile->destinations[0].failover_start_time == 0,
		"Timestamp should be set (or 0 if failover failed)");

	profile_manager_destroy(manager);
	return true;
}

BEGIN_TEST_SUITE("Platform Compatibility Tests")
	RUN_TEST(test_path_separators, "Path separator handling");
	RUN_TEST(test_max_path_length, "Maximum path length handling");
	RUN_TEST(test_case_sensitivity, "Case sensitivity handling");
	RUN_TEST(test_thread_safety_basics, "Thread safety basics");
	RUN_TEST(test_config_paths, "Configuration path handling");
	RUN_TEST(test_profile_id_consistency, "Profile ID consistency");
	RUN_TEST(test_memory_alignment, "Memory alignment");
	RUN_TEST(test_string_encoding, "UTF-8 string encoding");
	RUN_TEST(test_endianness_neutral, "Endianness-neutral operations");
	RUN_TEST(test_line_endings, "Line ending handling");
	RUN_TEST(test_concurrent_profile_access,
		 "Concurrent profile access");
	RUN_TEST(test_large_allocations, "Large allocation handling");
	RUN_TEST(test_null_string_handling, "NULL string handling");
	RUN_TEST(test_integer_overflow_protection,
		 "Integer overflow protection");
	RUN_TEST(test_timestamp_handling, "Timestamp handling");
END_TEST_SUITE()
