/*
 * Mock Restreamer Server Header
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start mock Restreamer server on specified port */
bool mock_restreamer_start(uint16_t port);

/* Stop mock server */
void mock_restreamer_stop(void);

/* Get the port the server is running on */
uint16_t mock_restreamer_get_port(void);

#ifdef __cplusplus
}
#endif
