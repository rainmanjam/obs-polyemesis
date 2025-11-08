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
              "  ✗ FAIL: %s\n    Expected: \"%s\", Actual: \"%s\"\n    at "  \
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

/* Run all multistream tests */
bool run_multistream_tests(void) {
  bool all_passed = true;

  all_passed &= test_orientation_detection();
  all_passed &= test_service_urls();
  all_passed &= test_service_names();
  all_passed &= test_add_destinations();
  all_passed &= test_empty_config();

  return all_passed;
}
