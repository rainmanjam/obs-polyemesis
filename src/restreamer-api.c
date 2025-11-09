#include "restreamer-api.h"
#include <curl/curl.h>
#include <jansson.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <string.h>
#include <time.h>
#include <util/bmem.h>
#include <util/dstr.h>

struct restreamer_api {
  restreamer_connection_t connection;
  CURL *curl;
  struct curl_slist *headers;
  char error_buffer[CURL_ERROR_SIZE];
  struct dstr last_error;
  char *access_token;     /* JWT access token */
  char *refresh_token;    /* JWT refresh token */
  time_t token_expires;   /* Token expiration timestamp */
};

/* Memory write callback for curl */
struct memory_struct {
  char *memory;
  size_t size;
};

// cppcheck-suppress constParameterCallback
static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct *)userp;

  char *ptr = brealloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    return 0;
  }

  mem->memory = ptr;
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

  /* Set up headers */
  api->headers =
      curl_slist_append(api->headers, "Content-Type: application/json");

  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, api->headers);
  curl_easy_setopt(api->curl, CURLOPT_ERRORBUFFER, api->error_buffer);
  curl_easy_setopt(api->curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(api->curl, CURLOPT_TIMEOUT, 10L);

  /* Initialize JWT token fields */
  api->access_token = NULL;
  api->refresh_token = NULL;
  api->token_expires = 0;

  return api;
}

void restreamer_api_destroy(restreamer_api_t *api) {
  if (!api) {
    return;
  }

  if (api->curl) {
    curl_easy_cleanup(api->curl);
  }

  if (api->headers) {
    curl_slist_free_all(api->headers);
  }

  bfree(api->connection.host);
  bfree(api->connection.username);
  bfree(api->connection.password);
  bfree(api->access_token);
  bfree(api->refresh_token);
  dstr_free(&api->last_error);

  bfree(api);
}

/* Login to get JWT token */
static bool restreamer_api_login(restreamer_api_t *api) {
  if (!api || !api->connection.username || !api->connection.password) {
    dstr_copy(&api->last_error, "Username and password required for login");
    return false;
  }

  /* Build login request */
  json_t *login_data = json_object();
  json_object_set_new(login_data, "username", json_string(api->connection.username));
  json_object_set_new(login_data, "password", json_string(api->connection.password));

  char *post_data = json_dumps(login_data, 0);
  json_decref(login_data);

  /* Make request without token (login doesn't need auth) */
  struct dstr url;
  dstr_init(&url);

  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d/api/login", protocol, api->connection.host,
              api->connection.port);

  struct memory_struct response;
  response.memory = bmalloc(1);
  response.size = 0;

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(api->curl, CURLOPT_POST, 1L);
  curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);

  CURLcode res = curl_easy_perform(api->curl);
  free(post_data);
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    bfree(response.memory);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "Login failed: HTTP %ld", http_code);
    bfree(response.memory);
    return false;
  }

  /* Parse response to get tokens */
  json_error_t error;
  json_t *root = json_loads(response.memory, 0, &error);
  bfree(response.memory);

  if (!root) {
    dstr_printf(&api->last_error, "JSON parse error: %s", error.text);
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
  bfree(api->access_token);
  api->access_token = bstrdup(json_string_value(access_token));

  if (refresh_token && json_is_string(refresh_token)) {
    bfree(api->refresh_token);
    api->refresh_token = bstrdup(json_string_value(refresh_token));
  }

  if (expires_at && json_is_integer(expires_at)) {
    api->token_expires = (time_t)json_integer_value(expires_at);
  } else {
    /* Default to 1 hour from now if no expiry provided */
    api->token_expires = time(NULL) + 3600;
  }

  json_decref(root);

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
  temp_headers = curl_slist_append(temp_headers, "Content-Type: application/json");
  temp_headers = curl_slist_append(temp_headers, auth_header.array);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, temp_headers);

  struct dstr url;
  dstr_init(&url);

  const char *protocol = api->connection.use_https ? "https" : "http";
  dstr_printf(&url, "%s://%s:%d%s", protocol, api->connection.host,
              api->connection.port, endpoint);

  response->memory = bmalloc(1);
  response->size = 0;

  curl_easy_setopt(api->curl, CURLOPT_URL, url.array);
  curl_easy_setopt(api->curl, CURLOPT_WRITEDATA, (void *)response);

  if (strcmp(method, "POST") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_POST, 1L);
    if (post_data) {
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);
    }
  } else if (strcmp(method, "DELETE") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  } else if (strcmp(method, "PUT") == 0) {
    curl_easy_setopt(api->curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (post_data) {
      curl_easy_setopt(api->curl, CURLOPT_POSTFIELDS, post_data);
    }
  } else {
    curl_easy_setopt(api->curl, CURLOPT_HTTPGET, 1L);
  }

  CURLcode res = curl_easy_perform(api->curl);

  /* Clean up temporary headers */
  curl_slist_free_all(temp_headers);
  curl_easy_setopt(api->curl, CURLOPT_HTTPHEADER, NULL);
  dstr_free(&auth_header);
  dstr_free(&url);

  if (res != CURLE_OK) {
    dstr_copy(&api->last_error, api->error_buffer);
    bfree(response->memory);
    return false;
  }

  long http_code = 0;
  curl_easy_getinfo(api->curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code < 200 || http_code >= 300) {
    dstr_printf(&api->last_error, "HTTP Error: %ld", http_code);
    bfree(response->memory);
    return false;
  }

  return true;
}

bool restreamer_api_test_connection(restreamer_api_t *api) {
  if (!api) {
    return false;
  }

  struct memory_struct response;
  bool result = make_request(api, "/api/v3/", "GET", NULL, &response);

  if (result) {
    bfree(response.memory);
  }

  return result;
}

bool restreamer_api_get_processes(restreamer_api_t *api,
                                  restreamer_process_list_t *list) {
  if (!api || !list) {
    return false;
  }

  struct memory_struct response;
  if (!make_request(api, "/api/v3/process", "GET", NULL, &response)) {
    return false;
  }

  json_error_t error;
  json_t *root = json_loads(response.memory, 0, &error);
  bfree(response.memory);

  if (!root) {
    dstr_printf(&api->last_error, "JSON parse error: %s", error.text);
    return false;
  }

  if (!json_is_array(root)) {
    json_decref(root);
    dstr_copy(&api->last_error, "Expected array response");
    return false;
  }

  size_t count = json_array_size(root);
  list->processes = bzalloc(sizeof(restreamer_process_t) * count);
  list->count = count;

  for (size_t i = 0; i < count; i++) {
    json_t *process_obj = json_array_get(root, i);
    restreamer_process_t *process = &list->processes[i];

    json_t *id = json_object_get(process_obj, "id");
    if (json_is_string(id)) {
      process->id = bstrdup(json_string_value(id));
    }

    json_t *reference = json_object_get(process_obj, "reference");
    if (json_is_string(reference)) {
      process->reference = bstrdup(json_string_value(reference));
    }

    json_t *state = json_object_get(process_obj, "state");
    if (json_is_string(state)) {
      process->state = bstrdup(json_string_value(state));
    }

    json_t *uptime = json_object_get(process_obj, "uptime");
    if (json_is_integer(uptime)) {
      process->uptime_seconds = json_integer_value(uptime);
    }

    json_t *cpu = json_object_get(process_obj, "cpu_usage");
    if (json_is_real(cpu)) {
      process->cpu_usage = json_real_value(cpu);
    }

    json_t *memory = json_object_get(process_obj, "memory");
    if (json_is_integer(memory)) {
      process->memory_bytes = json_integer_value(memory);
    }

    json_t *command = json_object_get(process_obj, "command");
    if (json_is_string(command)) {
      process->command = bstrdup(json_string_value(command));
    }
  }

  json_decref(root);
  return true;
}

bool restreamer_api_start_process(restreamer_api_t *api,
                                  const char *process_id) {
  if (!api || !process_id) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/command");

  json_t *root = json_object();
  json_object_set_new(root, "command", json_string("start"));

  char *post_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "POST", post_data, &response);

  free(post_data);
  dstr_free(&endpoint);

  if (result) {
    bfree(response.memory);
  }

  return result;
}

bool restreamer_api_stop_process(restreamer_api_t *api,
                                 const char *process_id) {
  if (!api || !process_id) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/command");

  json_t *root = json_object();
  json_object_set_new(root, "command", json_string("stop"));

  char *post_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "POST", post_data, &response);

  free(post_data);
  dstr_free(&endpoint);

  if (result) {
    bfree(response.memory);
  }

  return result;
}

bool restreamer_api_restart_process(restreamer_api_t *api,
                                    const char *process_id) {
  if (!api || !process_id) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/command");

  json_t *root = json_object();
  json_object_set_new(root, "command", json_string("restart"));

  char *post_data = json_dumps(root, 0);
  json_decref(root);

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "POST", post_data, &response);

  free(post_data);
  dstr_free(&endpoint);

  if (result) {
    bfree(response.memory);
  }

  return result;
}

bool restreamer_api_get_process(restreamer_api_t *api, const char *process_id,
                                restreamer_process_t *process) {
  if (!api || !process_id || !process) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result) {
    return false;
  }

  json_error_t error;
  json_t *root = json_loads(response.memory, 0, &error);
  bfree(response.memory);

  if (!root) {
    dstr_printf(&api->last_error, "JSON parse error: %s", error.text);
    return false;
  }

  json_t *id = json_object_get(root, "id");
  if (json_is_string(id)) {
    process->id = bstrdup(json_string_value(id));
  }

  json_t *reference = json_object_get(root, "reference");
  if (json_is_string(reference)) {
    process->reference = bstrdup(json_string_value(reference));
  }

  json_t *state = json_object_get(root, "state");
  if (json_is_string(state)) {
    process->state = bstrdup(json_string_value(state));
  }

  json_t *uptime = json_object_get(root, "uptime");
  if (json_is_integer(uptime)) {
    process->uptime_seconds = json_integer_value(uptime);
  }

  json_t *cpu = json_object_get(root, "cpu_usage");
  if (json_is_real(cpu)) {
    process->cpu_usage = json_real_value(cpu);
  }

  json_t *memory = json_object_get(root, "memory");
  if (json_is_integer(memory)) {
    process->memory_bytes = json_integer_value(memory);
  }

  json_t *command = json_object_get(root, "command");
  if (json_is_string(command)) {
    process->command = bstrdup(json_string_value(command));
  }

  json_decref(root);
  return true;
}

bool restreamer_api_get_process_logs(restreamer_api_t *api,
                                     const char *process_id,
                                     restreamer_log_list_t *logs) {
  if (!api || !process_id || !logs) {
    return false;
  }

  struct dstr endpoint;
  dstr_init_copy(&endpoint, "/api/v3/process/");
  dstr_cat(&endpoint, process_id);
  dstr_cat(&endpoint, "/log");

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "GET", NULL, &response);
  dstr_free(&endpoint);

  if (!result) {
    return false;
  }

  json_error_t error;
  json_t *root = json_loads(response.memory, 0, &error);
  bfree(response.memory);

  if (!root || !json_is_array(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  size_t count = json_array_size(root);
  logs->entries = bzalloc(sizeof(restreamer_log_entry_t) * count);
  logs->count = count;

  for (size_t i = 0; i < count; i++) {
    json_t *entry_obj = json_array_get(root, i);
    restreamer_log_entry_t *entry = &logs->entries[i];

    json_t *timestamp = json_object_get(entry_obj, "timestamp");
    if (json_is_string(timestamp)) {
      entry->timestamp = bstrdup(json_string_value(timestamp));
    }

    json_t *message = json_object_get(entry_obj, "message");
    if (json_is_string(message)) {
      entry->message = bstrdup(json_string_value(message));
    }

    json_t *level = json_object_get(entry_obj, "level");
    if (json_is_string(level)) {
      entry->level = bstrdup(json_string_value(level));
    }
  }

  json_decref(root);
  return true;
}

bool restreamer_api_get_sessions(restreamer_api_t *api,
                                 restreamer_session_list_t *sessions) {
  if (!api || !sessions) {
    return false;
  }

  struct memory_struct response;
  if (!make_request(api, "/api/v3/session", "GET", NULL, &response)) {
    return false;
  }

  json_error_t error;
  json_t *root = json_loads(response.memory, 0, &error);
  bfree(response.memory);

  if (!root || !json_is_array(root)) {
    if (root)
      json_decref(root);
    return false;
  }

  size_t count = json_array_size(root);
  sessions->sessions = bzalloc(sizeof(restreamer_session_t) * count);
  sessions->count = count;

  for (size_t i = 0; i < count; i++) {
    json_t *session_obj = json_array_get(root, i);
    restreamer_session_t *session = &sessions->sessions[i];

    json_t *session_id = json_object_get(session_obj, "id");
    if (json_is_string(session_id)) {
      session->session_id = bstrdup(json_string_value(session_id));
    }

    json_t *reference = json_object_get(session_obj, "reference");
    if (json_is_string(reference)) {
      session->reference = bstrdup(json_string_value(reference));
    }

    json_t *bytes_sent = json_object_get(session_obj, "bytes_sent");
    if (json_is_integer(bytes_sent)) {
      session->bytes_sent = json_integer_value(bytes_sent);
    }

    json_t *bytes_received = json_object_get(session_obj, "bytes_received");
    if (json_is_integer(bytes_received)) {
      session->bytes_received = json_integer_value(bytes_received);
    }

    json_t *remote_addr = json_object_get(session_obj, "remote_addr");
    if (json_is_string(remote_addr)) {
      session->remote_addr = bstrdup(json_string_value(remote_addr));
    }
  }

  json_decref(root);
  return true;
}

bool restreamer_api_create_process(restreamer_api_t *api, const char *reference,
                                   const char *input_url,
                                   const char **output_urls,
                                   size_t output_count,
                                   const char *video_filter) {
  if (!api || !reference || !input_url) {
    return false;
  }

  json_t *root = json_object();
  json_object_set_new(root, "reference", json_string(reference));

  /* Build FFmpeg command for multistreaming */
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

  struct memory_struct response;
  bool result =
      make_request(api, "/api/v3/process", "POST", post_data, &response);

  free(post_data);

  if (result) {
    bfree(response.memory);
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

  struct memory_struct response;
  bool result = make_request(api, endpoint.array, "DELETE", NULL, &response);
  dstr_free(&endpoint);

  if (result) {
    bfree(response.memory);
  }

  return result;
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
