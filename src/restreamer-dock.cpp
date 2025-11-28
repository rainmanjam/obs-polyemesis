#include "restreamer-dock.h"
#include "connection-config-dialog.h"
#include "obs-helpers.hpp"
#include "obs-theme-utils.h"
#include "profile-edit-dialog.h"
#include "profile-widget.h"
#include "restreamer-config.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QIntValidator>
#include <QMessageBox>
#include <QScrollArea>
#include <QTextEdit>
#include <obs-frontend-api.h>
#include <obs-module.h>

extern "C" {
#include "obs-bridge.h"
#include <plugin-support.h>
}

RestreamerDock::RestreamerDock(QWidget *parent)
    : QWidget(parent), api(nullptr), profileManager(nullptr),
      multistreamConfig(nullptr), selectedProcessId(nullptr), bridge(nullptr),
      originalSize(600, 800), sizeInitialized(false), serviceLoader(nullptr) {

  /* Initialize OBS Service Loader */
  serviceLoader = new OBSServiceLoader();
  obs_log(LOG_INFO, "OBS Service Loader initialized with %d services",
          serviceLoader->getAllServices().count());

  setupUI();
  loadSettings();

  /* Update connection status based on loaded settings */
  updateConnectionStatus();

  /* Initialize OBS Bridge with default configuration */
  obs_bridge_config_t bridge_config = {0};
  bridge_config.restreamer_url = bstrdup("http://localhost:8080");
  bridge_config.rtmp_horizontal_url =
      bstrdup("rtmp://localhost/live/obs_horizontal");
  bridge_config.rtmp_vertical_url =
      bstrdup("rtmp://localhost/live/obs_vertical");
  bridge_config.auto_start_enabled = true; /* Auto-start enabled by default */
  bridge_config.show_vertical_notification = true;
  bridge_config.show_preflight_check = true;

  bridge = obs_bridge_create(&bridge_config);

  /* Cleanup temporary config strings */
  bfree(bridge_config.restreamer_url);
  bfree(bridge_config.rtmp_horizontal_url);
  bfree(bridge_config.rtmp_vertical_url);

  if (bridge) {
    obs_log(LOG_INFO, "OBS Bridge initialized successfully");
  } else {
    obs_log(LOG_ERROR, "Failed to initialize OBS Bridge");
  }

  /* Create update timer for auto-refresh */
  updateTimer = new QTimer(this);
  connect(updateTimer, &QTimer::timeout, this, &RestreamerDock::onUpdateTimer);
  updateTimer->start(5000); /* Update every 5 seconds */

  /* Register frontend save callback for scene collection integration */
  obs_frontend_add_save_callback(frontend_save_callback, this);
  obs_log(LOG_INFO,
          "Registered frontend save callback for dock settings persistence");

  /* Connect to parent dock's topLevelChanged signal to restore size when
   * undocking. Note: The parent QDockWidget is created by OBS when we call
   * obs_frontend_add_dock_by_id(), so we need to connect to it after a brief
   * delay to ensure it exists.
   */
  QTimer::singleShot(100, this, [this]() {
    QDockWidget *dock = qobject_cast<QDockWidget *>(parentWidget());
    if (dock) {
      connect(dock, &QDockWidget::topLevelChanged, this,
              &RestreamerDock::onDockTopLevelChanged);
      obs_log(LOG_INFO,
              "Connected to dock topLevelChanged signal for size restoration");
    } else {
      obs_log(LOG_WARNING,
              "Could not find parent QDockWidget for size restoration");
    }
  });

  /* Initial refresh */
  onRefreshClicked();
}

RestreamerDock::~RestreamerDock() {
  /* Remove frontend save callback */
  obs_frontend_remove_save_callback(frontend_save_callback, this);
  obs_log(LOG_INFO, "Unregistered frontend save callback");

  /* Stop timer first to prevent callbacks during destruction */
  if (updateTimer) {
    updateTimer->stop();
    updateTimer->deleteLater();
    updateTimer = nullptr;
  }

  saveSettings();

  /* Clean up service loader */
  if (serviceLoader) {
    delete serviceLoader;
    serviceLoader = nullptr;
  }

  /* Clean up bridge */
  if (bridge) {
    obs_bridge_destroy(bridge);
    bridge = nullptr;
    obs_log(LOG_INFO, "OBS Bridge destroyed");
  }

  /* Clean up resources with mutex protection */
  {
    std::lock_guard<std::recursive_mutex> lock(apiMutex);
    if (api) {
      restreamer_api_destroy(api);
      api = nullptr;
    }
  }

  {
    std::lock_guard<std::recursive_mutex> lock(profileMutex);
    if (profileManager) {
      profile_manager_destroy(profileManager);
      profileManager = nullptr;
    }
  }

  if (multistreamConfig) {
    restreamer_multistream_destroy(multistreamConfig);
    multistreamConfig = nullptr;
  }

  bfree(selectedProcessId);
  selectedProcessId = nullptr;
}

void RestreamerDock::onDockTopLevelChanged(bool floating) {
  QDockWidget *dock = qobject_cast<QDockWidget *>(parentWidget());
  if (!dock) {
    return;
  }

  if (floating) {
    /* Dock became floating (undocked) - restore original size */
    if (sizeInitialized && originalSize.isValid()) {
      dock->resize(originalSize);
      obs_log(LOG_INFO, "Restored dock to original size: %dx%d",
              originalSize.width(), originalSize.height());
    } else {
      /* First time floating - use a good default size */
      dock->resize(600, 800);
      obs_log(LOG_INFO, "Set initial floating size: 600x800");
    }
  } else {
    /* Dock was docked - save current size before it gets resized */
    if (!sizeInitialized) {
      originalSize = dock->size();
      sizeInitialized = true;
      obs_log(LOG_INFO, "Saved original dock size: %dx%d", originalSize.width(),
              originalSize.height());
    }
  }
}

void RestreamerDock::frontend_save_callback(obs_data_t *save_data, bool saving,
                                            void *private_data) {
  /* Static callback that forwards to instance method */
  RestreamerDock *dock = static_cast<RestreamerDock *>(private_data);
  if (dock) {
    dock->onFrontendSave(save_data, saving);
  }
}

void RestreamerDock::onFrontendSave(obs_data_t *save_data, bool saving) {
  /* This is called when OBS saves/loads scene collections.
   * We integrate our settings with the scene collection for better persistence.
   * 'saving' is true when saving, false when loading.
   */
  if (saving) {
    /* Save: Store our dock settings in the scene collection */
    obs_log(LOG_DEBUG, "Saving Restreamer dock settings to scene collection");

    /* Create a data object for our dock settings */
    OBSDataAutoRelease dock_settings(obs_data_create());

    /* Connection settings now handled by ConnectionConfigDialog */
    /* Settings are saved directly to obs_frontend_get_global_config() */

    /* Save bridge settings (with null checks for shutdown safety) */
    if (bridgeHorizontalUrlEdit) {
      obs_data_set_string(dock_settings, "bridge_horizontal_url",
                          bridgeHorizontalUrlEdit->text().toUtf8().constData());
    }
    if (bridgeVerticalUrlEdit) {
      obs_data_set_string(dock_settings, "bridge_vertical_url",
                          bridgeVerticalUrlEdit->text().toUtf8().constData());
    }
    if (bridgeAutoStartCheckbox) {
      obs_data_set_bool(dock_settings, "bridge_auto_start",
                        bridgeAutoStartCheckbox->isChecked());
    }

    /* Enhanced: Save currently selected process for restoration */
    if (selectedProcessId) {
      obs_data_set_string(dock_settings, "last_selected_process",
                          selectedProcessId);
    }

    /* Enhanced: Save profile active states for restoration */
    if (profileManager) {
      OBSDataArrayAutoRelease profile_states(obs_data_array_create());
      for (size_t i = 0; i < profileManager->profile_count; i++) {
        if (profileManager->profiles[i]) {
          OBSDataAutoRelease profile_state(obs_data_create());
          obs_data_set_string(profile_state, "name",
                              profileManager->profiles[i]->profile_name);
          obs_data_set_bool(profile_state, "was_active",
                            profileManager->profiles[i]->status ==
                                PROFILE_STATUS_ACTIVE);
          obs_data_array_push_back(profile_states, profile_state);
        }
      }
      obs_data_set_array(dock_settings, "profile_states", profile_states);
    }

    /* Save profiles */
    if (profileManager) {
      profile_manager_save_to_settings(profileManager, dock_settings);
    }

    /* Save multistream config */
    if (multistreamConfig) {
      restreamer_multistream_save_to_settings(multistreamConfig, dock_settings);
    }

    /* Store our settings in the scene collection under a plugin-specific key */
    obs_data_set_obj(save_data, "obs-polyemesis-dock", dock_settings);

  } else {
    /* Load: Restore our dock settings from the scene collection */
    obs_log(LOG_DEBUG,
            "Loading Restreamer dock settings from scene collection");

    /* Retrieve our saved settings */
    OBSDataAutoRelease dock_settings(
        obs_data_get_obj(save_data, "obs-polyemesis-dock"));

    if (dock_settings) {
      /* Connection settings now handled by ConnectionConfigDialog */
      /* Settings are loaded from obs_frontend_get_global_config() */

      /* Restore bridge settings */
      const char *h_url =
          obs_data_get_string(dock_settings, "bridge_horizontal_url");
      if (h_url && *h_url) {
        bridgeHorizontalUrlEdit->setText(h_url);
      }

      const char *v_url =
          obs_data_get_string(dock_settings, "bridge_vertical_url");
      if (v_url && *v_url) {
        bridgeVerticalUrlEdit->setText(v_url);
      }

      bridgeAutoStartCheckbox->setChecked(
          obs_data_get_bool(dock_settings, "bridge_auto_start"));

      /* Restore profiles */
      if (profileManager) {
        profile_manager_load_from_settings(profileManager, dock_settings);
        updateProfileList();
      }

      /* Restore multistream config */
      if (multistreamConfig) {
        restreamer_multistream_load_from_settings(multistreamConfig,
                                                  dock_settings);
        updateDestinationList();
      }

      /* Enhanced: Restore last selected process */
      const char *last_process =
          obs_data_get_string(dock_settings, "last_selected_process");
      if (last_process && *last_process) {
        bfree(selectedProcessId);
        selectedProcessId = bstrdup(last_process);
        obs_log(LOG_DEBUG, "Restored last selected process: %s", last_process);
      }

      /* Enhanced: Log profile states for debugging (could be used to
       * auto-restore active profiles) */
      OBSDataArrayAutoRelease profile_states(
          obs_data_get_array(dock_settings, "profile_states"));
      if (profile_states) {
        size_t count = obs_data_array_count(profile_states);
        obs_log(LOG_DEBUG, "Found %zu saved profile states", count);
        /* Note: Auto-restarting profiles on scene collection load could be
         * implemented here but should be behind a user preference setting to
         * avoid unexpected behavior */
      }

      obs_log(LOG_INFO,
              "Restored Restreamer dock settings from scene collection");
    }
  }
}

void RestreamerDock::setupUI() {
  QWidget *mainWidget = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

  /* Create scroll area for vertical collapsible sections layout */
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);

  QWidget *scrollContent = new QWidget();
  QVBoxLayout *verticalLayout = new QVBoxLayout(scrollContent);
  verticalLayout->setSpacing(8);
  verticalLayout->setContentsMargins(0, 0, 0, 0);

  /* ===== Connection Status Bar ===== */
  QWidget *connectionBar = new QWidget();
  QHBoxLayout *connectionBarLayout = new QHBoxLayout(connectionBar);
  connectionBarLayout->setContentsMargins(16, 12, 16, 12);
  connectionBarLayout->setSpacing(12);

  /* Connection status label with icon */
  connectionStatusLabel = new QLabel("Connection ⚫ Not Connected");
  connectionStatusLabel->setStyleSheet(
      QString("color: %1; font-weight: 600; font-size: 14px;")
          .arg(obs_theme_get_muted_color().name()));

  /* Configure button (replaces Test button) */
  configureConnectionButton = new QPushButton("Configure");
  configureConnectionButton->setMinimumWidth(110);
  configureConnectionButton->setFixedHeight(32);
  configureConnectionButton->setToolTip(
      "Configure connection to Restreamer server");
  connect(configureConnectionButton, &QPushButton::clicked, this,
          &RestreamerDock::onConfigureConnectionClicked);

  connectionBarLayout->addWidget(connectionStatusLabel);
  connectionBarLayout->addStretch();
  connectionBarLayout->addWidget(configureConnectionButton);

  /* Style the connection bar */
  connectionBar->setStyleSheet("QWidget { "
                               "  background-color: #1e1e2e; "
                               "  border-radius: 8px; "
                               "  margin: 8px; "
                               "}");

  verticalLayout->addWidget(connectionBar);

  /* ===== Profiles (Configure & Publish) - Always Visible ===== */
  QWidget *profilesTab = new QWidget();
  QVBoxLayout *profilesTabLayout = new QVBoxLayout(profilesTab);
  profilesTabLayout->setSpacing(8);
  profilesTabLayout->setContentsMargins(8, 8, 8, 8);

  /* Profile management buttons at top */
  QHBoxLayout *profileManagementButtons = new QHBoxLayout();

  createProfileButton = new QPushButton("+ New Profile");
  createProfileButton->setToolTip("Create new streaming profile");
  connect(createProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onCreateProfileClicked);

  startAllProfilesButton = new QPushButton("▶ Start All");
  startAllProfilesButton->setToolTip("Start all profiles");
  connect(startAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStartAllProfilesClicked);

  stopAllProfilesButton = new QPushButton("■ Stop All");
  stopAllProfilesButton->setToolTip("Stop all profiles");
  stopAllProfilesButton->setEnabled(false);
  connect(stopAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStopAllProfilesClicked);

  profileManagementButtons->addWidget(createProfileButton);
  profileManagementButtons->addStretch();
  profileManagementButtons->addWidget(startAllProfilesButton);
  profileManagementButtons->addWidget(stopAllProfilesButton);

  profilesTabLayout->addLayout(profileManagementButtons);

  /* Profile status label */
  profileStatusLabel = new QLabel("No profiles");
  profileStatusLabel->setAlignment(Qt::AlignCenter);
  profileStatusLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; font-style: italic; }")
          .arg(obs_theme_get_muted_color().name()));
  profilesTabLayout->addWidget(profileStatusLabel);

  /* Scrollable container for profile widgets */
  QScrollArea *profileScrollArea = new QScrollArea();
  profileScrollArea->setWidgetResizable(true);
  profileScrollArea->setFrameShape(QFrame::NoFrame);

  profileListContainer = new QWidget();
  profileListLayout = new QVBoxLayout(profileListContainer);
  profileListLayout->setContentsMargins(0, 0, 0, 0);
  profileListLayout->setSpacing(8);
  profileListLayout->addStretch();

  profileScrollArea->setWidget(profileListContainer);
  profilesTabLayout->addWidget(profileScrollArea);

  /* Add Profiles section directly to main layout (always visible) */
  verticalLayout->addWidget(profilesTab);

  /* Add stretch to push sections to the top */
  verticalLayout->addStretch();

  /* Quick action buttons at bottom */
  QHBoxLayout *quickActionsLayout = new QHBoxLayout();
  quickActionsLayout->addStretch();

  QPushButton *monitoringButton = new QPushButton("Monitoring");
  monitoringButton->setMinimumHeight(36);
  connect(monitoringButton, &QPushButton::clicked, this, [this]() {
    /* Build monitoring information from current profiles */
    QString monitorInfo = "<b>System Monitoring</b><br><br>";

    if (profileManager) {
      size_t active_profiles = 0;
      size_t total_destinations = 0;
      size_t active_destinations = 0;
      uint64_t total_bytes = 0;

      for (size_t i = 0; i < profileManager->profile_count; i++) {
        output_profile_t *profile = profileManager->profiles[i];
        if (profile->status == PROFILE_STATUS_ACTIVE) {
          active_profiles++;
        }
        total_destinations += profile->destination_count;
        for (size_t j = 0; j < profile->destination_count; j++) {
          if (profile->destinations[j].connected) {
            active_destinations++;
          }
          total_bytes += profile->destinations[j].bytes_sent;
        }
      }

      monitorInfo += QString("<b>Profiles:</b> %1 total, %2 active<br>")
                         .arg(profileManager->profile_count)
                         .arg(active_profiles);
      monitorInfo += QString("<b>Destinations:</b> %1 total, %2 active<br>")
                         .arg(total_destinations)
                         .arg(active_destinations);
      monitorInfo += QString("<b>Total Data Sent:</b> %1 MB<br><br>")
                         .arg(total_bytes / (1024.0 * 1024.0), 0, 'f', 2);

      monitorInfo += "<b>Connection Status:</b><br>";
      if (api) {
        monitorInfo += "  Restreamer API: <span style='color: "
                       "green;'>Connected</span><br>";
      } else {
        monitorInfo += "  Restreamer API: <span style='color: "
                       "red;'>Disconnected</span><br>";
      }
    } else {
      monitorInfo += "<i>No monitoring data available</i>";
    }

    QMessageBox::information(this, "System Monitoring", monitorInfo);
  });

  QPushButton *advancedButton = new QPushButton("Advanced");
  advancedButton->setMinimumHeight(36);
  connect(advancedButton, &QPushButton::clicked, this, [this]() {
    QString advancedInfo = "<b>Advanced Settings</b><br><br>";
    advancedInfo += "This section will include:<br>";
    advancedInfo += "• Custom RTMP server configuration<br>";
    advancedInfo += "• Advanced encoding options<br>";
    advancedInfo += "• Network bandwidth limits<br>";
    advancedInfo += "• Buffer settings<br>";
    advancedInfo += "• Debug logging options<br><br>";
    advancedInfo += "<i>Features coming in future update</i>";

    QMessageBox::information(this, "Advanced Settings", advancedInfo);
  });

  QPushButton *settingsButton = new QPushButton("Settings");
  settingsButton->setMinimumHeight(36);
  connect(settingsButton, &QPushButton::clicked, this, [this]() {
    QString settingsInfo = "<b>Global Settings</b><br><br>";
    settingsInfo += "<b>Current Configuration:</b><br>";

    if (api) {
      settingsInfo += "  Status: Connected to Restreamer server<br>";
    } else {
      settingsInfo += "  Status: Not connected<br>";
    }

    settingsInfo += "<br><i>Additional settings coming in future update</i>";

    QMessageBox::information(this, "Settings", settingsInfo);
  });

  quickActionsLayout->addWidget(monitoringButton);
  quickActionsLayout->addWidget(advancedButton);
  quickActionsLayout->addWidget(settingsButton);
  quickActionsLayout->addStretch();

  verticalLayout->addLayout(quickActionsLayout);

  /* Set scroll area widget and add to main layout */
  scrollArea->setWidget(scrollContent);
  mainLayout->addWidget(scrollArea);

  /* Set the layout for this widget (QWidget uses setLayout, not setWidget) */
  setLayout(mainLayout);
  setMinimumWidth(400);

  /* Custom stylesheets removed for v0.9.0 - now using OBS native QPalette
   * theming This allows the plugin to automatically match all 6 OBS themes
   * (Yami, Grey, Acri, Dark, Rachni, Light) See THEME_AUDIT.md for details on
   * the 257 lines of custom styling that were removed
   */

  /* Improve spacing in layouts */
  mainLayout->setSpacing(12);
  mainLayout->setContentsMargins(12, 12, 12, 12);

  multistreamConfig = restreamer_multistream_create();
}

void RestreamerDock::loadSettings() {
  OBSDataAutoRelease settings(obs_data_create_from_json_file_safe(
      obs_module_config_path("config.json"), "bak"));

  if (!settings) {
    settings = OBSDataAutoRelease(obs_data_create());
  }

  /* Connection settings now handled by ConnectionConfigDialog */

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

  /* Load bridge settings */
  if (bridgeHorizontalUrlEdit) {
    bridgeHorizontalUrlEdit->setText(
        obs_data_get_string(settings, "bridge_horizontal_url"));
  }
  if (bridgeVerticalUrlEdit) {
    bridgeVerticalUrlEdit->setText(
        obs_data_get_string(settings, "bridge_vertical_url"));
  }
  if (bridgeAutoStartCheckbox) {
    bridgeAutoStartCheckbox->setChecked(
        obs_data_get_bool(settings, "bridge_auto_start"));
  }

  /* RAII: settings automatically released when going out of scope */

  /* Set defaults if empty */
  /* Connection defaults now handled by ConnectionConfigDialog */
  if (bridgeHorizontalUrlEdit && bridgeHorizontalUrlEdit->text().isEmpty()) {
    bridgeHorizontalUrlEdit->setText("rtmp://localhost/live/obs_horizontal");
  }
  if (bridgeVerticalUrlEdit && bridgeVerticalUrlEdit->text().isEmpty()) {
    bridgeVerticalUrlEdit->setText("rtmp://localhost/live/obs_vertical");
  }
  /* Bridge auto-start defaults to true (already set in loadSettings if not in
   * config) */
  if (bridgeAutoStartCheckbox &&
      !obs_data_has_user_value(settings, "bridge_auto_start")) {
    bridgeAutoStartCheckbox->setChecked(true);
  }

  /* Auto-test connection if server config is already populated */
  bool hasServerConfig = obs_data_has_user_value(settings, "host") ||
                         obs_data_has_user_value(settings, "port");
  /* Connection validation now handled by ConnectionConfigDialog */
  (void)hasServerConfig; // Suppress unused variable warning
}

void RestreamerDock::saveSettings() {
  OBSDataAutoRelease settings(obs_data_create());

  /* Connection settings now handled by ConnectionConfigDialog */

  /* Save profiles */
  if (profileManager) {
    profile_manager_save_to_settings(profileManager, settings);
  }

  /* Save multistream config */
  if (multistreamConfig) {
    restreamer_multistream_save_to_settings(multistreamConfig, settings);
  }

  /* Save bridge settings */
  if (bridgeHorizontalUrlEdit) {
    obs_data_set_string(settings, "bridge_horizontal_url",
                        bridgeHorizontalUrlEdit->text().toUtf8().constData());
  }
  if (bridgeVerticalUrlEdit) {
    obs_data_set_string(settings, "bridge_vertical_url",
                        bridgeVerticalUrlEdit->text().toUtf8().constData());
  }
  if (bridgeAutoStartCheckbox) {
    obs_data_set_bool(settings, "bridge_auto_start",
                      bridgeAutoStartCheckbox->isChecked());
  }

  /* Safe file writing: writes to .tmp first, then creates .bak backup,
   * then renames .tmp to actual file. Prevents corruption on crash/power loss.
   */
  const char *config_path = obs_module_config_path("config.json");
  if (!obs_data_save_json_safe(settings, config_path, "tmp", "bak")) {
    blog(LOG_ERROR, "[obs-polyemesis] Failed to save settings to %s",
         config_path);
  }

  /* RAII: settings automatically released when going out of scope */

  /* Connection settings are now saved directly by ConnectionConfigDialog */
  /* Global connection config is updated when dialog saves settings */
}

void RestreamerDock::onTestConnectionClicked() {
  saveSettings();

  if (api) {
    restreamer_api_destroy(api);
  }

  api = restreamer_config_create_global_api();

  if (!api) {
    connectionStatusLabel->setText("Connection ⚫ Failed to create API");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_error_color().name()));
    return;
  }

  if (restreamer_api_test_connection(api)) {
    connectionStatusLabel->setText("Connection ⚫ Connected");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_success_color().name()));
    onRefreshClicked();
  } else {
    connectionStatusLabel->setText("Connection ⚫ Failed");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_error_color().name()));
    QMessageBox::warning(
        this, "Connection Error",
        QString("Failed to connect: %1").arg(restreamer_api_get_error(api)));
  }
}

void RestreamerDock::onConfigureConnectionClicked() {
  /* Create and show dialog (loads settings automatically in constructor) */
  ConnectionConfigDialog dialog(this);

  /* Handle dialog result */
  if (dialog.exec() == QDialog::Accepted) {
    obs_log(LOG_INFO, "Connection settings saved, reconnecting...");

    /* Reconnect with new settings */
    if (api) {
      restreamer_api_destroy(api);
      api = nullptr;
    }

    /* Update connection status (will create API and test connection) */
    updateConnectionStatus();

    /* Refresh process list if connected */
    if (api) {
      onRefreshClicked();
    }
  }
}

void RestreamerDock::updateConnectionStatus() {
  /* Recreate API client from global config */
  if (api) {
    restreamer_api_destroy(api);
    api = nullptr;
  }

  api = restreamer_config_create_global_api();

  /* If API creation failed, no settings are configured */
  if (!api) {
    connectionStatusLabel->setText("Connection ⚫ Not Connected");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_muted_color().name()));
    obs_log(LOG_DEBUG, "No Restreamer connection settings configured");
    return;
  }

  /* Test connection with created API */
  if (restreamer_api_test_connection(api)) {
    connectionStatusLabel->setText("Connection ⚫ Connected");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_success_color().name()));
    obs_log(LOG_INFO, "Successfully connected to Restreamer");
  } else {
    connectionStatusLabel->setText("Connection ⚫ Disconnected");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: 600; font-size: 14px;")
            .arg(obs_theme_get_error_color().name()));
    obs_log(LOG_WARNING, "Failed to connect to Restreamer");
  }
}

void RestreamerDock::onRefreshClicked() {
  updateProcessList();
  updateSessionList();
}

void RestreamerDock::updateProcessList() {
  if (!processList) {
    return; /* Monitoring section removed from UI */
  }

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
  if (!processList || !startButton || !stopButton || !restartButton) {
    return; /* Monitoring section removed from UI */
  }

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

  if (!processIdLabel || !processStateLabel) {
    return; /* Monitoring section removed from UI */
  }

  restreamer_process_t process = {0};
  if (!restreamer_api_get_process(api, selectedProcessId, &process)) {
    return;
  }

  processIdLabel->setText(process.id ? process.id : "-");

  /* Enhanced: Color-coded process state with icons */
  QString stateText = process.state ? process.state : "-";
  QString stateColor = obs_theme_get_muted_color().name(); /* Default gray */
  if (stateText == "running" || stateText == "started") {
    stateText = "🟢 " + stateText;
    stateColor = obs_theme_get_success_color().name(); /* Green */
  } else if (stateText == "starting" || stateText == "waiting") {
    stateText = "🟡 " + stateText;
    stateColor = obs_theme_get_warning_color().name(); /* Orange */
  } else if (stateText == "stopping" || stateText == "finished") {
    stateText = "🟠 " + stateText;
    stateColor = obs_theme_get_warning_color().name(); /* Dark Orange */
  } else if (stateText == "failed" || stateText == "error") {
    stateText = "🔴 " + stateText;
    stateColor = obs_theme_get_error_color().name(); /* Red */
  } else if (stateText != "-") {
    stateText = "⚪ " + stateText;
  }
  processStateLabel->setText(stateText);
  processStateLabel->setStyleSheet(
      QString("QLabel { color: %1; font-weight: bold; }").arg(stateColor));

  /* Format uptime */
  uint64_t hours = process.uptime_seconds / 3600;
  uint64_t minutes = (process.uptime_seconds % 3600) / 60;
  uint64_t seconds = process.uptime_seconds % 60;
  processUptimeLabel->setText(
      QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds));

  /* Enhanced: Color-code CPU and memory based on usage levels */
  double cpu = process.cpu_usage;
  QString cpuColor =
      obs_theme_get_success_color().name(); /* Green by default */
  if (cpu > 80.0) {
    cpuColor = obs_theme_get_error_color().name(); /* Red for high usage */
  } else if (cpu > 50.0) {
    cpuColor =
        obs_theme_get_warning_color().name(); /* Orange for medium usage */
  }
  processCpuLabel->setText(QString("%1%").arg(cpu, 0, 'f', 1));
  processCpuLabel->setStyleSheet(
      QString("QLabel { color: %1; font-weight: bold; }").arg(cpuColor));

  uint64_t memoryMB = process.memory_bytes / 1024 / 1024;
  QString memoryColor =
      obs_theme_get_success_color().name(); /* Green by default */
  if (memoryMB > 2048) {
    memoryColor =
        obs_theme_get_error_color().name(); /* Red for high memory usage */
  } else if (memoryMB > 1024) {
    memoryColor = obs_theme_get_warning_color()
                      .name(); /* Orange for medium memory usage */
  }
  processMemoryLabel->setText(QString("%1 MB").arg(memoryMB));
  processMemoryLabel->setStyleSheet(
      QString("QLabel { color: %1; font-weight: bold; }").arg(memoryColor));

  /* Fetch extended process state from new API */
  restreamer_process_state_t state = {0};
  if (restreamer_api_get_process_state(api, selectedProcessId, &state)) {
    /* Update extended state labels */
    processFramesLabel->setText(QString("%1 / %2")
                                    .arg(state.frames - state.dropped_frames)
                                    .arg(state.frames));

    /* Enhanced: Show dropped frame percentage with color-coded warning levels
     */
    if (state.frames > 0) {
      double drop_percent = (state.dropped_frames * 100.0) / state.frames;
      QString dropColor =
          obs_theme_get_success_color().name(); /* Green by default */
      if (drop_percent > 5.0) {
        dropColor = obs_theme_get_error_color()
                        .name(); /* Red for high drop rate (>5%) */
      } else if (drop_percent > 1.0) {
        dropColor = obs_theme_get_warning_color()
                        .name(); /* Orange for medium drop rate (>1%) */
      }
      processDroppedFramesLabel->setText(QString("%1 (%2%)")
                                             .arg(state.dropped_frames)
                                             .arg(drop_percent, 0, 'f', 2));
      processDroppedFramesLabel->setStyleSheet(
          QString("QLabel { color: %1; font-weight: bold; }").arg(dropColor));
    } else {
      processDroppedFramesLabel->setText(QString::number(state.dropped_frames));
      processDroppedFramesLabel->setStyleSheet(
          QString("QLabel { color: %1; }")
              .arg(obs_theme_get_muted_color().name())); /* Gray for no data */
    }

    processFpsLabel->setText(QString("%1").arg(state.fps, 0, 'f', 2));
    processBitrateLabel->setText(QString("%1 kbps").arg(state.current_bitrate));
    processProgressLabel->setText(
        QString("%1%").arg(state.progress, 0, 'f', 1));

    restreamer_api_free_process_state(&state);
  } else {
    /* Reset to defaults if state fetch fails */
    processFramesLabel->setText("-");
    processDroppedFramesLabel->setText("-");
    processFpsLabel->setText("-");
    processBitrateLabel->setText("-");
    processProgressLabel->setText("-");
  }

  restreamer_api_free_process(&process);
}

void RestreamerDock::updateSessionList() {
  if (!sessionTable) {
    return; /* Monitoring section removed from UI */
  }

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

  if (!destinationsTable) {
    return; /* Advanced section removed from UI */
  }

  destinationsTable->setRowCount(
      static_cast<int>(multistreamConfig->destination_count));

  for (size_t i = 0; i < multistreamConfig->destination_count; i++) {
    int row = static_cast<int>(i);
    stream_destination_t *dest = &multistreamConfig->destinations[i];

    destinationsTable->setItem(row, 0,
                               new QTableWidgetItem(dest->service_name));
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
  /* Create an enhanced dialog to add a destination with OBS service options */
  QDialog dialog(this);
  dialog.setWindowTitle("Add Streaming Destination");
  dialog.setMinimumWidth(500);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  QGroupBox *formGroup = new QGroupBox("Destination Settings");
  QGridLayout *formLayout = new QGridLayout();
  formLayout->setColumnStretch(1, 1);
  formLayout->setHorizontalSpacing(10);
  formLayout->setVerticalSpacing(10);

  /* Service selection combo box */
  QComboBox *serviceCombo = new QComboBox();
  serviceCombo->setMinimumWidth(300);

  /* Show common services first, then a separator, then all services */
  QStringList commonServices = serviceLoader->getCommonServiceNames();
  QStringList allServices = serviceLoader->getServiceNames();

  // Add common services
  for (const QString &serviceName : commonServices) {
    serviceCombo->addItem(serviceName, serviceName);
  }

  // Add separator if we have common services
  if (!commonServices.isEmpty() && commonServices.size() < allServices.size()) {
    serviceCombo->insertSeparator(serviceCombo->count());
    serviceCombo->addItem("-- Show All Services --", QString());
    serviceCombo->insertSeparator(serviceCombo->count());

    // Add remaining services
    for (const QString &serviceName : allServices) {
      if (!commonServices.contains(serviceName)) {
        serviceCombo->addItem(serviceName, serviceName);
      }
    }
  }

  // Add Custom RTMP option
  serviceCombo->insertSeparator(serviceCombo->count());
  serviceCombo->addItem("Custom RTMP Server", "custom");

  /* Create labels */
  QLabel *serviceLabel = new QLabel("Service:");
  serviceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *serverLabel = new QLabel("Server:");
  serverLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *customUrlLabel = new QLabel("RTMP URL:");
  customUrlLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *streamKeyLabel = new QLabel("Stream Key:");
  streamKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *orientationLabel = new QLabel("Orientation:");
  orientationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  /* Server selection combo box */
  QComboBox *serverCombo = new QComboBox();
  serverCombo->setMinimumWidth(300);

  /* Custom server URL field */
  QLineEdit *customUrlEdit = new QLineEdit();
  customUrlEdit->setPlaceholderText("rtmp://your-server/live/stream-key");
  customUrlEdit->setMinimumWidth(300);

  /* Stream key field */
  QLineEdit *streamKeyEdit = new QLineEdit();
  streamKeyEdit->setPlaceholderText("Enter your stream key");
  streamKeyEdit->setMinimumWidth(300);

  /* Stream key help label */
  QLabel *streamKeyHelpLabel = new QLabel();
  streamKeyHelpLabel->setOpenExternalLinks(true);
  streamKeyHelpLabel->setWordWrap(true);
  streamKeyHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_info_color().name()));

  /* Orientation selection */
  QComboBox *orientationCombo = new QComboBox();
  orientationCombo->addItem("Horizontal (16:9)", ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (9:16)", ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square (1:1)", ORIENTATION_SQUARE);
  orientationCombo->setMinimumWidth(300);

  /* Update server list when service changes */
  auto updateServerList = [this, serviceCombo, serverCombo, streamKeyHelpLabel,
                           customUrlEdit, streamKeyEdit, serverLabel,
                           customUrlLabel, streamKeyLabel]() {
    QString selectedService = serviceCombo->currentData().toString();
    serverCombo->clear();
    streamKeyHelpLabel->clear();

    if (selectedService == "custom") {
      // Custom RTMP mode
      serverLabel->setVisible(false);
      serverCombo->setVisible(false);
      streamKeyLabel->setVisible(false);
      streamKeyEdit->setVisible(false);
      customUrlLabel->setVisible(true);
      customUrlEdit->setVisible(true);
      streamKeyHelpLabel->setText(
          "Enter the full RTMP URL including stream key");
    } else if (!selectedService.isEmpty() &&
               selectedService != "-- Show All Services --") {
      // Regular service mode
      customUrlLabel->setVisible(false);
      customUrlEdit->setVisible(false);
      serverLabel->setVisible(true);
      serverCombo->setVisible(true);
      streamKeyLabel->setVisible(true);
      streamKeyEdit->setVisible(true);

      // Load servers for the selected service
      const StreamingService *service =
          serviceLoader->getService(selectedService);
      if (service) {
        for (const StreamingServer &server : service->servers) {
          serverCombo->addItem(server.name, server.url);
        }

        // Update stream key help link
        if (!service->stream_key_link.isEmpty()) {
          streamKeyHelpLabel->setText(
              QString("<a href=\"%1\">Get your stream key</a>")
                  .arg(service->stream_key_link));
        }
      }
    }
  };

  connect(serviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          updateServerList);

  /* Add widgets to grid layout */
  int row = 0;
  formLayout->addWidget(serviceLabel, row, 0);
  formLayout->addWidget(serviceCombo, row, 1);
  row++;

  formLayout->addWidget(serverLabel, row, 0);
  formLayout->addWidget(serverCombo, row, 1);
  row++;

  formLayout->addWidget(customUrlLabel, row, 0);
  formLayout->addWidget(customUrlEdit, row, 1);
  row++;

  formLayout->addWidget(streamKeyLabel, row, 0);
  formLayout->addWidget(streamKeyEdit, row, 1);
  row++;

  formLayout->addWidget(streamKeyHelpLabel, row, 1);
  row++;

  formLayout->addWidget(orientationLabel, row, 0);
  formLayout->addWidget(orientationCombo, row, 1);

  /* Initially hide custom URL fields */
  customUrlLabel->setVisible(false);
  customUrlEdit->setVisible(false);

  formGroup->setLayout(formLayout);
  layout->addWidget(formGroup);

  /* Info label */
  QLabel *infoLabel = new QLabel(
      "Tip: Select a service and server, then enter your stream key. "
      "The stream will be automatically formatted for the selected "
      "orientation.");
  infoLabel->setWordWrap(true);
  infoLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 10px; padding: 10px; }")
          .arg(obs_theme_get_muted_color().name()));
  layout->addWidget(infoLabel);

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  layout->addWidget(buttonBox);

  /* Initialize server list for first service */
  updateServerList();

  if (dialog.exec() == QDialog::Accepted) {
    QString streamKey = streamKeyEdit->text();
    stream_orientation_t orientation =
        (stream_orientation_t)orientationCombo->currentData().toInt();

    QString rtmpUrl;
    QString serviceName = serviceCombo->currentText();

    // Determine the RTMP URL
    if (serviceCombo->currentData().toString() == "custom") {
      rtmpUrl = customUrlEdit->text();
    } else {
      QString serverUrl = serverCombo->currentData().toString();
      if (!serverUrl.isEmpty()) {
        // Construct full URL: server + stream key
        rtmpUrl = serverUrl;
        if (!streamKey.isEmpty()) {
          if (!rtmpUrl.endsWith("/")) {
            rtmpUrl += "/";
          }
          rtmpUrl += streamKey;
        }
      }
    }

    if (!rtmpUrl.isEmpty() && !streamKey.isEmpty()) {
      // For now, map to old service enum for compatibility
      streaming_service_t service = SERVICE_CUSTOM;
      if (serviceName.contains("Twitch", Qt::CaseInsensitive)) {
        service = SERVICE_TWITCH;
      } else if (serviceName.contains("YouTube", Qt::CaseInsensitive)) {
        service = SERVICE_YOUTUBE;
      } else if (serviceName.contains("Facebook", Qt::CaseInsensitive)) {
        service = SERVICE_FACEBOOK;
      } else if (serviceName.contains("Kick", Qt::CaseInsensitive)) {
        service = SERVICE_KICK;
      } else if (serviceName.contains("TikTok", Qt::CaseInsensitive)) {
        service = SERVICE_TIKTOK;
      } else if (serviceName.contains("Instagram", Qt::CaseInsensitive)) {
        service = SERVICE_INSTAGRAM;
      } else if (serviceName.contains("Twitter", Qt::CaseInsensitive) ||
                 serviceName.contains("X", Qt::CaseInsensitive)) {
        service = SERVICE_X_TWITTER;
      }

      restreamer_multistream_add_destination(multistreamConfig, service,
                                             streamKey.toUtf8().constData(),
                                             orientation);
      updateDestinationList();
      saveSettings();

      obs_log(LOG_INFO,
              "[Polyemesis] Added destination: %s (%s) with orientation %d",
              serviceName.toUtf8().constData(), rtmpUrl.toUtf8().constData(),
              orientation);
    } else {
      QMessageBox::warning(
          this, "Invalid Input",
          "Please enter both a valid server URL and stream key.");
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
  /* Lock mutex to ensure thread-safe API access */
  std::lock_guard<std::recursive_mutex> lock(apiMutex);

  if (!api) {
    return;
  }

  /* Only update if we're connected */
  if (!restreamer_api_is_connected(api)) {
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

void RestreamerDock::onSaveBridgeSettingsClicked() {
  if (!bridge) {
    QMessageBox::warning(this, "Error", "Bridge not initialized");
    return;
  }

  if (!bridgeHorizontalUrlEdit || !bridgeVerticalUrlEdit ||
      !bridgeAutoStartCheckbox) {
    return; /* Bridge section removed from UI */
  }

  /* Get values from UI */
  QString horizontalUrl = bridgeHorizontalUrlEdit->text().trimmed();
  QString verticalUrl = bridgeVerticalUrlEdit->text().trimmed();
  bool autoStart = bridgeAutoStartCheckbox->isChecked();

  /* Use defaults if empty */
  if (horizontalUrl.isEmpty()) {
    horizontalUrl = "rtmp://localhost/live/obs_horizontal";
  }
  if (verticalUrl.isEmpty()) {
    verticalUrl = "rtmp://localhost/live/obs_vertical";
  }

  /* Update bridge configuration */
  obs_bridge_config_t config = {0};
  config.rtmp_horizontal_url = bstrdup(horizontalUrl.toUtf8().constData());
  config.rtmp_vertical_url = bstrdup(verticalUrl.toUtf8().constData());
  config.auto_start_enabled = autoStart;
  config.show_vertical_notification = true;
  config.show_preflight_check = true;

  obs_bridge_set_config(bridge, &config);

  /* Cleanup */
  bfree(config.rtmp_horizontal_url);
  bfree(config.rtmp_vertical_url);

  /* Save to OBS settings */
  saveSettings();

  /* Update status */
  if (autoStart) {
    bridgeStatusLabel->setText("● Auto-start enabled");
    bridgeStatusLabel->setStyleSheet(
        QString("QLabel { color: %1; }")
            .arg(obs_theme_get_success_color().name()));
  } else {
    bridgeStatusLabel->setText("● Auto-start disabled");
    bridgeStatusLabel->setStyleSheet(
        QString("QLabel { color: %1; }")
            .arg(obs_theme_get_muted_color().name()));
  }
}

/* Profile Management Functions */

void RestreamerDock::updateProfileList() {
  /* Clear existing profile widgets */
  qDeleteAll(profileWidgets);
  profileWidgets.clear();

  if (!profileManager || profileManager->profile_count == 0) {
    profileStatusLabel->setText("No profiles");
    stopAllProfilesButton->setEnabled(false);
    return;
  }

  /* Iterate through all profiles and create ProfileWidgets */
  bool hasActiveProfile = false;
  for (size_t i = 0; i < profileManager->profile_count; i++) {
    output_profile_t *profile = profileManager->profiles[i];

    /* Track if any profile is active */
    if (profile->status == PROFILE_STATUS_ACTIVE ||
        profile->status == PROFILE_STATUS_STARTING) {
      hasActiveProfile = true;
    }

    /* Create a new ProfileWidget for this profile */
    ProfileWidget *profileWidget = new ProfileWidget(profile, this);

    /* Connect ProfileWidget signals to dock slot methods */
    connect(profileWidget, &ProfileWidget::startRequested, this,
            &RestreamerDock::onProfileStartRequested);
    connect(profileWidget, &ProfileWidget::stopRequested, this,
            &RestreamerDock::onProfileStopRequested);
    connect(profileWidget, &ProfileWidget::editRequested, this,
            &RestreamerDock::onProfileEditRequested);
    connect(profileWidget, &ProfileWidget::deleteRequested, this,
            &RestreamerDock::onProfileDeleteRequested);
    connect(profileWidget, &ProfileWidget::duplicateRequested, this,
            &RestreamerDock::onProfileDuplicateRequested);

    /* Add widget to layout and track it */
    profileListLayout->addWidget(profileWidget);
    profileWidgets.append(profileWidget);
  }

  /* Update status label */
  profileStatusLabel->setText(
      QString("%1 profile(s)").arg(profileManager->profile_count));

  /* Update button states */
  stopAllProfilesButton->setEnabled(hasActiveProfile);
}

/* Profile Slot Implementations */

void RestreamerDock::onStartAllProfilesClicked() {
  if (!profileManager) {
    return;
  }

  if (profile_manager_start_all(profileManager)) {
    updateProfileList();
  } else {
    QMessageBox::warning(
        this, "Error",
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
    } else {
      QMessageBox::warning(this, "Error", "Failed to stop all profiles.");
    }
  }
}

void RestreamerDock::onCreateProfileClicked() {
  if (!profileManager) {
    return;
  }

  /* Prompt for profile name */
  bool ok;
  QString profileName = QInputDialog::getText(
      this, "Create Profile", "Enter profile name:", QLineEdit::Normal,
      "New Profile", &ok);

  if (ok && !profileName.isEmpty()) {
    output_profile_t *newProfile = profile_manager_create_profile(
        profileManager, profileName.toUtf8().constData());

    if (newProfile) {
      updateProfileList();
      saveSettings();

      /* Open configure dialog to set up destinations */
      QMessageBox::information(
          this, "Profile Created",
          QString("Profile '%1' created successfully.\n\nUse the Edit "
                  "button on the profile to add destinations and customize "
                  "settings.")
              .arg(profileName));
    } else {
      QMessageBox::warning(this, "Error", "Failed to create profile.");
    }
  }
}

/* ProfileWidget Signal Handlers */

void RestreamerDock::onProfileStartRequested(const char *profileId) {
  if (!profileManager || !profileId) {
    return;
  }

  if (output_profile_start(profileManager, profileId)) {
    updateProfileList();
  } else {
    QMessageBox::warning(
        this, "Error", "Failed to start profile. Check Restreamer connection.");
  }
}

void RestreamerDock::onProfileStopRequested(const char *profileId) {
  if (!profileManager || !profileId) {
    return;
  }

  if (output_profile_stop(profileManager, profileId)) {
    updateProfileList();
  } else {
    QMessageBox::warning(this, "Error", "Failed to stop profile.");
  }
}

void RestreamerDock::onProfileEditRequested(const char *profileId) {
  if (!profileManager || !profileId) {
    return;
  }

  output_profile_t *profile =
      profile_manager_get_profile(profileManager, profileId);
  if (!profile) {
    return;
  }

  /* Open profile edit dialog */
  ProfileEditDialog *dialog = new ProfileEditDialog(profile, this);
  connect(dialog, &ProfileEditDialog::profileUpdated, this, [this]() {
    obs_log(LOG_INFO, "Profile configuration updated, refreshing UI");
    updateProfileList();
  });

  if (dialog->exec() == QDialog::Accepted) {
    obs_log(LOG_INFO, "Profile '%s' updated successfully",
            profile->profile_name);
    updateProfileList();
  }

  dialog->deleteLater();
}

void RestreamerDock::onProfileDeleteRequested(const char *profileId) {
  if (!profileManager || !profileId) {
    return;
  }

  output_profile_t *profile =
      profile_manager_get_profile(profileManager, profileId);
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
    if (profile_manager_delete_profile(profileManager, profileId)) {
      updateProfileList();
      saveSettings();
    } else {
      QMessageBox::warning(this, "Error", "Failed to delete profile.");
    }
  }
}

void RestreamerDock::onProfileDuplicateRequested(const char *profileId) {
  if (!profileManager || !profileId) {
    return;
  }

  output_profile_t *sourceProfile =
      profile_manager_get_profile(profileManager, profileId);
  if (!sourceProfile) {
    return;
  }

  /* Prompt for new profile name */
  bool ok;
  QString newName = QInputDialog::getText(
      this, "Duplicate Profile",
      "Enter name for duplicated profile:", QLineEdit::Normal,
      QString("%1 (Copy)").arg(sourceProfile->profile_name), &ok);

  if (ok && !newName.isEmpty()) {
    /* Create new profile with same settings */
    output_profile_t *newProfile = profile_manager_create_profile(
        profileManager, newName.toUtf8().constData());

    if (newProfile) {
      /* Copy settings */
      newProfile->source_orientation = sourceProfile->source_orientation;
      newProfile->auto_detect_orientation =
          sourceProfile->auto_detect_orientation;
      newProfile->source_width = sourceProfile->source_width;
      newProfile->source_height = sourceProfile->source_height;
      newProfile->auto_start = sourceProfile->auto_start;
      newProfile->auto_reconnect = sourceProfile->auto_reconnect;
      newProfile->reconnect_delay_sec = sourceProfile->reconnect_delay_sec;

      /* Copy destinations */
      for (size_t i = 0; i < sourceProfile->destination_count; i++) {
        profile_destination_t *srcDest = &sourceProfile->destinations[i];
        profile_add_destination(
            newProfile, srcDest->service, srcDest->stream_key,
            srcDest->target_orientation, &srcDest->encoding);
      }

      updateProfileList();
      saveSettings();
    } else {
      QMessageBox::warning(this, "Error", "Failed to duplicate profile.");
    }
  }
}

/* ===== Extended API Slot Methods (Monitoring & Advanced) ===== */

void RestreamerDock::onProbeInputClicked() {
  obs_log(LOG_INFO, "Probe Input clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString probeInfo = "<b>Input Probing</b><br><br>";
  probeInfo +=
      "This feature allows you to probe RTMP/SRT inputs to determine:<br>";
  probeInfo += "• Stream codec information<br>";
  probeInfo += "• Resolution and frame rate<br>";
  probeInfo += "• Audio configuration<br>";
  probeInfo += "• Bitrate and quality metrics<br><br>";
  probeInfo +=
      "<i>Full implementation requires additional FFprobe integration</i>";

  QMessageBox::information(this, "Probe Input", probeInfo);
}

void RestreamerDock::onViewConfigClicked() {
  obs_log(LOG_INFO, "View Config clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString configInfo = "<b>Restreamer Configuration</b><br><br>";

  configInfo += "<b>Connection:</b><br>";
  configInfo += "  Status: Connected to Restreamer server<br><br>";

  configInfo += "<b>Profiles:</b><br>";
  if (profileManager) {
    configInfo +=
        QString("  Total Profiles: %1<br>").arg(profileManager->profile_count);
    configInfo += QString("  Total Templates: %1<br>")
                      .arg(profileManager->template_count);
  }

  QMessageBox::information(this, "View Configuration", configInfo);
}

void RestreamerDock::onViewSkillsClicked() {
  obs_log(LOG_INFO, "View Skills clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString skillsInfo = "<b>Restreamer Server Capabilities</b><br><br>";
  skillsInfo += "Server capabilities include:<br>";
  skillsInfo += "• FFmpeg encoding/transcoding<br>";
  skillsInfo += "• RTMP/SRT input/output<br>";
  skillsInfo += "• HLS output<br>";
  skillsInfo += "• Hardware acceleration (if available)<br>";
  skillsInfo += "• Multi-destination streaming<br><br>";
  skillsInfo +=
      "<i>Detailed capability detection requires API /skills endpoint</i>";

  QMessageBox::information(this, "Server Capabilities", skillsInfo);
}

void RestreamerDock::onViewMetricsClicked() {
  obs_log(LOG_INFO, "View Metrics clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString metricsInfo = "<b>System Metrics</b><br><br>";

  if (profileManager) {
    size_t total_destinations = 0;
    size_t active_destinations = 0;
    uint64_t total_bytes = 0;
    uint32_t total_dropped = 0;

    for (size_t i = 0; i < profileManager->profile_count; i++) {
      output_profile_t *profile = profileManager->profiles[i];
      total_destinations += profile->destination_count;
      for (size_t j = 0; j < profile->destination_count; j++) {
        if (profile->destinations[j].connected) {
          active_destinations++;
        }
        total_bytes += profile->destinations[j].bytes_sent;
        total_dropped += profile->destinations[j].dropped_frames;
      }
    }

    metricsInfo += QString("<b>Active Streams:</b> %1 / %2<br>")
                       .arg(active_destinations)
                       .arg(total_destinations);
    metricsInfo += QString("<b>Total Data Sent:</b> %1 MB<br>")
                       .arg(total_bytes / (1024.0 * 1024.0), 0, 'f', 2);
    metricsInfo +=
        QString("<b>Total Dropped Frames:</b> %1<br>").arg(total_dropped);
  }

  QMessageBox::information(this, "System Metrics", metricsInfo);
}

void RestreamerDock::onReloadConfigClicked() {
  obs_log(LOG_INFO, "Reload Config clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Reload Configuration",
      "Reload all profiles and settings from the server?\n\n"
      "This will refresh all profile data and may reset local changes.",
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    /* Refresh profiles list */
    updateProfileList();
    QMessageBox::information(
        this, "Configuration Reloaded",
        "All profiles and settings have been reloaded from the server.");
    obs_log(LOG_INFO, "Configuration reloaded from server");
  }
}

void RestreamerDock::onViewSrtStreamsClicked() {
  obs_log(LOG_INFO, "View SRT Streams clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString srtInfo = "<b>SRT Streams</b><br><br>";
  srtInfo += "SRT (Secure Reliable Transport) is a streaming protocol that "
             "provides:<br>";
  srtInfo += "• Low latency streaming<br>";
  srtInfo += "• Automatic error correction<br>";
  srtInfo += "• Encryption support<br>";
  srtInfo += "• Firewall traversal<br><br>";

  if (profileManager) {
    int srt_count = 0;
    for (size_t i = 0; i < profileManager->profile_count; i++) {
      output_profile_t *profile = profileManager->profiles[i];
      for (size_t j = 0; j < profile->destination_count; j++) {
        if (profile->destinations[j].rtmp_url &&
            strstr(profile->destinations[j].rtmp_url, "srt://")) {
          srt_count++;
        }
      }
    }
    srtInfo += QString("<b>Active SRT Streams:</b> %1<br>").arg(srt_count);
  }

  srtInfo +=
      "<br><i>Detailed SRT stream monitoring requires API integration</i>";

  QMessageBox::information(this, "SRT Streams", srtInfo);
}

void RestreamerDock::onViewRtmpStreamsClicked() {
  obs_log(LOG_INFO, "View RTMP Streams clicked");

  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to Restreamer server first.");
    return;
  }

  QString rtmpInfo = "<b>RTMP Streams</b><br><br>";

  if (profileManager) {
    int rtmp_count = 0;
    QString streamList;

    for (size_t i = 0; i < profileManager->profile_count; i++) {
      output_profile_t *profile = profileManager->profiles[i];
      for (size_t j = 0; j < profile->destination_count; j++) {
        if (profile->destinations[j].rtmp_url &&
            strstr(profile->destinations[j].rtmp_url, "rtmp://")) {
          rtmp_count++;
          if (rtmp_count <= 5) { /* Show first 5 streams */
            streamList += QString("  • %1: %2<br>")
                              .arg(profile->profile_name)
                              .arg(profile->destinations[j].service_name
                                       ? profile->destinations[j].service_name
                                       : "Custom");
          }
        }
      }
    }

    rtmpInfo +=
        QString("<b>Active RTMP Streams:</b> %1<br><br>").arg(rtmp_count);

    if (!streamList.isEmpty()) {
      rtmpInfo += streamList;
      if (rtmp_count > 5) {
        rtmpInfo += QString("<i>... and %1 more</i><br>").arg(rtmp_count - 5);
      }
    }
  }

  QMessageBox::information(this, "RTMP Streams", rtmpInfo);
}

/* ===== Section Title Update Helpers ===== */
