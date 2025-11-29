#include "restreamer-api.h"
#include <curl/curl.h>
#include <jansson.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>

/* Login retry constants */
#define MAX_LOGIN_RETRIES 3
#define INITIAL_BACKOFF_MS 1000

/* Testing support: Make internal functions visible when TESTING_MODE is defined */
#ifdef TESTING_MODE
#define STATIC_TESTABLE
#else
#define STATIC_TESTABLE static
#endif

struct restreamer_api {
  restreamer_connection_t connection;
  CURL *curl;
  char error_buffer[CURL_ERROR_SIZE];
  struct dstr last_error;
  char *access_token;   /* JWT access token */
  char *refresh_token;  /* JWT refresh token */
  time_t token_expires; /* Token expiration timestamp */
  /* Login retry with exponential backoff */
  time_t last_login_attempt;
  int login_backoff_ms;
  int login_retry_count;
};

/* Security: Securely clear memory that won't be optimized away by compiler.
 * Uses volatile pointer to prevent dead-store elimination. */
STATIC_TESTABLE void secure_memzero(void *ptr, size_t len) {
  volatile unsigned char *p = (volatile unsigned char *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

/* Security: Securely free sensitive string data by clearing memory first */
STATIC_TESTABLE void secure_free(char *ptr) {
  if (ptr) {
    /* SECURITY: strlen is safe here - ptr is verified non-NULL by the if condition above */
    size_t len = strlen(ptr);
    if (len > 0) {
      secure_memzero(ptr, len);
    }
    bfree(ptr);
  }
}

/* Security: Securely free dstr containing sensitive data */
/* Currently unused but kept for future use with sensitive dstr data */
#if 0
static void secure_dstr_free(struct dstr *str) {
  if (str && str->array) {
    if (str->len > 0) {
      memset(str->array, 0, str->len);
    }
  }
  dstr_free(str);
}
#endif

/* Memory write callback for curl */
struct memory_struct {
  char *memory;
  size_t size;
};

/* Forward declaration for JSON parsing helper */
STATIC_TESTABLE json_t *parse_json_response(restreamer_api_t *api,
                                            struct memory_struct *response);

// cppcheck-suppress constParameterCallback
STATIC_TESTABLE size_t write_callback(void *contents, size_t size, size_t nmemb,
                                      void *userp) {
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct *)userp;

  /* Use standard C library realloc for CURL compatibility */
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    return 0;
  }

  mem->memory = ptr;
  /* Security: memcpy is safe here - buffer size validated by realloc above */
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

restreamer_api_t *restreamer_api_create(restreamer_connection_t *connection) {
  if (!connection || !connection->host) {
    return NULL;
  }

  restreamer_api_t *api = bzalloc(sizeof(restreamer_api_t));
  api->connection.host = bstrdup(connection->host);
  api->connection.port = connection->port ? connection->port : 8080;
  api->connection.use_https = connection->use_https;

  if (connection->username) {
    api->connection.username = bstrdup(connection->username);
  }
  if (connection->password) {
    api->connection.password = bstrdup(connection->password);
  }

  api->curl = curl_easy_init();
  if (!api->curl) {
    restreamer_api_destroy(api);
    return NULL;
  }

  dstr_init(&api->last_error);

  /* Set up CURL defaults - headers are managed per-request */
  curl_easy_setopt(api->curl, CURLOPT_ERRORBUFFER, api->error_buffer);
  curl_easy_setopt(api->curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(api->curl, CURLOPT_TIMEOUT, 10L);

  /* Security: Enable HTTPS certificate verification to prevent MITM attacks */
  curl_easy_setopt(api->curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(api->curl, CURLOPT_SSL_VERIFYHOST, 2L);

  /* Thread-safety options for multi-threaded environments */
  curl_easy_setopt(api->curl, CURLOPT_NOSIGNAL,
                   1L); /* Disable signals - required for thread safety */

  /* Initialize JWT token fields */
  api->access_token = NULL;
  api->refresh_token = NULL;
  api->token_expires = 0;

  /* Initialize login retry fields */
  api->last_login_attempt = 0;
  api->login_backoff_ms = INITIAL_BACKOFF_MS;
  api->login_retry_count = 0;

  return api;
}

void restreamer_api_destroy(restreamer_api_t *api) {
  if (!api) {
    return;
  }

  if (api->curl) {
    curl_easy_cleanup(api->curl);
  }

  bfree(api->connection.host);
  bfree(api->connection.username);
  secure_free(
      api->connection.password);  /* Security: Clear password from memory */
  secure_free(api->access_token); /* Security: Clear access token from memory */
  secure_free(
      api->refresh_token); /* Security: Clear refresh token from memory */
  dstr_free(&api->last_error);

  bfree(api);
}

/* Helper: Handle login failure with exponential backoff */
STATIC_TESTABLE void handle_login_failure(restreamer_api_t *api, long http_code) {
  api->login_retry_count++;
  api->last_login_attempt = time(NULL);

  if (api->login_retry_count < MAX_LOGIN_RETRIES) {
    api->login_backoff_ms *= 2;
    if (http_code > 0) {
      obs_log(LOG_WARNING,
              "[obs-polyemesis] Login failed with HTTP %ld (attempt %d/%d), "
              "backing off %d ms",
              http_code, api->login_retry_count, MAX_LOGIN_RETRIES,
              api->login_backoff_ms);
    } else {
      obs_log(
          LOG_WARNING,
          "[obs-polyemesis] Login failed (attempt %d/%d), backing off %d ms",
          api->login_retry_count, MAX_LOGIN_RETRIES, api->login_backoff_ms);
    }
  } else {
    obs_log(LOG_ERROR, "[obs-polyemesis] Login failed after %d attempts",
            MAX_LOGIN_RETRIES);
  }
}

/* Helper: Check if login is throttled by backoff */
STATIC_TESTABLE bool is_login_throttled(restreamer_api_t *api) {
  if (api->login_retry_count > 0 && api->last_login_attempt > 0) {
    time_t elapsed = time(NULL) - api->last_login_attempt;
    time_t backoff_seconds = api->login_backoff_ms / 1000;
    if (elapsed < backoff_seconds) {
      dstr_printf(&api->last_error, "Login throttled, retry in %ld seconds",
                  backoff_seconds - elapsed);
      return true;
    }
  }
  return false;
}

/* Login to get JWT token */
static bool restreamer_api_login(restreamer_api_t *api) {
  /* Check api separately first to avoid NULL dereference */
  if (!api) {
    return false;
  }

  if (!api->connection.username || !api->connection.password) {
    dstr_copy(&api->last_error, "Username and password required for login");
    return false;
  }

  /* Check if we need to apply backoff before attempting login */
  if (is_login_throttled(api)) {
    return false;
  }

  /* Build login request */
  json_t *login_data = json_object();
  json_object_set_new(login_data, "username",
                      json_string(api->connection.username));
  json_object_set_new(login_data, "password",
                      json_string(api->connection.password));

  char *post_data = json_dumps(login_data, 0);
  json_decref(login_data);

  if (!post_data) {
    dstr_copy(&api->last_error, "Failed to encode login JSON");
    return false;
  }

  /* Make request without token (login doesn't need auth) */
  struct dstr url;
  dstr_init(&url);

  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d/api/login", protocol, api->connection.host,
              api->connection.port);

  struct memory_struct response = {.memory = NULL, .size = 0};

  /* Set headers for login request - no auth needed */
  struct curl_slist *login_headers = NULL;
  login_headers =
      curl_slist_append(login_headers, "Content-Type: application/json");

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, login_headers);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(api->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));

  CURLcode res = curl_easy_perform(api->curl);

  /* Clean up headers first - must be done before freeing the list */
  curl_slist_free_all(login_headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, NULL);

  /* Reset CURL state after POST request - must reset size too! */
  curl_easy_setopt(api->curl, CURLOPT_POST, 0L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);

  /* Security: Clear login credentials from memory before freeing */
  /* SECURITY: strlen is safe - post_data is guaranteed non-NULL (checked at line 196) */
  secure_memzero(post_data, strlen(post_data));
  free(post_data);
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    handle_login_failure(api, 0);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "Login failed: HTTP %ld", http_code);
    free(response.memory);
    handle_login_failure(api, http_code);
    return false;
  }

  /* Parse response to get tokens */
  json_t *root = parse_json_response(api, &response);
  if (!root) {
    return false;
  }

  json_t *access_token = json_object_get(root, "access_token");
  json_t *refresh_token = json_object_get(root, "refresh_token");
  json_t *expires_at = json_object_get(root, "expires_at");

  if (!access_token || !json_is_string(access_token)) {
    dstr_copy(&api->last_error, "No access token in login response");
    json_decref(root);
    return false;
  }

  /* Store tokens */
  secure_free(api->access_token); /* Security: Clear access token from memory */
  api->access_token = bstrdup(json_string_value(access_token));

  if (refresh_token && json_is_string(refresh_token)) {
    secure_free(
        api->refresh_token); /* Security: Clear refresh token from memory */
    api->refresh_token = bstrdup(json_string_value(refresh_token));
  }

  if (expires_at && json_is_integer(expires_at)) {
    api->token_expires = (time_t)json_integer_value(expires_at);
  } else {
    /* Default to 1 hour from now if no expiry provided */
    api->token_expires = time(NULL) + 3600;
  }

  json_decref(root);

  /* Reset retry tracking on successful login */
  api->login_retry_count = 0;
  api->login_backoff_ms = INITIAL_BACKOFF_MS;

  obs_log(LOG_INFO, "[obs-polyemesis] Successfully logged in to Restreamer");

  return true;
}

static bool make_request(restreamer_api_t *api, const char *endpoint,
                         const char *method, const char *post_data,
                         struct memory_struct *response) {
  /* Check if we need to login or refresh token */
  if (!api->access_token || time(NULL) >= api->token_expires) {
    if (!restreamer_api_login(api)) {
      return false;
    }
  }

  /* Add Authorization header with Bearer token */
  struct dstr auth_header;
  dstr_init(&auth_header);
  dstr_printf(&auth_header, "Authorization: Bearer %s", api->access_token);

  /* Create temporary headers list for this request */
  struct curl_slist *temp_headers = NULL;
  temp_headers =
      curl_slist_append(temp_headers, "Content-Type: application/json");
  temp_headers = curl_slist_append(temp_headers, auth_header.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, temp_headers);

  struct dstr url;
  dstr_init(&url);

  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint);

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)response);

  if (strcmp(method, "POST") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_POST, 1L);
    if (post_data) {
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
    }
  } else if (strcmp(method, "DELETE") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  } else if (strcmp(method, "PUT") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (post_data) {
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
    }
  } else {
    curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);
  }

  CURLcode res = curl_easy_perform(api->curl);

  /* Reset POST state to avoid affecting subsequent requests */
  curl_easy_setopt(api->curl, CURLOPT_POST, 0L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, NULL);

  /* Clean up temporary headers */
  curl_slist_free_all(temp_headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, NULL);
  dstr_free(&auth_header);
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    if (response->memory) {
      free(response->memory);
      response->memory = NULL;
    }
    response->size = 0;
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "HTTP Error: %ld", http_code);
    if (response->memory) {
      free(response->memory);
      response->memory = NULL;
    }
    response->size = 0;
    return false;
  }

  return true;
}

bool restreamer_api_test_connection(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  /* Test connection by attempting to login, which validates credentials */
  return restreamer_api_login(api);
}

bool restreamer_api_is_connected(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  return (api->access_token != NULL);
}

/* Forward declarations for helper functions */
static void parse_process_fields(const json_t *json_obj,
                                 restreamer_process_t *process);
static void parse_log_entry_fields(const json_t *json_obj,
                                   restreamer_log_entry_t *entry);
static void parse_session_fields(const json_t *json_obj,
                                 restreamer_session_t *session);
static void parse_fs_entry_fields(const json_t *json_obj,
                                  restreamer_fs_entry_t *entry);
static bool process_command_helper(restreamer_api_t *api,
                                   const char *process_id, const char *command);
static bool get_protocol_streams_helper(restreamer_api_t *api,
                                        const char *endpoint,
                                        char **streams_json);

bool restreamer_api_get_processes(restreamer_api_t *api,
                                  restreamer_process_list_t *list) {
  if (!api || !list) {
    return false;
  }

  /* Initialize list to safe defaults */
  list->processes = NULL;
  list->count = 0;

  struct memory_struct response = {NULL, 0};
  if (!make_request(api, "/api/v3/process", "GET", NULL, &response)) {
    return false;
  }

  /* Check if we have a valid response */
  if (!response.memory || response.size == 0) {
    dstr_copy(&api->last_error, "Empty response from server");
    return false;
  }

  json_t *root = parse_json_response(api, &response);
  if (!root) {
    return false;
  }

  if (!json_is_array(root)) {
    json_decref(root);
    dstr_copy(&api->last_error, "Expected array response");
    return false;
  }

  size_t count = json_array_size(root);

  /* Handle empty array case */
  if (count == 0) {
    json_decref(root);
    return true;
  }

  /* Allocate memory for processes */
  list->processes = bzalloc(sizeof(restreamer_process_t) * count);
  if (!list->processes) {
    json_decref(root);
    dstr_copy(&api->last_error, "Failed to allocate memory for processes");
    return false;
  }

  list->count = count;

  for (size_t i = 0; i < count; i++) {
    const json_t *process_obj = json_array_get(root, i);
    restreamer_process_t *process = &list->processes[i];
    parse_process_fields(process_obj, process);
  }

  json_decref(root);
  return true;
}

/* ========================================================================
 * Helper Functions
 * ======================================================================== */

/* Helper function to parse JSON response and handle errors */
STATIC_TESTABLE json_t *parse_json_response(restreamer_api_t *api,
                                            struct memory_struct *response) {
  if (!api || !response || !response->memory) {
    return NULL;
  }

  json_error_t error;
  json_t *root = json_loads(response->memory, 0, &error);
  free(response->memory);
  response->memory = NULL;
  response->size = 0;

  if (!root) {
    dstr_printf(&api->last_error, "JSON parse error: %s", error.text);
    return NULL;
  }

  return root;
}

/* Helper function to parse JSON object into restreamer_process_t */
static void parse_process_fields(const json_t *json_obj,
                                 restreamer_process_t *process) {
  if (!json_obj || !process) {
    return;
  }

  json_t *id = json_object_get(json_obj, "id");
  if (json_is_string(id)) {
    process->id = bstrdup(json_string_value(id));
  }

  json_t *reference = json_object_get(json_obj, "reference");
  if (json_is_string(reference)) {
    process->reference = bstrdup(json_string_value(reference));
  }

  json_t *state = json_object_get(json_obj, "state");
  if (json_is_string(state)) {
    process->state = bstrdup(json_string_value(state));
  }

  json_t *uptime = json_object_get(json_obj, "uptime");
  if (json_is_integer(uptime)) {
    process->uptime_seconds = json_integer_value(uptime);
  }

  json_t *cpu = json_object_get(json_obj, "cpu_usage");
  if (json_is_real(cpu)) {
    process->cpu_usage = json_real_value(cpu);
  }

  json_t *memory = json_object_get(json_obj, "memory");
  if (json_is_integer(memory)) {
    process->memory_bytes = json_integer_value(memory);
  }

  json_t *command = json_object_get(json_obj, "command");
  if (json_is_string(command)) {
    process->command = bstrdup(json_string_value(command));
  }
}

/* Helper function to parse JSON object into restreamer_log_entry_t */
static void parse_log_entry_fields(const json_t *json_obj,
                                   restreamer_log_entry_t *entry) {
  if (!json_obj || !entry) {
    return;
  }

  json_t *timestamp = json_object_get(json_obj, "timestamp");
  if (json_is_string(timestamp)) {
    entry->timestamp = bstrdup(json_string_value(timestamp));
  }

  json_t *message = json_object_get(json_obj, "message");
  if (json_is_string(message)) {
    entry->message = bstrdup(json_string_value(message));
  }

  json_t *level = json_object_get(json_obj, "level");
  if (json_is_string(level)) {
    entry->level = bstrdup(json_string_value(level));
  }
}

/* Helper function to parse JSON object into restreamer_session_t */
static void parse_session_fields(const json_t *json_obj,
                                 restreamer_session_t *session) {
  if (!json_obj || !session) {
    return;
  }

  json_t *session_id = json_object_get(json_obj, "id");
  if (json_is_string(session_id)) {
    session->session_id = bstrdup(json_string_value(session_id));
  }

  json_t *reference = json_object_get(json_obj, "reference");
  if (json_is_string(reference)) {
    session->reference = bstrdup(json_string_value(reference));
  }

  json_t *bytes_sent = json_object_get(json_obj, "bytes_sent");
  if (json_is_integer(bytes_sent)) {
    session->bytes_sent = json_integer_value(bytes_sent);
  }

  json_t *bytes_received = json_object_get(json_obj, "bytes_received");
  if (json_is_integer(bytes_received)) {
    session->bytes_received = json_integer_value(bytes_received);
  }

  json_t *remote_addr = json_object_get(json_obj, "remote_addr");
  if (json_is_string(remote_addr)) {
    session->remote_addr = bstrdup(json_string_value(remote_addr));
  }
}

/* Helper function to parse JSON object into restreamer_fs_entry_t */
static void parse_fs_entry_fields(const json_t *json_obj,
                                  restreamer_fs_entry_t *entry) {
  if (!json_obj || !entry) {
    return;
  }

  json_t *name = json_object_get(json_obj, "name");
  if (name && json_is_string(name)) {
    entry->name = bstrdup(json_string_value(name));
  }

  json_t *path = json_object_get(json_obj, "path");
  if (path && json_is_string(path)) {
    entry->path = bstrdup(json_string_value(path));
  }

  json_t *size = json_object_get(json_obj, "size");
  if (size && json_is_integer(size)) {
    entry->size = (uint64_t)json_integer_value(size);
  }

  json_t *modified = json_object_get(json_obj, "modified");
  if (modified && json_is_integer(modified)) {
    entry->modified = json_integer_value(modified);
  }

  json_t *is_dir = json_object_get(json_obj, "is_directory");
  if (is_dir && json_is_boolean(is_dir)) {
    entry->is_directory = json_boolean_value(is_dir);
  }
}

/* Helper function for process control commands (start/stop/restart) */
static bool process_command_helper(restreamer_api_t *api,
                                   const char *process_id,
                                   const char *command) {
  if (!api || !process_id || process_id[0] == '\0' || !command) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/command");

  json_t *root = json_object();
  json_object_set_new(root, "command", json_string(command));

  char *post_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "POST", post_data, &response);

  free(post_data);
  dstr_free(&endpoint);

  if (result) {
    free(response.memory);
  }

  return result;
}

/* ========================================================================
 * Process Control API
 * ======================================================================== */

bool restreamer_api_start_process(restreamer_api_t *api,
                                  const char *process_id) {
  return process_command_helper(api, process_id, "start");
}

bool restreamer_api_stop_process(restreamer_api_t *api,
                                 const char *process_id) {
  return process_command_helper(api, process_id, "stop");
}

bool restreamer_api_restart_process(restreamer_api_t *api,
                                    const char *process_id) {
  return process_command_helper(api, process_id, "restart");
}

bool restreamer_api_get_process(restreamer_api_t *api, const char *process_id,
                                restreamer_process_t *process) {
  if (!api || !process_id || process_id[0] == '\0' || !process) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result) {
    return false;
  }

  json_t *root = parse_json_response(api, &response);
  if (!root) {
    return false;
  }

  parse_process_fields(root, process);

  json_decref(root);
  return true;
}

bool restreamer_api_get_process_logs(restreamer_api_t *api,
                                     const char *process_id,
                                     restreamer_log_list_t *logs) {
  if (!api || !process_id || !logs || process_id[0] == '\0') {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/log");

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result) {
    return false;
  }

  json_t *root = parse_json_response(api, &response);
  if (!root || !json_is_array(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  size_t count = json_array_size(root);
  logs->entries = bzalloc(sizeof(restreamer_log_entry_t) * count);
  logs->count = count;

  for (size_t i = 0; i < count; i++) {
    const json_t *entry_obj = json_array_get(root, i);
    restreamer_log_entry_t *entry = &logs->entries[i];
    parse_log_entry_fields(entry_obj, entry);
  }

  json_decref(root);
  return true;
}

bool restreamer_api_get_sessions(restreamer_api_t *api,
                                 restreamer_session_list_t *sessions) {
  if (!api || !sessions) {
    return false;
  }

  struct memory_struct response = {NULL, 0};
  if (!make_request(api, "/api/v3/sessions", "GET", NULL, &response)) {
    return false;
  }

  json_t *root = parse_json_response(api, &response);
  if (!root || !json_is_object(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  json_t *sessions_array = json_object_get(root, "sessions");
  if (!sessions_array || !json_is_array(sessions_array)) {
    json_decref(root);
    return false;
  }

  size_t count = json_array_size(sessions_array);
  sessions->sessions = bzalloc(sizeof(restreamer_session_t) * count);
  sessions->count = count;

  for (size_t i = 0; i < count; i++) {
    const json_t *session_obj = json_array_get(sessions_array, i);
    restreamer_session_t *session = &sessions->sessions[i];
    parse_session_fields(session_obj, session);
  }

  json_decref(root);
  return true;
}

bool restreamer_api_create_process(restreamer_api_t *api, const char *reference,
                                   const char *input_url,
                                   const char **output_urls,
                                   size_t output_count,
                                   const char *video_filter) {
  if (!api || !reference || !input_url || !output_urls || output_count == 0) {
    return false;
  }

  json_t *root = json_object();
  json_object_set_new(root, "reference", json_string(reference));

  /* Build FFmpeg command for multistreaming
   * Security: This command contains stream keys in output_urls - never log it
   */
  struct dstr command;
  dstr_init(&command);
  dstr_printf(&command,
              "-re -i %s -c:v copy -c:a copy -f tee -map 0:v -map 0:a ",
              input_url);

  if (video_filter) {
    dstr_cat(&command, "-vf ");
    dstr_cat(&command, video_filter);
    dstr_cat(&command, " ");
  }

  /* Add output URLs */
  dstr_cat(&command, "\"");
  for (size_t i = 0; i < output_count; i++) {
    if (i > 0) {
      dstr_cat(&command, "|");
    }
    dstr_cat(&command, "[f=flv]");
    dstr_cat(&command, output_urls[i]);
  }
  dstr_cat(&command, "\"");

  json_object_set_new(root, "command", json_string(command.array));
  json_object_set_new(root, "autostart", json_true());

  char *post_data = json_dumps(root, 0);
  json_decref(root);
  dstr_free(&command);

  struct memory_struct response = {NULL, 0};
  bool result =
      make_request(api, "/api/v3/process", "POST", post_data, &response);

  free(post_data);

  if (result) {
    free(response.memory);
  }

  return result;
}

bool restreamer_api_delete_process(restreamer_api_t *api,
                                   const char *process_id) {
  if (!api || !process_id) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "DELETE", NULL, &response);
  dstr_free(&endpoint);

  if (result) {
    free(response.memory);
  }

  return result;
}

/* ========================================================================
 * Dynamic Process Modification API Implementation
 * ======================================================================== */

bool restreamer_api_add_process_output(restreamer_api_t *api,
                                       const char *process_id,
                                       const char *output_id,
                                       const char *output_url,
                                       const char *video_filter) {
  if (!api || !process_id || !output_id || !output_url) {
    return false;
  }

  /* Build endpoint: /api/v3/process/{id}/outputs */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs");

  /* Build request body */
  json_t *root = json_object();
  json_object_set_new(root, "id", json_string(output_id));
  json_object_set_new(root, "url", json_string(output_url));

  if (video_filter) {
    json_object_set_new(root, "video_filter", json_string(video_filter));
  }

  char *post_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "POST", post_data, &response);

  dstr_free(&endpoint);
  free(post_data);

  if (result) {
    free(response.memory);
    obs_log(LOG_INFO, "Added output %s to process %s", output_id, process_id);
  } else {
    obs_log(LOG_ERROR, "Failed to add output %s to process %s: %s", output_id,
            process_id, restreamer_api_get_error(api));
  }

  return result;
}

bool restreamer_api_remove_process_output(restreamer_api_t *api,
                                          const char *process_id,
                                          const char *output_id) {
  if (!api || !process_id || !output_id) {
    return false;
  }

  /* Build endpoint: /api/v3/process/{id}/outputs/{output_id} */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs/");
  dstr_cat(&endpoint, output_id);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "DELETE", NULL, &response);
  dstr_free(&endpoint);

  if (result) {
    free(response.memory);
    obs_log(LOG_INFO, "Removed output %s from process %s", output_id,
            process_id);
  } else {
    obs_log(LOG_ERROR, "Failed to remove output %s from process %s: %s",
            output_id, process_id, restreamer_api_get_error(api));
  }

  return result;
}

bool restreamer_api_update_process_output(restreamer_api_t *api,
                                          const char *process_id,
                                          const char *output_id,
                                          const char *output_url,
                                          const char *video_filter) {
  if (!api || !process_id || !output_id) {
    return false;
  }

  /* Build endpoint: /api/v3/process/{id}/outputs/{output_id} */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs/");
  dstr_cat(&endpoint, output_id);

  /* Build request body */
  json_t *root = json_object();

  if (output_url) {
    json_object_set_new(root, "url", json_string(output_url));
  }

  if (video_filter) {
    json_object_set_new(root, "video_filter", json_string(video_filter));
  }

  char *put_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "PUT", put_data, &response);

  dstr_free(&endpoint);
  free(put_data);

  if (result) {
    free(response.memory);
    obs_log(LOG_INFO, "Updated output %s in process %s", output_id, process_id);
  } else {
    obs_log(LOG_ERROR, "Failed to update output %s in process %s: %s",
            output_id, process_id, restreamer_api_get_error(api));
  }

  return result;
}

bool restreamer_api_get_process_outputs(restreamer_api_t *api,
                                        const char *process_id,
                                        char ***output_ids,
                                        size_t *output_count) {
  if (!api || !process_id || !output_ids || !output_count) {
    return false;
  }

  /* Build endpoint: /api/v3/process/{id}/outputs */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs");

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result || !response.memory) {
    return false;
  }

  /* Parse JSON response */
  json_t *root = parse_json_response(api, &response);
  if (!root || !json_is_object(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  json_t *outputs_array = json_object_get(root, "outputs");
  if (!outputs_array || !json_is_array(outputs_array)) {
    json_decref(root);
    return false;
  }

  size_t count = json_array_size(outputs_array);
  char **ids = bzalloc(sizeof(char *) * count);

  for (size_t i = 0; i < count; i++) {
    json_t *output_obj = json_array_get(outputs_array, i);
    json_t *id_obj = json_object_get(output_obj, "id");

    if (json_is_string(id_obj)) {
      ids[i] = bstrdup(json_string_value(id_obj));
    }
  }

  *output_ids = ids;
  *output_count = count;

  json_decref(root);
  return true;
}

void restreamer_api_free_outputs_list(char **output_ids, size_t count) {
  if (!output_ids) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    bfree(output_ids[i]);
  }

  bfree(output_ids);
}

/* ========================================================================
 * Live Encoding Settings API Implementation
 * ======================================================================== */

bool restreamer_api_update_output_encoding(restreamer_api_t *api,
                                           const char *process_id,
                                           const char *output_id,
                                           encoding_params_t *params) {
  if (!api || !process_id || !output_id || !params) {
    return false;
  }

  /* Build endpoint: /api/v3/process/{id}/outputs/{output_id}/encoding */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs/");
  dstr_cat(&endpoint, output_id);
  dstr_cat(&endpoint, "/encoding");

  /* Build request body with encoding parameters */
  json_t *root = json_object();

  if (params->video_bitrate_kbps > 0) {
    json_object_set_new(root, "video_bitrate",
                        json_integer(params->video_bitrate_kbps * 1000));
  }

  if (params->audio_bitrate_kbps > 0) {
    json_object_set_new(root, "audio_bitrate",
                        json_integer(params->audio_bitrate_kbps * 1000));
  }

  if (params->width > 0 && params->height > 0) {
    json_t *resolution = json_object();
    json_object_set_new(resolution, "width", json_integer(params->width));
    json_object_set_new(resolution, "height", json_integer(params->height));
    json_object_set_new(root, "resolution", resolution);
  }

  if (params->fps_num > 0 && params->fps_den > 0) {
    json_t *fps = json_object();
    json_object_set_new(fps, "num", json_integer(params->fps_num));
    json_object_set_new(fps, "den", json_integer(params->fps_den));
    json_object_set_new(root, "fps", fps);
  }

  if (params->preset) {
    json_object_set_new(root, "preset", json_string(params->preset));
  }

  if (params->profile) {
    json_object_set_new(root, "profile", json_string(params->profile));
  }

  char *put_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "PUT", put_data, &response);

  dstr_free(&endpoint);
  free(put_data);

  if (result) {
    free(response.memory);
    obs_log(LOG_INFO, "Updated encoding settings for output %s in process %s",
            output_id, process_id);
  } else {
    obs_log(LOG_ERROR,
            "Failed to update encoding for output %s in process %s: %s",
            output_id, process_id, restreamer_api_get_error(api));
  }

  return result;
}

bool restreamer_api_get_output_encoding(restreamer_api_t *api,
                                        const char *process_id,
                                        const char *output_id,
                                        encoding_params_t *params) {
  if (!api || !process_id || !output_id || !params) {
    return false;
  }

  /* Initialize params */
  memset(params, 0, sizeof(encoding_params_t));

  /* Build endpoint: /api/v3/process/{id}/outputs/{output_id}/encoding */
  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/outputs/");
  dstr_cat(&endpoint, output_id);
  dstr_cat(&endpoint, "/encoding");

  struct memory_struct response = {NULL, 0};
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result || !response.memory) {
    return false;
  }

  /* Parse JSON response */
  json_t *root = parse_json_response(api, &response);
  if (!root || !json_is_object(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  /* Extract encoding parameters */
  json_t *video_bitrate = json_object_get(root, "video_bitrate");
  if (json_is_integer(video_bitrate)) {
    params->video_bitrate_kbps =
        (int)(json_integer_value(video_bitrate) / 1000);
  }

  json_t *audio_bitrate = json_object_get(root, "audio_bitrate");
  if (json_is_integer(audio_bitrate)) {
    params->audio_bitrate_kbps =
        (int)(json_integer_value(audio_bitrate) / 1000);
  }

  json_t *resolution = json_object_get(root, "resolution");
  if (json_is_object(resolution)) {
    json_t *width = json_object_get(resolution, "width");
    json_t *height = json_object_get(resolution, "height");
    if (json_is_integer(width) && json_is_integer(height)) {
      params->width = (int)json_integer_value(width);
      params->height = (int)json_integer_value(height);
    }
  }

  json_t *fps = json_object_get(root, "fps");
  if (json_is_object(fps)) {
    json_t *num = json_object_get(fps, "num");
    json_t *den = json_object_get(fps, "den");
    if (json_is_integer(num) && json_is_integer(den)) {
      params->fps_num = (int)json_integer_value(num);
      params->fps_den = (int)json_integer_value(den);
    }
  }

  json_t *preset = json_object_get(root, "preset");
  if (json_is_string(preset)) {
    params->preset = bstrdup(json_string_value(preset));
  }

  json_t *profile = json_object_get(root, "profile");
  if (json_is_string(profile)) {
    params->profile = bstrdup(json_string_value(profile));
  }

  json_decref(root);
  return true;
}

void restreamer_api_free_encoding_params(encoding_params_t *params) {
  if (!params) {
    return;
  }

  bfree(params->preset);
  bfree(params->profile);

  memset(params, 0, sizeof(encoding_params_t));
}

void restreamer_api_free_process_list(restreamer_process_list_t *list) {
  if (!list) {
    return;
  }

  for (size_t i = 0; i < list->count; i++) {
    restreamer_api_free_process(&list->processes[i]);
  }

  bfree(list->processes);
  list->processes = NULL;
  list->count = 0;
}

void restreamer_api_free_session_list(restreamer_session_list_t *list) {
  if (!list) {
    return;
  }

  for (size_t i = 0; i < list->count; i++) {
    bfree(list->sessions[i].session_id);
    bfree(list->sessions[i].reference);
    bfree(list->sessions[i].remote_addr);
  }

  bfree(list->sessions);
  list->sessions = NULL;
  list->count = 0;
}

void restreamer_api_free_log_list(restreamer_log_list_t *list) {
  if (!list) {
    return;
  }

  for (size_t i = 0; i < list->count; i++) {
    bfree(list->entries[i].timestamp);
    bfree(list->entries[i].message);
    bfree(list->entries[i].level);
  }

  bfree(list->entries);
  list->entries = NULL;
  list->count = 0;
}

// cppcheck-suppress staticFunction
void restreamer_api_free_process(restreamer_process_t *process) {
  if (!process) {
    return;
  }

  bfree(process->id);
  bfree(process->reference);
  bfree(process->state);
  bfree(process->command);

  memset(process, 0, sizeof(restreamer_process_t));
}

const char *restreamer_api_get_error(restreamer_api_t *api) {
  if (!api) {
    return "Invalid API instance";
  }

  return api->last_error.array;
}

/* ========================================================================
 * Helper Functions for HTTP Requests
 * ======================================================================== */

/* Generic HTTP GET request with JSON response */
static bool api_request_json(restreamer_api_t *api, const char *endpoint,
                             json_t **response_json) {
  if (!api || !endpoint) {
    return false;
  }

  /* Ensure we have a valid token */
  if (!api->access_token && api->connection.username &&
      api->connection.password) {
    if (!restreamer_api_login(api)) {
      return false;
    }
  }

  /* Build URL */
  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint);

  /* Setup request */
  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  /* Add authorization header */
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  /* Build authorization header if needed - keep it alive until after request
   * completes */
  struct dstr auth_header;
  bool has_auth = false;
  if (api->access_token) {
    dstr_init(&auth_header);
    dstr_printf(&auth_header, "Authorization: Bearer %s", api->access_token);
    headers = curl_slist_append(headers, auth_header.array);
    has_auth = true;
  }

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);

  /* Ensure GET method by clearing POST state and enabling HTTPGET */
  curl_easy_setopt(api->curl, CURLOPT_POST, 0L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, NULL);
  curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);

  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  curl_slist_free_all(headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, NULL);
  if (has_auth) {
    dstr_free(&auth_header); /* Only free if we initialized it */
  }
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "HTTP %ld", http_code);
    free(response.memory);
    return false;
  }

  /* Parse JSON response */
  if (response_json) {
    *response_json = parse_json_response(api, &response);
    if (!*response_json) {
      return false;
    }
  } else {
    free(response.memory);
  }

  return true;
}

/* Generic HTTP PUT request */
static bool api_request_put_json(restreamer_api_t *api, const char *endpoint,
                                 const char *body_json,
                                 json_t **response_json) {
  if (!api || !endpoint) {
    return false;
  }

  /* Ensure we have a valid token */
  if (!api->access_token && api->connection.username &&
      api->connection.password) {
    if (!restreamer_api_login(api)) {
      return false;
    }
  }

  /* Build URL */
  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint);

  /* Setup request */
  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  /* Add Content-Type header */
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  /* Build authorization header if needed - keep it alive until after request
   * completes */
  struct dstr auth_header;
  bool has_auth = false;
  if (api->access_token) {
    dstr_init(&auth_header);
    dstr_printf(&auth_header, "Authorization: Bearer %s", api->access_token);
    headers = curl_slist_append(headers, auth_header.array);
    has_auth = true;
  }

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 0L); /* Reset GET */
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "PUT");
  if (body_json) {
    curl_easy_setopt(api->curl, CURLOPT_POST,
                     1L); /* Enable POST mode for body */
    curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, body_json);
    curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, strlen(body_json));
  } else {
    curl_easy_setopt(api->curl, CURLOPT_POST, 0L);
    curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  }
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  /* Reset CURL state to avoid affecting subsequent requests */
  curl_easy_setopt(api->curl, CURLOPT_POST, 0L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, NULL);

  /* Clean up headers - must match order in make_request() */
  curl_slist_free_all(headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, NULL);
  if (has_auth) {
    dstr_free(&auth_header); /* Only free if we initialized it */
  }
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "HTTP %ld", http_code);
    free(response.memory);
    return false;
  }

  /* Parse JSON response */
  if (response_json && response.size > 0) {
    *response_json = parse_json_response(api, &response);
    if (!*response_json) {
      return false;
    }
  } else {
    free(response.memory);
  }

  return true;
}

/* ========================================================================
 * Extended API Implementations
 * ======================================================================== */

/* Process State API */
bool restreamer_api_get_process_state(restreamer_api_t *api,
                                      const char *process_id,
                                      restreamer_process_state_t *state) {
  if (!api || !process_id || !state) {
    return false;
  }

  memset(state, 0, sizeof(restreamer_process_state_t));

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/state", process_id);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  /* Parse state response */
  json_t *order = json_object_get(response, "order");
  if (order && json_is_string(order)) {
    state->order = bstrdup(json_string_value(order));
  }

  json_t *progress = json_object_get(response, "progress");
  if (progress && json_is_object(progress)) {
    json_t *frames = json_object_get(progress, "frames");
    if (frames && json_is_integer(frames)) {
      state->frames = (uint64_t)json_integer_value(frames);
    }

    json_t *dropped = json_object_get(progress, "dropped_frames");
    if (dropped && json_is_integer(dropped)) {
      state->dropped_frames = (uint64_t)json_integer_value(dropped);
    }

    json_t *bitrate = json_object_get(progress, "bitrate");
    if (bitrate && json_is_integer(bitrate)) {
      state->current_bitrate = (uint32_t)json_integer_value(bitrate);
    }

    json_t *fps = json_object_get(progress, "fps");
    if (fps && json_is_number(fps)) {
      state->fps = json_number_value(fps);
    }

    json_t *bytes = json_object_get(progress, "size_kb");
    if (bytes && json_is_integer(bytes)) {
      state->bytes_written = (uint64_t)json_integer_value(bytes) * 1024;
    }

    json_t *packets = json_object_get(progress, "packets");
    if (packets && json_is_integer(packets)) {
      state->packets_sent = (uint64_t)json_integer_value(packets);
    }

    json_t *percent = json_object_get(progress, "percent");
    if (percent && json_is_number(percent)) {
      state->progress = json_number_value(percent);
    }
  }

  json_t *running = json_object_get(response, "running");
  if (running && json_is_boolean(running)) {
    state->is_running = json_boolean_value(running);
  }

  json_decref(response);
  return true;
}

void restreamer_api_free_process_state(restreamer_process_state_t *state) {
  if (!state) {
    return;
  }

  bfree(state->order);
  memset(state, 0, sizeof(restreamer_process_state_t));
}

/* Helper to safely get a string from JSON and duplicate it */
STATIC_TESTABLE char *json_get_string_dup(const json_t *obj, const char *key) {
  const json_t *val = json_object_get(obj, key);
  return (val && json_is_string(val)) ? bstrdup(json_string_value(val)) : NULL;
}

/* Helper to safely get an integer from JSON */
STATIC_TESTABLE uint32_t json_get_uint32(const json_t *obj, const char *key) {
  const json_t *val = json_object_get(obj, key);
  return (val && json_is_integer(val)) ? (uint32_t)json_integer_value(val) : 0;
}

/* Helper to safely parse a string number from JSON */
STATIC_TESTABLE uint32_t json_get_string_as_uint32(const json_t *obj,
                                                   const char *key) {
  const json_t *val = json_object_get(obj, key);
  if (!val || !json_is_string(val)) {
    return 0;
  }
  char *endptr;
  const char *str = json_string_value(val);
  unsigned long num = strtoul(str, &endptr, 10);
  /* Check for valid parse and within uint32_t range */
  return (endptr != str && num <= UINT32_MAX) ? (uint32_t)num : 0;
}

/* Helper function to parse a single stream from probe response */
static void parse_stream_info(const json_t *stream, restreamer_stream_info_t *s) {
  if (!stream || !s) {
    return;
  }

  /* Parse string fields */
  s->codec_name = json_get_string_dup(stream, "codec_name");
  s->codec_long_name = json_get_string_dup(stream, "codec_long_name");
  s->codec_type = json_get_string_dup(stream, "codec_type");
  s->pix_fmt = json_get_string_dup(stream, "pix_fmt");
  s->profile = json_get_string_dup(stream, "profile");

  /* Parse integer fields */
  s->width = json_get_uint32(stream, "width");
  s->height = json_get_uint32(stream, "height");
  s->channels = json_get_uint32(stream, "channels");

  /* Parse string-encoded numbers */
  s->bitrate = json_get_string_as_uint32(stream, "bit_rate");
  s->sample_rate = json_get_string_as_uint32(stream, "sample_rate");

  /* Parse FPS from r_frame_rate */
  const json_t *fps = json_object_get(stream, "r_frame_rate");
  if (fps && json_is_string(fps)) {
    sscanf(json_string_value(fps), "%u/%u", &s->fps_num, &s->fps_den);
  }
}

/* Input Probe API */
bool restreamer_api_probe_input(restreamer_api_t *api, const char *process_id,
                                restreamer_probe_info_t *info) {
  if (!api || !process_id || !info) {
    return false;
  }

  memset(info, 0, sizeof(restreamer_probe_info_t));

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/probe", process_id);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  /* Parse format info */
  json_t *format = json_object_get(response, "format");
  if (format && json_is_object(format)) {
    json_t *format_name = json_object_get(format, "format_name");
    if (format_name && json_is_string(format_name)) {
      info->format_name = bstrdup(json_string_value(format_name));
    }

    json_t *format_long = json_object_get(format, "format_long_name");
    if (format_long && json_is_string(format_long)) {
      info->format_long_name = bstrdup(json_string_value(format_long));
    }

    json_t *duration = json_object_get(format, "duration");
    if (duration && json_is_string(duration)) {
      info->duration = (int64_t)(atof(json_string_value(duration)) * 1000000);
    }

    json_t *size = json_object_get(format, "size");
    if (size && json_is_string(size)) {
      info->size = (uint64_t)atoll(json_string_value(size));
    }

    json_t *bitrate = json_object_get(format, "bit_rate");
    if (bitrate && json_is_string(bitrate)) {
      /* Security: Use strtol instead of atoi for better error handling */
      char *endptr;
      long bitrate_val = strtol(json_string_value(bitrate), &endptr, 10);
      if (endptr != json_string_value(bitrate) && bitrate_val >= 0) {
        info->bitrate = (uint32_t)bitrate_val;
      }
    }
  }

  /* Parse streams */
  json_t *streams = json_object_get(response, "streams");
  if (streams && json_is_array(streams)) {
    size_t stream_count = json_array_size(streams);
    info->stream_count = stream_count;
    info->streams = bzalloc(sizeof(restreamer_stream_info_t) * stream_count);

    for (size_t i = 0; i < stream_count; i++) {
      json_t *stream = json_array_get(streams, i);
      parse_stream_info(stream, &info->streams[i]);
    }
  }

  json_decref(response);
  return true;
}

void restreamer_api_free_probe_info(restreamer_probe_info_t *info) {
  if (!info) {
    return;
  }

  bfree(info->format_name);
  bfree(info->format_long_name);

  for (size_t i = 0; i < info->stream_count; i++) {
    bfree(info->streams[i].codec_name);
    bfree(info->streams[i].codec_long_name);
    bfree(info->streams[i].codec_type);
    bfree(info->streams[i].pix_fmt);
    bfree(info->streams[i].profile);
  }

  bfree(info->streams);
  memset(info, 0, sizeof(restreamer_probe_info_t));
}

/* Configuration Management API */
bool restreamer_api_get_config(restreamer_api_t *api, char **config_json) {
  if (!api || !config_json) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/config", &response);

  if (!result || !response) {
    return false;
  }

  *config_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*config_json) {
    dstr_copy(&api->last_error, "Failed to serialize config JSON");
    return false;
  }
  return true;
}

bool restreamer_api_set_config(restreamer_api_t *api, const char *config_json) {
  if (!api || !config_json) {
    return false;
  }

  return api_request_put_json(api, "/api/v3/config", config_json, NULL);
}

bool restreamer_api_reload_config(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  return api_request_json(api, "/api/v3/config/reload", NULL);
}

/* ========================================================================
 * Metrics API
 * ======================================================================== */

bool restreamer_api_get_metrics_list(restreamer_api_t *api,
                                     char **metrics_json) {
  if (!api || !metrics_json) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/metrics", &response);

  if (!result || !response) {
    return false;
  }

  *metrics_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*metrics_json) {
    dstr_copy(&api->last_error, "Failed to serialize metrics JSON");
    return false;
  }
  return true;
}

bool restreamer_api_query_metrics(restreamer_api_t *api, const char *query_json,
                                  char **result_json) {
  if (!api || !query_json || !result_json) {
    return false;
  }

  json_t *response = NULL;
  bool result =
      api_request_put_json(api, "/api/v3/metrics", query_json, &response);

  if (!result || !response) {
    return false;
  }

  *result_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*result_json) {
    dstr_copy(&api->last_error, "Failed to serialize result JSON");
    return false;
  }
  return true;
}

bool restreamer_api_get_prometheus_metrics(restreamer_api_t *api,
                                           char **prometheus_text) {
  if (!api || !prometheus_text) {
    return false;
  }

  /* Build URL */
  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d/metrics", protocol, api->connection.host,
              api->connection.port);

  /* Setup request */
  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  *prometheus_text = response.memory;
  return true;
}

void restreamer_api_free_metrics(restreamer_metrics_t *metrics) {
  if (!metrics) {
    return;
  }

  for (size_t i = 0; i < metrics->count; i++) {
    bfree(metrics->metrics[i].name);
    bfree(metrics->metrics[i].labels);
  }

  bfree(metrics->metrics);
  memset(metrics, 0, sizeof(restreamer_metrics_t));
}

/* ========================================================================
 * Metadata API
 * ======================================================================== */

bool restreamer_api_get_metadata(restreamer_api_t *api, const char *key,
                                 char **value) {
  if (!api || !key || !value) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/metadata/%s", key);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  *value = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*value) {
    dstr_copy(&api->last_error, "Failed to serialize value JSON");
    return false;
  }
  return true;
}

bool restreamer_api_set_metadata(restreamer_api_t *api, const char *key,
                                 const char *value) {
  if (!api || !key || !value) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/metadata/%s", key);

  bool result = api_request_put_json(api, endpoint.array, value, NULL);
  dstr_free(&endpoint);

  return result;
}

bool restreamer_api_get_process_metadata(restreamer_api_t *api,
                                         const char *process_id,
                                         const char *key, char **value) {
  if (!api || !process_id || !key || !value) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/metadata/%s", process_id, key);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  *value = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*value) {
    dstr_copy(&api->last_error, "Failed to serialize value JSON");
    return false;
  }
  return true;
}

bool restreamer_api_set_process_metadata(restreamer_api_t *api,
                                         const char *process_id,
                                         const char *key, const char *value) {
  if (!api || !process_id || !key || !value) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/metadata/%s", process_id, key);

  bool result = api_request_put_json(api, endpoint.array, value, NULL);
  dstr_free(&endpoint);

  return result;
}

/* ========================================================================
 * Playout Management API
 * ======================================================================== */

bool restreamer_api_get_playout_status(restreamer_api_t *api,
                                       const char *process_id,
                                       const char *input_id,
                                       restreamer_playout_status_t *status) {
  if (!api || !process_id || !input_id || !status) {
    return false;
  }

  memset(status, 0, sizeof(restreamer_playout_status_t));

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/playout/%s/status", process_id,
              input_id);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  status->input_id = bstrdup(input_id);

  json_t *url = json_object_get(response, "url");
  if (url && json_is_string(url)) {
    status->url = bstrdup(json_string_value(url));
  }

  json_t *state = json_object_get(response, "state");
  if (state && json_is_string(state)) {
    status->state = bstrdup(json_string_value(state));
  }

  json_t *connected = json_object_get(response, "connected");
  if (connected && json_is_boolean(connected)) {
    status->is_connected = json_boolean_value(connected);
  }

  json_t *bytes = json_object_get(response, "bytes");
  if (bytes && json_is_integer(bytes)) {
    status->bytes_received = (uint64_t)json_integer_value(bytes);
  }

  json_t *bitrate = json_object_get(response, "bitrate");
  if (bitrate && json_is_integer(bitrate)) {
    status->bitrate = (uint32_t)json_integer_value(bitrate);
  }

  json_decref(response);
  return true;
}

bool restreamer_api_switch_input_stream(restreamer_api_t *api,
                                        const char *process_id,
                                        const char *input_id,
                                        const char *new_url) {
  if (!api || !process_id || !input_id || !new_url) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/playout/%s/stream", process_id,
              input_id);

  /* Create JSON body */
  json_t *body = json_object();
  json_object_set_new(body, "url", json_string(new_url));
  char *body_json = json_dumps(body, 0);
  json_decref(body);

  bool result = api_request_put_json(api, endpoint.array, body_json, NULL);

  free(body_json);
  dstr_free(&endpoint);

  return result;
}

bool restreamer_api_reopen_input(restreamer_api_t *api, const char *process_id,
                                 const char *input_id) {
  if (!api || !process_id || !input_id) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/playout/%s/reopen", process_id,
              input_id);

  bool result = api_request_json(api, endpoint.array, NULL);
  dstr_free(&endpoint);

  return result;
}

bool restreamer_api_get_keyframe(restreamer_api_t *api, const char *process_id,
                                 const char *input_id, const char *name,
                                 unsigned char **data, size_t *size) {
  if (!api || !process_id || !input_id || !name || !data || !size) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/playout/%s/keyframe/%s",
              process_id, input_id, name);

  /* Setup request for binary data */
  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  /* Build URL */
  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint.array);

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  dstr_free(&url);
  dstr_free(&endpoint);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  *data = (unsigned char *)response.memory;
  *size = response.size;

  return true;
}

void restreamer_api_free_playout_status(restreamer_playout_status_t *status) {
  if (!status) {
    return;
  }

  bfree(status->input_id);
  bfree(status->url);
  bfree(status->state);
  memset(status, 0, sizeof(restreamer_playout_status_t));
}

/* ========================================================================
 * Token Refresh API
 * ======================================================================== */

bool restreamer_api_refresh_token(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  if (!api->refresh_token) {
    dstr_copy(&api->last_error, "No refresh token available");
    return false;
  }

  /* Build refresh request */
  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d/api/v3/refresh", protocol, api->connection.host,
              api->connection.port);

  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  /* Build authorization header with refresh token - keep it alive until after
   * request completes */
  struct dstr auth_header;
  dstr_init(&auth_header);
  dstr_printf(&auth_header, "Authorization: Bearer %s", api->refresh_token);

  /* Add refresh token to authorization header */
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, auth_header.array);

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(api->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, "");
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  curl_slist_free_all(headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER,
                   NULL);  /* Reset headers to avoid use-after-free */
  dstr_free(&auth_header); /* Now safe to free */
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "Token refresh failed: HTTP %ld", http_code);
    free(response.memory);
    return false;
  }

  /* Parse response to get new access token */
  json_t *root = parse_json_response(api, &response);
  if (!root) {
    return false;
  }

  json_t *access_token = json_object_get(root, "access_token");
  json_t *expires_at = json_object_get(root, "expires_at");

  if (!access_token || !json_is_string(access_token)) {
    dstr_copy(&api->last_error, "No access token in refresh response");
    json_decref(root);
    return false;
  }

  /* Update access token */
  secure_free(api->access_token); /* Security: Clear access token from memory */
  api->access_token = bstrdup(json_string_value(access_token));

  if (expires_at && json_is_integer(expires_at)) {
    api->token_expires = (time_t)json_integer_value(expires_at);
  } else {
    api->token_expires = time(NULL) + 3600;
  }

  json_decref(root);
  obs_log(LOG_INFO, "Access token refreshed successfully");

  return true;
}

bool restreamer_api_force_login(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  /* Clear existing tokens */
  secure_free(api->access_token); /* Security: Clear access token from memory */
  api->access_token = NULL;
  secure_free(
      api->refresh_token); /* Security: Clear refresh token from memory */
  api->refresh_token = NULL;
  api->token_expires = 0;

  /* Perform fresh login */
  return restreamer_api_login(api);
}

/* ========================================================================
 * File System API
 * ======================================================================== */

bool restreamer_api_list_filesystems(restreamer_api_t *api,
                                     char **filesystems_json) {
  if (!api || !filesystems_json) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/fs", &response);

  if (!result || !response) {
    return false;
  }

  *filesystems_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*filesystems_json) {
    dstr_copy(&api->last_error, "Failed to serialize filesystems JSON");
    return false;
  }
  return true;
}

bool restreamer_api_list_files(restreamer_api_t *api, const char *storage,
                               const char *glob_pattern,
                               restreamer_fs_list_t *files) {
  if (!api || !storage || !files) {
    return false;
  }

  memset(files, 0, sizeof(restreamer_fs_list_t));

  struct dstr endpoint;
  dstr_init(&endpoint);

  if (glob_pattern) {
    /* URL encode the glob pattern */
    char *encoded = curl_easy_escape(api->curl, glob_pattern, 0);
    dstr_printf(&endpoint, "/api/v3/fs/%s?glob=%s", storage, encoded);
    curl_free(encoded);
  } else {
    dstr_printf(&endpoint, "/api/v3/fs/%s", storage);
  }

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);
  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  /* Parse file list */
  if (json_is_array(response)) {
    size_t count = json_array_size(response);
    files->count = count;
    files->entries = bzalloc(sizeof(restreamer_fs_entry_t) * count);

    for (size_t i = 0; i < count; i++) {
      const json_t *entry = json_array_get(response, i);
      restreamer_fs_entry_t *f = &files->entries[i];
      parse_fs_entry_fields(entry, f);
    }
  }

  json_decref(response);
  return true;
}

bool restreamer_api_download_file(restreamer_api_t *api, const char *storage,
                                  const char *filepath, unsigned char **data,
                                  size_t *size) {
  if (!api || !storage || !filepath || !data || !size) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/fs/%s/%s", storage, filepath);

  /* Setup request for binary data */
  struct memory_struct response;
  response.memory = NULL; /* realloc(NULL, size) behaves like malloc(size) */
  response.size = 0;

  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint.array);

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);
  /* Clear any POST/PUT fields from previous requests */
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, NULL);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  dstr_free(&url);
  dstr_free(&endpoint);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    free(response.memory);
    return false;
  }

  *data = (unsigned char *)response.memory;
  *size = response.size;

  return true;
}

bool restreamer_api_upload_file(restreamer_api_t *api, const char *storage,
                                const char *filepath, const unsigned char *data,
                                size_t size) {
  if (!api || !storage || !filepath || !data) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/fs/%s/%s", storage, filepath);

  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint.array);

  /* Setup response buffer for upload response */
  struct memory_struct response;
  response.memory = NULL;
  response.size = 0;

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, data);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDSIZE, (long)size);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  /* Free response buffer */
  free(response.memory);

  dstr_free(&url);
  dstr_free(&endpoint);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    return false;
  }

  return true;
}

bool restreamer_api_delete_file(restreamer_api_t *api, const char *storage,
                                const char *filepath) {
  if (!api || !storage || !filepath) {
    return false;
  }

  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/fs/%s/%s", storage, filepath);

  struct dstr url;
  dstr_init(&url);
  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint.array);

  /* Setup response buffer for delete response */
  struct memory_struct response;
  response.memory = NULL;
  response.size = 0;

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(api->curl);

  /* Free response buffer */
  free(response.memory);

  dstr_free(&url);
  dstr_free(&endpoint);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    return false;
  }

  return true;
}

void restreamer_api_free_fs_list(restreamer_fs_list_t *list) {
  if (!list) {
    return;
  }

  for (size_t i = 0; i < list->count; i++) {
    bfree(list->entries[i].name);
    bfree(list->entries[i].path);
  }

  bfree(list->entries);
  memset(list, 0, sizeof(restreamer_fs_list_t));
}

/* ========================================================================
 * Protocol Monitoring API
 * ======================================================================== */

/* Helper function for getting protocol streams (RTMP/SRT) */
static bool get_protocol_streams_helper(restreamer_api_t *api,
                                        const char *endpoint,
                                        char **streams_json) {
  if (!api || !streams_json || !endpoint) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint, &response);

  if (!result || !response) {
    return false;
  }

  *streams_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*streams_json) {
    dstr_copy(&api->last_error, "Failed to serialize streams JSON");
    return false;
  }
  return true;
}

bool restreamer_api_get_rtmp_streams(restreamer_api_t *api,
                                     char **streams_json) {
  return get_protocol_streams_helper(api, "/api/v3/rtmp", streams_json);
}

bool restreamer_api_get_srt_streams(restreamer_api_t *api,
                                    char **streams_json) {
  return get_protocol_streams_helper(api, "/api/v3/srt", streams_json);
}

/* ========================================================================
 * FFmpeg Capabilities API
 * ======================================================================== */

bool restreamer_api_get_skills(restreamer_api_t *api, char **skills_json) {
  if (!api || !skills_json) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/skills", &response);

  if (!result || !response) {
    return false;
  }

  *skills_json = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!*skills_json) {
    dstr_copy(&api->last_error, "Failed to serialize skills JSON");
    return false;
  }
  return true;
}

bool restreamer_api_reload_skills(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  return api_request_json(api, "/api/v3/skills/reload", NULL);
}

/* ========================================================================
 * Server Info & Diagnostics API
 * ======================================================================== */

bool restreamer_api_ping(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/ping", &response);

  if (!result || !response) {
    return false;
  }

  /* Check if response contains "pong" */
  const char *pong = json_string_value(response);
  bool is_pong = (pong && strcmp(pong, "pong") == 0);

  json_decref(response);

  if (!is_pong) {
    dstr_copy(&api->last_error, "Server did not respond with 'pong'");
    return false;
  }

  return true;
}

bool restreamer_api_get_info(restreamer_api_t *api,
                             restreamer_api_info_t *info) {
  if (!api || !info) {
    return false;
  }

  /* Initialize output structure */
  memset(info, 0, sizeof(restreamer_api_info_t));

  json_t *response = NULL;
  bool result = api_request_json(api, "/api", &response);

  if (!result || !response) {
    return false;
  }

  /* Parse API info fields */
  const json_t *name_obj = json_object_get(response, "name");
  if (json_is_string(name_obj)) {
    info->name = bstrdup(json_string_value(name_obj));
  }

  const json_t *version_obj = json_object_get(response, "version");
  if (json_is_string(version_obj)) {
    info->version = bstrdup(json_string_value(version_obj));
  }

  const json_t *build_date_obj = json_object_get(response, "build_date");
  if (json_is_string(build_date_obj)) {
    info->build_date = bstrdup(json_string_value(build_date_obj));
  }

  const json_t *commit_obj = json_object_get(response, "commit");
  if (json_is_string(commit_obj)) {
    info->commit = bstrdup(json_string_value(commit_obj));
  }

  json_decref(response);
  return true;
}

void restreamer_api_free_info(restreamer_api_info_t *info) {
  if (!info) {
    return;
  }

  bfree(info->name);
  bfree(info->version);
  bfree(info->build_date);
  bfree(info->commit);

  memset(info, 0, sizeof(restreamer_api_info_t));
}

bool restreamer_api_get_logs(restreamer_api_t *api, char **logs_text) {
  if (!api || !logs_text) {
    return false;
  }

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/log", &response);

  if (!result || !response) {
    return false;
  }

  /* If response is a string, use it directly */
  if (json_is_string(response)) {
    *logs_text = bstrdup(json_string_value(response));
    json_decref(response);
    return true;
  }

  /* Otherwise serialize JSON to string */
  char *json_str = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!json_str) {
    dstr_copy(&api->last_error, "Failed to serialize logs JSON");
    return false;
  }

  *logs_text = bstrdup(json_str);
  free(json_str);
  return true;
}

bool restreamer_api_get_active_sessions(
    restreamer_api_t *api, restreamer_active_sessions_t *sessions) {
  if (!api || !sessions) {
    return false;
  }

  /* Initialize output structure */
  memset(sessions, 0, sizeof(restreamer_active_sessions_t));

  json_t *response = NULL;
  bool result = api_request_json(api, "/api/v3/session/active", &response);

  if (!result || !response) {
    return false;
  }

  /* Parse session summary fields */
  const json_t *session_count_obj = json_object_get(response, "session_count");
  if (json_is_integer(session_count_obj)) {
    sessions->session_count = (size_t)json_integer_value(session_count_obj);
  } else if (json_is_number(session_count_obj)) {
    sessions->session_count = (size_t)json_number_value(session_count_obj);
  }

  const json_t *rx_bytes_obj = json_object_get(response, "total_rx_bytes");
  if (json_is_integer(rx_bytes_obj)) {
    sessions->total_rx_bytes = (uint64_t)json_integer_value(rx_bytes_obj);
  } else if (json_is_number(rx_bytes_obj)) {
    sessions->total_rx_bytes = (uint64_t)json_number_value(rx_bytes_obj);
  }

  const json_t *tx_bytes_obj = json_object_get(response, "total_tx_bytes");
  if (json_is_integer(tx_bytes_obj)) {
    sessions->total_tx_bytes = (uint64_t)json_integer_value(tx_bytes_obj);
  } else if (json_is_number(tx_bytes_obj)) {
    sessions->total_tx_bytes = (uint64_t)json_number_value(tx_bytes_obj);
  }

  json_decref(response);
  return true;
}

bool restreamer_api_get_process_config(restreamer_api_t *api,
                                       const char *process_id,
                                       char **config_json) {
  if (!api || !process_id || !config_json) {
    return false;
  }

  /* Build endpoint URL */
  struct dstr endpoint;
  dstr_init(&endpoint);
  dstr_printf(&endpoint, "/api/v3/process/%s/config", process_id);

  json_t *response = NULL;
  bool result = api_request_json(api, endpoint.array, &response);

  dstr_free(&endpoint);

  if (!result || !response) {
    return false;
  }

  /* Serialize JSON response to string */
  char *json_str = json_dumps(response, JSON_INDENT(2));
  json_decref(response);

  if (!json_str) {
    dstr_copy(&api->last_error, "Failed to serialize process config JSON");
    return false;
  }

  *config_json = bstrdup(json_str);
  free(json_str);
  return true;
}
