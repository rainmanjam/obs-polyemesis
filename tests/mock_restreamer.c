/*
 * Mock Restreamer Server for Testing
 *
 * Implements a minimal HTTP server that simulates the datarhei Restreamer API
 * for integration testing purposes.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "mock_restreamer.h"

/* Mock server state */
typedef struct {
  int socket_fd;
  uint16_t port;
  bool running;
  pthread_t thread;
} mock_server_t;

static mock_server_t g_server = {0};

/* Mock API responses */
static const char *RESPONSE_PROCESSES =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 253\r\n"
    "\r\n"
    "["
    "  {"
    "    \"id\": \"test-process-1\","
    "    \"reference\": \"test-stream\","
    "    \"state\": \"running\","
    "    \"uptime\": 3600,"
    "    \"cpu_usage\": 25.5,"
    "    \"memory_bytes\": 104857600,"
    "    \"command\": \"ffmpeg -i rtmp://input -c copy rtmp://output\""
    "  }"
    "]";

static const char *RESPONSE_PROCESS_START = "HTTP/1.1 200 OK\r\n"
                                            "Content-Type: application/json\r\n"
                                            "Content-Length: 34\r\n"
                                            "\r\n"
                                            "{\"status\": \"process_started\"}";

static const char *RESPONSE_PROCESS_STOP = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type: application/json\r\n"
                                           "Content-Length: 33\r\n"
                                           "\r\n"
                                           "{\"status\": \"process_stopped\"}";

static const char *RESPONSE_UNAUTHORIZED = "HTTP/1.1 401 Unauthorized\r\n"
                                           "Content-Type: application/json\r\n"
                                           "Content-Length: 30\r\n"
                                           "\r\n"
                                           "{\"error\": \"unauthorized\"}";

static const char *RESPONSE_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n"
                                        "Content-Type: application/json\r\n"
                                        "Content-Length: 26\r\n"
                                        "\r\n"
                                        "{\"error\": \"not_found\"}";

/* Handle HTTP request */
static void handle_request(int client_fd, const char *request) {
  const char *response = RESPONSE_NOT_FOUND;

  /* Parse request line */
  if (strstr(request, "GET /api/v3/process") != NULL) {
    /* Check for auth header */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = RESPONSE_PROCESSES;
    }
  } else if (strstr(request, "PUT /api/v3/process/") != NULL &&
             strstr(request, "/command/start") != NULL) {
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = RESPONSE_PROCESS_START;
    }
  } else if (strstr(request, "PUT /api/v3/process/") != NULL &&
             strstr(request, "/command/stop") != NULL) {
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = RESPONSE_PROCESS_STOP;
    }
  }

  /* Send response */
  send(client_fd, response, strlen(response), 0);
}

/* Server thread function */
static void *server_thread(void *arg) {
  (void)arg;

  while (g_server.running) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(g_server.socket_fd, (struct sockaddr *)&client_addr,
                           &client_len);

    if (client_fd < 0) {
      if (g_server.running) {
        perror("accept failed");
      }
      continue;
    }

    /* Read request */
    char buffer[4096] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      handle_request(client_fd, buffer);
    }

#ifdef _WIN32
    closesocket(client_fd);
#else
    close(client_fd);
#endif
  }

  return NULL;
}

/* Start mock server */
bool mock_restreamer_start(uint16_t port) {
#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return false;
  }
#endif

  /* Create socket */
  g_server.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (g_server.socket_fd < 0) {
    return false;
  }

  /* Set socket options */
  int opt = 1;
  setsockopt(g_server.socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
             sizeof(opt));

  /* Bind to port */
  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(g_server.socket_fd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
#ifdef _WIN32
    closesocket(g_server.socket_fd);
#else
    close(g_server.socket_fd);
#endif
    return false;
  }

  /* Listen for connections */
  if (listen(g_server.socket_fd, 5) < 0) {
#ifdef _WIN32
    closesocket(g_server.socket_fd);
#else
    close(g_server.socket_fd);
#endif
    return false;
  }

  g_server.port = port;
  g_server.running = true;

  /* Start server thread */
  if (pthread_create(&g_server.thread, NULL, server_thread, NULL) != 0) {
#ifdef _WIN32
    closesocket(g_server.socket_fd);
#else
    close(g_server.socket_fd);
#endif
    g_server.running = false;
    return false;
  }

  return true;
}

/* Stop mock server */
void mock_restreamer_stop(void) {
  if (!g_server.running) {
    return;
  }

  g_server.running = false;

  /* Close socket to unblock accept() */
#ifdef _WIN32
  closesocket(g_server.socket_fd);
#else
  close(g_server.socket_fd);
#endif

  /* Wait for thread to finish */
  pthread_join(g_server.thread, NULL);

#ifdef _WIN32
  WSACleanup();
#endif
}

/* Get server port */
uint16_t mock_restreamer_get_port(void) { return g_server.port; }
