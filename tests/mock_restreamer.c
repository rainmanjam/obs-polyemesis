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
  volatile bool running;  /* volatile for thread-safe access */
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

static const char *RESPONSE_PROCESS_RESTART = "HTTP/1.1 200 OK\r\n"
                                               "Content-Type: application/json\r\n"
                                               "Content-Length: 31\r\n"
                                               "\r\n"
                                               "{\"status\": \"process_restarted\"}";

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

static const char *RESPONSE_LOGIN = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: application/json\r\n"
                                    "Content-Length: 114\r\n"
                                    "\r\n"
                                    "{\"access_token\": \"mock_access_token_12345\", "
                                    "\"refresh_token\": \"mock_refresh_token_67890\", "
                                    "\"expires_at\": 9999999999}";

/* Handle HTTP request */
static void handle_request(socket_t client_fd, const char *request) {
  const char *response = RESPONSE_NOT_FOUND;

  /* Extract first line for logging */
  char request_line[256] = {0};
  const char *line_end = strstr(request, "\r\n");
  if (line_end) {
    size_t len = (line_end - request) < 255 ? (line_end - request) : 255;
    strncpy(request_line, request, len);
    request_line[len] = '\0';
  }
  printf("[MOCK] Request: %s\n", request_line);

  /* Parse request line */
  /* JWT Login endpoint */
  if (strstr(request, "POST /api/login ") != NULL || strstr(request, "POST /api/v3/login ") != NULL) {
    printf("[MOCK] -> Matched: POST /api/login\n");
    response = RESPONSE_LOGIN;
  } else if (strstr(request, "POST /api/refresh") != NULL || strstr(request, "POST /api/v3/refresh") != NULL) {
    /* Refresh token */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 93\r\n"
               "\r\n"
               "{\"access_token\": \"refreshed_token\", \"refresh_token\": \"new_refresh\", \"expires_at\": 9999999999}";
  } else if (strstr(request, "GET /api/v3/ ") != NULL ||
      strstr(request, "GET /api/v3 ") != NULL) {
    /* Base API endpoint - used by test_connection() */
    printf("[MOCK] -> Matched: GET /api/v3/ (base endpoint)\n");
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 2\r\n"
               "\r\n"
               "{}";
  } else if (strstr(request, "GET /api/v3/config") != NULL) {
    /* Get config */
    printf("[MOCK] -> Matched: GET /api/v3/config\n");
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 38\r\n"
               "\r\n"
               "{\"config\": \"test\", \"setting\": \"value\"}";
  } else if (strstr(request, "PUT /api/v3/config") != NULL || strstr(request, "POST /api/v3/config ") != NULL) {
    /* Set config */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "POST /api/v3/config/reload") != NULL) {
    /* Reload config */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 30\r\n"
               "\r\n"
               "{\"status\": \"reloaded\"}";
  } else if (strstr(request, "GET /api/v3/metrics/prometheus") != NULL) {
    /* Prometheus metrics */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain\r\n"
               "Content-Length: 50\r\n"
               "\r\n"
               "# TYPE cpu_usage gauge\ncpu_usage 25.5\n";
  } else if (strstr(request, "POST /api/v3/metrics/query") != NULL ||
             strstr(request, "PUT /api/v3/metrics") != NULL) {
    /* Query metrics */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 30\r\n"
               "\r\n"
               "{\"results\": [{\"value\": 25.5}]}";
  } else if (strstr(request, "GET /api/v3/metrics") != NULL) {
    /* Get metrics list */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 47\r\n"
               "\r\n"
               "{\"metrics\": [\"cpu_usage\", \"memory\", \"bitrate\"]}";
  } else if (strstr(request, "GET /api/v3/sessions") != NULL) {
    /* Get sessions */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 68\r\n"
               "\r\n"
               "{\"sessions\": [{\"id\": \"session1\", \"active\": true, \"duration\": 3600}]}";
  } else if (strstr(request, "GET /api/v3/metadata/") != NULL) {
    /* Get metadata */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 26\r\n"
               "\r\n"
               "{\"data\": \"metadata_value\"}";
  } else if (strstr(request, "PUT /api/v3/metadata/") != NULL) {
    /* Set metadata */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "DELETE /api/v3/process/") != NULL) {
    /* Delete process */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 28\r\n"
               "\r\n"
               "{\"status\": \"deleted\"}";
  } else if (strstr(request, "POST /api/v3/process ") != NULL || strstr(request, "POST /api/v3/process\r\n") != NULL) {
    /* Create process */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 82\r\n"
               "\r\n"
               "{\"id\": \"new-process\", \"reference\": \"new-stream\", \"state\": \"idle\", \"created\": true}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/state") != NULL) {
    /* Get process state */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 80\r\n"
               "\r\n"
               "{\"state\": \"running\", \"uptime\": 3600, \"cpu\": 25.5, \"memory\": 104857600}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/logs") != NULL) {
    /* Get process logs */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 68\r\n"
               "\r\n"
               "{\"logs\": [{\"time\": 1234567890, \"level\": \"info\", \"message\": \"test\"}]}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/playout/") != NULL && strstr(request, "/status") != NULL) {
    /* Get playout status */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 119\r\n"
               "\r\n"
               "{\"url\": \"rtmp://localhost:1935/live/test\", \"state\": \"running\", \"connected\": true, \"bytes\": 1024000, \"bitrate\": 5000000}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/playout/") != NULL && strstr(request, "/reopen") != NULL) {
    /* Reopen input */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "PUT /api/v3/process/") != NULL && strstr(request, "/playout/") != NULL && strstr(request, "/stream") != NULL) {
    /* Switch input stream */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/metadata/") != NULL) {
    /* Get process metadata */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 30\r\n"
               "\r\n"
               "{\"proc_data\": \"process_value\"}";
  } else if (strstr(request, "PUT /api/v3/process/") != NULL && strstr(request, "/metadata/") != NULL) {
    /* Set process metadata */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/probe") != NULL) {
    /* Probe input */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 336\r\n"
               "\r\n"
               "{\"format\":{\"format_name\":\"rtmp\",\"format_long_name\":\"RTMP\",\"duration\":\"0\",\"size\":\"0\",\"bit_rate\":\"5000000\"},\"streams\":[{\"codec_name\":\"h264\",\"codec_long_name\":\"H.264\",\"codec_type\":\"video\",\"width\":1920,\"height\":1080,\"bit_rate\":\"5000000\"},{\"codec_name\":\"aac\",\"codec_long_name\":\"AAC\",\"codec_type\":\"audio\",\"sample_rate\":\"48000\",\"channels\":2}]}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/snapshot") != NULL) {
    /* Get keyframe */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 50\r\n"
               "\r\n"
               "{\"data\": \"base64encodedimagedata\", \"size\": 1024}";
  } else if (strstr(request, "POST /api/v3/process/") != NULL && strstr(request, "/switch") != NULL) {
    /* Switch input stream */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 28\r\n"
               "\r\n"
               "{\"status\": \"switched\"}";
  } else if (strstr(request, "POST /api/v3/process/") != NULL && strstr(request, "/reopen") != NULL) {
    /* Reopen input */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 28\r\n"
               "\r\n"
               "{\"status\": \"reopened\"}";
  } else if (strstr(request, "PUT /api/v3/process/") != NULL && strstr(request, "/outputs/") != NULL && strstr(request, "/encoding") != NULL) {
    /* Update output encoding */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "GET /api/v3/process/") != NULL && strstr(request, "/outputs/") != NULL && strstr(request, "/encoding") != NULL) {
    /* Get output encoding */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 111\r\n"
               "\r\n"
               "{\"video_bitrate\": 4500000, \"audio_bitrate\": 192000, \"width\": 1920, \"height\": 1080, \"fps_num\": 30, \"fps_den\": 1}";
  } else if (strstr(request, "GET /api/v3/process") != NULL) {
    /* Check for auth header */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = RESPONSE_PROCESSES;
    }
  } else if (strstr(request, "GET /api/v3/process/test-process-1 ") != NULL ||
             strstr(request, "GET /api/v3/process/test-process-1\r\n") != NULL) {
    /* Get single process */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else {
      response = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: 161\r\n"
                 "\r\n"
                 "{"
                 "\"id\": \"test-process-1\","
                 "\"reference\": \"test-stream\","
                 "\"state\": \"running\","
                 "\"uptime\": 3600,"
                 "\"cpu_usage\": 25.5,"
                 "\"memory_bytes\": 104857600"
                 "}";
    }
  } else if (strstr(request, "POST /api/v3/process/") != NULL &&
             strstr(request, "/command ") != NULL) {
    /* Check for start, stop, or restart in the request body */
    if (strstr(request, "Authorization:") == NULL) {
      response = RESPONSE_UNAUTHORIZED;
    } else if (strstr(request, "\"start\"") != NULL) {
      response = RESPONSE_PROCESS_START;
    } else if (strstr(request, "\"stop\"") != NULL) {
      response = RESPONSE_PROCESS_STOP;
    } else if (strstr(request, "\"restart\"") != NULL) {
      response = RESPONSE_PROCESS_RESTART;
    } else {
      response = RESPONSE_NOT_FOUND;
    }
  } else if (strstr(request, "PUT /api/v3/fs/") != NULL) {
    /* Upload file */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 16\r\n"
               "\r\n"
               "{\"status\": \"ok\"}";
  } else if (strstr(request, "DELETE /api/v3/fs/") != NULL) {
    /* Delete file */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 21\r\n"
               "\r\n"
               "{\"status\": \"deleted\"}";
  } else if (strstr(request, "GET /api/v3/fs ") != NULL || strstr(request, "GET /api/v3/fs\r\n") != NULL) {
    /* List filesystems */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 49\r\n"
               "\r\n"
               "{\"filesystems\": [{\"path\": \"/\", \"type\": \"local\"}]}";
  } else if (strstr(request, "GET /api/v3/fs/") != NULL) {
    /* Get file or list files - need to differentiate by counting slashes */
    const char *fs_path = strstr(request, "/api/v3/fs/");
    if (fs_path) {
      const char *after_fs = fs_path + 11; /* Skip "/api/v3/fs/" */
      const char *second_slash = strchr(after_fs, '/');
      if (second_slash && second_slash < strstr(after_fs, " HTTP")) {
        /* Download file - has storage/filepath */
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/octet-stream\r\n"
                   "Content-Length: 17\r\n"
                   "\r\n"
                   "Test file content";
      } else {
        /* List files in storage */
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Content-Length: 50\r\n"
                   "\r\n"
                   "{\"files\": [{\"name\": \"test.mp4\", \"size\": 1024000}]}";
      }
    }
  } else if (strstr(request, "GET /api/v3/rtmp") != NULL) {
    /* Get RTMP streams */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 49\r\n"
               "\r\n"
               "{\"streams\": [{\"app\": \"live\", \"name\": \"stream1\"}]}";
  } else if (strstr(request, "GET /api/v3/srt") != NULL) {
    /* Get SRT streams */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 43\r\n"
               "\r\n"
               "{\"streams\": [{\"port\": 9000, \"id\": \"srt1\"}]}";
  } else if (strstr(request, "GET /api/v3/skills/reload") != NULL) {
    /* Reload skills */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 22\r\n"
               "\r\n"
               "{\"status\": \"reloaded\"}";
  } else if (strstr(request, "GET /api/v3/skills") != NULL) {
    /* Get skills */
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 78\r\n"
               "\r\n"
               "{\"skills\": {\"encoders\": [\"libx264\", \"libx265\"], \"decoders\": [\"h264\", \"hevc\"]}}";
  }

  /* Send response - loop to ensure all bytes are sent */
  size_t total_len = strlen(response);
  size_t sent = 0;
  printf("[MOCK] Sending response: %zu bytes total\n", total_len);
  fflush(stdout);
  while (sent < total_len) {
    ssize_t n = send(client_fd, response + sent, (int)(total_len - sent), 0);
    if (n < 0) {
      /* Send error */
#ifdef _WIN32
      fprintf(stderr, "[MOCK] send() error: %d\n", WSAGetLastError());
      fflush(stderr);
#else
      perror("[MOCK] send() error");
#endif
      break;
    } else if (n == 0) {
      /* Connection closed */
      fprintf(stderr, "[MOCK] send() returned 0, connection closed\n");
      fflush(stderr);
      break;
    }
    sent += (size_t)n;
    printf("[MOCK] Sent %d bytes, total so far: %zu/%zu\n", (int)n, sent, total_len);
    fflush(stdout);
  }

  if (sent < total_len) {
    fprintf(stderr, "[MOCK] WARNING: Only sent %zu of %zu bytes\n", sent, total_len);
    fflush(stderr);
  } else {
    printf("[MOCK] Successfully sent all %zu bytes\n", sent);
    fflush(stdout);
  }
}

/* Server thread function */
#ifdef _WIN32
static DWORD WINAPI server_thread(LPVOID arg) {
  (void)arg;
#else
static void *server_thread(void *arg) {
  (void)arg;
#endif
  /* Set stdout to unbuffered for immediate output */
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  printf("[MOCK] Server thread started, entering accept loop\n");

  while (g_server.running) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("[MOCK] Waiting for client connection...\n");
    fflush(stdout);
    socket_t client_fd = accept(g_server.socket_fd,
                                (struct sockaddr *)&client_addr, &client_len);

    if (IS_SOCKET_ERROR(client_fd)) {
      if (g_server.running) {
#ifdef _WIN32
        fprintf(stderr, "[MOCK] ERROR: accept() failed with error %d\n",
                WSAGetLastError());
        fflush(stderr);
#else
        perror("[MOCK] ERROR: accept() failed");
#endif
      }
      continue;
    }

    printf("[MOCK] Client connected, reading request...\n");
    fflush(stdout);

    /* Read request */
    char buffer[4096] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      printf("[MOCK] Received %d bytes, handling request\n", (int)bytes_read);
      fflush(stdout);

      /* For PUT/POST requests, consume the request body if present */
      if (strstr(buffer, "PUT ") != NULL || strstr(buffer, "POST ") != NULL) {
        char *content_length_str = strstr(buffer, "Content-Length:");
        if (content_length_str) {
          int content_length = atoi(content_length_str + 15);
          if (content_length > 0) {
            /* Check if body is already in buffer */
            char *body_start = strstr(buffer, "\r\n\r\n");
            int body_received = 0;
            if (body_start) {
              body_start += 4;
              body_received = (int)(bytes_read - (body_start - buffer));
            }

            /* Read remaining body data if needed */
            int remaining = content_length - body_received;
            if (remaining > 0) {
              char body_buffer[8192];
              while (remaining > 0) {
                ssize_t n = recv(client_fd, body_buffer,
                    remaining < (int)sizeof(body_buffer) ? remaining : (int)sizeof(body_buffer), 0);
                if (n <= 0) break;
                remaining -= n;
              }
            }
            printf("[MOCK] Consumed request body (%d bytes)\n", content_length);
            fflush(stdout);
          }
        }
      }

      handle_request(client_fd, buffer);
      printf("[MOCK] Response sent\n");
      fflush(stdout);
    } else {
      printf("[MOCK] No data received (bytes_read=%d)\n", (int)bytes_read);
      fflush(stdout);
    }

    /* Give client time to process response before closing socket */
#ifdef _WIN32
    Sleep(100); /* 100ms delay */
    closesocket(client_fd);
#else
    usleep(100000); /* 100ms delay */
    close(client_fd);
#endif
    printf("[MOCK] Client connection closed\n");
    fflush(stdout); /* Ensure output is written before thread may be terminated */
  }

  printf("[MOCK] Server thread exiting\n");
  fflush(stdout); /* Ensure output is written before thread exits */

#ifdef _WIN32
  return 0;
#else
  return NULL;
#endif
}

/* Kill any process using the specified port */
static void kill_port_process(uint16_t port) {
#ifdef _WIN32
  /* Windows: Use netstat and taskkill */
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
           "for /f \"tokens=5\" %%a in ('netstat -aon ^| findstr \":%d\" ^| findstr \"LISTENING\"') do taskkill /F /PID %%a >nul 2>&1",
           port);
  /* Best effort cleanup - ignore return value */
  int ret = system(cmd);
  (void)ret;
#else
  /* macOS/Linux: Use lsof and kill */
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "lsof -ti:%d 2>/dev/null | xargs kill -9 2>/dev/null", port);
  /* Best effort cleanup - ignore return value */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
  system(cmd);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif
  /* Give the OS time to release the port - retry up to 10 times */
  for (int i = 0; i < 10; i++) {
#ifdef _WIN32
    Sleep(100); /* 100ms */
#else
    usleep(100000); /* 100ms */
#endif
    /* Check if port is now free by attempting a quick bind test */
    socket_t test_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (!IS_SOCKET_ERROR(test_socket)) {
      int opt = 1;
      setsockopt(test_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
      struct sockaddr_in test_addr = {0};
      test_addr.sin_family = AF_INET;
      test_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      test_addr.sin_port = htons(port);
      if (bind(test_socket, (struct sockaddr *)&test_addr, sizeof(test_addr)) == 0) {
        /* Port is free */
#ifdef _WIN32
        closesocket(test_socket);
#else
        close(test_socket);
#endif
        return;
      }
#ifdef _WIN32
      closesocket(test_socket);
#else
      close(test_socket);
#endif
    }
  }
  printf("[MOCK] Warning: Port %d may still be in use after cleanup\n", port);
}

/* Start mock server */
bool mock_restreamer_start(uint16_t port) {
  printf("[MOCK] Starting mock server on port %d...\n", port);

  /* Kill any process using this port before starting */
  printf("[MOCK] Cleaning up port %d...\n", port);
  kill_port_process(port);

  /* Ensure server is not already running */
  if (g_server.running) {
    fprintf(stderr, "[MOCK] ERROR: Server already running\n");
    return false;
  }

  /* Set port for new server instance */
  g_server.port = port;

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
    fprintf(stderr, "[MOCK] ERROR: socket() failed with error %d\n",
            WSAGetLastError());
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
    fprintf(stderr, "[MOCK] ERROR: bind() failed with error %d\n",
            WSAGetLastError());
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
    fprintf(stderr, "[MOCK] ERROR: listen() failed with error %d\n",
            WSAGetLastError());
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
    fprintf(stderr, "[MOCK] ERROR: CreateThread failed with error %lu\n",
            GetLastError());
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
