// OBS Polyemesis - Restreamer API Utility Functions
// Helper functions for URL validation, endpoint construction, and API utilities

#ifndef RESTREAMER_API_UTILS_H
#define RESTREAMER_API_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Validates if a string is a valid Restreamer URL
 *
 * @param url URL string to validate
 * @return true if valid, false otherwise
 */
bool is_valid_restreamer_url(const char *url);

/**
 * Builds a complete API endpoint URL from base URL and endpoint path
 *
 * @param base_url Base URL (e.g., "http://localhost:8080")
 * @param endpoint Endpoint path (e.g., "/api/v3/process")
 * @return Allocated string with complete URL (caller must free), NULL on error
 */
char *build_api_endpoint(const char *base_url, const char *endpoint);

/**
 * Parses URL components into host, port, and scheme
 *
 * @param url Full URL to parse
 * @param host Output: allocated host string (caller must free)
 * @param port Output: port number (0 if not specified)
 * @param use_https Output: true if HTTPS, false if HTTP
 * @return true on success, false on parse error
 */
bool parse_url_components(const char *url, char **host, int *port,
                          bool *use_https);

/**
 * Sanitizes URL input by removing trailing slashes and whitespace
 *
 * @param url URL to sanitize
 * @return Allocated sanitized string (caller must free), NULL on error
 */
char *sanitize_url_input(const char *url);

/**
 * Validates if a port number is in valid range
 *
 * @param port Port number to validate
 * @return true if valid (1-65535), false otherwise
 */
bool is_valid_port(int port);

/**
 * Builds authentication header value for Basic Auth
 *
 * @param username Username
 * @param password Password
 * @return Allocated "Basic <base64>" string (caller must free), NULL on error
 */
char *build_auth_header(const char *username, const char *password);

#ifdef __cplusplus
}
#endif

#endif // RESTREAMER_API_UTILS_H
