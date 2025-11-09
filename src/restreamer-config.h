#pragma once

#include "restreamer-api.h"
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global configuration management */

/* Initialize global config */
void restreamer_config_init(void);

/* Get global connection settings */
restreamer_connection_t *restreamer_config_get_global_connection(void);

/* Set global connection settings */
void restreamer_config_set_global_connection(
    restreamer_connection_t *connection);

/* Create API instance from global settings */
restreamer_api_t *restreamer_config_create_global_api(void);

/* Load settings from OBS config */
void restreamer_config_load(obs_data_t *settings);

/* Save settings to OBS config */
void restreamer_config_save(obs_data_t *settings);

/* Get default properties for settings dialog */
obs_properties_t *restreamer_config_get_properties(void);

/* Clean up global config */
void restreamer_config_destroy(void);

/* Per-source configuration helpers */

/* Load connection from source settings */
restreamer_connection_t *
restreamer_config_load_from_settings(obs_data_t *settings);

/* Save connection to source settings */
void restreamer_config_save_to_settings(obs_data_t *settings,
                                        restreamer_connection_t *connection);

/* Free connection structure */
void restreamer_config_free_connection(restreamer_connection_t *connection);

#ifdef __cplusplus
}
#endif
