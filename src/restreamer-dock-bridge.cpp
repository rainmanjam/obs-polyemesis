#include "restreamer-dock.h"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

extern "C" {

void *restreamer_dock_create(void)
{
	QMainWindow *main_window =
		(QMainWindow *)obs_frontend_get_main_window();

	if (!main_window) {
		obs_log(LOG_ERROR,
			"Failed to get main window for dock creation");
		return nullptr;
	}

	RestreamerDock *dock = new RestreamerDock();
	obs_frontend_add_dock_by_id("RestreamerControl", "Restreamer Control", dock);

	return dock;
}

void restreamer_dock_destroy(void *dock)
{
	if (dock) {
		RestreamerDock *dockWidget = (RestreamerDock *)dock;
		delete dockWidget;
	}
}

} // extern "C"
