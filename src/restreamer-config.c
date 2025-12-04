#include "restreamer-config.h"
#include <obs-module.h>
#include <util/bmem.h>
#include <util/platform.h>
#include <string.h>

static restreamer_connection_t global_connection = {0};
static bool initialized = false;

/* Security: Securely clear memory that won't be optimized away by compiler.
 * Uses volatile pointer to prevent dead-store elimination. */
static void secure_memzero(void *ptr, size_t len) {
  volatile unsigned char *p = (volatile unsigned char *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

/* Security: Securely free password string by clearing memory first */
static void secure_password_free(char *password) {
  if (password) {
    size_t len = strlen(password);
    if (len > 0) {
      secure_memzero(password, len);
    }
    bfree(password);
  }
}

// cppcheck-suppress staticFunction
void restreamer_config_init(void) {
  if (initialized) {
    return;
  }

  memset(&global_connection, 0, sizeof(restreamer_connection_t));
  global_connection.host = bstrdup("localhost");
  global_connection.port = 8080;
  global_connection.use_https = false;

  initialized = true;
}

restreamer_connection_t *restreamer_config_get_global_connection(void) {
  if (!initialized) {
    restreamer_config_init();
  }

  return &global_connection;
}

void restreamer_config_set_global_connection(
    restreamer_connection_t *connection) {
  if (!connection) {
    return;
  }

  if (!initialized) {
    restreamer_config_init();
  }

  /* Free old values */
  bfree(global_connection.host);
  bfree(global_connection.username);
  secure_password_free(global_connection.password);

  /* Copy new values */
  global_connection.host = connection->host ? bstrdup(connection->host) : NULL;
  global_connection.port = connection->port;
  global_connection.username =
      connection->username ? bstrdup(connection->username) : NULL;
  global_connection.password =
      connection->password ? bstrdup(connection->password) : NULL;
  global_connection.use_https = connection->use_https;
}

restreamer_api_t *restreamer_config_create_global_api(void) {
  if (!initialized) {
    restreamer_config_init();
  }

  return restreamer_api_create(&global_connection);
}

void restreamer_config_load(obs_data_t *settings) {
  if (!settings) {
    return;
  }

  if (!initialized) {
    restreamer_config_init();
  }

  if (initialized) {
    bfree(global_connection.host);
    bfree(global_connection.username);
    secure_password_free(global_connection.password);
  }

  const char *host = obs_data_get_string(settings, "host");
  const char *username = obs_data_get_string(settings, "username");
  const char *password = obs_data_get_string(settings, "password");

  global_connection.host = (host && strlen(host) > 0) ? bstrdup(host) : bstrdup("localhost");
  global_connection.port = (uint16_t)obs_data_get_int(settings, "port");
  global_connection.username = (username && strlen(username) > 0) ? bstrdup(username) : NULL;
  global_connection.password = (password && strlen(password) > 0) ? bstrdup(password) : NULL;
  global_connection.use_https = obs_data_get_bool(settings, "use_https");

  /* Set defaults if not present */
  if (global_connection.port == 0) {
    global_connection.port = 8080;
  }
}

void restreamer_config_save(obs_data_t *settings) {
  if (!settings || !initialized) {
    return;
  }

  obs_data_set_string(settings, "host",
                      global_connection.host ? global_connection.host : "localhost");
  obs_data_set_int(settings, "port", global_connection.port);
  obs_data_set_string(settings, "username",
                      global_connection.username ? global_connection.username
                                                 : "");
  obs_data_set_string(settings, "password",
                      global_connection.password ? global_connection.password
                                                 : "");
  obs_data_set_bool(settings, "use_https", global_connection.use_https);
}

obs_properties_t *restreamer_config_get_properties(void) {
  obs_properties_t *props = obs_properties_create();

  obs_properties_add_text(props, "host", "Restreamer Host", OBS_TEXT_DEFAULT);
  obs_properties_add_int(props, "port", "Port", 1, 65535, 1);
  obs_properties_add_bool(props, "use_https", "Use HTTPS");
  obs_properties_add_text(props, "username", "Username (optional)",
                          OBS_TEXT_DEFAULT);
  obs_properties_add_text(props, "password", "Password (optional)",
                          OBS_TEXT_PASSWORD);

  return props;
}

void restreamer_config_destroy(void) {
  if (!initialized) {
    return;
  }

  bfree(global_connection.host);
  bfree(global_connection.username);
  secure_password_free(global_connection.password);

  memset(&global_connection, 0, sizeof(restreamer_connection_t));
  initialized = false;
}

restreamer_connection_t *
restreamer_config_load_from_settings(obs_data_t *settings) {
  if (!settings) {
    return NULL;
  }

  restreamer_connection_t *connection =
      bzalloc(sizeof(restreamer_connection_t));

  const char *host = obs_data_get_string(settings, "host");
  connection->host =
      host && strlen(host) > 0 ? bstrdup(host) : bstrdup("localhost");

  connection->port = (uint16_t)obs_data_get_int(settings, "port");
  if (connection->port == 0) {
    connection->port = 8080;
  }

  const char *username = obs_data_get_string(settings, "username");
  if (username && strlen(username) > 0) {
    connection->username = bstrdup(username);
  }

  const char *password = obs_data_get_string(settings, "password");
  if (password && strlen(password) > 0) {
    connection->password = bstrdup(password);
  }

  connection->use_https = obs_data_get_bool(settings, "use_https");

  return connection;
}

void restreamer_config_save_to_settings(obs_data_t *settings,
                                        restreamer_connection_t *connection) {
  if (!settings || !connection) {
    return;
  }

  obs_data_set_string(settings, "host",
                      connection->host ? connection->host : "localhost");

  int port = connection->port;
  if (port <= 0 || port > 65535) {
    port = 8080;
  }
  obs_data_set_int(settings, "port", port);

  obs_data_set_string(settings, "username",
                      connection->username ? connection->username : "");
  obs_data_set_string(settings, "password",
                      connection->password ? connection->password : "");
  obs_data_set_bool(settings, "use_https", connection->use_https);
}

void restreamer_config_free_connection(restreamer_connection_t *connection) {
  if (!connection) {
    return;
  }

  bfree(connection->host);
  bfree(connection->username);
  secure_password_free(connection->password);
  bfree(connection);
}
