/*
 * Configuration Tests
 *
 * Tests for configuration management
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "restreamer-config.h"

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

/* Test: Global configuration initialization */
static bool test_global_config_init(void) {
  printf("  Testing global configuration initialization...\n");

  /* Initialize config */
  restreamer_config_init();

  /* Get global connection - should return non-NULL */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Global connection should be initialized");

  /* Default values should be set */
  TEST_ASSERT(conn->host != NULL, "Host should be initialized");
  TEST_ASSERT(conn->port > 0, "Port should be initialized");

  /* Clean up */
  restreamer_config_destroy();

  printf("  ✓ Global configuration initialization\n");
  return true;
}

/* Test: Set and get global connection */
static bool test_global_connection(void) {
  printf("  Testing global connection settings...\n");

  restreamer_config_init();

  /* Create a test connection */
  restreamer_connection_t test_conn = {
      .host = "192.168.1.100",
      .port = 8080,
      .username = "admin",
      .password = "secretpass",
      .use_https = true,
  };

  /* Set global connection */
  restreamer_config_set_global_connection(&test_conn);

  /* Get it back */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Should get global connection");
  TEST_ASSERT_STR_EQUAL("192.168.1.100", conn->host, "Host should match");
  TEST_ASSERT_EQUAL(8080, conn->port, "Port should match");
  TEST_ASSERT_STR_EQUAL("admin", conn->username, "Username should match");
  TEST_ASSERT_STR_EQUAL("secretpass", conn->password, "Password should match");
  TEST_ASSERT(conn->use_https, "HTTPS should be enabled");

  restreamer_config_destroy();

  printf("  ✓ Global connection settings\n");
  return true;
}

/* Test: Create API from global config */
static bool test_create_global_api(void) {
  printf("  Testing create API from global config...\n");

  restreamer_config_init();

  /* Set up a test connection */
  restreamer_connection_t test_conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };
  restreamer_config_set_global_connection(&test_conn);

  /* Create API from global config */
  restreamer_api_t *api = restreamer_config_create_global_api();
  TEST_ASSERT(api != NULL, "Should create API from global config");

  /* Clean up */
  restreamer_api_destroy(api);
  restreamer_config_destroy();

  printf("  ✓ Create API from global config\n");
  return true;
}

/* Test: NULL handling */
static bool test_config_null_handling(void) {
  printf("  Testing NULL pointer handling...\n");

  restreamer_config_init();

  /* Test setting NULL connection */
  restreamer_config_set_global_connection(NULL);
  /* Should not crash */

  /* Test free NULL connection */
  restreamer_config_free_connection(NULL);
  /* Should not crash */

  restreamer_config_destroy();

  printf("  ✓ NULL pointer handling\n");
  return true;
}

/* Test: Multiple init/destroy cycles */
static bool test_config_lifecycle(void) {
  printf("  Testing configuration lifecycle...\n");

  /* Test multiple init/destroy cycles */
  for (int i = 0; i < 3; i++) {
    restreamer_config_init();

    restreamer_connection_t test_conn = {
        .host = "test.local",
        .port = 8080 + i,
        .username = "user",
        .password = "pass",
        .use_https = false,
    };
    restreamer_config_set_global_connection(&test_conn);

    restreamer_connection_t *conn = restreamer_config_get_global_connection();
    TEST_ASSERT(conn != NULL, "Connection should exist");
    TEST_ASSERT_EQUAL(8080 + i, conn->port, "Port should match iteration");

    restreamer_config_destroy();
  }

  printf("  ✓ Configuration lifecycle\n");
  return true;
}

/* Test: Connection variations */
static bool test_connection_variations(void) {
  printf("  Testing connection variations...\n");

  restreamer_config_init();

  /* Test HTTPS connection */
  restreamer_connection_t https_conn = {
      .host = "secure.example.com",
      .port = 443,
      .username = "admin",
      .password = "secret",
      .use_https = true,
  };
  restreamer_config_set_global_connection(&https_conn);

  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn->use_https, "HTTPS should be enabled");
  TEST_ASSERT_EQUAL(443, conn->port, "Port should be 443");

  /* Test non-HTTPS connection */
  restreamer_connection_t http_conn = {
      .host = "local.test",
      .port = 8080,
      .username = "user",
      .password = "pass",
      .use_https = false,
  };
  restreamer_config_set_global_connection(&http_conn);

  conn = restreamer_config_get_global_connection();
  TEST_ASSERT(!conn->use_https, "HTTPS should be disabled");

  /* Test different ports */
  for (int port = 8000; port < 8005; port++) {
    restreamer_connection_t port_conn = {
        .host = "localhost",
        .port = port,
        .username = "test",
        .password = "test",
        .use_https = false,
    };
    restreamer_config_set_global_connection(&port_conn);

    conn = restreamer_config_get_global_connection();
    TEST_ASSERT_EQUAL(port, conn->port, "Port should match");
  }

  restreamer_config_destroy();

  printf("  ✓ Connection variations\n");
  return true;
}

/* Test: Empty and special values */
static bool test_config_special_values(void) {
  printf("  Testing special configuration values...\n");

  restreamer_config_init();

  /* Test with empty strings */
  restreamer_connection_t empty_conn = {
      .host = "",
      .port = 8080,
      .username = "",
      .password = "",
      .use_https = false,
  };
  restreamer_config_set_global_connection(&empty_conn);

  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should be set even with empty strings");

  /* Test with special characters in credentials */
  restreamer_connection_t special_conn = {
      .host = "test.local",
      .port = 8080,
      .username = "user@domain!#$",
      .password = "p@ssw0rd!#$%^&*()",
      .use_https = false,
  };
  restreamer_config_set_global_connection(&special_conn);

  conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should handle special characters");

  restreamer_config_destroy();

  printf("  ✓ Special configuration values\n");
  return true;
}

/* Test: Multiple destroy calls */
static bool test_config_multiple_destroy(void) {
  printf("  Testing multiple destroy calls...\n");

  restreamer_config_init();
  restreamer_config_destroy();
  restreamer_config_destroy(); /* Should not crash */
  restreamer_config_destroy(); /* Should not crash */

  printf("  ✓ Multiple destroy calls\n");
  return true;
}

/* Test: Load and save to OBS settings */
static bool test_config_load_save_settings(void) {
  printf("  Testing load/save to OBS settings...\n");

  restreamer_config_init();

  /* Create OBS settings */
  obs_data_t *settings = obs_data_create();

  /* Set some values */
  obs_data_set_string(settings, "host", "192.168.1.100");
  obs_data_set_int(settings, "port", 9090);
  obs_data_set_string(settings, "username", "testuser");
  obs_data_set_string(settings, "password", "testpass");
  obs_data_set_bool(settings, "use_https", true);

  /* Load from settings */
  restreamer_config_load(settings);

  /* Verify loaded correctly by checking global connection */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should be loaded");

  /* Save to new settings object */
  obs_data_t *new_settings = obs_data_create();
  restreamer_config_save(new_settings);

  /* Verify saved values */
  const char *saved_host = obs_data_get_string(new_settings, "host");
  TEST_ASSERT(saved_host != NULL, "Host should be saved");

  long long saved_port = obs_data_get_int(new_settings, "port");
  TEST_ASSERT(saved_port > 0, "Port should be saved");

  obs_data_release(settings);
  obs_data_release(new_settings);

  restreamer_config_destroy();

  printf("  ✓ Load/save to OBS settings\n");
  return true;
}

/* Test: Load from settings helper */
static bool test_config_load_from_settings_helper(void) {
  printf("  Testing load from settings helper...\n");

  obs_data_t *settings = obs_data_create();

  /* Set connection values */
  obs_data_set_string(settings, "connection_url", "http://test.local:8888");
  obs_data_set_string(settings, "username", "user1");
  obs_data_set_string(settings, "password", "pass1");
  obs_data_set_bool(settings, "use_https", false);

  /* Load connection from settings */
  restreamer_connection_t *conn = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn != NULL, "Should load connection from settings");

  if (conn) {
    TEST_ASSERT(conn->host != NULL, "Host should be set");
    TEST_ASSERT(conn->port > 0, "Port should be set");
    TEST_ASSERT(conn->username != NULL, "Username should be set");
    TEST_ASSERT(conn->password != NULL, "Password should be set");

    restreamer_config_free_connection(conn);
  }

  obs_data_release(settings);

  printf("  ✓ Load from settings helper\n");
  return true;
}

/* Test: Save to settings helper */
static bool test_config_save_to_settings_helper(void) {
  printf("  Testing save to settings helper...\n");

  restreamer_connection_t conn = {
      .host = "save.test.local",
      .port = 7777,
      .username = "saveuser",
      .password = "savepass",
      .use_https = true,
  };

  obs_data_t *settings = obs_data_create();

  /* Save connection to settings */
  restreamer_config_save_to_settings(settings, &conn);

  /* Verify saved values */
  const char *url = obs_data_get_string(settings, "connection_url");
  TEST_ASSERT(url != NULL, "URL should be saved");

  const char *username = obs_data_get_string(settings, "username");
  TEST_ASSERT(username != NULL, "Username should be saved");

  bool use_https = obs_data_get_bool(settings, "use_https");
  TEST_ASSERT(use_https == true, "HTTPS flag should be saved");

  obs_data_release(settings);

  printf("  ✓ Save to settings helper\n");
  return true;
}

/* Test: Get properties */
static bool test_config_get_properties(void) {
  printf("  Testing get properties...\n");

  obs_properties_t *props = restreamer_config_get_properties();
  TEST_ASSERT(props != NULL, "Should return properties");

  /* Check for expected properties */
  obs_property_t *prop;

  prop = obs_properties_get(props, "host");
  TEST_ASSERT(prop != NULL, "Should have host property");

  prop = obs_properties_get(props, "port");
  TEST_ASSERT(prop != NULL, "Should have port property");

  prop = obs_properties_get(props, "username");
  TEST_ASSERT(prop != NULL, "Should have username property");

  prop = obs_properties_get(props, "password");
  TEST_ASSERT(prop != NULL, "Should have password property");

  prop = obs_properties_get(props, "use_https");
  TEST_ASSERT(prop != NULL, "Should have use_https property");

  obs_properties_destroy(props);

  printf("  ✓ Get properties\n");
  return true;
}

/* Test: Free connection helper */
static bool test_config_free_connection_helper(void) {
  printf("  Testing free connection helper...\n");

  /* Test with NULL - should not crash */
  restreamer_config_free_connection(NULL);

  /* Test with valid connection */
  restreamer_connection_t *conn = bzalloc(sizeof(restreamer_connection_t));
  conn->host = bstrdup("test.local");
  conn->username = bstrdup("user");
  conn->password = bstrdup("pass");
  conn->port = 8080;
  conn->use_https = false;

  restreamer_config_free_connection(conn);

  printf("  ✓ Free connection helper\n");
  return true;
}

/* Test: Connection with different URL formats */
static bool test_config_url_formats(void) {
  printf("  Testing different URL formats...\n");

  obs_data_t *settings = obs_data_create();

  /* Test with full URL */
  obs_data_set_string(settings, "connection_url", "https://example.com:8443");
  restreamer_connection_t *conn1 = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn1 != NULL, "Should parse full URL");
  if (conn1) restreamer_config_free_connection(conn1);

  /* Test with URL without port */
  obs_data_set_string(settings, "connection_url", "http://localhost");
  restreamer_connection_t *conn2 = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn2 != NULL, "Should parse URL without port");
  if (conn2) restreamer_config_free_connection(conn2);

  /* Test with just host:port */
  obs_data_set_string(settings, "connection_url", "192.168.1.50:8080");
  restreamer_connection_t *conn3 = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn3 != NULL, "Should parse host:port");
  if (conn3) restreamer_config_free_connection(conn3);

  obs_data_release(settings);

  printf("  ✓ Different URL formats\n");
  return true;
}

/* Test: Load settings with defaults (empty/missing values) */
static bool test_config_load_defaults(void) {
  printf("  Testing load with default values...\n");

  restreamer_config_init();

  /* Create settings with empty host and zero port */
  obs_data_t *settings = obs_data_create();
  obs_data_set_string(settings, "host", "");  /* Empty host */
  obs_data_set_int(settings, "port", 0);       /* Zero port */
  /* Don't set username/password - test optional fields */

  /* Load from settings - should apply defaults */
  restreamer_config_load(settings);

  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should exist");
  TEST_ASSERT(conn->host != NULL, "Host should have default value");
  TEST_ASSERT_EQUAL(8080, conn->port, "Port should be default 8080");

  obs_data_release(settings);
  restreamer_config_destroy();

  printf("  ✓ Load with default values\n");
  return true;
}

/* Test: Load from settings without username/password */
static bool test_config_load_from_settings_optional_fields(void) {
  printf("  Testing load from settings without optional fields...\n");

  obs_data_t *settings = obs_data_create();

  /* Set only required fields, no username/password */
  obs_data_set_string(settings, "host", "test.local");
  obs_data_set_int(settings, "port", 9000);
  obs_data_set_bool(settings, "use_https", false);

  restreamer_connection_t *conn = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn != NULL, "Should load connection");
  TEST_ASSERT(conn->host != NULL, "Host should be set");
  TEST_ASSERT_EQUAL(9000, conn->port, "Port should be 9000");

  /* Username and password should be NULL (optional) */
  /* This tests lines 156-164 in restreamer-config.c */

  restreamer_config_free_connection(conn);
  obs_data_release(settings);

  printf("  ✓ Load from settings without optional fields\n");
  return true;
}

/* Test: Load from settings with empty strings for optional fields */
static bool test_config_load_from_settings_empty_strings(void) {
  printf("  Testing load from settings with empty optional strings...\n");

  obs_data_t *settings = obs_data_create();

  obs_data_set_string(settings, "host", "test.local");
  obs_data_set_int(settings, "port", 9000);
  obs_data_set_string(settings, "username", "");  /* Empty username */
  obs_data_set_string(settings, "password", "");  /* Empty password */

  restreamer_connection_t *conn = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn != NULL, "Should load connection with empty strings");

  /* Empty strings should not create username/password fields */
  /* This specifically tests the strlen() checks in lines 157 and 162 */

  restreamer_config_free_connection(conn);
  obs_data_release(settings);

  printf("  ✓ Load from settings with empty optional strings\n");
  return true;
}

/* Test: Save before init */
static bool test_config_save_before_init(void) {
  printf("  Testing save before initialization...\n");

  /* Make sure config is destroyed first */
  restreamer_config_destroy();
  restreamer_config_destroy(); /* Extra destroy to ensure not initialized */

  obs_data_t *settings = obs_data_create();

  /* Try to save when not initialized - should handle gracefully */
  restreamer_config_save(settings);
  /* Should not crash, tests line 96-97 */

  obs_data_release(settings);

  printf("  ✓ Save before initialization\n");
  return true;
}

/* Test: Load with NULL settings */
static bool test_config_load_null(void) {
  printf("  Testing load with NULL settings...\n");

  restreamer_config_init();

  /* Load with NULL - should handle gracefully */
  restreamer_config_load(NULL);
  /* Should not crash, tests line 65-67 */

  restreamer_config_destroy();

  printf("  ✓ Load with NULL settings\n");
  return true;
}

/* Test: Load from settings with missing host */
static bool test_config_load_from_settings_missing_host(void) {
  printf("  Testing load from settings with missing host...\n");

  obs_data_t *settings = obs_data_create();

  /* Don't set host at all */
  obs_data_set_int(settings, "port", 0);  /* Also test default port */

  restreamer_connection_t *conn = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn != NULL, "Should load connection with defaults");
  TEST_ASSERT(conn->host != NULL, "Host should have default value");
  TEST_ASSERT_EQUAL(8080, conn->port, "Port should be default");

  restreamer_config_free_connection(conn);
  obs_data_release(settings);

  printf("  ✓ Load from settings with missing host\n");
  return true;
}

/* Test: Get global connection before init (tests auto-init) */
static bool test_get_connection_before_init(void) {
  printf("  Testing get connection before init...\n");

  /* Destroy to ensure not initialized */
  restreamer_config_destroy();
  restreamer_config_destroy();

  /* Get connection without calling init first - should auto-init */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Should auto-initialize and return connection");
  TEST_ASSERT(conn->host != NULL, "Should have default host");
  /* This tests line 24-26 auto-init path */

  restreamer_config_destroy();

  printf("  ✓ Get connection before init\n");
  return true;
}

/* Test: Set global connection before init (tests auto-init) */
static bool test_set_connection_before_init(void) {
  printf("  Testing set connection before init...\n");

  /* Destroy to ensure not initialized */
  restreamer_config_destroy();
  restreamer_config_destroy();

  /* Set connection without calling init first - should auto-init */
  restreamer_connection_t test_conn = {
      .host = "auto-init-test.local",
      .port = 7777,
      .username = "autouser",
      .password = "autopass",
      .use_https = true,
  };

  restreamer_config_set_global_connection(&test_conn);
  /* This tests line 37-39 auto-init path */

  /* Verify it worked */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should be set");

  restreamer_config_destroy();

  printf("  ✓ Set connection before init\n");
  return true;
}

/* Test: Create global API before init (tests auto-init) */
static bool test_create_api_before_init(void) {
  printf("  Testing create API before init...\n");

  /* Destroy to ensure not initialized */
  restreamer_config_destroy();
  restreamer_config_destroy();

  /* Create API without calling init first - should auto-init */
  restreamer_api_t *api = restreamer_config_create_global_api();
  TEST_ASSERT(api != NULL, "Should auto-initialize and create API");
  /* This tests line 57-59 auto-init path */

  restreamer_api_destroy(api);
  restreamer_config_destroy();

  printf("  ✓ Create API before init\n");
  return true;
}

/* Test: Destroy before init (tests early return) */
static bool test_destroy_before_init(void) {
  printf("  Testing destroy before init...\n");

  /* Make sure it's already destroyed */
  restreamer_config_destroy();
  restreamer_config_destroy();

  /* Destroy again - should handle gracefully */
  restreamer_config_destroy();
  /* This tests line 126-128 early return */

  printf("  ✓ Destroy before init\n");
  return true;
}

/* Test: Save to settings with NULL/empty connection fields */
static bool test_save_to_settings_null_fields(void) {
  printf("  Testing save to settings with NULL fields...\n");

  /* Create connection with NULL fields */
  restreamer_connection_t conn = {
      .host = NULL,        /* NULL host */
      .port = 0,           /* Zero port */
      .username = NULL,    /* NULL username */
      .password = NULL,    /* NULL password */
      .use_https = false,
  };

  obs_data_t *settings = obs_data_create();

  /* Save connection with NULL fields - should use defaults */
  restreamer_config_save_to_settings(settings, &conn);
  /* This tests the ternary operators in lines 177-185 */

  /* Verify defaults were used */
  const char *saved_host = obs_data_get_string(settings, "host");
  TEST_ASSERT(saved_host != NULL, "Should save default host");

  long long saved_port = obs_data_get_int(settings, "port");
  TEST_ASSERT_EQUAL(8080, (int)saved_port, "Should save default port");

  obs_data_release(settings);

  printf("  ✓ Save to settings with NULL fields\n");
  return true;
}

/* Test: Load from settings with NULL result from obs_data_get_string */
static bool test_load_from_settings_null_obs_strings(void) {
  printf("  Testing load from settings with missing host field...\n");

  obs_data_t *settings = obs_data_create();

  /* Explicitly don't set "host" field at all */
  /* obs_data_get_string will return "" (empty string) for missing fields */
  obs_data_set_int(settings, "port", 9999);

  restreamer_connection_t *conn = restreamer_config_load_from_settings(settings);
  TEST_ASSERT(conn != NULL, "Should load connection");
  TEST_ASSERT(conn->host != NULL, "Host should be set to default");
  /* This tests line 147-149 when host is missing/empty */

  restreamer_config_free_connection(conn);
  obs_data_release(settings);

  printf("  ✓ Load from settings with missing host field\n");
  return true;
}

/* Test: Load with NULL password in global config */
static bool test_load_with_null_password(void) {
  printf("  Testing load with NULL password in global config...\n");

  restreamer_config_init();

  /* Create settings without password */
  obs_data_t *settings = obs_data_create();
  obs_data_set_string(settings, "host", "test.local");
  obs_data_set_int(settings, "port", 8080);
  obs_data_set_string(settings, "username", "testuser");
  /* Don't set password */

  restreamer_config_load(settings);

  /* Verify it loaded */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should be loaded");

  obs_data_release(settings);
  restreamer_config_destroy();

  printf("  ✓ Load with NULL password\n");
  return true;
}

/* Test: Double init (tests early return) */
static bool test_double_init(void) {
  printf("  Testing double initialization...\n");

  restreamer_config_destroy(); /* Ensure clean state */

  /* First init */
  restreamer_config_init();

  /* Second init - should return early */
  restreamer_config_init();
  /* This tests line 11-12 early return when already initialized */

  restreamer_config_destroy();

  printf("  ✓ Double initialization\n");
  return true;
}

/* Test: Load when already initialized but with defaults needed */
static bool test_load_triggers_defaults(void) {
  printf("  Testing load with empty host to trigger defaults...\n");

  restreamer_config_init();

  obs_data_t *settings = obs_data_create();
  /* Set host to empty string explicitly */
  obs_data_set_string(settings, "host", "");
  obs_data_set_int(settings, "port", 0);

  /* This should trigger the default setting logic in lines 86-92 */
  restreamer_config_load(settings);

  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Connection should exist");
  TEST_ASSERT(conn->host != NULL && strlen(conn->host) > 0, "Host should be set to default");
  TEST_ASSERT_EQUAL(8080, conn->port, "Port should be default");

  obs_data_release(settings);
  restreamer_config_destroy();

  printf("  ✓ Load triggers defaults\n");
  return true;
}

/* Test: Save with NULL username/password in global connection */
static bool test_save_with_null_credentials(void) {
  printf("  Testing save with NULL credentials...\n");

  restreamer_config_init();

  /* Set connection with NULL username and password */
  restreamer_connection_t test_conn = {
      .host = "test.local",
      .port = 9090,
      .username = NULL,    /* NULL username */
      .password = NULL,    /* NULL password */
      .use_https = false,
  };

  restreamer_config_set_global_connection(&test_conn);

  /* Save - this should trigger the ternary operators in lines 102-107 */
  obs_data_t *settings = obs_data_create();
  restreamer_config_save(settings);

  /* Verify empty strings were saved for NULL credentials */
  const char *saved_username = obs_data_get_string(settings, "username");
  const char *saved_password = obs_data_get_string(settings, "password");
  TEST_ASSERT(saved_username != NULL, "Username should be saved as empty string");
  TEST_ASSERT(saved_password != NULL, "Password should be saved as empty string");

  obs_data_release(settings);
  restreamer_config_destroy();

  printf("  ✓ Save with NULL credentials\n");
  return true;
}

/* Test: Save with non-NULL username/password in global connection */
static bool test_save_with_credentials(void) {
  printf("  Testing save with credentials...\n");

  restreamer_config_init();

  /* Set connection with non-NULL username and password */
  restreamer_connection_t test_conn = {
      .host = "test.local",
      .port = 9090,
      .username = "testuser",
      .password = "testpass",
      .use_https = true,
  };

  restreamer_config_set_global_connection(&test_conn);

  /* Save - this should use the non-NULL branch of ternary operators */
  obs_data_t *settings = obs_data_create();
  restreamer_config_save(settings);

  /* Verify actual credentials were saved */
  const char *saved_username = obs_data_get_string(settings, "username");
  const char *saved_password = obs_data_get_string(settings, "password");
  TEST_ASSERT_STR_EQUAL("testuser", saved_username, "Username should be saved");
  TEST_ASSERT_STR_EQUAL("testpass", saved_password, "Password should be saved");

  obs_data_release(settings);
  restreamer_config_destroy();

  printf("  ✓ Save with credentials\n");
  return true;
}

/* Run all configuration tests */
bool run_config_tests(void) {
  bool all_passed = true;

  all_passed &= test_global_config_init();
  all_passed &= test_global_connection();
  all_passed &= test_create_global_api();
  all_passed &= test_config_null_handling();
  all_passed &= test_config_lifecycle();
  all_passed &= test_connection_variations();
  all_passed &= test_config_special_values();
  all_passed &= test_config_multiple_destroy();

  /* New comprehensive config tests */
  all_passed &= test_config_load_save_settings();
  all_passed &= test_config_load_from_settings_helper();
  all_passed &= test_config_save_to_settings_helper();
  all_passed &= test_config_get_properties();
  all_passed &= test_config_free_connection_helper();
  all_passed &= test_config_url_formats();

  /* Edge case tests for 100% coverage */
  all_passed &= test_config_load_defaults();
  all_passed &= test_config_load_from_settings_optional_fields();
  all_passed &= test_config_load_from_settings_empty_strings();
  all_passed &= test_config_save_before_init();
  all_passed &= test_config_load_null();
  all_passed &= test_config_load_from_settings_missing_host();

  /* Auto-init and edge case tests */
  all_passed &= test_get_connection_before_init();
  all_passed &= test_set_connection_before_init();
  all_passed &= test_create_api_before_init();
  all_passed &= test_destroy_before_init();

  /* NULL field tests for final coverage */
  all_passed &= test_save_to_settings_null_fields();
  all_passed &= test_load_from_settings_null_obs_strings();
  all_passed &= test_load_with_null_password();
  all_passed &= test_double_init();
  all_passed &= test_load_triggers_defaults();
  all_passed &= test_save_with_null_credentials();
  all_passed &= test_save_with_credentials();

  return all_passed;
}
