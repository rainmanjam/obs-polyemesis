#pragma once

#include "restreamer-api.h"
#include "restreamer-output-profile.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the global profile manager instance
 * @return Profile manager pointer, or NULL if not initialized
 */
profile_manager_t *plugin_get_profile_manager(void);

/**
 * @brief Get the global API client instance
 * @return API client pointer, or NULL if not initialized
 */
restreamer_api_t *plugin_get_api_client(void);

/**
 * @brief Set the global dock widget instance (internal use)
 * @param dock Pointer to dock widget
 */
void plugin_set_dock_widget(void *dock);

#ifdef __cplusplus
}
#endif
