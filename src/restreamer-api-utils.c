// OBS Polyemesis - Restreamer API Utility Functions Implementation

#include "restreamer-api-utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <util/base.h>
#include <util/bmem.h>
#include <util/dstr.h>

// URL validation
bool is_valid_restreamer_url(const char *url) {
  if (!url || *url == '\0') {
    return false;
  }

  // Must start with http:// or https://
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    return false;
  }

  // Must have something after the protocol
  const char *after_protocol = strstr(url, "://");
  if (!after_protocol || strlen(after_protocol + 3) == 0) {
    return false;
  }

  return true;
}

// Build complete API endpoint
char *build_api_endpoint(const char *base_url, const char *endpoint) {
  if (!base_url || !endpoint) {
    return NULL;
  }

  struct dstr result;
  dstr_init(&result);

  // Add base URL (remove trailing slash if present)
  dstr_copy(&result, base_url);
  if (result.len > 0 && result.array[result.len - 1] == '/') {
    dstr_resize(&result, result.len - 1);
  }

  // Add endpoint (ensure leading slash)
  if (*endpoint != '/') {
    dstr_cat(&result, "/");
  }
  dstr_cat(&result, endpoint);

  // Return as regular C string
  char *output = bstrdup(result.array);
  dstr_free(&result);

  return output;
}

// Parse URL components
bool parse_url_components(const char *url, char **host, int *port,
                          bool *use_https) {
  if (!url || !host || !port || !use_https) {
    return false;
  }

  // Initialize outputs
  *host = NULL;
  *port = 0;
  *use_https = false;

  // Check protocol
  const char *protocol_end = strstr(url, "://");
  if (!protocol_end) {
    return false;
  }

  // Determine if HTTPS
  if (strncmp(url, "https://", 8) == 0) {
    *use_https = true;
    protocol_end += 3;
  } else if (strncmp(url, "http://", 7) == 0) {
    *use_https = false;
    protocol_end += 3;
  } else {
    return false;
  }

  // Find port separator or path start
  const char *host_start = protocol_end;
  const char *port_start = strchr(host_start, ':');
  const char *path_start = strchr(host_start, '/');

  // Extract host
  size_t host_len;
  if (port_start && (!path_start || port_start < path_start)) {
    // Has port
    host_len = port_start - host_start;
  } else if (path_start) {
    // Has path but no port
    host_len = path_start - host_start;
  } else {
    // Just host
    host_len = strlen(host_start);
  }

  *host = (char *)bmalloc(host_len + 1);
  strncpy(*host, host_start, host_len);
  (*host)[host_len] = '\0';

  // Extract port if present
  if (port_start && (!path_start || port_start < path_start)) {
    *port = atoi(port_start + 1);
  } else {
    // Default ports
    *port = *use_https ? 443 : 80;
  }

  return true;
}

// Sanitize URL input
char *sanitize_url_input(const char *url) {
  if (!url) {
    return NULL;
  }

  // Skip leading whitespace
  while (*url && isspace(*url)) {
    url++;
  }

  if (*url == '\0') {
    return bstrdup("");
  }

  // Copy to mutable buffer
  struct dstr result;
  dstr_init(&result);
  dstr_copy(&result, url);

  // Remove trailing whitespace
  while (result.len > 0 && isspace(result.array[result.len - 1])) {
    dstr_resize(&result, result.len - 1);
  }

  // Remove trailing slashes
  while (result.len > 0 && result.array[result.len - 1] == '/') {
    dstr_resize(&result, result.len - 1);
  }

  char *output = bstrdup(result.array);
  dstr_free(&result);

  return output;
}

// Validate port number
bool is_valid_port(int port) { return port > 0 && port <= 65535; }

// Build Basic Auth header
char *build_auth_header(const char *username, const char *password) {
  // TODO: Implement base64 encoding when needed
  // OBS doesn't provide base64_encode() function
  // For now, authentication is handled by curl/http client directly
  // This function is reserved for future use
  (void)username;
  (void)password;
  return NULL;
}
