/*
 * Multistream Tests
 *
 * Tests for multistreaming logic and orientation detection
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "restreamer-multistream.h"

/* Test macros */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message)                       \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: \"%s\", Actual: \"%s\"\n    at "    \
              "%s:%d\n",                                                       \
              message, (expected), (actual), __FILE__, __LINE__);              \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: Orientation detection */
static bool test_orientation_detection(void) {
  printf("  Testing orientation detection...\n");

  /* Test landscape (16:9) */
  stream_orientation_t orientation =
      restreamer_multistream_detect_orientation(1920, 1080);
  TEST_ASSERT_EQUAL(ORIENTATION_HORIZONTAL, orientation,
                    "1920x1080 should be horizontal");

  /* Test portrait (9:16) */
  orientation = restreamer_multistream_detect_orientation(1080, 1920);
  TEST_ASSERT_EQUAL(ORIENTATION_VERTICAL, orientation,
                    "1080x1920 should be vertical");

  /* Test square */
  orientation = restreamer_multistream_detect_orientation(1080, 1080);
  TEST_ASSERT_EQUAL(ORIENTATION_SQUARE, orientation,
                    "1080x1080 should be square");

  printf("  ✓ Orientation detection\n");
  return true;
}

/* Test: Service URL generation */
static bool test_service_urls(void) {
  printf("  Testing service URL generation...\n");

  const char *url;

  /* Test Twitch */
  url = restreamer_multistream_get_service_url(SERVICE_TWITCH,
                                               ORIENTATION_HORIZONTAL);
  TEST_ASSERT(url != NULL, "Twitch URL should not be NULL");
  TEST_ASSERT_STR_EQUAL("rtmp://live.twitch.tv/app", url,
                        "Twitch URL should match");

  /* Test YouTube */
  url = restreamer_multistream_get_service_url(SERVICE_YOUTUBE,
                                               ORIENTATION_HORIZONTAL);
  TEST_ASSERT(url != NULL, "YouTube URL should not be NULL");
  TEST_ASSERT_STR_EQUAL("rtmp://a.rtmp.youtube.com/live2", url,
                        "YouTube URL should match");

  /* Test TikTok with different orientations */
  url = restreamer_multistream_get_service_url(SERVICE_TIKTOK,
                                               ORIENTATION_HORIZONTAL);
  TEST_ASSERT_STR_EQUAL("rtmp://live.tiktok.com/live/horizontal", url,
                        "TikTok horizontal URL should match");

  url = restreamer_multistream_get_service_url(SERVICE_TIKTOK,
                                               ORIENTATION_VERTICAL);
  TEST_ASSERT_STR_EQUAL("rtmp://live.tiktok.com/live", url,
                        "TikTok vertical URL should match");

  printf("  ✓ Service URL generation\n");
  return true;
}

/* Test: Service names */
static bool test_service_names(void) {
  printf("  Testing service names...\n");

  TEST_ASSERT_STR_EQUAL("Twitch",
                        restreamer_multistream_get_service_name(SERVICE_TWITCH),
                        "Twitch name should match");
  TEST_ASSERT_STR_EQUAL(
      "YouTube", restreamer_multistream_get_service_name(SERVICE_YOUTUBE),
      "YouTube name should match");
  TEST_ASSERT_STR_EQUAL(
      "Instagram", restreamer_multistream_get_service_name(SERVICE_INSTAGRAM),
      "Instagram name should match");

  printf("  ✓ Service names\n");
  return true;
}

/* Test: Adding destinations */
static bool test_add_destinations(void) {
  printf("  Testing add destinations...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Should create multistream config");

  /* Initially no destinations */
  TEST_ASSERT_EQUAL(0, config->destination_count,
                    "Should have 0 destinations initially");

  /* Add Twitch destination */
  bool result = restreamer_multistream_add_destination(
      config, SERVICE_TWITCH, "test_key_123", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "Should add Twitch destination");
  TEST_ASSERT_EQUAL(1, config->destination_count, "Should have 1 destination");

  /* Add YouTube destination */
  result = restreamer_multistream_add_destination(
      config, SERVICE_YOUTUBE, "youtube_key", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "Should add YouTube destination");
  TEST_ASSERT_EQUAL(2, config->destination_count, "Should have 2 destinations");

  /* Add Instagram (portrait-only) */
  result = restreamer_multistream_add_destination(
      config, SERVICE_INSTAGRAM, "instagram_key", ORIENTATION_VERTICAL);
  TEST_ASSERT(result, "Should add Instagram destination");
  TEST_ASSERT_EQUAL(3, config->destination_count, "Should have 3 destinations");

  /* Verify first destination */
  TEST_ASSERT_EQUAL(SERVICE_TWITCH, config->destinations[0].service,
                    "First destination should be Twitch");
  TEST_ASSERT_STR_EQUAL("test_key_123", config->destinations[0].stream_key,
                        "First destination key should match");

  restreamer_multistream_destroy(config);

  printf("  ✓ Add destinations\n");
  return true;
}

/* Test: Empty configuration */
static bool test_empty_config(void) {
  printf("  Testing empty configuration...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Should create multistream config");

  /* No destinations added */
  TEST_ASSERT_EQUAL(0, config->destination_count,
                    "Should have 0 destinations initially");
  TEST_ASSERT(config->auto_detect_orientation,
              "Auto-detect should be enabled by default");

  restreamer_multistream_destroy(config);

  printf("  ✓ Empty configuration\n");
  return true;
}

/* Test: NULL handling */
static bool test_multistream_null_handling(void) {
  printf("  Testing NULL pointer handling...\n");

  /* Test with NULL config */
  restreamer_multistream_destroy(NULL); /* Should not crash */

  printf("  ✓ NULL pointer handling\n");
  return true;
}

/* Test: Maximum destinations */
static bool test_max_destinations(void) {
  printf("  Testing maximum destinations...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Add many destinations */
  for (int i = 0; i < 10; i++) {
    restreamer_multistream_add_destination(
        config,
        (streaming_service_t)(i % 5), /* Cycle through services */
        "test_key",
        ORIENTATION_HORIZONTAL
    );
  }

  TEST_ASSERT(config->destination_count >= 10, "Should have many destinations");

  restreamer_multistream_destroy(config);

  printf("  ✓ Maximum destinations\n");
  return true;
}

/* Test: Mixed orientations */
static bool test_mixed_orientations(void) {
  printf("  Testing mixed orientations...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Add horizontal destinations */
  restreamer_multistream_add_destination(config, SERVICE_TWITCH, "h_key_1", ORIENTATION_HORIZONTAL);
  restreamer_multistream_add_destination(config, SERVICE_YOUTUBE, "h_key_2", ORIENTATION_HORIZONTAL);

  /* Add vertical destinations */
  restreamer_multistream_add_destination(config, SERVICE_TIKTOK, "v_key_1", ORIENTATION_VERTICAL);
  restreamer_multistream_add_destination(config, SERVICE_INSTAGRAM, "v_key_2", ORIENTATION_VERTICAL);

  /* Add auto orientation */
  restreamer_multistream_add_destination(config, SERVICE_FACEBOOK, "a_key", ORIENTATION_AUTO);

  TEST_ASSERT(config->destination_count == 5, "Should have 5 destinations");

  restreamer_multistream_destroy(config);

  printf("  ✓ Mixed orientations\n");
  return true;
}

/* Test: All services */
static bool test_all_services(void) {
  printf("  Testing all streaming services...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Test all service types */
  streaming_service_t services[] = {
      SERVICE_TWITCH,
      SERVICE_YOUTUBE,
      SERVICE_FACEBOOK,
      SERVICE_TIKTOK,
      SERVICE_INSTAGRAM,
      SERVICE_KICK,
      SERVICE_CUSTOM
  };

  for (size_t i = 0; i < sizeof(services) / sizeof(services[0]); i++) {
    const char *name = restreamer_multistream_get_service_name(services[i]);
    TEST_ASSERT(name != NULL, "Service name should exist");

    const char *url_h = restreamer_multistream_get_service_url(services[i], ORIENTATION_HORIZONTAL);
    const char *url_v = restreamer_multistream_get_service_url(services[i], ORIENTATION_VERTICAL);
    /* URLs may be NULL for CUSTOM service, which is ok */
    (void)url_h;
    (void)url_v;

    restreamer_multistream_add_destination(config, services[i], "test_key", ORIENTATION_HORIZONTAL);
  }

  TEST_ASSERT(config->destination_count == 7, "Should have all service types");

  restreamer_multistream_destroy(config);

  printf("  ✓ All services\n");
  return true;
}

/* Test: Orientation detection edge cases */
static bool test_orientation_edge_cases(void) {
  printf("  Testing orientation detection edge cases...\n");

  /* Test square aspect ratio (1:1) */
  stream_orientation_t orientation = restreamer_multistream_detect_orientation(1080, 1080);
  TEST_ASSERT_EQUAL(ORIENTATION_SQUARE, orientation,
                    "Square should be ORIENTATION_SQUARE");

  /* Test very wide (21:9) */
  orientation = restreamer_multistream_detect_orientation(2560, 1080);
  TEST_ASSERT_EQUAL(ORIENTATION_HORIZONTAL, orientation, "Ultra-wide should be horizontal");

  /* Test portrait mobile (9:16) */
  orientation = restreamer_multistream_detect_orientation(1080, 1920);
  TEST_ASSERT_EQUAL(ORIENTATION_VERTICAL, orientation, "9:16 should be vertical");

  /* Test very tall (9:21) */
  orientation = restreamer_multistream_detect_orientation(1080, 2520);
  TEST_ASSERT_EQUAL(ORIENTATION_VERTICAL, orientation, "Very tall should be vertical");

  /* Test tiny dimensions */
  orientation = restreamer_multistream_detect_orientation(10, 20);
  TEST_ASSERT(orientation != ORIENTATION_AUTO, "Small dimensions should have orientation");

  /* Test large 4K dimensions */
  orientation = restreamer_multistream_detect_orientation(3840, 2160);
  TEST_ASSERT_EQUAL(ORIENTATION_HORIZONTAL, orientation, "4K should be horizontal");

  printf("  ✓ Orientation detection edge cases\n");
  return true;
}

/* Test: Duplicate destinations */
static bool test_duplicate_destinations(void) {
  printf("  Testing duplicate destinations...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Add same destination multiple times */
  restreamer_multistream_add_destination(config, SERVICE_TWITCH, "same_key", ORIENTATION_HORIZONTAL);
  restreamer_multistream_add_destination(config, SERVICE_TWITCH, "same_key", ORIENTATION_HORIZONTAL);
  restreamer_multistream_add_destination(config, SERVICE_TWITCH, "same_key", ORIENTATION_HORIZONTAL);

  /* Should allow duplicates */
  TEST_ASSERT(config->destination_count == 3, "Should allow duplicate destinations");

  restreamer_multistream_destroy(config);

  printf("  ✓ Duplicate destinations\n");
  return true;
}

/* Test: Stream key validation */
static bool test_stream_key_validation(void) {
  printf("  Testing stream key validation...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Test with NULL key */
  bool result = restreamer_multistream_add_destination(config, SERVICE_TWITCH, NULL, ORIENTATION_HORIZONTAL);
  TEST_ASSERT(!result, "Should reject NULL stream key");

  /* Test with empty key */
  result = restreamer_multistream_add_destination(config, SERVICE_YOUTUBE, "", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(!result, "Should reject empty stream key");

  /* Test with valid key */
  result = restreamer_multistream_add_destination(config, SERVICE_FACEBOOK, "valid_key_123", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "Should accept valid stream key");

  /* Test with very long key */
  char long_key[1024];
  memset(long_key, 'a', sizeof(long_key) - 1);
  long_key[sizeof(long_key) - 1] = '\0';
  result = restreamer_multistream_add_destination(config, SERVICE_KICK, long_key, ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "Should handle long stream keys");

  restreamer_multistream_destroy(config);

  printf("  ✓ Stream key validation\n");
  return true;
}

/* Test: Auto orientation detection */
static bool test_auto_orientation(void) {
  printf("  Testing auto orientation detection...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Default should have auto-detect enabled */
  TEST_ASSERT(config->auto_detect_orientation, "Auto-detect should be enabled by default");

  /* Test adding with AUTO orientation */
  bool result = restreamer_multistream_add_destination(config, SERVICE_TWITCH, "key1", ORIENTATION_AUTO);
  TEST_ASSERT(result, "Should accept AUTO orientation");

  /* Manually disable auto-detect */
  config->auto_detect_orientation = false;
  TEST_ASSERT(!config->auto_detect_orientation, "Should be able to disable auto-detect");

  /* Re-enable */
  config->auto_detect_orientation = true;
  TEST_ASSERT(config->auto_detect_orientation, "Should be able to re-enable auto-detect");

  restreamer_multistream_destroy(config);

  printf("  ✓ Auto orientation detection\n");
  return true;
}

/* Test: Service-specific constraints */
static bool test_service_constraints(void) {
  printf("  Testing service-specific constraints...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Test Instagram (portrait-only platform) */
  bool result = restreamer_multistream_add_destination(config, SERVICE_INSTAGRAM, "insta_key", ORIENTATION_VERTICAL);
  TEST_ASSERT(result, "Instagram should accept vertical");

  /* Test TikTok with vertical */
  result = restreamer_multistream_add_destination(config, SERVICE_TIKTOK, "tiktok_key", ORIENTATION_VERTICAL);
  TEST_ASSERT(result, "TikTok should accept vertical");

  /* Test TikTok with horizontal */
  result = restreamer_multistream_add_destination(config, SERVICE_TIKTOK, "tiktok_h", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "TikTok should accept horizontal");

  /* Test traditional platforms */
  result = restreamer_multistream_add_destination(config, SERVICE_TWITCH, "twitch", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "Twitch should accept horizontal");

  result = restreamer_multistream_add_destination(config, SERVICE_YOUTUBE, "yt", ORIENTATION_HORIZONTAL);
  TEST_ASSERT(result, "YouTube should accept horizontal");

  restreamer_multistream_destroy(config);

  printf("  ✓ Service-specific constraints\n");
  return true;
}

/* Test: Config with process reference */
static bool test_process_reference(void) {
  printf("  Testing process reference...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Set process reference */
  if (config->process_reference) {
    bfree(config->process_reference);
  }
  config->process_reference = bstrdup("my-test-process");
  TEST_ASSERT(config->process_reference != NULL, "Process reference should be set");
  TEST_ASSERT(strcmp(config->process_reference, "my-test-process") == 0,
              "Process reference should match");

  restreamer_multistream_destroy(config);

  printf("  ✓ Process reference\n");
  return true;
}

/* Test: Large configuration */
static bool test_large_configuration(void) {
  printf("  Testing large configuration...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Add many destinations across all services */
  for (int i = 0; i < 20; i++) {
    streaming_service_t service = (streaming_service_t)(i % 7);
    char key[32];
    snprintf(key, sizeof(key), "key_%d", i);
    stream_orientation_t orientation = (i % 2 == 0) ? ORIENTATION_HORIZONTAL : ORIENTATION_VERTICAL;

    bool result = restreamer_multistream_add_destination(config, service, key, orientation);
    TEST_ASSERT(result, "Should add destination in large config");
  }

  TEST_ASSERT(config->destination_count == 20, "Should have 20 destinations");

  restreamer_multistream_destroy(config);

  printf("  ✓ Large configuration\n");
  return true;
}

/* Test: Custom service configuration */
static bool test_custom_service(void) {
  printf("  Testing custom service configuration...\n");

  multistream_config_t *config = restreamer_multistream_create();
  TEST_ASSERT(config != NULL, "Config should be created");

  /* Add custom service */
  bool result = restreamer_multistream_add_destination(config, SERVICE_CUSTOM, "custom_key", ORIENTATION_AUTO);
  TEST_ASSERT(result, "Should add custom service");

  /* Verify added */
  TEST_ASSERT(config->destination_count == 1, "Should have 1 destination");
  TEST_ASSERT(config->destinations[0].service == SERVICE_CUSTOM, "Should be custom service");

  restreamer_multistream_destroy(config);

  printf("  ✓ Custom service configuration\n");
  return true;
}

/* Run all multistream tests */
bool run_multistream_tests(void) {
  bool all_passed = true;

  all_passed &= test_orientation_detection();
  all_passed &= test_service_urls();
  all_passed &= test_service_names();
  all_passed &= test_add_destinations();
  all_passed &= test_empty_config();
  all_passed &= test_multistream_null_handling();
  all_passed &= test_max_destinations();
  all_passed &= test_mixed_orientations();
  all_passed &= test_all_services();
  all_passed &= test_orientation_edge_cases();
  all_passed &= test_duplicate_destinations();

  /* New comprehensive multistream tests */
  all_passed &= test_stream_key_validation();
  all_passed &= test_auto_orientation();
  all_passed &= test_service_constraints();
  all_passed &= test_process_reference();
  all_passed &= test_large_configuration();
  all_passed &= test_custom_service();

  return all_passed;
}
