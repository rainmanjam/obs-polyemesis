#include "restreamer-dock.h"
#include "restreamer-config.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <obs-frontend-api.h>
#include <obs-module.h>

RestreamerDock::RestreamerDock(QWidget *parent)
    : QDockWidget("Restreamer Control", parent), api(nullptr),
      profileManager(nullptr), multistreamConfig(nullptr),
      selectedProcessId(nullptr) {
  setupUI();
  loadSettings();

  /* Create update timer for auto-refresh */
  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &RestreamerDock::onUpdateTimer);
  updateTimer->start(5000); /* Update every 5 seconds */

  /* Initial refresh */
  onRefreshClicked();
}

RestreamerDock::~RestreamerDock() {
  saveSettings();

  if (profileManager) {
    profile_manager_destroy(profileManager);
  }

  if (api) {
    restreamer_api_destroy(api);
  }

  if (multistreamConfig) {
    restreamer_multistream_destroy(multistreamConfig);
  }

  bfree(selectedProcessId);
}

void RestreamerDock::setupUI() {
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

  /* Output Profiles Group - Aitum Multi style */
  QGroupBox *profilesGroup = new QGroupBox("Output Profiles");
  QVBoxLayout *profilesLayout = new QVBoxLayout();

  /* Profile list */
  profileListWidget = new QListWidget();
  connect(profileListWidget, &QListWidget::currentRowChanged, this,
          &RestreamerDock::onProfileSelected);

  /* Profile buttons - Top row: Create, Delete, Duplicate, Configure */
  QHBoxLayout *profileButtonsRow1 = new QHBoxLayout();
  createProfileButton = new QPushButton("Create Profile");
  deleteProfileButton = new QPushButton("Delete");
  duplicateProfileButton = new QPushButton("Duplicate");
  configureProfileButton = new QPushButton("Configure");

  deleteProfileButton->setEnabled(false);
  duplicateProfileButton->setEnabled(false);
  configureProfileButton->setEnabled(false);

  connect(createProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onCreateProfileClicked);
  connect(deleteProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onDeleteProfileClicked);
  connect(duplicateProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onDuplicateProfileClicked);
  connect(configureProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onConfigureProfileClicked);

  profileButtonsRow1->addWidget(createProfileButton);
  profileButtonsRow1->addWidget(deleteProfileButton);
  profileButtonsRow1->addWidget(duplicateProfileButton);
  profileButtonsRow1->addWidget(configureProfileButton);

  /* Profile buttons - Bottom row: Start, Stop, Start All, Stop All */
  QHBoxLayout *profileButtonsRow2 = new QHBoxLayout();
  startProfileButton = new QPushButton("Start");
  stopProfileButton = new QPushButton("Stop");
  startAllProfilesButton = new QPushButton("Start All");
  stopAllProfilesButton = new QPushButton("Stop All");

  startProfileButton->setEnabled(false);
  stopProfileButton->setEnabled(false);
  stopAllProfilesButton->setEnabled(false);

  connect(startProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onStartProfileClicked);
  connect(stopProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onStopProfileClicked);
  connect(startAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStartAllProfilesClicked);
  connect(stopAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStopAllProfilesClicked);

  profileButtonsRow2->addWidget(startProfileButton);
  profileButtonsRow2->addWidget(stopProfileButton);
  profileButtonsRow2->addWidget(startAllProfilesButton);
  profileButtonsRow2->addWidget(stopAllProfilesButton);

  /* Profile status label */
  profileStatusLabel = new QLabel("No profiles");

  /* Profile destinations table (shows destinations for selected profile) */
  profileDestinationsTable = new QTableWidget();
  profileDestinationsTable->setColumnCount(4);
  profileDestinationsTable->setHorizontalHeaderLabels(
      {"Destination", "Resolution", "Bitrate", "Status"});
  profileDestinationsTable->horizontalHeader()->setStretchLastSection(true);
  profileDestinationsTable->setMaximumHeight(150);

  profilesLayout->addWidget(profileListWidget);
  profilesLayout->addLayout(profileButtonsRow1);
  profilesLayout->addLayout(profileButtonsRow2);
  profilesLayout->addWidget(profileStatusLabel);
  profilesLayout->addWidget(profileDestinationsTable);
  profilesGroup->setLayout(profilesLayout);
  mainLayout->addWidget(profilesGroup);

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
  QGroupBox *multistreamGroup = new QGroupBox("Multistream Configuration");
  QVBoxLayout *multistreamLayout = new QVBoxLayout();

  QFormLayout *orientationLayout = new QFormLayout();
  autoDetectOrientationCheck = new QCheckBox("Auto-detect orientation");
  autoDetectOrientationCheck->setChecked(true);

  orientationCombo = new QComboBox();
  orientationCombo->addItem("Horizontal (Landscape)", ORIENTATION_HORIZONTAL);
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

void RestreamerDock::loadSettings() {
  obs_data_t *settings = obs_data_create_from_json_file_safe(
      obs_module_config_path("config.json"), "bak");

  if (!settings) {
    settings = obs_data_create();
  }

  hostEdit->setText(obs_data_get_string(settings, "host"));
  portEdit->setText(QString::number(obs_data_get_int(settings, "port")));
  httpsCheckbox->setChecked(obs_data_get_bool(settings, "use_https"));
  usernameEdit->setText(obs_data_get_string(settings, "username"));
  passwordEdit->setText(obs_data_get_string(settings, "password"));

  /* Load global config */
  restreamer_config_load(settings);

  /* Create profile manager if not already created */
  if (!profileManager) {
    profileManager = profile_manager_create(api);
  }

  /* Load profiles from settings */
  if (profileManager) {
    profile_manager_load_from_settings(profileManager, settings);
    updateProfileList();
  }

  /* Load multistream config */
  if (multistreamConfig) {
    restreamer_multistream_load_from_settings(multistreamConfig, settings);
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

void RestreamerDock::saveSettings() {
  obs_data_t *settings = obs_data_create();

  obs_data_set_string(settings, "host", hostEdit->text().toUtf8().constData());
  obs_data_set_int(settings, "port", portEdit->text().toInt());
  obs_data_set_bool(settings, "use_https", httpsCheckbox->isChecked());
  obs_data_set_string(settings, "username",
                      usernameEdit->text().toUtf8().constData());
  obs_data_set_string(settings, "password",
                      passwordEdit->text().toUtf8().constData());

  /* Save profiles */
  if (profileManager) {
    profile_manager_save_to_settings(profileManager, settings);
  }

  /* Save multistream config */
  if (multistreamConfig) {
    restreamer_multistream_save_to_settings(multistreamConfig, settings);
  }

  obs_data_save_json_safe(settings, obs_module_config_path("config.json"),
                          "tmp", "bak");
  obs_data_release(settings);

  /* Update global config */
  restreamer_connection_t connection = {0};
  connection.host = bstrdup(hostEdit->text().toUtf8().constData());
  connection.port = (uint16_t)portEdit->text().toInt();
  connection.use_https = httpsCheckbox->isChecked();
  if (!usernameEdit->text().isEmpty()) {
    connection.username = bstrdup(usernameEdit->text().toUtf8().constData());
  }
  if (!passwordEdit->text().isEmpty()) {
    connection.password = bstrdup(passwordEdit->text().toUtf8().constData());
  }

  restreamer_config_set_global_connection(&connection);

  bfree(connection.host);
  bfree(connection.username);
  bfree(connection.password);
}

void RestreamerDock::onTestConnectionClicked() {
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
        QString("Failed to connect: %1").arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::onRefreshClicked() {
  updateProcessList();
  updateSessionList();
}

void RestreamerDock::updateProcessList() {
  processList->clear();

  if (!api) {
    return;
  }

  restreamer_process_list_t list = {0};
  if (!restreamer_api_get_processes(api, &list)) {
    return;
  }

  for (size_t i = 0; i < list.count; i++) {
    const char *name = list.processes[i].reference ? list.processes[i].reference
                                                   : list.processes[i].id;

    QString displayText =
        QString("%1 [%2]").arg(name).arg(list.processes[i].state);

    QListWidgetItem *item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, QString(list.processes[i].id));
    processList->addItem(item);
  }

  restreamer_api_free_process_list(&list);
}

void RestreamerDock::onProcessSelected() {
  QListWidgetItem *item = processList->currentItem();
  if (!item) {
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
    restartButton->setEnabled(false);
    return;
  }

  bfree(selectedProcessId);
  selectedProcessId =
      bstrdup(item->data(Qt::UserRole).toString().toUtf8().constData());

  startButton->setEnabled(true);
  stopButton->setEnabled(true);
  restartButton->setEnabled(true);

  updateProcessDetails();
}

void RestreamerDock::updateProcessDetails() {
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

void RestreamerDock::updateSessionList() {
  sessionTable->setRowCount(0);

  if (!api) {
    return;
  }

  restreamer_session_list_t sessions = {0};
  if (!restreamer_api_get_sessions(api, &sessions)) {
    return;
  }

  sessionTable->setRowCount(static_cast<int>(sessions.count));

  for (size_t i = 0; i < sessions.count; i++) {
    int row = static_cast<int>(i);
    sessionTable->setItem(
        row, 0, new QTableWidgetItem(sessions.sessions[i].session_id));
    sessionTable->setItem(
        row, 1, new QTableWidgetItem(sessions.sessions[i].remote_addr));
    sessionTable->setItem(row, 2,
                          new QTableWidgetItem(QString::number(
                              sessions.sessions[i].bytes_sent / 1024 / 1024)));
  }

  restreamer_api_free_session_list(&sessions);
}

void RestreamerDock::onStartProcessClicked() {
  if (!api || !selectedProcessId) {
    return;
  }

  if (restreamer_api_start_process(api, selectedProcessId)) {
    QMessageBox::information(this, "Success", "Process started");
    onRefreshClicked();
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Failed to start process: %1")
                             .arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::onStopProcessClicked() {
  if (!api || !selectedProcessId) {
    return;
  }

  if (restreamer_api_stop_process(api, selectedProcessId)) {
    QMessageBox::information(this, "Success", "Process stopped");
    onRefreshClicked();
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Failed to stop process: %1")
                             .arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::onRestartProcessClicked() {
  if (!api || !selectedProcessId) {
    return;
  }

  if (restreamer_api_restart_process(api, selectedProcessId)) {
    QMessageBox::information(this, "Success", "Process restarted");
    onRefreshClicked();
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Failed to restart process: %1")
                             .arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::updateDestinationList() {
  if (!multistreamConfig) {
    return;
  }

  destinationsTable->setRowCount(static_cast<int>(multistreamConfig->destination_count));

  for (size_t i = 0; i < multistreamConfig->destination_count; i++) {
    int row = static_cast<int>(i);
    stream_destination_t *dest = &multistreamConfig->destinations[i];

    destinationsTable->setItem(row, 0, new QTableWidgetItem(dest->service_name));
    destinationsTable->setItem(row, 1, new QTableWidgetItem(dest->stream_key));

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

    destinationsTable->setItem(row, 2, new QTableWidgetItem(orientation_str));

    QCheckBox *enabledCheck = new QCheckBox();
    enabledCheck->setChecked(dest->enabled);
    destinationsTable->setCellWidget(row, 3, enabledCheck);
  }
}

void RestreamerDock::onAddDestinationClicked() {
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

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    streaming_service_t service =
        (streaming_service_t)serviceCombo->currentData().toInt();
    QString streamKey = streamKeyEdit->text();
    stream_orientation_t orientation =
        (stream_orientation_t)orientationCombo->currentData().toInt();

    if (!streamKey.isEmpty()) {
      restreamer_multistream_add_destination(multistreamConfig, service,
                                             streamKey.toUtf8().constData(),
                                             orientation);
      updateDestinationList();
      saveSettings();
    }
  }
}

void RestreamerDock::onRemoveDestinationClicked() {
  int row = destinationsTable->currentRow();
  if (row >= 0) {
    restreamer_multistream_remove_destination(multistreamConfig, row);
    updateDestinationList();
    saveSettings();
  }
}

void RestreamerDock::onCreateMultistreamClicked() {
  if (!api || !multistreamConfig) {
    QMessageBox::warning(this, "Error", "API not initialized");
    return;
  }

  /* Update multistream config from UI */
  multistreamConfig->auto_detect_orientation =
      autoDetectOrientationCheck->isChecked();
  multistreamConfig->source_orientation =
      (stream_orientation_t)orientationCombo->currentData().toInt();

  /* For now, use a placeholder input URL
     In production, this would come from OBS output */
  const char *input_url = "rtmp://localhost/live/obs_input";

  if (restreamer_multistream_start(api, multistreamConfig, input_url)) {
    QMessageBox::information(this, "Success",
                             "Multistream started successfully");
    onRefreshClicked();
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Failed to start multistream: %1")
                             .arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::onUpdateTimer() {
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

void RestreamerDock::onSaveSettingsClicked() {
  saveSettings();
  QMessageBox::information(this, "Success", "Settings saved");
}

/* Profile Management Functions */

void RestreamerDock::updateProfileList() {
  profileListWidget->clear();

  if (!profileManager || profileManager->profile_count == 0) {
    profileStatusLabel->setText("No profiles");
    deleteProfileButton->setEnabled(false);
    duplicateProfileButton->setEnabled(false);
    configureProfileButton->setEnabled(false);
    startProfileButton->setEnabled(false);
    stopProfileButton->setEnabled(false);
    stopAllProfilesButton->setEnabled(false);
    return;
  }

  /* Iterate through all profiles */
  bool hasActiveProfile = false;
  for (size_t i = 0; i < profileManager->profile_count; i++) {
    output_profile_t *profile = profileManager->profiles[i];

    /* Create status indicator based on profile status */
    QString statusIcon;
    switch (profile->status) {
    case PROFILE_STATUS_ACTIVE:
      statusIcon = "🟢";
      hasActiveProfile = true;
      break;
    case PROFILE_STATUS_STARTING:
    case PROFILE_STATUS_STOPPING:
      statusIcon = "🟡";
      hasActiveProfile = true;
      break;
    case PROFILE_STATUS_ERROR:
      statusIcon = "🔴";
      break;
    case PROFILE_STATUS_INACTIVE:
    default:
      statusIcon = "⚫";
      break;
    }

    /* Create list item with profile name, status, and destination count */
    QString itemText = QString("%1 %2 (%3 destinations)")
                           .arg(statusIcon)
                           .arg(profile->profile_name)
                           .arg(profile->destination_count);

    QListWidgetItem *item = new QListWidgetItem(itemText);
    item->setData(Qt::UserRole, QString(profile->profile_id));
    profileListWidget->addItem(item);
  }

  /* Update status label */
  profileStatusLabel->setText(
      QString("%1 profile(s)").arg(profileManager->profile_count));

  /* Update button states */
  stopAllProfilesButton->setEnabled(hasActiveProfile);
}

void RestreamerDock::updateProfileDetails() {
  int currentRow = profileListWidget->currentRow();
  if (currentRow < 0 || !profileManager ||
      currentRow >= (int)profileManager->profile_count) {
    /* No selection */
    profileDestinationsTable->setRowCount(0);
    deleteProfileButton->setEnabled(false);
    duplicateProfileButton->setEnabled(false);
    configureProfileButton->setEnabled(false);
    startProfileButton->setEnabled(false);
    stopProfileButton->setEnabled(false);
    return;
  }

  /* Get selected profile */
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();
  output_profile_t *profile =
      profile_manager_get_profile(profileManager, profileId.toUtf8().constData());

  if (!profile) {
    return;
  }

  /* Update button states based on profile status */
  deleteProfileButton->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);
  duplicateProfileButton->setEnabled(true);
  configureProfileButton->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);
  startProfileButton->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);
  stopProfileButton->setEnabled(profile->status == PROFILE_STATUS_ACTIVE ||
                                 profile->status == PROFILE_STATUS_STARTING);

  /* Populate destinations table */
  profileDestinationsTable->setRowCount(static_cast<int>(profile->destination_count));

  for (size_t i = 0; i < profile->destination_count; i++) {
    int row = static_cast<int>(i);
    profile_destination_t *dest = &profile->destinations[i];

    /* Destination name */
    QTableWidgetItem *nameItem = new QTableWidgetItem(dest->service_name);
    profileDestinationsTable->setItem(row, 0, nameItem);

    /* Resolution */
    QString resolution;
    if (dest->encoding.width == 0 || dest->encoding.height == 0) {
      resolution = "Source";
    } else {
      resolution = QString("%1x%2")
                       .arg(dest->encoding.width)
                       .arg(dest->encoding.height);
    }
    QTableWidgetItem *resItem = new QTableWidgetItem(resolution);
    profileDestinationsTable->setItem(row, 1, resItem);

    /* Bitrate */
    QString bitrate;
    if (dest->encoding.bitrate == 0) {
      bitrate = "Default";
    } else {
      bitrate = QString("%1 kbps").arg(dest->encoding.bitrate);
    }
    QTableWidgetItem *bitrateItem = new QTableWidgetItem(bitrate);
    profileDestinationsTable->setItem(row, 2, bitrateItem);

    /* Status */
    QString status = dest->enabled ? "Enabled" : "Disabled";
    QTableWidgetItem *statusItem = new QTableWidgetItem(status);
    profileDestinationsTable->setItem(row, 3, statusItem);
  }
}

/* Profile Slot Implementations */

void RestreamerDock::onProfileSelected() {
  updateProfileDetails();
}

void RestreamerDock::onStartProfileClicked() {
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem || !profileManager) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();

  if (output_profile_start(profileManager, profileId.toUtf8().constData())) {
    updateProfileList();
    updateProfileDetails();
  } else {
    QMessageBox::warning(this, "Error",
                         "Failed to start profile. Check Restreamer connection.");
  }
}

void RestreamerDock::onStopProfileClicked() {
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem || !profileManager) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();

  if (output_profile_stop(profileManager, profileId.toUtf8().constData())) {
    updateProfileList();
    updateProfileDetails();
  } else {
    QMessageBox::warning(this, "Error", "Failed to stop profile.");
  }
}

void RestreamerDock::onDeleteProfileClicked() {
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem || !profileManager) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();
  output_profile_t *profile =
      profile_manager_get_profile(profileManager, profileId.toUtf8().constData());

  if (!profile) {
    return;
  }

  /* Confirm deletion */
  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Delete Profile",
      QString("Are you sure you want to delete profile '%1'?")
          .arg(profile->profile_name),
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    if (profile_manager_delete_profile(profileManager, profileId.toUtf8().constData())) {
      updateProfileList();
      saveSettings();
    } else {
      QMessageBox::warning(this, "Error", "Failed to delete profile.");
    }
  }
}

void RestreamerDock::onStartAllProfilesClicked() {
  if (!profileManager) {
    return;
  }

  if (profile_manager_start_all(profileManager)) {
    updateProfileList();
    updateProfileDetails();
  } else {
    QMessageBox::warning(this, "Error",
                         "Failed to start all profiles. Check Restreamer connection.");
  }
}

void RestreamerDock::onStopAllProfilesClicked() {
  if (!profileManager) {
    return;
  }

  /* Confirm stop all */
  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Stop All Profiles",
      "Are you sure you want to stop all active profiles?",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    if (profile_manager_stop_all(profileManager)) {
      updateProfileList();
      updateProfileDetails();
    } else {
      QMessageBox::warning(this, "Error", "Failed to stop all profiles.");
    }
  }
}

void RestreamerDock::onDuplicateProfileClicked() {
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem || !profileManager) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();
  output_profile_t *sourceProfile =
      profile_manager_get_profile(profileManager, profileId.toUtf8().constData());

  if (!sourceProfile) {
    return;
  }

  /* Prompt for new profile name */
  bool ok;
  QString newName = QInputDialog::getText(
      this, "Duplicate Profile", "Enter name for duplicated profile:",
      QLineEdit::Normal, QString("%1 (Copy)").arg(sourceProfile->profile_name), &ok);

  if (ok && !newName.isEmpty()) {
    /* Create new profile with same settings */
    output_profile_t *newProfile =
        profile_manager_create_profile(profileManager, newName.toUtf8().constData());

    if (newProfile) {
      /* Copy settings */
      newProfile->source_orientation = sourceProfile->source_orientation;
      newProfile->auto_detect_orientation = sourceProfile->auto_detect_orientation;
      newProfile->source_width = sourceProfile->source_width;
      newProfile->source_height = sourceProfile->source_height;
      newProfile->auto_start = sourceProfile->auto_start;
      newProfile->auto_reconnect = sourceProfile->auto_reconnect;
      newProfile->reconnect_delay_sec = sourceProfile->reconnect_delay_sec;

      /* Copy destinations */
      for (size_t i = 0; i < sourceProfile->destination_count; i++) {
        profile_destination_t *srcDest = &sourceProfile->destinations[i];
        profile_add_destination(newProfile, srcDest->service, srcDest->stream_key,
                                srcDest->target_orientation, &srcDest->encoding);
      }

      updateProfileList();
      saveSettings();

      /* Select the new profile */
      for (int i = 0; i < profileListWidget->count(); i++) {
        QListWidgetItem *item = profileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == newProfile->profile_id) {
          profileListWidget->setCurrentRow(i);
          break;
        }
      }
    } else {
      QMessageBox::warning(this, "Error", "Failed to duplicate profile.");
    }
  }
}

void RestreamerDock::onCreateProfileClicked() {
  if (!profileManager) {
    return;
  }

  /* Prompt for profile name */
  bool ok;
  QString profileName =
      QInputDialog::getText(this, "Create Profile", "Enter profile name:",
                            QLineEdit::Normal, "New Profile", &ok);

  if (ok && !profileName.isEmpty()) {
    output_profile_t *newProfile =
        profile_manager_create_profile(profileManager, profileName.toUtf8().constData());

    if (newProfile) {
      updateProfileList();
      saveSettings();

      /* Select the new profile */
      for (int i = 0; i < profileListWidget->count(); i++) {
        QListWidgetItem *item = profileListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == newProfile->profile_id) {
          profileListWidget->setCurrentRow(i);
          break;
        }
      }

      /* Open configure dialog to set up destinations */
      QMessageBox::information(
          this, "Profile Created",
          QString("Profile '%1' created successfully.\n\nUse the Configure "
                  "button to add destinations and customize settings.")
              .arg(profileName));
    } else {
      QMessageBox::warning(this, "Error", "Failed to create profile.");
    }
  }
}

void RestreamerDock::onConfigureProfileClicked() {
  QListWidgetItem *currentItem = profileListWidget->currentItem();
  if (!currentItem || !profileManager) {
    return;
  }

  QString profileId = currentItem->data(Qt::UserRole).toString();
  output_profile_t *profile =
      profile_manager_get_profile(profileManager, profileId.toUtf8().constData());

  if (!profile) {
    return;
  }

  /* Create configuration dialog */
  QDialog dialog(this);
  dialog.setWindowTitle(QString("Configure Profile: %1").arg(profile->profile_name));
  dialog.setMinimumWidth(500);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

  /* Basic profile settings group */
  QGroupBox *basicGroup = new QGroupBox("Basic Settings");
  QFormLayout *basicLayout = new QFormLayout();

  /* Profile name */
  QLineEdit *nameEdit = new QLineEdit(profile->profile_name);
  basicLayout->addRow("Profile Name:", nameEdit);

  /* Source orientation */
  QComboBox *orientationCombo = new QComboBox();
  orientationCombo->addItem("Horizontal (16:9)", (int)ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (9:16)", (int)ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square (1:1)", (int)ORIENTATION_SQUARE);
  orientationCombo->setCurrentIndex((int)profile->source_orientation);
  basicLayout->addRow("Source Orientation:", orientationCombo);

  /* Auto-detect orientation */
  QCheckBox *autoDetectCheck = new QCheckBox("Auto-detect orientation from source");
  autoDetectCheck->setChecked(profile->auto_detect_orientation);
  basicLayout->addRow("", autoDetectCheck);

  /* Auto-start */
  QCheckBox *autoStartCheck = new QCheckBox("Auto-start with OBS streaming");
  autoStartCheck->setChecked(profile->auto_start);
  basicLayout->addRow("", autoStartCheck);

  /* Auto-reconnect */
  QCheckBox *autoReconnectCheck = new QCheckBox("Auto-reconnect on disconnect");
  autoReconnectCheck->setChecked(profile->auto_reconnect);
  basicLayout->addRow("", autoReconnectCheck);

  /* Input URL */
  QLineEdit *inputUrlEdit = new QLineEdit(profile->input_url);
  inputUrlEdit->setPlaceholderText("rtmp://localhost/live/obs_input");
  basicLayout->addRow("Input URL:", inputUrlEdit);

  basicGroup->setLayout(basicLayout);
  mainLayout->addWidget(basicGroup);

  /* Destinations group */
  QGroupBox *destGroup = new QGroupBox("Destinations");
  QVBoxLayout *destLayout = new QVBoxLayout();

  QLabel *destLabel = new QLabel(
      QString("This profile has %1 destination(s).\n\nDestination management "
              "(add/remove/configure) will be available in a future update.")
          .arg(profile->destination_count));
  destLabel->setWordWrap(true);
  destLayout->addWidget(destLabel);

  /* Show existing destinations */
  if (profile->destination_count > 0) {
    QListWidget *destList = new QListWidget();
    for (size_t i = 0; i < profile->destination_count; i++) {
      profile_destination_t *dest = &profile->destinations[i];
      QString destText =
          QString("%1 - %2 (%3)")
              .arg(dest->service_name)
              .arg(dest->stream_key)
              .arg(dest->enabled ? "Enabled" : "Disabled");
      destList->addItem(destText);
    }
    destList->setMaximumHeight(150);
    destLayout->addWidget(destList);
  }

  destGroup->setLayout(destLayout);
  mainLayout->addWidget(destGroup);

  /* Dialog buttons */
  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  mainLayout->addWidget(buttonBox);

  /* Show dialog and apply changes if accepted */
  if (dialog.exec() == QDialog::Accepted) {
    /* Validate input URL */
    QString inputUrl = inputUrlEdit->text().trimmed();
    if (inputUrl.isEmpty()) {
      QMessageBox::warning(this, "Validation Error",
                           "Input URL cannot be empty.");
      return;
    }

    if (!inputUrl.startsWith("rtmp://") && !inputUrl.startsWith("rtmps://")) {
      QMessageBox::warning(
          this, "Validation Error",
          "Input URL must start with rtmp:// or rtmps://\n\nExample: "
          "rtmp://localhost/live/obs_input");
      return;
    }

    /* Basic format validation: rtmp://host/app/key */
    QStringList urlParts = inputUrl.mid(7).split('/'); // Skip "rtmp://"
    if (urlParts.size() < 3) {
      QMessageBox::warning(
          this, "Validation Error",
          "Input URL must include host, application, and stream key.\n\nFormat: "
          "rtmp://host/application/streamkey\nExample: "
          "rtmp://localhost/live/obs_input");
      return;
    }

    /* Update profile settings */
    if (profile->profile_name) {
      bfree(profile->profile_name);
    }
    profile->profile_name = bstrdup(nameEdit->text().toUtf8().constData());

    profile->source_orientation =
        (stream_orientation_t)orientationCombo->currentData().toInt();
    profile->auto_detect_orientation = autoDetectCheck->isChecked();
    profile->auto_start = autoStartCheck->isChecked();
    profile->auto_reconnect = autoReconnectCheck->isChecked();

    /* Update input URL (use validated and trimmed value) */
    if (profile->input_url) {
      bfree(profile->input_url);
    }
    profile->input_url = bstrdup(inputUrl.toUtf8().constData());

    updateProfileList();
    updateProfileDetails();
    saveSettings();

    QMessageBox::information(this, "Success", "Profile settings updated.");
  }
}
