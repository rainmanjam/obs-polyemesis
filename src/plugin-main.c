/*
OBS Polyemesis - Restreamer Control Plugin
Copyright (C) 2024

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include "restreamer-config.h"

#ifdef ENABLE_QT
#include <obs-frontend-api.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

/* External declarations for source and output */
extern struct obs_source_info restreamer_source_info;
extern struct obs_output_info restreamer_output_info;

#ifdef __cplusplus
extern "C" {
#endif

/* Dock widget (Qt) */
extern void *restreamer_dock_create(void);
extern void restreamer_dock_destroy(void *dock);

#ifdef __cplusplus
}
#endif

#ifdef ENABLE_QT
static void *dock_widget = NULL;
#endif

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "obs-polyemesis plugin loaded (version %s)", PLUGIN_VERSION);

	/* Initialize configuration system */
	restreamer_config_init();

	/* Register source */
	obs_register_source(&restreamer_source_info);
	obs_log(LOG_INFO, "Registered restreamer source");

	/* Register output */
	obs_register_output(&restreamer_output_info);
	obs_log(LOG_INFO, "Registered restreamer output");

#ifdef ENABLE_QT
	/* Create and register dock widget */
	obs_frontend_add_event_callback(
		[](enum obs_frontend_event event, void *private_data) {
			if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
				dock_widget = restreamer_dock_create();
				obs_log(LOG_INFO, "Restreamer dock created");
			}
		},
		NULL);
#endif

	obs_log(LOG_INFO, "obs-polyemesis initialized successfully");
	obs_log(LOG_INFO, "Features: Source, Output, Multistreaming, Dock UI");

	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "Unloading obs-polyemesis plugin");

#ifdef ENABLE_QT
	if (dock_widget) {
		restreamer_dock_destroy(dock_widget);
		dock_widget = NULL;
	}
#endif

	/* Cleanup config */
	restreamer_config_destroy();

	obs_log(LOG_INFO, "obs-polyemesis unloaded");
}

const char *obs_module_description(void)
{
	return "Remote control and monitoring for Restreamer with multistreaming support";
}

const char *obs_module_name(void)
{
	return "OBS Polyemesis - Restreamer Control";
}
