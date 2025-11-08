#include "restreamer-dock.h"
#include "restreamer-config.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <obs-module.h>
#include <obs-frontend-api.h>

RestreamerDock::RestreamerDock(QWidget *parent)
	: QDockWidget("Restreamer Control", parent),
	  api(nullptr),
	  multistreamConfig(nullptr),
	  selectedProcessId(nullptr)
{
	setupUI();
	loadSettings();

	/* Create update timer for auto-refresh */
	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this,
		&RestreamerDock::onUpdateTimer);
	updateTimer->start(5000); /* Update every 5 seconds */

	/* Initial refresh */
	onRefreshClicked();
}

RestreamerDock::~RestreamerDock()
{
	saveSettings();

	if (api) {
		restreamer_api_destroy(api);
	}

	if (multistreamConfig) {
		restreamer_multistream_destroy(multistreamConfig);
	}

	bfree(selectedProcessId);
}

void RestreamerDock::setupUI()
{
	QWidget *mainWidget = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

	/* Connection Settings Group */
	QGroupBox *connectionGroup = new QGroupBox("Connection Settings");
	QFormLayout *connectionLayout = new QFormLayout();

	hostEdit = new QLineEdit();
	portEdit = new QLineEdit();
	httpsCheckbox = new QCheckBox();
	usernameEdit = new QLineEdit();
	passwordEdit = new QLineEdit();
	passwordEdit->setEchoMode(QLineEdit::Password);

	connectionLayout->addRow("Host:", hostEdit);
	connectionLayout->addRow("Port:", portEdit);
	connectionLayout->addRow("Use HTTPS:", httpsCheckbox);
	connectionLayout->addRow("Username:", usernameEdit);
	connectionLayout->addRow("Password:", passwordEdit);

	QHBoxLayout *connectionButtonLayout = new QHBoxLayout();
	testConnectionButton = new QPushButton("Test Connection");
	connectionStatusLabel = new QLabel("Not connected");
	connectionButtonLayout->addWidget(testConnectionButton);
	connectionButtonLayout->addWidget(connectionStatusLabel);

	connect(testConnectionButton, &QPushButton::clicked, this,
		&RestreamerDock::onTestConnectionClicked);

	connectionLayout->addRow(connectionButtonLayout);
	connectionGroup->setLayout(connectionLayout);
	mainLayout->addWidget(connectionGroup);

	/* Process List Group */
	QGroupBox *processGroup = new QGroupBox("Processes");
	QVBoxLayout *processLayout = new QVBoxLayout();

	processList = new QListWidget();
	connect(processList, &QListWidget::currentRowChanged, this,
		&RestreamerDock::onProcessSelected);

	QHBoxLayout *processButtonLayout = new QHBoxLayout();
	refreshButton = new QPushButton("Refresh");
	startButton = new QPushButton("Start");
	stopButton = new QPushButton("Stop");
	restartButton = new QPushButton("Restart");

	startButton->setEnabled(false);
	stopButton->setEnabled(false);
	restartButton->setEnabled(false);

	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerDock::onRefreshClicked);
	connect(startButton, &QPushButton::clicked, this,
		&RestreamerDock::onStartProcessClicked);
	connect(stopButton, &QPushButton::clicked, this,
		&RestreamerDock::onStopProcessClicked);
	connect(restartButton, &QPushButton::clicked, this,
		&RestreamerDock::onRestartProcessClicked);

	processButtonLayout->addWidget(refreshButton);
	processButtonLayout->addWidget(startButton);
	processButtonLayout->addWidget(stopButton);
	processButtonLayout->addWidget(restartButton);

	processLayout->addWidget(processList);
	processLayout->addLayout(processButtonLayout);
	processGroup->setLayout(processLayout);
	mainLayout->addWidget(processGroup);

	/* Process Details Group */
	QGroupBox *detailsGroup = new QGroupBox("Process Details");
	QFormLayout *detailsLayout = new QFormLayout();

	processIdLabel = new QLabel("-");
	processStateLabel = new QLabel("-");
	processUptimeLabel = new QLabel("-");
	processCpuLabel = new QLabel("-");
	processMemoryLabel = new QLabel("-");

	detailsLayout->addRow("ID:", processIdLabel);
	detailsLayout->addRow("State:", processStateLabel);
	detailsLayout->addRow("Uptime:", processUptimeLabel);
	detailsLayout->addRow("CPU:", processCpuLabel);
	detailsLayout->addRow("Memory:", processMemoryLabel);

	detailsGroup->setLayout(detailsLayout);
	mainLayout->addWidget(detailsGroup);

	/* Sessions Group */
	QGroupBox *sessionsGroup = new QGroupBox("Active Sessions");
	QVBoxLayout *sessionsLayout = new QVBoxLayout();

	sessionTable = new QTableWidget();
	sessionTable->setColumnCount(3);
	sessionTable->setHorizontalHeaderLabels(
		{"Session ID", "Remote Address", "Bytes Sent"});
	sessionTable->horizontalHeader()->setStretchLastSection(true);

	sessionsLayout->addWidget(sessionTable);
	sessionsGroup->setLayout(sessionsLayout);
	mainLayout->addWidget(sessionsGroup);

	/* Multistream Configuration Group */
	QGroupBox *multistreamGroup =
		new QGroupBox("Multistream Configuration");
	QVBoxLayout *multistreamLayout = new QVBoxLayout();

	QFormLayout *orientationLayout = new QFormLayout();
	autoDetectOrientationCheck =
		new QCheckBox("Auto-detect orientation");
	autoDetectOrientationCheck->setChecked(true);

	orientationCombo = new QComboBox();
	orientationCombo->addItem("Horizontal (Landscape)",
				  ORIENTATION_HORIZONTAL);
	orientationCombo->addItem("Vertical (Portrait)", ORIENTATION_VERTICAL);
	orientationCombo->addItem("Square", ORIENTATION_SQUARE);

	orientationLayout->addRow(autoDetectOrientationCheck);
	orientationLayout->addRow("Orientation:", orientationCombo);
	multistreamLayout->addLayout(orientationLayout);

	/* Destinations table */
	destinationsTable = new QTableWidget();
	destinationsTable->setColumnCount(4);
	destinationsTable->setHorizontalHeaderLabels(
		{"Service", "Stream Key", "Orientation", "Enabled"});
	destinationsTable->horizontalHeader()->setStretchLastSection(true);

	QHBoxLayout *destButtonLayout = new QHBoxLayout();
	addDestinationButton = new QPushButton("Add Destination");
	removeDestinationButton = new QPushButton("Remove Selected");
	createMultistreamButton = new QPushButton("Start Multistream");

	connect(addDestinationButton, &QPushButton::clicked, this,
		&RestreamerDock::onAddDestinationClicked);
	connect(removeDestinationButton, &QPushButton::clicked, this,
		&RestreamerDock::onRemoveDestinationClicked);
	connect(createMultistreamButton, &QPushButton::clicked, this,
		&RestreamerDock::onCreateMultistreamClicked);

	destButtonLayout->addWidget(addDestinationButton);
	destButtonLayout->addWidget(removeDestinationButton);
	destButtonLayout->addWidget(createMultistreamButton);

	multistreamLayout->addWidget(destinationsTable);
	multistreamLayout->addLayout(destButtonLayout);
	multistreamGroup->setLayout(multistreamLayout);
	mainLayout->addWidget(multistreamGroup);

	mainLayout->addStretch();

	setWidget(mainWidget);
	setMinimumWidth(400);

	multistreamConfig = restreamer_multistream_create();
}

void RestreamerDock::loadSettings()
{
	obs_data_t *settings = obs_data_create_from_json_file_safe(
		obs_module_config_path("config.json"), "bak");

	if (!settings) {
		settings = obs_data_create();
	}

	hostEdit->setText(obs_data_get_string(settings, "host"));
	portEdit->setText(
		QString::number(obs_data_get_int(settings, "port")));
	httpsCheckbox->setChecked(obs_data_get_bool(settings, "use_https"));
	usernameEdit->setText(obs_data_get_string(settings, "username"));
	passwordEdit->setText(obs_data_get_string(settings, "password"));

	/* Load global config */
	restreamer_config_load(settings);

	/* Load multistream config */
	if (multistreamConfig) {
		restreamer_multistream_load_from_settings(multistreamConfig,
							  settings);
		updateDestinationList();
	}

	obs_data_release(settings);

	/* Set defaults if empty */
	if (hostEdit->text().isEmpty()) {
		hostEdit->setText("localhost");
	}
	if (portEdit->text().isEmpty()) {
		portEdit->setText("8080");
	}
}

void RestreamerDock::saveSettings()
{
	obs_data_t *settings = obs_data_create();

	obs_data_set_string(settings, "host", hostEdit->text().toUtf8().constData());
	obs_data_set_int(settings, "port", portEdit->text().toInt());
	obs_data_set_bool(settings, "use_https", httpsCheckbox->isChecked());
	obs_data_set_string(settings, "username",
			    usernameEdit->text().toUtf8().constData());
	obs_data_set_string(settings, "password",
			    passwordEdit->text().toUtf8().constData());

	/* Save multistream config */
	if (multistreamConfig) {
		restreamer_multistream_save_to_settings(multistreamConfig,
							settings);
	}

	obs_data_save_json_safe(settings,
				 obs_module_config_path("config.json"), "tmp",
				 "bak");
	obs_data_release(settings);

	/* Update global config */
	restreamer_connection_t connection = {0};
	connection.host = bstrdup(hostEdit->text().toUtf8().constData());
	connection.port = (uint16_t)portEdit->text().toInt();
	connection.use_https = httpsCheckbox->isChecked();
	if (!usernameEdit->text().isEmpty()) {
		connection.username =
			bstrdup(usernameEdit->text().toUtf8().constData());
	}
	if (!passwordEdit->text().isEmpty()) {
		connection.password =
			bstrdup(passwordEdit->text().toUtf8().constData());
	}

	restreamer_config_set_global_connection(&connection);

	bfree(connection.host);
	bfree(connection.username);
	bfree(connection.password);
}

void RestreamerDock::onTestConnectionClicked()
{
	saveSettings();

	if (api) {
		restreamer_api_destroy(api);
	}

	api = restreamer_config_create_global_api();

	if (!api) {
		connectionStatusLabel->setText("Failed to create API");
		connectionStatusLabel->setStyleSheet("color: red;");
		return;
	}

	if (restreamer_api_test_connection(api)) {
		connectionStatusLabel->setText("Connected");
		connectionStatusLabel->setStyleSheet("color: green;");
		onRefreshClicked();
	} else {
		connectionStatusLabel->setText("Connection failed");
		connectionStatusLabel->setStyleSheet("color: red;");
		QMessageBox::warning(
			this, "Connection Error",
			QString("Failed to connect: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerDock::onRefreshClicked()
{
	updateProcessList();
	updateSessionList();
}

void RestreamerDock::updateProcessList()
{
	processList->clear();

	if (!api) {
		return;
	}

	restreamer_process_list_t list = {0};
	if (!restreamer_api_get_processes(api, &list)) {
		return;
	}

	for (size_t i = 0; i < list.count; i++) {
		const char *name = list.processes[i].reference
					   ? list.processes[i].reference
					   : list.processes[i].id;

		QString displayText = QString("%1 [%2]")
					      .arg(name)
					      .arg(list.processes[i].state);

		QListWidgetItem *item = new QListWidgetItem(displayText);
		item->setData(Qt::UserRole, QString(list.processes[i].id));
		processList->addItem(item);
	}

	restreamer_api_free_process_list(&list);
}

void RestreamerDock::onProcessSelected()
{
	QListWidgetItem *item = processList->currentItem();
	if (!item) {
		startButton->setEnabled(false);
		stopButton->setEnabled(false);
		restartButton->setEnabled(false);
		return;
	}

	bfree(selectedProcessId);
	selectedProcessId = bstrdup(item->data(Qt::UserRole).toString().toUtf8().constData());

	startButton->setEnabled(true);
	stopButton->setEnabled(true);
	restartButton->setEnabled(true);

	updateProcessDetails();
}

void RestreamerDock::updateProcessDetails()
{
	if (!api || !selectedProcessId) {
		return;
	}

	restreamer_process_t process = {0};
	if (!restreamer_api_get_process(api, selectedProcessId, &process)) {
		return;
	}

	processIdLabel->setText(process.id ? process.id : "-");
	processStateLabel->setText(process.state ? process.state : "-");

	/* Format uptime */
	uint64_t hours = process.uptime_seconds / 3600;
	uint64_t minutes = (process.uptime_seconds % 3600) / 60;
	uint64_t seconds = process.uptime_seconds % 60;
	processUptimeLabel->setText(
		QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds));

	processCpuLabel->setText(QString("%1%").arg(process.cpu_usage, 0, 'f', 1));
	processMemoryLabel->setText(
		QString("%1 MB").arg(process.memory_bytes / 1024 / 1024));

	restreamer_api_free_process(&process);
}

void RestreamerDock::updateSessionList()
{
	sessionTable->setRowCount(0);

	if (!api) {
		return;
	}

	restreamer_session_list_t sessions = {0};
	if (!restreamer_api_get_sessions(api, &sessions)) {
		return;
	}

	sessionTable->setRowCount(sessions.count);

	for (size_t i = 0; i < sessions.count; i++) {
		sessionTable->setItem(
			i, 0,
			new QTableWidgetItem(sessions.sessions[i].session_id));
		sessionTable->setItem(
			i, 1,
			new QTableWidgetItem(
				sessions.sessions[i].remote_addr));
		sessionTable->setItem(
			i, 2,
			new QTableWidgetItem(QString::number(
				sessions.sessions[i].bytes_sent / 1024 /
				1024)));
	}

	restreamer_api_free_session_list(&sessions);
}

void RestreamerDock::onStartProcessClicked()
{
	if (!api || !selectedProcessId) {
		return;
	}

	if (restreamer_api_start_process(api, selectedProcessId)) {
		QMessageBox::information(this, "Success", "Process started");
		onRefreshClicked();
	} else {
		QMessageBox::warning(
			this, "Error",
			QString("Failed to start process: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerDock::onStopProcessClicked()
{
	if (!api || !selectedProcessId) {
		return;
	}

	if (restreamer_api_stop_process(api, selectedProcessId)) {
		QMessageBox::information(this, "Success", "Process stopped");
		onRefreshClicked();
	} else {
		QMessageBox::warning(
			this, "Error",
			QString("Failed to stop process: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerDock::onRestartProcessClicked()
{
	if (!api || !selectedProcessId) {
		return;
	}

	if (restreamer_api_restart_process(api, selectedProcessId)) {
		QMessageBox::information(this, "Success", "Process restarted");
		onRefreshClicked();
	} else {
		QMessageBox::warning(
			this, "Error",
			QString("Failed to restart process: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerDock::updateDestinationList()
{
	if (!multistreamConfig) {
		return;
	}

	destinationsTable->setRowCount(multistreamConfig->destination_count);

	for (size_t i = 0; i < multistreamConfig->destination_count; i++) {
		stream_destination_t *dest = &multistreamConfig->destinations[i];

		destinationsTable->setItem(
			i, 0, new QTableWidgetItem(dest->service_name));
		destinationsTable->setItem(i, 1,
					   new QTableWidgetItem(dest->stream_key));

		const char *orientation_str = "Unknown";
		switch (dest->supported_orientation) {
		case ORIENTATION_HORIZONTAL:
			orientation_str = "Horizontal";
			break;
		case ORIENTATION_VERTICAL:
			orientation_str = "Vertical";
			break;
		case ORIENTATION_SQUARE:
			orientation_str = "Square";
			break;
		default:
			break;
		}

		destinationsTable->setItem(i, 2,
					   new QTableWidgetItem(orientation_str));

		QCheckBox *enabledCheck = new QCheckBox();
		enabledCheck->setChecked(dest->enabled);
		destinationsTable->setCellWidget(i, 3, enabledCheck);
	}
}

void RestreamerDock::onAddDestinationClicked()
{
	/* Create a dialog to add a destination */
	QDialog dialog(this);
	dialog.setWindowTitle("Add Destination");

	QVBoxLayout *layout = new QVBoxLayout(&dialog);
	QFormLayout *formLayout = new QFormLayout();

	QComboBox *serviceCombo = new QComboBox();
	serviceCombo->addItem("Twitch", SERVICE_TWITCH);
	serviceCombo->addItem("YouTube", SERVICE_YOUTUBE);
	serviceCombo->addItem("Facebook", SERVICE_FACEBOOK);
	serviceCombo->addItem("Kick", SERVICE_KICK);
	serviceCombo->addItem("TikTok", SERVICE_TIKTOK);
	serviceCombo->addItem("Instagram", SERVICE_INSTAGRAM);
	serviceCombo->addItem("X (Twitter)", SERVICE_X_TWITTER);
	serviceCombo->addItem("Custom RTMP", SERVICE_CUSTOM);

	QLineEdit *streamKeyEdit = new QLineEdit();

	QComboBox *orientationCombo = new QComboBox();
	orientationCombo->addItem("Horizontal", ORIENTATION_HORIZONTAL);
	orientationCombo->addItem("Vertical", ORIENTATION_VERTICAL);
	orientationCombo->addItem("Square", ORIENTATION_SQUARE);

	formLayout->addRow("Service:", serviceCombo);
	formLayout->addRow("Stream Key:", streamKeyEdit);
	formLayout->addRow("Orientation:", orientationCombo);

	layout->addLayout(formLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, &dialog,
		&QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog,
		&QDialog::reject);

	layout->addWidget(buttonBox);

	if (dialog.exec() == QDialog::Accepted) {
		streaming_service_t service = (streaming_service_t)
			serviceCombo->currentData().toInt();
		QString streamKey = streamKeyEdit->text();
		stream_orientation_t orientation = (stream_orientation_t)
			orientationCombo->currentData().toInt();

		if (!streamKey.isEmpty()) {
			restreamer_multistream_add_destination(
				multistreamConfig, service,
				streamKey.toUtf8().constData(), orientation);
			updateDestinationList();
			saveSettings();
		}
	}
}

void RestreamerDock::onRemoveDestinationClicked()
{
	int row = destinationsTable->currentRow();
	if (row >= 0) {
		restreamer_multistream_remove_destination(multistreamConfig,
							  row);
		updateDestinationList();
		saveSettings();
	}
}

void RestreamerDock::onCreateMultistreamClicked()
{
	if (!api || !multistreamConfig) {
		QMessageBox::warning(this, "Error", "API not initialized");
		return;
	}

	/* Update multistream config from UI */
	multistreamConfig->auto_detect_orientation =
		autoDetectOrientationCheck->isChecked();
	multistreamConfig->source_orientation = (stream_orientation_t)
		orientationCombo->currentData().toInt();

	/* For now, use a placeholder input URL
	   In production, this would come from OBS output */
	const char *input_url = "rtmp://localhost/live/obs_input";

	if (restreamer_multistream_start(api, multistreamConfig, input_url)) {
		QMessageBox::information(this, "Success",
					 "Multistream started successfully");
		onRefreshClicked();
	} else {
		QMessageBox::warning(
			this, "Error",
			QString("Failed to start multistream: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerDock::onUpdateTimer()
{
	if (!api) {
		return;
	}

	/* Auto-refresh process list and details */
	updateProcessList();

	if (selectedProcessId) {
		updateProcessDetails();
	}

	updateSessionList();
}

void RestreamerDock::onSaveSettingsClicked()
{
	saveSettings();
	QMessageBox::information(this, "Success", "Settings saved");
}
