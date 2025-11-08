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

/* Platform-specific thread type */
#ifdef _WIN32
typedef HANDLE thread_handle_t;
typedef int ssize_t; /* Windows doesn't define ssize_t */
typedef SOCKET socket_t;
#define SOCKET_ERROR_VALUE INVALID_SOCKET
#define IS_SOCKET_ERROR(s) ((s) == INVALID_SOCKET)
#else
typedef pthread_t thread_handle_t;
typedef int socket_t;
#define SOCKET_ERROR_VALUE (-1)
#define IS_SOCKET_ERROR(s) ((s) < 0)
#endif

/* Mock server state */
typedef struct {
  socket_t socket_fd;
  uint16_t port;
  bool running;
  thread_handle_t thread;
} mock_server_t;

static mock_server_t g_server = {0};

/* Mock API responses */
static const char *RESPONSE_PROCESSES =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 221\r\n"
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
                                            "Content-Length: 29\r\n"
                                            "\r\n"
                                            "{\"status\": \"process_started\"}";

static const char *RESPONSE_PROCESS_STOP = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type: application/json\r\n"
                                           "Content-Length: 29\r\n"
                                           "\r\n"
                                           "{\"status\": \"process_stopped\"}";

static const char *RESPONSE_UNAUTHORIZED = "HTTP/1.1 401 Unauthorized\r\n"
                                           "Content-Type: application/json\r\n"
                                           "Content-Length: 25\r\n"
                                           "\r\n"
                                           "{\"error\": \"unauthorized\"}";

static const char *RESPONSE_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n"
                                        "Content-Type: application/json\r\n"
                                        "Content-Length: 22\r\n"
                                        "\r\n"
                                        "{\"error\": \"not_found\"}";

/* Handle HTTP request */
static void handle_request(socket_t client_fd, const char *request) {
  const char *response = RESPONSE_NOT_FOUND;

  /* Parse request line */
  /* Base API endpoint - used by test_connection() */
  if (strstr(request, "GET /api/v3/ ") != NULL ||
      strstr(request, "GET /api/v3 ") != NULL) {
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 2\r\n"
               "\r\n"
               "{}";
  } else if (strstr(request, "GET /api/v3/process") != NULL) {
    /* Check for auth header */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = RESPONSE_PROCESSES;
    }
  } else if (strstr(request, "POST /api/v3/process/") != NULL &&
             strstr(request, "/command ") != NULL) {
    /* Check for start or stop in the request body */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else if (strstr(request, "\"start\"") != NULL) {
      response = RESPONSE_PROCESS_START;
    } else if (strstr(request, "\"stop\"") != NULL) {
      response = RESPONSE_PROCESS_STOP;
    } else {
      response = RESPONSE_NOT_FOUND;
    }
  }

  /* Send response */
  send(client_fd, response, (int)strlen(response), 0);
}

/* Server thread function */
#ifdef _WIN32
static DWORD WINAPI server_thread(LPVOID arg) {
  (void)arg;
#else
static void *server_thread(void *arg) {
  (void)arg;
#endif
  printf("[MOCK] Server thread started, entering accept loop\n");

  while (g_server.running) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("[MOCK] Waiting for client connection...\n");
    socket_t client_fd = accept(g_server.socket_fd, (struct sockaddr *)&client_addr,
                           &client_len);

    if (IS_SOCKET_ERROR(client_fd)) {
      if (g_server.running) {
#ifdef _WIN32
        fprintf(stderr, "[MOCK] ERROR: accept() failed with error %d\n", WSAGetLastError());
#else
        perror("[MOCK] ERROR: accept() failed");
#endif
      }
      continue;
    }

    printf("[MOCK] Client connected, reading request...\n");

    /* Read request */
    char buffer[4096] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      printf("[MOCK] Received %d bytes, handling request\n", (int)bytes_read);
      handle_request(client_fd, buffer);
      printf("[MOCK] Response sent\n");
    } else {
      printf("[MOCK] No data received (bytes_read=%d)\n", (int)bytes_read);
    }

#ifdef _WIN32
    closesocket(client_fd);
#else
    close(client_fd);
#endif
    printf("[MOCK] Client connection closed\n");
  }

  printf("[MOCK] Server thread exiting\n");

#ifdef _WIN32
  return 0;
#else
  return NULL;
#endif
}

/* Start mock server */
bool mock_restreamer_start(uint16_t port) {
  printf("[MOCK] Starting mock server on port %d...\n", port);

#ifdef _WIN32
  WSADATA wsa_data;
  printf("[MOCK] Initializing Winsock...\n");
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    fprintf(stderr, "[MOCK] ERROR: WSAStartup failed\n");
    return false;
  }
  printf("[MOCK] Winsock initialized\n");
#endif

  /* Create socket */
  printf("[MOCK] Creating socket...\n");
  g_server.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (IS_SOCKET_ERROR(g_server.socket_fd)) {
#ifdef _WIN32
    fprintf(stderr, "[MOCK] ERROR: socket() failed with error %d\n", WSAGetLastError());
#else
    perror("[MOCK] ERROR: socket() failed");
#endif
    return false;
  }
  printf("[MOCK] Socket created successfully\n");

  /* Set socket options */
  printf("[MOCK] Setting SO_REUSEADDR...\n");
  int opt = 1;
  setsockopt(g_server.socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
             sizeof(opt));

  /* Bind to port */
  printf("[MOCK] Binding to port %d...\n", port);
  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_port = htons(port);

  if (bind(g_server.socket_fd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
#ifdef _WIN32
    fprintf(stderr, "[MOCK] ERROR: bind() failed with error %d\n", WSAGetLastError());
    closesocket(g_server.socket_fd);
#else
    perror("[MOCK] ERROR: bind() failed");
    close(g_server.socket_fd);
#endif
    return false;
  }
  printf("[MOCK] Bound to port %d\n", port);

  /* Listen for connections */
  printf("[MOCK] Starting listen...\n");
  if (listen(g_server.socket_fd, 5) < 0) {
#ifdef _WIN32
    fprintf(stderr, "[MOCK] ERROR: listen() failed with error %d\n", WSAGetLastError());
    closesocket(g_server.socket_fd);
#else
    perror("[MOCK] ERROR: listen() failed");
    close(g_server.socket_fd);
#endif
    return false;
  }
  printf("[MOCK] Listening for connections\n");

  g_server.port = port;
  g_server.running = true;

  /* Start server thread */
  printf("[MOCK] Creating server thread...\n");
#ifdef _WIN32
  g_server.thread = CreateThread(NULL, 0, server_thread, NULL, 0, NULL);
  if (g_server.thread == NULL) {
    fprintf(stderr, "[MOCK] ERROR: CreateThread failed with error %lu\n", GetLastError());
#else
  if (pthread_create(&g_server.thread, NULL, server_thread, NULL) != 0) {
    perror("[MOCK] ERROR: pthread_create failed");
#endif
#ifdef _WIN32
    closesocket(g_server.socket_fd);
#else
    close(g_server.socket_fd);
#endif
    g_server.running = false;
    return false;
  }
  printf("[MOCK] Server thread created, mock server ready\n");

  return true;
}

/* Stop mock server */
void mock_restreamer_stop(void) {
  if (!g_server.running) {
    return;
  }

  g_server.running = false;

  /* Shutdown socket first to unblock accept() */
#ifdef _WIN32
  shutdown(g_server.socket_fd, SD_BOTH);
  closesocket(g_server.socket_fd);
#else
  shutdown(g_server.socket_fd, SHUT_RDWR);
  close(g_server.socket_fd);
#endif

  /* Wait for thread to finish */
#ifdef _WIN32
  WaitForSingleObject(g_server.thread, INFINITE);
  CloseHandle(g_server.thread);
#else
  pthread_join(g_server.thread, NULL);
#endif

#ifdef _WIN32
  WSACleanup();
#endif
}

/* Get server port */
uint16_t mock_restreamer_get_port(void) { return g_server.port; }
