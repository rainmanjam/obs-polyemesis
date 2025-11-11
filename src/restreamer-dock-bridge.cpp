#include "restreamer-dock.h"
#include "restreamer-output-profile.h"
#include "obs-bridge.h"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <QMainWindow>

extern "C" {

void *restreamer_dock_create(void) {
  QMainWindow *main_window = (QMainWindow *)obs_frontend_get_main_window();

  if (!main_window) {
    obs_log(LOG_ERROR, "Failed to get main window for dock creation");
    return nullptr;
  }

  /* Create widget (NOT QDockWidget - OBS wraps it in its own dock) */
  RestreamerDock *widget = new RestreamerDock(main_window);

  /* Set object name for dock state persistence - must match the ID used in obs_frontend_add_dock_by_id */
  widget->setObjectName("RestreamerControl");

  /* Set a reasonable minimum size */
  widget->setMinimumSize(300, 200);

  /* Register widget with OBS - OBS will wrap it in a managed QDockWidget.
   * OBS handles all docking behavior, visibility toggling, and state persistence.
   * The widget content will appear in View > Docks menu as "Restreamer Control".
   */
  obs_frontend_add_dock_by_id("RestreamerControl", "Restreamer Control", widget);

  obs_log(LOG_INFO, "Restreamer Control widget created and registered with OBS");

  return widget;
}

void restreamer_dock_destroy(void *dock) {
  if (dock) {
    RestreamerDock *dockWidget = (RestreamerDock *)dock;
    delete dockWidget;
  }
}

profile_manager_t *restreamer_dock_get_profile_manager(void *dock) {
  if (dock) {
    RestreamerDock *dockWidget = (RestreamerDock *)dock;
    return dockWidget->getProfileManager();
  }
  return nullptr;
}

restreamer_api_t *restreamer_dock_get_api_client(void *dock) {
  if (dock) {
    RestreamerDock *dockWidget = (RestreamerDock *)dock;
    return dockWidget->getApiClient();
  }
  return nullptr;
}

obs_bridge_t *restreamer_dock_get_bridge(void *dock) {
  if (dock) {
    RestreamerDock *dockWidget = (RestreamerDock *)dock;
    return dockWidget->getBridge();
  }
  return nullptr;
}

} // extern "C"
