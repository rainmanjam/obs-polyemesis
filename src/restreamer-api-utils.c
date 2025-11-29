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
  // SECURITY: HTTP support is intentional for local development (localhost/127.0.0.1).
  // Production deployments should always use HTTPS. The connection dialog warns users.
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    return false;
  }

  // Must have something after the protocol
  const char *after_protocol = strstr(url, "://");
  // SECURITY: strlen is safe here - after_protocol+3 is guaranteed valid if strstr found "://"
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
  // SECURITY: HTTP support is intentional for local development environments.
  // Production deployments should use HTTPS. The UI warns users about HTTP risks.
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
    // SECURITY: strlen is safe - host_start is derived from validated protocol_end pointer
    host_len = strlen(host_start);
  }

  // SECURITY: Buffer overflow protection - allocate exact size needed + null terminator
  *host = (char *)bmalloc(host_len + 1);
  // SECURITY: strncpy is safe here - we explicitly null-terminate on the next line,
  // and host_len is calculated from the actual source string length
  strncpy(*host, host_start, host_len);
  (*host)[host_len] = '\0';

  // Extract port if present
  if (port_start && (!path_start || port_start < path_start)) {
    /* Security: Use strtol instead of atoi for better error handling */
    char *endptr;
    long port_val = strtol(port_start + 1, &endptr, 10);

    /* Validate port is a valid number and in valid range */
    if (endptr != port_start + 1 && port_val > 0 && port_val <= 65535) {
      *port = (int)port_val;
    } else {
      /* Invalid port, use default */
      *port = *use_https ? 443 : 80;
    }
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
  // NOTE: This function is currently unimplemented as a placeholder.
  // OBS does not provide a native base64 encoding function, and
  // authentication is handled directly by the curl/http client.
  // If base64 encoding becomes available or is needed in the future,
  // this function should encode credentials in "Basic <base64>" format.
  (void)username;
  (void)password;
  return NULL;
}
