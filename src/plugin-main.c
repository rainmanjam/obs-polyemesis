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

#include "restreamer-config.h"
#include "plugin-main.h"
#include "obs-bridge.h"
#include <obs-module.h>
#include <plugin-support.h>

#ifdef ENABLE_QT
#include <obs-frontend-api.h>
// #include "websocket-api.h" // TODO: Enable when obs-websocket-api headers are available
#endif

#include <util/platform.h>
#include <sys/stat.h>
#include <jansson.h>

// cppcheck-suppress unknownMacro
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

/* External declarations for source and output */
extern struct obs_source_info restreamer_source_info;
extern struct obs_output_info restreamer_output_info;

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for bridge */
typedef struct obs_bridge obs_bridge_t;

/* Dock widget (Qt) */
extern void *restreamer_dock_create(void);
extern void restreamer_dock_destroy(void *dock);
extern profile_manager_t *restreamer_dock_get_profile_manager(void *dock);
extern restreamer_api_t *restreamer_dock_get_api_client(void *dock);
extern obs_bridge_t *restreamer_dock_get_bridge(void *dock);

#ifdef __cplusplus
}
#endif

#ifdef ENABLE_QT
static void *dock_widget = NULL;

/* Hotkey IDs */
static obs_hotkey_id hotkey_start_all_profiles;
static obs_hotkey_id hotkey_stop_all_profiles;
static obs_hotkey_id hotkey_start_horizontal;
static obs_hotkey_id hotkey_start_vertical;

/* Hotkey callbacks */
static void hotkey_callback_start_all_profiles(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
  (void)data;
  (void)id;
  (void)hotkey;

  if (!pressed) return;

  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    profile_manager_start_all(pm);
    obs_log(LOG_INFO, "Hotkey: Started all profiles");
  }
}

static void hotkey_callback_stop_all_profiles(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
  (void)data;
  (void)id;
  (void)hotkey;

  if (!pressed) return;

  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    profile_manager_stop_all(pm);
    obs_log(LOG_INFO, "Hotkey: Stopped all profiles");
  }
}

static void hotkey_callback_start_horizontal(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
  (void)data;
  (void)id;
  (void)hotkey;

  if (!pressed) return;

  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    /* Find and start horizontal profile */
    for (size_t i = 0; i < pm->profile_count; i++) {
      if (pm->profiles[i] && strstr(pm->profiles[i]->profile_name, "Horizontal")) {
        output_profile_start(pm, pm->profiles[i]->profile_id);
        obs_log(LOG_INFO, "Hotkey: Started horizontal profile");
        break;
      }
    }
  }
}

static void hotkey_callback_start_vertical(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed) {
  (void)data;
  (void)id;
  (void)hotkey;

  if (!pressed) return;

  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    /* Find and start vertical profile */
    for (size_t i = 0; i < pm->profile_count; i++) {
      if (pm->profiles[i] && strstr(pm->profiles[i]->profile_name, "Vertical")) {
        output_profile_start(pm, pm->profiles[i]->profile_id);
        obs_log(LOG_INFO, "Hotkey: Started vertical profile");
        break;
      }
    }
  }
}

/* Tools menu callbacks */
static void tools_menu_start_all_profiles(void *data) {
  (void)data;
  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    profile_manager_start_all(pm);
    obs_log(LOG_INFO, "Tools menu: Started all profiles");
  }
}

static void tools_menu_stop_all_profiles(void *data) {
  (void)data;
  profile_manager_t *pm = plugin_get_profile_manager();
  if (pm) {
    profile_manager_stop_all(pm);
    obs_log(LOG_INFO, "Tools menu: Stopped all profiles");
  }
}

static void tools_menu_open_settings(void *data) {
  (void)data;
  /* This will be handled by the dock widget directly - just bring focus to it */
  obs_log(LOG_INFO, "Tools menu: Open settings requested");
}

/* Pre-load callback for early initialization */
static void frontend_preload_callback(obs_data_t *save_data, bool saving, void *private_data) {
  (void)save_data;
  (void)saving;
  (void)private_data;
  obs_log(LOG_INFO, "Pre-load callback: Preparing plugin state");

  /* Initialize any state that needs to be ready before scene collection loads */
  /* This is called before OBS loads scene collections */
}

static void frontend_event_callback(enum obs_frontend_event event, void *private_data) {
  (void)private_data;  /* Unused */

  if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
    dock_widget = restreamer_dock_create();
    obs_log(LOG_INFO, "Restreamer dock created");

    /* Register hotkeys */
    hotkey_start_all_profiles = obs_hotkey_register_frontend(
      "obs_polyemesis.start_all_profiles",
      "Polyemesis: Start All Profiles",
      hotkey_callback_start_all_profiles,
      NULL
    );

    hotkey_stop_all_profiles = obs_hotkey_register_frontend(
      "obs_polyemesis.stop_all_profiles",
      "Polyemesis: Stop All Profiles",
      hotkey_callback_stop_all_profiles,
      NULL
    );

    hotkey_start_horizontal = obs_hotkey_register_frontend(
      "obs_polyemesis.start_horizontal",
      "Polyemesis: Start Horizontal Profile",
      hotkey_callback_start_horizontal,
      NULL
    );

    hotkey_start_vertical = obs_hotkey_register_frontend(
      "obs_polyemesis.start_vertical",
      "Polyemesis: Start Vertical Profile",
      hotkey_callback_start_vertical,
      NULL
    );

    obs_log(LOG_INFO, "Registered Polyemesis hotkeys");

    /* Add tools menu items */
    obs_frontend_add_tools_menu_item("Polyemesis: Start All Profiles", tools_menu_start_all_profiles, NULL);
    obs_frontend_add_tools_menu_item("Polyemesis: Stop All Profiles", tools_menu_stop_all_profiles, NULL);
    obs_frontend_add_tools_menu_item("Polyemesis: Open Settings", tools_menu_open_settings, NULL);
    obs_log(LOG_INFO, "Added Polyemesis tools menu items");

    /* TODO: Initialize WebSocket API after dock is ready (requires obs-websocket-api headers) */
    /* if (websocket_api_init()) {
      obs_log(LOG_INFO, "WebSocket Vendor API initialized");
    } else {
      obs_log(LOG_WARNING, "WebSocket Vendor API initialization failed (obs-websocket may not be available)");
    } */
  }

  /* Forward all frontend events to the bridge for auto-start functionality */
  if (dock_widget) {
    obs_bridge_t *bridge = restreamer_dock_get_bridge(dock_widget);
    if (bridge) {
      obs_bridge_handle_frontend_event(bridge, event);
    }
  }
}
#endif

/* Global accessor functions */
profile_manager_t *plugin_get_profile_manager(void) {
#ifdef ENABLE_QT
  if (dock_widget) {
    return restreamer_dock_get_profile_manager(dock_widget);
  }
#endif
  return NULL;
}

restreamer_api_t *plugin_get_api_client(void) {
#ifdef ENABLE_QT
  if (dock_widget) {
    return restreamer_dock_get_api_client(dock_widget);
  }
#endif
  return NULL;
}

void plugin_set_dock_widget(void *dock) {
#ifdef ENABLE_QT
  dock_widget = dock;
#else
  (void)dock;
#endif
}

/* Install Polyemesis service definitions for OBS Stream settings */
static void install_service_definition(void) {
  /* Get OBS config path */
  char *config_path = obs_module_get_config_path(obs_current_module(), "");
  if (!config_path) {
    obs_log(LOG_WARNING, "Failed to get config path for service definition");
    return;
  }

  /* Navigate to rtmp-services directory */
  char service_dir[1024];
  char service_file[1024];

  /* Navigate from plugin_config/obs-polyemesis to plugin_config/rtmp-services */
  /* First, remove any trailing slash */
  size_t len = strlen(config_path);
  if (len > 0 && config_path[len - 1] == '/') {
    config_path[len - 1] = '\0';
  }

  /* Go up one level: plugin_config/obs-polyemesis -> plugin_config */
  char *last_sep = strrchr(config_path, '/');
  if (!last_sep) {
    bfree(config_path);
    return;
  }
  *last_sep = '\0';  /* Remove /obs-polyemesis */

  snprintf(service_dir, sizeof(service_dir), "%s/rtmp-services", config_path);
  snprintf(service_file, sizeof(service_file), "%s/services.json", service_dir);

  /* Create directory if it doesn't exist */
  os_mkdirs(service_dir);

  obs_log(LOG_INFO, "Installing Polyemesis services to: %s", service_file);

  /* Load existing services.json */
  json_t *root = json_load_file(service_file, 0, NULL);
  if (!root) {
    obs_log(LOG_WARNING, "Failed to load existing services.json, cannot add Polyemesis services");
    bfree(config_path);
    return;
  }

  json_t *services_array = json_object_get(root, "services");
  if (!services_array || !json_is_array(services_array)) {
    obs_log(LOG_WARNING, "Invalid services.json format");
    json_decref(root);
    bfree(config_path);
    return;
  }

  /* Check if Polyemesis services already exist */
  bool has_horizontal = false;
  bool has_vertical = false;
  size_t index;
  json_t *value;
  json_array_foreach(services_array, index, value) {
    json_t *name = json_object_get(value, "name");
    if (name && json_is_string(name)) {
      const char *name_str = json_string_value(name);
      if (strcmp(name_str, "Polyemesis Horizontal") == 0) {
        has_horizontal = true;
      } else if (strcmp(name_str, "Polyemesis Vertical") == 0) {
        has_vertical = true;
      }
    }
  }

  /* Add Polyemesis Horizontal service if it doesn't exist */
  if (!has_horizontal) {
    json_t *horizontal_service = json_pack(
      "{"
      "s:s,"  // name
      "s:b,"  // common
      "s:s,"  // key (default stream key)
      "s:[{s:s, s:s}, {s:s, s:s}],"  // servers
      "s:[s],"  // supported video codecs
      "s:{s:i, s:s, s:i, s:i, s:s, s:i}"  // recommended
      "}",
      "name", "Polyemesis Horizontal",
      "common", 1,
      "key", "obs_horizontal",
      "servers",
        "name", "Local Restreamer",
        "url", "rtmp://localhost/live",
        "name", "Custom Server",
        "url", "rtmp://your-server/live",
      "supported video codecs", "h264",
      "recommended",
        "keyint", 2,
        "output", "rtmp_output",
        "max audio bitrate", 160,
        "max video bitrate", 6000,
        "profile", "main",
        "bframes", 2
    );
    if (horizontal_service) {
      json_array_append_new(services_array, horizontal_service);
      obs_log(LOG_INFO, "Added Polyemesis Horizontal service");
    }
  }

  /* Add Polyemesis Vertical service if it doesn't exist */
  if (!has_vertical) {
    json_t *vertical_service = json_pack(
      "{"
      "s:s,"  // name
      "s:b,"  // common
      "s:s,"  // key (default stream key)
      "s:[{s:s, s:s}, {s:s, s:s}],"  // servers
      "s:[s],"  // supported video codecs
      "s:{s:i, s:s, s:i, s:i, s:s, s:i}"  // recommended
      "}",
      "name", "Polyemesis Vertical",
      "common", 1,
      "key", "obs_vertical",
      "servers",
        "name", "Local Restreamer",
        "url", "rtmp://localhost/live",
        "name", "Custom Server",
        "url", "rtmp://your-server/live",
      "supported video codecs", "h264",
      "recommended",
        "keyint", 2,
        "output", "rtmp_output",
        "max audio bitrate", 160,
        "max video bitrate", 6000,
        "profile", "main",
        "bframes", 2
    );
    if (vertical_service) {
      json_array_append_new(services_array, vertical_service);
      obs_log(LOG_INFO, "Added Polyemesis Vertical service");
    }
  }

  /* Save updated services.json */
  if (!has_horizontal || !has_vertical) {
    if (json_dump_file(root, service_file, JSON_INDENT(2)) == 0) {
      obs_log(LOG_INFO, "Successfully updated services.json with Polyemesis services");
    } else {
      obs_log(LOG_WARNING, "Failed to save updated services.json");
    }
  } else {
    obs_log(LOG_INFO, "Polyemesis services already installed");
  }

  json_decref(root);
  bfree(config_path);
}

bool obs_module_load(void) {
  obs_log(LOG_INFO, "obs-polyemesis plugin loaded (version %s)",
          PLUGIN_VERSION);

  /* Initialize configuration system */
  restreamer_config_init();

  /* Install service definition for OBS Stream settings */
  install_service_definition();

  /* Register source */
  obs_register_source(&restreamer_source_info);
  obs_log(LOG_INFO, "Registered restreamer source");

  /* Register output */
  obs_register_output(&restreamer_output_info);
  obs_log(LOG_INFO, "Registered restreamer output");

#ifdef ENABLE_QT
  /* Register pre-load callback for early initialization */
  obs_frontend_add_preload_callback(frontend_preload_callback, NULL);

  /* Create and register dock widget */
  obs_frontend_add_event_callback(frontend_event_callback, NULL);
#endif

  obs_log(LOG_INFO, "obs-polyemesis initialized successfully");
  obs_log(LOG_INFO, "Features: Source, Output, Multistreaming, Dock UI");

  return true;
}

void obs_module_unload(void) {
  obs_log(LOG_INFO, "Unloading obs-polyemesis plugin");

#ifdef ENABLE_QT
  /* TODO: Shutdown WebSocket API first (requires obs-websocket-api headers) */
  /* websocket_api_shutdown(); */

  /* Remove event callback to prevent callbacks on unloaded module */
  obs_frontend_remove_event_callback(frontend_event_callback, NULL);

  if (dock_widget) {
    /* OBS owns and will destroy the dock widget via obs_frontend_add_dock_by_id */
    dock_widget = NULL;
  }
#endif

  /* Cleanup config */
  restreamer_config_destroy();

  obs_log(LOG_INFO, "obs-polyemesis unloaded");
}

const char *obs_module_description(void) {
  return "Remote control and monitoring for Restreamer with multistreaming "
         "support";
}

const char *obs_module_name(void) {
  return "OBS Polyemesis - Restreamer Control";
}
