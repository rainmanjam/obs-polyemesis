#include "restreamer-dock.h"
#include <obs-frontend-api.h>
#include <obs-module.h>

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

	RestreamerDock *dock = new RestreamerDock(main_window);
	obs_frontend_add_dock(dock);

	return dock;
}

void restreamer_dock_destroy(void *dock)
{
	if (dock) {
		RestreamerDock *dockWidget = (RestreamerDock *)dock;
		obs_frontend_remove_dock(dockWidget);
		delete dockWidget;
	}
}

} // extern "C"
