#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.hpp> // Use C++ wrappers

#include <QObject> // Required for Qt integration
#include <QMainWindow>   // For main window access
#include <QDockWidget>   // For creating dockable UI
#include <QVBoxLayout>   // For layout
#include <QWidget>       // For main widget in dock
#include <QLabel>        // For placeholder labels
#include <QTabWidget>    // To organize sections
#include <QPushButton>   // For buttons
#include <QLineEdit>     // For text input
#include <QHBoxLayout>   // For horizontal layout
#include <QVBoxLayout>   // Added missing include

#include "restreamer-api-client.hpp" // Include the API client header

// Forward declaration for our main UI widget
class PolyemesisDock;
static PolyemesisDock *main_dock = nullptr;
static RestreamerApiClient *api_client = nullptr; // Add static pointer for API client

// --- Plugin Definition ---

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("polyemesis", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Integrates Datarhei/Restreamer and manages multiple streaming platforms.";
}

// --- Module Load/Unload ---

// Function to create the dockable widget (implementation later)
void CreatePolyemesisDock();

bool obs_module_load(void)
{
	obs_frontend_add_event_callback([](obs_frontend_event event, void *private_data) {
		if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
			// Create the dockable widget after the frontend is loaded
			CreatePolyemesisDock();
		}
	}, nullptr);

	// Initialize the API client
	api_client = new RestreamerApiClient();
	// TODO: Set Base URL and API Key from settings later
	// api_client->setBaseUrl(QUrl("http://your-restreamer-instance"));
	// api_client->setApiKey("your-api-key");

	return true;
}

void obs_module_unload(void)
{
	// Cleanup API client
	if (api_client) {
		delete api_client; // Delete the client instance
		api_client = nullptr;
	}

	// Cleanup UI
	if (main_dock) {
		// Dock widgets are owned by Qt's main window,
		// we just need to make sure ours is hidden and potentially deleted later.
		main_dock->setVisible(false);
		// obs_frontend_remove_dock(main_dock); // This might be needed depending on how it's added
	}
}

// --- UI Creation ---

// Placeholder for the main dock widget class
class PolyemesisDock : public QDockWidget {
	Q_OBJECT // Required for signals/slots

public:
	PolyemesisDock(QWidget *parent = nullptr) : QDockWidget(parent)
	{
		setObjectName("PolyemesisDock");
		setWindowTitle(obs_module_text("Polyemesis")); // Use localized string

		// Create main widget and layout
		QWidget *main_widget = new QWidget();
		QVBoxLayout *main_layout = new QVBoxLayout();
		main_widget->setLayout(main_layout);

		// Create a tab widget to organize sections
		QTabWidget *tab_widget = new QTabWidget();
		main_layout->addWidget(tab_widget);

		// --- Create Controls Tab ---
		QWidget *controls_widget = new QWidget();
		QVBoxLayout *controls_layout = new QVBoxLayout();
		controls_widget->setLayout(controls_layout);

		// Process ID Input
		QHBoxLayout *process_id_layout = new QHBoxLayout();
		process_id_edit = new QLineEdit(); // Make process_id_edit a member variable
		process_id_edit->setPlaceholderText("Enter Process ID (e.g., 'rtmp-youtube')");
		process_id_layout->addWidget(new QLabel("Process ID:"));
		process_id_layout->addWidget(process_id_edit);
		controls_layout->addLayout(process_id_layout);

		// Start/Stop Buttons
		QHBoxLayout *button_layout = new QHBoxLayout();
		QPushButton *start_button = new QPushButton("Start Process");
		QPushButton *stop_button = new QPushButton("Stop Process");
		button_layout->addWidget(start_button);
		button_layout->addWidget(stop_button);
		button_layout->addStretch(); // Push buttons to the left
		controls_layout->addLayout(button_layout);
		controls_layout->addStretch(); // Push controls to the top

		tab_widget->addTab(controls_widget, "Controls");

		// --- Add other placeholder tabs ---
		tab_widget->addTab(new QLabel("TODO: Stream Analytics (Bitrate, Dropped Frames, etc.)"), "Analytics");
		tab_widget->addTab(new QLabel("TODO: Metadata Management (Title, Description, Tags)"), "Metadata");
		tab_widget->addTab(new QLabel("TODO: Platform Authentication & Status"), "Platforms");

		// Set the main widget for the dock
		setWidget(main_widget);

		// Connect button signals
		connect(start_button, &QPushButton::clicked, this, &PolyemesisDock::onStartClicked);
		connect(stop_button, &QPushButton::clicked, this, &PolyemesisDock::onStopClicked);

		// Make dock closable and movable
		setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	}

	// Optional: Override close event if needed
	// void closeEvent(QCloseEvent *event) override {
	// 	obs_module_unload(); // Or specific cleanup
	// 	QDockWidget::closeEvent(event);
	// }

private:
	QLineEdit *process_id_edit; // Member variable for process ID input

public slots:
	// Slot to handle Start button click
	void onStartClicked() {
		QString processId = process_id_edit->text().trimmed();
		if (!processId.isEmpty() && api_client) {
			blog(LOG_INFO, "[Polyemesis UI] Start button clicked for process: %s", processId.toStdString().c_str());
			api_client->startProcess(processId);
		} else if (processId.isEmpty()) {
			blog(LOG_WARNING, "[Polyemesis UI] Start button clicked: Process ID is empty.");
			// Optionally show a message to the user
		}
	}

	// Slot to handle Stop button click
	void onStopClicked() {
		QString processId = process_id_edit->text().trimmed();
		if (!processId.isEmpty() && api_client) {
			blog(LOG_INFO, "[Polyemesis UI] Stop button clicked for process: %s", processId.toStdString().c_str());
			api_client->stopProcess(processId);
		} else if (processId.isEmpty()) {
			blog(LOG_WARNING, "[Polyemesis UI] Stop button clicked: Process ID is empty.");
			// Optionally show a message to the user
		}
	}

	// Example slot to handle received status
	void handleStatusUpdate(const QByteArray &data) {
		// TODO: Parse data (e.g., JSON) and update UI elements
		blog(LOG_INFO, "[Polyemesis UI] Received status update.");
		// Example: Find a label in the Analytics tab and update it
		// QLabel *statusLabel = findChild<QLabel*>("statusLabel");
		// if (statusLabel) {
		//     QJsonDocument doc = QJsonDocument::fromJson(data);
		//     statusLabel->setText(doc.toJson(QJsonDocument::Indented));
		// }
	}

	// Example slot to handle errors
	void handleError(const QString &errorString) {
		// TODO: Display error to the user (e.g., in a status bar or message box)
		blog(LOG_WARNING, "[Polyemesis UI] API Error: %s", errorString.toStdString().c_str());
		// Example: Update a status label
		// QLabel *errorLabel = findChild<QLabel*>("errorLabel");
		// if (errorLabel) {
		//     errorLabel->setText("Error: " + errorString);
		// }
	}
};

// Function to create and register the dock
void CreatePolyemesisDock()
{
	if (main_dock) {
		// Already created
		return;
	}

	QMainWindow *main_window = (QMainWindow *)obs_frontend_get_main_window();
	if (!main_window) {
		blog(LOG_WARNING, "[Polyemesis] Could not get main window to create dock.");
		return;
	}

	main_dock = new PolyemesisDock(main_window);

	// Connect API client signals to UI slots
	if (api_client) {
		QObject::connect(api_client, &RestreamerApiClient::statusReceived,
				 main_dock, &PolyemesisDock::handleStatusUpdate);
		QObject::connect(api_client, &RestreamerApiClient::errorOccurred,
				 main_dock, &PolyemesisDock::handleError);

		// Example: Trigger an initial status fetch
		// api_client->getStatus();
	}

	// Register the dock with OBS frontend
	obs_frontend_add_dock(main_dock);

	// Optional: Restore dock state if saved previously
	// obs_frontend_add_save_callback(save_dock_state, nullptr);
	// obs_frontend_add_load_callback(load_dock_state, nullptr);
}

// Required for AUTOMOC
#include "plugin-main.moc"
