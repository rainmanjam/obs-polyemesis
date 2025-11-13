#include "restreamer-dock.h"
#include "collapsible-section.h"
#include "obs-helpers.hpp"
#include "obs-theme-utils.h"
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

    /* Save connection settings */
    obs_data_set_string(dock_settings, "host",
                        hostEdit->text().toUtf8().constData());
    obs_data_set_int(dock_settings, "port", portEdit->text().toInt());
    obs_data_set_bool(dock_settings, "use_https", httpsCheckbox->isChecked());
    obs_data_set_string(dock_settings, "username",
                        usernameEdit->text().toUtf8().constData());
    obs_data_set_string(dock_settings, "password",
                        passwordEdit->text().toUtf8().constData());

    /* Save bridge settings */
    obs_data_set_string(dock_settings, "bridge_horizontal_url",
                        bridgeHorizontalUrlEdit->text().toUtf8().constData());
    obs_data_set_string(dock_settings, "bridge_vertical_url",
                        bridgeVerticalUrlEdit->text().toUtf8().constData());
    obs_data_set_bool(dock_settings, "bridge_auto_start",
                      bridgeAutoStartCheckbox->isChecked());

    /* Enhanced: Save last active profile for quick restoration */
    if (profileListWidget->currentItem()) {
      QString profileId =
          profileListWidget->currentItem()->data(Qt::UserRole).toString();
      obs_data_set_string(dock_settings, "last_active_profile",
                          profileId.toUtf8().constData());
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
      /* Restore connection settings */
      const char *host = obs_data_get_string(dock_settings, "host");
      if (host && *host) {
        hostEdit->setText(host);
      }

      int port = obs_data_get_int(dock_settings, "port");
      if (port > 0) {
        portEdit->setText(QString::number(port));
      }

      httpsCheckbox->setChecked(obs_data_get_bool(dock_settings, "use_https"));

      const char *username = obs_data_get_string(dock_settings, "username");
      if (username && *username) {
        usernameEdit->setText(username);
      }

      const char *password = obs_data_get_string(dock_settings, "password");
      if (password && *password) {
        passwordEdit->setText(password);
      }

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

      /* Enhanced: Restore last active profile selection */
      const char *last_profile =
          obs_data_get_string(dock_settings, "last_active_profile");
      if (last_profile && *last_profile) {
        for (int i = 0; i < profileListWidget->count(); i++) {
          QListWidgetItem *item = profileListWidget->item(i);
          if (item && item->data(Qt::UserRole).toString() == last_profile) {
            profileListWidget->setCurrentItem(item);
            obs_log(LOG_DEBUG, "Restored last active profile: %s",
                    last_profile);
            break;
          }
        }
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

  /* ===== Tab 1: Connection (Setup - Step 1) ===== */
  QWidget *connectionTab = new QWidget();
  QVBoxLayout *connectionTabLayout = new QVBoxLayout(connectionTab);

  /* Add help label for consistency with other tabs */
  QLabel *connectionHelpLabel =
      new QLabel("Configure connection to Restreamer server");
  QString mutedColor = obs_theme_get_muted_color().name();
  connectionHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }").arg(mutedColor));
  connectionHelpLabel->setAlignment(Qt::AlignCenter);
  connectionTabLayout->addWidget(connectionHelpLabel);

  /* ===== Sub-group 1: Server Configuration ===== */
  QGroupBox *serverConfigGroup = new QGroupBox("Server Configuration");
  QVBoxLayout *serverConfigLayout = new QVBoxLayout();

  /* Center all form fields */
  QFormLayout *connectionFormLayout = new QFormLayout();
  connectionFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  connectionFormLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  connectionFormLayout->setLabelAlignment(Qt::AlignRight);

  hostEdit = new QLineEdit();
  hostEdit->setPlaceholderText("localhost");
  hostEdit->setToolTip("Restreamer server hostname or IP address");
  hostEdit->setMaximumWidth(300);
  hostEdit->setMinimumHeight(30); /* Taller field */
  hostEdit->setFrame(true); /* Border box */
  hostEdit->setStyleSheet("QLineEdit { border: 1px solid palette(mid); padding: 4px; }");

  portEdit = new QLineEdit();
  portEdit->setPlaceholderText("8080");
  portEdit->setToolTip("Restreamer server port (1-65535)");
  portEdit->setMaximumWidth(300);
  portEdit->setMinimumHeight(30); /* Taller field */
  portEdit->setFrame(true); /* Border box */
  portEdit->setStyleSheet("QLineEdit { border: 1px solid palette(mid); padding: 4px; }");
  /* Add port validator to ensure only valid port numbers are entered */
  QIntValidator *portValidator = new QIntValidator(1, 65535, portEdit);
  portEdit->setValidator(portValidator);

  httpsCheckbox = new QCheckBox();
  httpsCheckbox->setToolTip("Use HTTPS for secure connection to Restreamer");
  usernameEdit = new QLineEdit();
  usernameEdit->setPlaceholderText("admin");
  usernameEdit->setToolTip("Restreamer username for authentication");
  usernameEdit->setMaximumWidth(300);
  usernameEdit->setMinimumHeight(30); /* Taller field */
  usernameEdit->setFrame(true); /* Border box */
  usernameEdit->setStyleSheet("QLineEdit { border: 1px solid palette(mid); padding: 4px; }");

  passwordEdit = new QLineEdit();
  passwordEdit->setEchoMode(QLineEdit::Password);
  passwordEdit->setPlaceholderText("Password");
  passwordEdit->setToolTip("Restreamer password for authentication");
  passwordEdit->setMaximumWidth(300);
  passwordEdit->setMinimumHeight(30); /* Taller field */
  passwordEdit->setFrame(true); /* Border box */
  passwordEdit->setStyleSheet("QLineEdit { border: 1px solid palette(mid); padding: 4px; }");

  connectionFormLayout->addRow("Host:", hostEdit);
  connectionFormLayout->addRow("Port:", portEdit);
  connectionFormLayout->addRow("Use HTTPS:", httpsCheckbox);
  connectionFormLayout->addRow("Username:", usernameEdit);
  connectionFormLayout->addRow("Password:", passwordEdit);

  serverConfigLayout->addLayout(connectionFormLayout);
  serverConfigGroup->setLayout(serverConfigLayout);
  connectionTabLayout->addWidget(serverConfigGroup);

  /* ===== Sub-group 2: Connection Status ===== */
  QGroupBox *connectionStatusGroup = new QGroupBox("Connection Status");
  QVBoxLayout *connectionStatusLayout = new QVBoxLayout();

  /* Center the button and status */
  QHBoxLayout *connectionButtonLayout = new QHBoxLayout();
  connectionButtonLayout->addStretch();
  testConnectionButton = new QPushButton("Test Connection");
  testConnectionButton->setToolTip("Test connection to Restreamer server");
  testConnectionButton->setMinimumWidth(150);
  connectionStatusLabel = new QLabel("● Not connected");
  connectionStatusLabel->setStyleSheet(
      QString("QLabel { color: %1; }").arg(obs_theme_get_muted_color().name()));
  connectionButtonLayout->addWidget(testConnectionButton);
  connectionButtonLayout->addWidget(connectionStatusLabel);
  connectionButtonLayout->addStretch();

  connect(testConnectionButton, &QPushButton::clicked, this,
          &RestreamerDock::onTestConnectionClicked);

  connectionStatusLayout->addLayout(connectionButtonLayout);
  connectionStatusGroup->setLayout(connectionStatusLayout);
  connectionTabLayout->addWidget(connectionStatusGroup);

  connectionTabLayout->addStretch();

  /* Add Connection tab to collapsible section */
  connectionSection = new CollapsibleSection("Connection");

  /* Add quick action button to Connection header */
  QPushButton *quickTestConnectionButton = new QPushButton("Test");
  quickTestConnectionButton->setMaximumWidth(60);
  quickTestConnectionButton->setToolTip("Test connection to Restreamer server");
  connect(quickTestConnectionButton, &QPushButton::clicked, this,
          &RestreamerDock::onTestConnectionClicked);
  connectionSection->addHeaderButton(quickTestConnectionButton);

  connectionSection->setContent(connectionTab);
  connectionSection->setExpanded(true, false); /* Expanded by default */
  verticalLayout->addWidget(connectionSection);

  /* ===== Tab 2: Bridge Settings ===== */
  QWidget *bridgeTab = new QWidget();
  QVBoxLayout *bridgeTabLayout = new QVBoxLayout(bridgeTab);

  QLabel *bridgeHelpLabel =
      new QLabel("Configure automatic RTMP bridge from OBS to Restreamer");
  bridgeHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_muted_color().name()));
  bridgeHelpLabel->setAlignment(Qt::AlignCenter);
  bridgeTabLayout->addWidget(bridgeHelpLabel);

  /* ===== Sub-group 1: Bridge Configuration ===== */
  QGroupBox *bridgeConfigGroup = new QGroupBox("Bridge Configuration");
  QVBoxLayout *bridgeConfigLayout = new QVBoxLayout();

  /* Center all form fields */
  QFormLayout *bridgeFormLayout = new QFormLayout();
  bridgeFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  bridgeFormLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  bridgeFormLayout->setLabelAlignment(Qt::AlignRight);

  bridgeHorizontalUrlEdit = new QLineEdit();
  bridgeHorizontalUrlEdit->setPlaceholderText(
      "rtmp://localhost/live/obs_horizontal");
  bridgeHorizontalUrlEdit->setToolTip(
      "RTMP URL for horizontal (landscape) video format");
  bridgeHorizontalUrlEdit->setMaximumWidth(350);

  /* Add copy button for horizontal URL */
  QHBoxLayout *horizontalUrlLayout = new QHBoxLayout();
  horizontalUrlLayout->addWidget(bridgeHorizontalUrlEdit);
  QPushButton *copyHorizontalButton = new QPushButton("Copy");
  copyHorizontalButton->setMaximumWidth(60);
  copyHorizontalButton->setToolTip("Copy horizontal RTMP URL to clipboard");
  connect(copyHorizontalButton, &QPushButton::clicked, this, [this]() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(bridgeHorizontalUrlEdit->text());
  });
  horizontalUrlLayout->addWidget(copyHorizontalButton);

  bridgeVerticalUrlEdit = new QLineEdit();
  bridgeVerticalUrlEdit->setPlaceholderText(
      "rtmp://localhost/live/obs_vertical");
  bridgeVerticalUrlEdit->setToolTip(
      "RTMP URL for vertical (portrait) video format");
  bridgeVerticalUrlEdit->setMaximumWidth(350);

  /* Add copy button for vertical URL */
  QHBoxLayout *verticalUrlLayout = new QHBoxLayout();
  verticalUrlLayout->addWidget(bridgeVerticalUrlEdit);
  QPushButton *copyVerticalButton = new QPushButton("Copy");
  copyVerticalButton->setMaximumWidth(60);
  copyVerticalButton->setToolTip("Copy vertical RTMP URL to clipboard");
  connect(copyVerticalButton, &QPushButton::clicked, this, [this]() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(bridgeVerticalUrlEdit->text());
  });
  verticalUrlLayout->addWidget(copyVerticalButton);

  bridgeAutoStartCheckbox = new QCheckBox();
  bridgeAutoStartCheckbox->setChecked(true);
  bridgeAutoStartCheckbox->setToolTip(
      "Automatically start RTMP outputs when OBS streaming starts");

  bridgeFormLayout->addRow("Horizontal RTMP URL:", horizontalUrlLayout);
  bridgeFormLayout->addRow("Vertical RTMP URL:", verticalUrlLayout);
  bridgeFormLayout->addRow("Auto-start on stream:", bridgeAutoStartCheckbox);

  bridgeConfigLayout->addLayout(bridgeFormLayout);

  /* Center the save button */
  QHBoxLayout *bridgeSaveButtonLayout = new QHBoxLayout();
  bridgeSaveButtonLayout->addStretch();
  saveBridgeSettingsButton = new QPushButton("Save Settings");
  saveBridgeSettingsButton->setMinimumWidth(150);
  saveBridgeSettingsButton->setToolTip("Save bridge configuration");
  connect(saveBridgeSettingsButton, &QPushButton::clicked, this,
          &RestreamerDock::onSaveBridgeSettingsClicked);
  bridgeSaveButtonLayout->addWidget(saveBridgeSettingsButton);
  bridgeSaveButtonLayout->addStretch();

  bridgeConfigLayout->addLayout(bridgeSaveButtonLayout);
  bridgeConfigGroup->setLayout(bridgeConfigLayout);
  bridgeTabLayout->addWidget(bridgeConfigGroup);

  /* ===== Sub-group 2: Bridge Status ===== */
  QGroupBox *bridgeStatusGroup = new QGroupBox("Bridge Status");
  QVBoxLayout *bridgeStatusLayout = new QVBoxLayout();

  bridgeStatusLabel = new QLabel("● Bridge idle");
  bridgeStatusLabel->setStyleSheet(
      QString("QLabel { color: %1; }").arg(obs_theme_get_muted_color().name()));
  bridgeStatusLabel->setAlignment(Qt::AlignCenter);

  bridgeStatusLayout->addWidget(bridgeStatusLabel);
  bridgeStatusGroup->setLayout(bridgeStatusLayout);
  bridgeTabLayout->addWidget(bridgeStatusGroup);

  bridgeTabLayout->addStretch();

  /* Add Bridge tab to collapsible section */
  bridgeSection = new CollapsibleSection("Bridge");

  /* Add quick action toggle to Bridge header */
  QPushButton *quickBridgeToggleButton = new QPushButton("Enable");
  quickBridgeToggleButton->setMaximumWidth(70);
  quickBridgeToggleButton->setCheckable(true);
  quickBridgeToggleButton->setToolTip("Toggle bridge auto-start");
  quickBridgeToggleButton->setChecked(bridgeAutoStartCheckbox->isChecked());
  connect(quickBridgeToggleButton, &QPushButton::toggled, this,
          [this, quickBridgeToggleButton](bool checked) {
            bridgeAutoStartCheckbox->setChecked(checked);
            quickBridgeToggleButton->setText(checked ? "Disable" : "Enable");
            onSaveBridgeSettingsClicked();
          });
  bridgeSection->addHeaderButton(quickBridgeToggleButton);

  bridgeSection->setContent(bridgeTab);
  bridgeSection->setExpanded(false, false); /* Collapsed by default */
  verticalLayout->addWidget(bridgeSection);

  /* ===== Tab 3: Profiles (Configure & Publish - Step 2) ===== */
  QWidget *profilesTab = new QWidget();
  QVBoxLayout *profilesTabLayout = new QVBoxLayout(profilesTab);

  QLabel *profilesHelpLabel =
      new QLabel("Create and manage streaming profiles");
  profilesHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_muted_color().name()));
  profilesHelpLabel->setAlignment(Qt::AlignCenter);
  profilesTabLayout->addWidget(profilesHelpLabel);

  /* ===== Sub-group 1: Profile Management ===== */
  QGroupBox *profileManagementGroup = new QGroupBox("Profile Management");
  QVBoxLayout *profileManagementLayout = new QVBoxLayout();

  /* Profile list */
  profileListWidget = new QListWidget();
  profileListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  profileListWidget->setMaximumHeight(100);
  connect(profileListWidget, &QListWidget::currentRowChanged, this,
          &RestreamerDock::onProfileSelected);
  connect(profileListWidget, &QListWidget::customContextMenuRequested, this,
          &RestreamerDock::onProfileListContextMenu);

  /* Profile management buttons */
  QHBoxLayout *profileManagementButtons = new QHBoxLayout();

  createProfileButton = new QPushButton("+ New");
  createProfileButton->setToolTip("Create new streaming profile");
  createProfileButton->setFixedWidth(75);

  configureProfileButton = new QPushButton("Edit");
  configureProfileButton->setToolTip("Configure profile destinations");
  configureProfileButton->setFixedWidth(75);
  configureProfileButton->setEnabled(false);

  duplicateProfileButton = new QPushButton("Copy");
  duplicateProfileButton->setToolTip("Duplicate selected profile");
  duplicateProfileButton->setFixedWidth(75);
  duplicateProfileButton->setEnabled(false);

  deleteProfileButton = new QPushButton("Delete");
  deleteProfileButton->setToolTip("Delete selected profile");
  deleteProfileButton->setFixedWidth(75);
  deleteProfileButton->setEnabled(false);

  connect(createProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onCreateProfileClicked);
  connect(deleteProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onDeleteProfileClicked);
  connect(duplicateProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onDuplicateProfileClicked);
  connect(configureProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onConfigureProfileClicked);

  profileManagementButtons->addStretch();
  profileManagementButtons->addWidget(createProfileButton);
  profileManagementButtons->addWidget(configureProfileButton);
  profileManagementButtons->addWidget(duplicateProfileButton);
  profileManagementButtons->addWidget(deleteProfileButton);
  profileManagementButtons->addStretch();

  profileManagementLayout->addWidget(profileListWidget);
  profileManagementLayout->addLayout(profileManagementButtons);
  profileManagementGroup->setLayout(profileManagementLayout);
  profilesTabLayout->addWidget(profileManagementGroup);

  /* ===== Sub-group 2: Profile Actions ===== */
  QGroupBox *profileActionsGroup = new QGroupBox("Profile Actions");
  QHBoxLayout *profileActionsLayout = new QHBoxLayout();

  startProfileButton = new QPushButton("▶ Start");
  startProfileButton->setToolTip("Start selected profile");
  startProfileButton->setFixedWidth(75);
  startProfileButton->setEnabled(false);

  stopProfileButton = new QPushButton("■ Stop");
  stopProfileButton->setToolTip("Stop selected profile");
  stopProfileButton->setFixedWidth(75);
  stopProfileButton->setEnabled(false);

  startAllProfilesButton = new QPushButton("▶ All");
  startAllProfilesButton->setToolTip("Start all profiles");
  startAllProfilesButton->setFixedWidth(75);

  stopAllProfilesButton = new QPushButton("■ All");
  stopAllProfilesButton->setToolTip("Stop all profiles");
  stopAllProfilesButton->setFixedWidth(75);
  stopAllProfilesButton->setEnabled(false);

  connect(startProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onStartProfileClicked);
  connect(stopProfileButton, &QPushButton::clicked, this,
          &RestreamerDock::onStopProfileClicked);
  connect(startAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStartAllProfilesClicked);
  connect(stopAllProfilesButton, &QPushButton::clicked, this,
          &RestreamerDock::onStopAllProfilesClicked);

  profileActionsLayout->addStretch();
  profileActionsLayout->addWidget(startProfileButton);
  profileActionsLayout->addWidget(stopProfileButton);
  profileActionsLayout->addWidget(startAllProfilesButton);
  profileActionsLayout->addWidget(stopAllProfilesButton);
  profileActionsLayout->addStretch();

  profileActionsGroup->setLayout(profileActionsLayout);
  profilesTabLayout->addWidget(profileActionsGroup);

  /* ===== Sub-group 3: Profile Details ===== */
  QGroupBox *profileDetailsGroup = new QGroupBox("Profile Details");
  QVBoxLayout *profileDetailsLayout = new QVBoxLayout();

  /* Profile status label */
  profileStatusLabel = new QLabel("No profiles");
  profileStatusLabel->setAlignment(Qt::AlignCenter);

  /* Profile destinations table (shows destinations for selected profile) */
  profileDestinationsTable = new QTableWidget();
  profileDestinationsTable->setColumnCount(4);
  profileDestinationsTable->setHorizontalHeaderLabels(
      {"Destination", "Resolution", "Bitrate", "Status"});
  profileDestinationsTable->horizontalHeader()->setStretchLastSection(true);
  profileDestinationsTable->setMaximumHeight(150);

  profileDetailsLayout->addWidget(profileStatusLabel);
  profileDetailsLayout->addWidget(profileDestinationsTable);
  profileDetailsGroup->setLayout(profileDetailsLayout);
  profilesTabLayout->addWidget(profileDetailsGroup);

  profilesTabLayout->addStretch();

  /* Add Profiles tab to collapsible section */
  profilesSection = new CollapsibleSection("Profiles");

  /* Add quick action toggle to Profiles header */
  quickProfileToggleButton = new QPushButton("Start");
  quickProfileToggleButton->setMaximumWidth(60);
  quickProfileToggleButton->setToolTip("Start/Stop selected profile");
  quickProfileToggleButton->setEnabled(
      false); /* Disabled until profile is selected */
  connect(quickProfileToggleButton, &QPushButton::clicked, this, [this]() {
    if (!profileListWidget->currentItem()) {
      return;
    }

    QString profileId =
        profileListWidget->currentItem()->data(Qt::UserRole).toString();
    output_profile_t *profile = profile_manager_get_profile(
        profileManager, profileId.toUtf8().constData());

    if (!profile) {
      return;
    }

    if (profile->status == PROFILE_STATUS_ACTIVE ||
        profile->status == PROFILE_STATUS_STARTING) {
      onStopProfileClicked();
    } else {
      onStartProfileClicked();
    }
  });

  /* Connect to profile selection to update button state */
  connect(
      profileListWidget, &QListWidget::currentRowChanged, this,
      [this](int row) {
        quickProfileToggleButton->setEnabled(row >= 0);
        if (row >= 0 && profileListWidget->currentItem()) {
          QString profileId =
              profileListWidget->currentItem()->data(Qt::UserRole).toString();
          output_profile_t *profile = profile_manager_get_profile(
              profileManager, profileId.toUtf8().constData());
          if (profile) {
            bool isActive = (profile->status == PROFILE_STATUS_ACTIVE ||
                             profile->status == PROFILE_STATUS_STARTING);
            quickProfileToggleButton->setText(isActive ? "Stop" : "Start");
          }
        }
      });

  profilesSection->addHeaderButton(quickProfileToggleButton);

  profilesSection->setContent(profilesTab);
  profilesSection->setExpanded(true, false); /* Expanded by default */
  verticalLayout->addWidget(profilesSection);

  /* ===== Tab 3: Monitoring (Watch - Step 3) ===== */
  QWidget *monitoringTab = new QWidget();
  QVBoxLayout *monitoringTabLayout = new QVBoxLayout(monitoringTab);

  QLabel *monitoringHelpLabel =
      new QLabel("Monitor active streams and performance");
  monitoringHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_muted_color().name()));
  monitoringHelpLabel->setAlignment(Qt::AlignCenter);
  monitoringTabLayout->addWidget(monitoringHelpLabel);

  /* ===== Sub-group 1: Process Information ===== */
  QGroupBox *processInfoGroup = new QGroupBox("Process Information");
  QVBoxLayout *processInfoLayout = new QVBoxLayout();

  processList = new QListWidget();
  processList->setMaximumHeight(80);
  processList->setIconSize(QSize(48, 48)); /* Larger icon size for better visibility */
  connect(processList, &QListWidget::currentRowChanged, this,
          &RestreamerDock::onProcessSelected);

  QHBoxLayout *processButtonLayout = new QHBoxLayout();
  refreshButton = new QPushButton("🔄");
  refreshButton->setToolTip("Refresh process list");
  refreshButton->setMinimumSize(50, 36); /* Larger to fit icon */
  refreshButton->setMaximumSize(50, 36);
  refreshButton->setStyleSheet("font-size: 20px;"); /* Larger icon */
  startButton = new QPushButton("▶");
  startButton->setToolTip("Start selected process");
  startButton->setMinimumSize(50, 36); /* Larger to fit icon */
  startButton->setMaximumSize(50, 36);
  startButton->setStyleSheet("font-size: 20px;"); /* Larger icon */
  stopButton = new QPushButton("■");
  stopButton->setToolTip("Stop selected process");
  stopButton->setMinimumSize(50, 36); /* Larger to fit icon */
  stopButton->setMaximumSize(50, 36);
  stopButton->setStyleSheet("font-size: 20px;"); /* Larger icon */
  restartButton = new QPushButton("↻");
  restartButton->setToolTip("Restart selected process");
  restartButton->setMinimumSize(50, 36); /* Larger to fit icon */
  restartButton->setMaximumSize(50, 36);
  restartButton->setStyleSheet("font-size: 20px;"); /* Larger icon */

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

  processButtonLayout->addStretch();
  processButtonLayout->addWidget(refreshButton);
  processButtonLayout->addWidget(startButton);
  processButtonLayout->addWidget(stopButton);
  processButtonLayout->addWidget(restartButton);
  processButtonLayout->addStretch();

  processInfoLayout->addWidget(processList);
  processInfoLayout->addLayout(processButtonLayout);
  processInfoGroup->setLayout(processInfoLayout);
  monitoringTabLayout->addWidget(processInfoGroup);

  /* ===== Sub-group 2: Performance Metrics ===== */
  QGroupBox *metricsGroup = new QGroupBox("Performance Metrics");
  QVBoxLayout *metricsMainLayout = new QVBoxLayout();

  /* Two-column layout for metrics with proper alignment */
  QHBoxLayout *metricsColumnsLayout = new QHBoxLayout();

  /* Left column - System metrics */
  QFormLayout *metricsLeftLayout = new QFormLayout();
  metricsLeftLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  metricsLeftLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
  metricsLeftLayout->setLabelAlignment(Qt::AlignRight);

  processIdLabel = new QLabel("-");
  processStateLabel = new QLabel("-");
  processUptimeLabel = new QLabel("-");
  processCpuLabel = new QLabel("-");
  processMemoryLabel = new QLabel("-");

  metricsLeftLayout->addRow("Process ID:", processIdLabel);
  metricsLeftLayout->addRow("State:", processStateLabel);
  metricsLeftLayout->addRow("Uptime:", processUptimeLabel);
  metricsLeftLayout->addRow("CPU Usage:", processCpuLabel);
  metricsLeftLayout->addRow("Memory:", processMemoryLabel);

  /* Right column - Stream metrics */
  QFormLayout *metricsRightLayout = new QFormLayout();
  metricsRightLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  metricsRightLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
  metricsRightLayout->setLabelAlignment(Qt::AlignRight);

  processFramesLabel = new QLabel("-");
  processDroppedFramesLabel = new QLabel("-");
  processFpsLabel = new QLabel("-");
  processBitrateLabel = new QLabel("-");
  processProgressLabel = new QLabel("-");

  metricsRightLayout->addRow("Frames:", processFramesLabel);
  metricsRightLayout->addRow("Dropped:", processDroppedFramesLabel);
  metricsRightLayout->addRow("FPS:", processFpsLabel);
  metricsRightLayout->addRow("Bitrate:", processBitrateLabel);
  metricsRightLayout->addRow("Progress:", processProgressLabel);

  metricsColumnsLayout->addLayout(metricsLeftLayout);
  metricsColumnsLayout->addSpacing(40); /* Fixed spacing between columns */
  metricsColumnsLayout->addLayout(metricsRightLayout);
  metricsColumnsLayout->addStretch(); /* Push everything to the left */
  metricsMainLayout->addLayout(metricsColumnsLayout);

  /* Action buttons - aligned with metrics columns above */
  /* Create two button containers that mirror the form layout structure */
  QHBoxLayout *metricsButtonLayout = new QHBoxLayout();

  /* Left button - mirrors left form layout width */
  QVBoxLayout *leftButtonContainer = new QVBoxLayout();
  probeInputButton = new QPushButton("Probe Input");
  probeInputButton->setToolTip("Probe input stream details");
  leftButtonContainer->addWidget(probeInputButton);

  /* Right button - mirrors right form layout width */
  QVBoxLayout *rightButtonContainer = new QVBoxLayout();
  viewMetricsButton = new QPushButton("View Metrics");
  viewMetricsButton->setToolTip("View performance metrics");
  rightButtonContainer->addWidget(viewMetricsButton);

  /* Add button containers with same spacing as columns above */
  metricsButtonLayout->addLayout(leftButtonContainer);
  metricsButtonLayout->addSpacing(40); /* Match column spacing */
  metricsButtonLayout->addLayout(rightButtonContainer);
  metricsButtonLayout->addStretch(); /* Push to left like columns */
  metricsMainLayout->addLayout(metricsButtonLayout);

  connect(probeInputButton, &QPushButton::clicked, this,
          &RestreamerDock::onProbeInputClicked);
  connect(viewMetricsButton, &QPushButton::clicked, this,
          &RestreamerDock::onViewMetricsClicked);

  metricsGroup->setLayout(metricsMainLayout);
  monitoringTabLayout->addWidget(metricsGroup);

  /* ===== Sub-group 3: Active Sessions ===== */
  QGroupBox *sessionsGroup = new QGroupBox("Active Sessions");
  QVBoxLayout *sessionsLayout = new QVBoxLayout();

  sessionTable = new QTableWidget();
  sessionTable->setColumnCount(3);
  sessionTable->setHorizontalHeaderLabels(
      {"Session ID", "Remote Address", "Bytes Sent"});
  sessionTable->horizontalHeader()->setStretchLastSection(true);
  sessionTable->setMaximumHeight(60);

  sessionsLayout->addWidget(sessionTable);
  sessionsGroup->setLayout(sessionsLayout);
  monitoringTabLayout->addWidget(sessionsGroup);
  monitoringTabLayout->addStretch();

  /* Add Monitoring tab to collapsible section */
  monitoringSection = new CollapsibleSection("Monitoring");
  monitoringSection->setContent(monitoringTab);
  monitoringSection->setExpanded(false, false); /* Collapsed by default */
  verticalLayout->addWidget(monitoringSection);

  /* ===== Tab 4: System (Settings - Step 4) ===== */
  QWidget *systemTab = new QWidget();
  QVBoxLayout *systemTabLayout = new QVBoxLayout(systemTab);

  QLabel *systemHelpLabel =
      new QLabel("Restreamer server configuration and settings");
  systemHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_muted_color().name()));
  systemHelpLabel->setAlignment(Qt::AlignCenter);
  systemTabLayout->addWidget(systemHelpLabel);

  /* Configuration Management */
  QGroupBox *configGroup = new QGroupBox("Server Configuration");
  QVBoxLayout *configLayout = new QVBoxLayout();

  QPushButton *viewConfigButton = new QPushButton("View/Edit Config");
  viewConfigButton->setMinimumWidth(150);
  viewConfigButton->setToolTip("View and edit Restreamer configuration");
  QPushButton *reloadConfigButton = new QPushButton("Reload Config");
  reloadConfigButton->setMinimumWidth(150);
  reloadConfigButton->setToolTip("Reload configuration from server");

  connect(viewConfigButton, &QPushButton::clicked, this,
          &RestreamerDock::onViewConfigClicked);
  connect(reloadConfigButton, &QPushButton::clicked, this,
          &RestreamerDock::onReloadConfigClicked);

  /* Center the buttons */
  QHBoxLayout *configButtonLayout = new QHBoxLayout();
  configButtonLayout->addStretch();
  configButtonLayout->addWidget(viewConfigButton);
  configButtonLayout->addWidget(reloadConfigButton);
  configButtonLayout->addStretch();

  configLayout->addLayout(configButtonLayout);
  configGroup->setLayout(configLayout);
  systemTabLayout->addWidget(configGroup);

  systemTabLayout->addStretch();

  /* Add System tab to collapsible section */
  systemSection = new CollapsibleSection("System");
  systemSection->setContent(systemTab);
  systemSection->setExpanded(false, false); /* Collapsed by default */
  verticalLayout->addWidget(systemSection);

  /* ===== Tab 5: Advanced (Expert Mode - Step 5) ===== */
  QWidget *advancedTab = new QWidget();
  QVBoxLayout *advancedTabLayout = new QVBoxLayout(advancedTab);

  QLabel *advancedHelpLabel = new QLabel("Advanced features for expert users");
  advancedHelpLabel->setStyleSheet(
      QString("QLabel { color: %1; font-size: 11px; }")
          .arg(obs_theme_get_muted_color().name()));
  advancedHelpLabel->setAlignment(Qt::AlignCenter);
  advancedTabLayout->addWidget(advancedHelpLabel);

  /* Multistream Manual Configuration */
  QGroupBox *multistreamGroup = new QGroupBox("Manual Multistream Setup");
  QVBoxLayout *multistreamLayout = new QVBoxLayout();

  QFormLayout *orientationLayout = new QFormLayout();
  orientationLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  orientationLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  orientationLayout->setLabelAlignment(Qt::AlignRight);

  autoDetectOrientationCheck = new QCheckBox("Auto-detect orientation");
  autoDetectOrientationCheck->setChecked(true);
  autoDetectOrientationCheck->setToolTip(
      "Automatically detect video orientation from stream");

  orientationCombo = new QComboBox();
  orientationCombo->addItem("Horizontal (Landscape)", ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (Portrait)", ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square", ORIENTATION_SQUARE);
  orientationCombo->setToolTip("Set the orientation for multistream output");
  orientationCombo->setMaximumWidth(300);

  orientationLayout->addRow(autoDetectOrientationCheck);
  orientationLayout->addRow("Orientation:", orientationCombo);
  multistreamLayout->addLayout(orientationLayout);

  /* Destinations table */
  destinationsTable = new QTableWidget();
  destinationsTable->setColumnCount(4);
  destinationsTable->setHorizontalHeaderLabels(
      {"Service", "Stream Key", "Orientation", "Enabled"});
  destinationsTable->horizontalHeader()->setStretchLastSection(true);
  destinationsTable->setMaximumHeight(150);

  QHBoxLayout *destButtonLayout = new QHBoxLayout();
  destButtonLayout->addStretch();
  addDestinationButton = new QPushButton("Add Destination");
  addDestinationButton->setMinimumWidth(140);
  addDestinationButton->setToolTip("Add new streaming destination");
  removeDestinationButton = new QPushButton("Remove");
  removeDestinationButton->setMinimumWidth(140);
  removeDestinationButton->setToolTip("Remove selected destination");
  createMultistreamButton = new QPushButton("Start Multistream");
  createMultistreamButton->setMinimumWidth(140);
  createMultistreamButton->setToolTip("Start multistream to all destinations");

  connect(addDestinationButton, &QPushButton::clicked, this,
          &RestreamerDock::onAddDestinationClicked);
  connect(removeDestinationButton, &QPushButton::clicked, this,
          &RestreamerDock::onRemoveDestinationClicked);
  connect(createMultistreamButton, &QPushButton::clicked, this,
          &RestreamerDock::onCreateMultistreamClicked);

  destButtonLayout->addWidget(addDestinationButton);
  destButtonLayout->addWidget(removeDestinationButton);
  destButtonLayout->addWidget(createMultistreamButton);
  destButtonLayout->addStretch();

  multistreamLayout->addWidget(destinationsTable);
  multistreamLayout->addLayout(destButtonLayout);
  multistreamGroup->setLayout(multistreamLayout);
  advancedTabLayout->addWidget(multistreamGroup);

  /* FFmpeg Capabilities group */
  QGroupBox *skillsGroup = new QGroupBox("FFmpeg Capabilities");
  QVBoxLayout *skillsLayout = new QVBoxLayout();

  QPushButton *viewSkillsButton = new QPushButton("View Codecs & Formats");
  viewSkillsButton->setMinimumWidth(160);
  viewSkillsButton->setToolTip("View available FFmpeg codecs and formats");
  connect(viewSkillsButton, &QPushButton::clicked, this,
          &RestreamerDock::onViewSkillsClicked);

  /* Center the button */
  QHBoxLayout *skillsButtonLayout = new QHBoxLayout();
  skillsButtonLayout->addStretch();
  skillsButtonLayout->addWidget(viewSkillsButton);
  skillsButtonLayout->addStretch();

  skillsLayout->addLayout(skillsButtonLayout);
  skillsGroup->setLayout(skillsLayout);
  advancedTabLayout->addWidget(skillsGroup);

  /* Protocol Monitoring group */
  QGroupBox *protocolGroup = new QGroupBox("Protocol Monitoring");
  QVBoxLayout *protocolLayout = new QVBoxLayout();

  QHBoxLayout *protocolButtonLayout = new QHBoxLayout();
  protocolButtonLayout->addStretch();
  QPushButton *viewRtmpButton = new QPushButton("View RTMP Streams");
  viewRtmpButton->setMinimumWidth(160);
  viewRtmpButton->setToolTip("View active RTMP streams");
  QPushButton *viewSrtButton = new QPushButton("View SRT Streams");
  viewSrtButton->setMinimumWidth(160);
  viewSrtButton->setToolTip("View active SRT streams");

  connect(viewRtmpButton, &QPushButton::clicked, this,
          &RestreamerDock::onViewRtmpStreamsClicked);
  connect(viewSrtButton, &QPushButton::clicked, this,
          &RestreamerDock::onViewSrtStreamsClicked);

  protocolButtonLayout->addWidget(viewRtmpButton);
  protocolButtonLayout->addWidget(viewSrtButton);
  protocolButtonLayout->addStretch();
  protocolLayout->addLayout(protocolButtonLayout);
  protocolGroup->setLayout(protocolLayout);
  advancedTabLayout->addWidget(protocolGroup);

  advancedTabLayout->addStretch();

  /* Add Advanced tab to collapsible section */
  advancedSection = new CollapsibleSection("Advanced");
  advancedSection->setContent(advancedTab);
  advancedSection->setExpanded(false, false); /* Collapsed by default */
  verticalLayout->addWidget(advancedSection);

  /* Add stretch to push sections to the top */
  verticalLayout->addStretch();

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

  /* Load bridge settings */
  bridgeHorizontalUrlEdit->setText(
      obs_data_get_string(settings, "bridge_horizontal_url"));
  bridgeVerticalUrlEdit->setText(
      obs_data_get_string(settings, "bridge_vertical_url"));
  bridgeAutoStartCheckbox->setChecked(
      obs_data_get_bool(settings, "bridge_auto_start"));

  /* RAII: settings automatically released when going out of scope */

  /* Set defaults if empty */
  if (hostEdit->text().isEmpty()) {
    hostEdit->setText("localhost");
  }
  if (portEdit->text().isEmpty()) {
    portEdit->setText("8080");
  }
  if (bridgeHorizontalUrlEdit->text().isEmpty()) {
    bridgeHorizontalUrlEdit->setText("rtmp://localhost/live/obs_horizontal");
  }
  if (bridgeVerticalUrlEdit->text().isEmpty()) {
    bridgeVerticalUrlEdit->setText("rtmp://localhost/live/obs_vertical");
  }
  /* Bridge auto-start defaults to true (already set in loadSettings if not in
   * config) */
  if (!obs_data_has_user_value(settings, "bridge_auto_start")) {
    bridgeAutoStartCheckbox->setChecked(true);
  }

  /* Auto-test connection if server config is already populated */
  bool hasServerConfig = obs_data_has_user_value(settings, "host") ||
                         obs_data_has_user_value(settings, "port");
  if (hasServerConfig && !hostEdit->text().isEmpty() &&
      !portEdit->text().isEmpty()) {
    obs_log(LOG_INFO,
            "[obs-polyemesis] Server configuration detected, testing connection "
            "automatically");
    /* Delay the test slightly to let UI finish initializing */
    QTimer::singleShot(500, this, &RestreamerDock::onTestConnectionClicked);
  }
}

void RestreamerDock::saveSettings() {
  OBSDataAutoRelease settings(obs_data_create());

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

  /* Save bridge settings */
  obs_data_set_string(settings, "bridge_horizontal_url",
                      bridgeHorizontalUrlEdit->text().toUtf8().constData());
  obs_data_set_string(settings, "bridge_vertical_url",
                      bridgeVerticalUrlEdit->text().toUtf8().constData());
  obs_data_set_bool(settings, "bridge_auto_start",
                    bridgeAutoStartCheckbox->isChecked());

  /* Safe file writing: writes to .tmp first, then creates .bak backup,
   * then renames .tmp to actual file. Prevents corruption on crash/power loss.
   */
  const char *config_path = obs_module_config_path("config.json");
  if (!obs_data_save_json_safe(settings, config_path, "tmp", "bak")) {
    blog(LOG_ERROR, "[obs-polyemesis] Failed to save settings to %s",
         config_path);
  }

  /* RAII: settings automatically released when going out of scope */

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
    connectionStatusLabel->setStyleSheet(
        QString("color: %1;").arg(obs_theme_get_error_color().name()));
    updateConnectionSectionTitle();
    return;
  }

  if (restreamer_api_test_connection(api)) {
    connectionStatusLabel->setText("Connected");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1;").arg(obs_theme_get_success_color().name()));
    updateConnectionSectionTitle();
    onRefreshClicked();
  } else {
    connectionStatusLabel->setText("Connection failed");
    connectionStatusLabel->setStyleSheet(
        QString("color: %1;").arg(obs_theme_get_error_color().name()));
    updateConnectionSectionTitle();
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
  updateMonitoringSectionTitle();

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
  updateBridgeSectionTitle();
}

/* Profile Management Functions */

void RestreamerDock::updateProfileList() {
  profileListWidget->clear();

  if (!profileManager || profileManager->profile_count == 0) {
    profileStatusLabel->setText("No profiles");
    updateProfilesSectionTitle();
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
  updateProfilesSectionTitle();

  /* Update button states */
  stopAllProfilesButton->setEnabled(hasActiveProfile);

  /* Update profile details for current selection (or select first if none
   * selected) */
  if (profileListWidget->currentRow() < 0 && profileListWidget->count() > 0) {
    profileListWidget->setCurrentRow(0);
  }
  updateProfileDetails();
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
  output_profile_t *profile = profile_manager_get_profile(
      profileManager, profileId.toUtf8().constData());

  if (!profile) {
    return;
  }

  /* Debug: Log profile status */
  blog(LOG_INFO,
       "[obs-polyemesis] Profile '%s' status: %d (0=INACTIVE, 1=STARTING, "
       "2=ACTIVE, 3=STOPPING, 4=ERROR)",
       profile->profile_name, profile->status);

  /* Enhanced: Update status label with color-coded visual feedback */
  QString statusText;
  QString statusColor;
  switch (profile->status) {
  case PROFILE_STATUS_INACTIVE:
    statusText = "⚫ Inactive";
    statusColor = obs_theme_get_muted_color().name(); /* Gray */
    break;
  case PROFILE_STATUS_STARTING:
    statusText = "🟡 Starting...";
    statusColor = obs_theme_get_warning_color().name(); /* Orange */
    break;
  case PROFILE_STATUS_ACTIVE:
    statusText = "🟢 Active";
    statusColor = obs_theme_get_success_color().name(); /* Green */
    break;
  case PROFILE_STATUS_STOPPING:
    statusText = "🟠 Stopping...";
    statusColor = obs_theme_get_warning_color().name(); /* Dark Orange */
    break;
  case PROFILE_STATUS_ERROR:
    statusText = "🔴 Error";
    statusColor = obs_theme_get_error_color().name(); /* Red */
    break;
  default:
    statusText = "❓ Unknown";
    statusColor = obs_theme_get_muted_color().name(); /* Light Gray */
    break;
  }
  profileStatusLabel->setText(statusText);
  profileStatusLabel->setStyleSheet(
      QString("QLabel { color: %1; font-weight: bold; }").arg(statusColor));
  updateProfilesSectionTitle();

  /* Update quick action toggle button text */
  if (quickProfileToggleButton) {
    bool isActive = (profile->status == PROFILE_STATUS_ACTIVE ||
                     profile->status == PROFILE_STATUS_STARTING);
    quickProfileToggleButton->setText(isActive ? "Stop" : "Start");
  }

  /* Update button states based on profile status */
  deleteProfileButton->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);
  duplicateProfileButton->setEnabled(true);
  configureProfileButton->setEnabled(profile->status ==
                                     PROFILE_STATUS_INACTIVE);
  startProfileButton->setEnabled(profile->status == PROFILE_STATUS_INACTIVE);
  stopProfileButton->setEnabled(profile->status == PROFILE_STATUS_ACTIVE ||
                                profile->status == PROFILE_STATUS_STARTING);

  /* Populate destinations table */
  profileDestinationsTable->setRowCount(
      static_cast<int>(profile->destination_count));

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
      resolution =
          QString("%1x%2").arg(dest->encoding.width).arg(dest->encoding.height);
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

void RestreamerDock::onProfileSelected() { updateProfileDetails(); }

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
    QMessageBox::warning(
        this, "Error", "Failed to start profile. Check Restreamer connection.");
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
  output_profile_t *profile = profile_manager_get_profile(
      profileManager, profileId.toUtf8().constData());

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
    if (profile_manager_delete_profile(profileManager,
                                       profileId.toUtf8().constData())) {
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
  output_profile_t *sourceProfile = profile_manager_get_profile(
      profileManager, profileId.toUtf8().constData());

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
  QString profileName = QInputDialog::getText(
      this, "Create Profile", "Enter profile name:", QLineEdit::Normal,
      "New Profile", &ok);

  if (ok && !profileName.isEmpty()) {
    output_profile_t *newProfile = profile_manager_create_profile(
        profileManager, profileName.toUtf8().constData());

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
  output_profile_t *profile = profile_manager_get_profile(
      profileManager, profileId.toUtf8().constData());

  if (!profile) {
    return;
  }

  /* Create configuration dialog */
  QDialog dialog(this);
  dialog.setWindowTitle(
      QString("Configure Profile: %1").arg(profile->profile_name));
  dialog.setMinimumWidth(500);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

  /* Basic profile settings group */
  QGroupBox *basicGroup = new QGroupBox("Basic Settings");
  QGridLayout *basicLayout = new QGridLayout();
  basicLayout->setColumnStretch(1, 1);
  basicLayout->setHorizontalSpacing(10);
  basicLayout->setVerticalSpacing(10);

  /* Create labels */
  QLabel *nameLabel = new QLabel("Profile Name:");
  nameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *orientLabel = new QLabel("Source Orientation:");
  orientLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *inputUrlLabel = new QLabel("Input URL:");
  inputUrlLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  /* Profile name */
  QLineEdit *nameEdit = new QLineEdit(profile->profile_name);
  nameEdit->setMinimumWidth(300);

  /* Source orientation */
  QComboBox *orientationCombo = new QComboBox();
  orientationCombo->addItem("Horizontal (16:9)", (int)ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (9:16)", (int)ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square (1:1)", (int)ORIENTATION_SQUARE);
  orientationCombo->setCurrentIndex((int)profile->source_orientation);
  orientationCombo->setMinimumWidth(300);

  /* Auto-detect orientation */
  QCheckBox *autoDetectCheck =
      new QCheckBox("Auto-detect orientation from source");
  autoDetectCheck->setChecked(profile->auto_detect_orientation);

  /* Auto-start */
  QCheckBox *autoStartCheck = new QCheckBox("Auto-start with OBS streaming");
  autoStartCheck->setChecked(profile->auto_start);

  /* Auto-reconnect */
  QCheckBox *autoReconnectCheck = new QCheckBox("Auto-reconnect on disconnect");
  autoReconnectCheck->setChecked(profile->auto_reconnect);

  /* Input URL */
  QLineEdit *inputUrlEdit = new QLineEdit(profile->input_url);
  inputUrlEdit->setPlaceholderText("rtmp://localhost/live/obs_input");
  inputUrlEdit->setMinimumWidth(300);

  /* Add widgets to grid layout */
  int row = 0;
  basicLayout->addWidget(nameLabel, row, 0);
  basicLayout->addWidget(nameEdit, row, 1);
  row++;

  basicLayout->addWidget(orientLabel, row, 0);
  basicLayout->addWidget(orientationCombo, row, 1);
  row++;

  basicLayout->addWidget(autoDetectCheck, row, 1);
  row++;

  basicLayout->addWidget(autoStartCheck, row, 1);
  row++;

  basicLayout->addWidget(autoReconnectCheck, row, 1);
  row++;

  basicLayout->addWidget(inputUrlLabel, row, 0);
  basicLayout->addWidget(inputUrlEdit, row, 1);

  basicGroup->setLayout(basicLayout);
  mainLayout->addWidget(basicGroup);

  /* Destinations group */
  QGroupBox *destGroup = new QGroupBox("Destinations");
  QVBoxLayout *destLayout = new QVBoxLayout();

  /* Destinations table */
  QTableWidget *destTable = new QTableWidget();
  destTable->setColumnCount(4);
  destTable->setHorizontalHeaderLabels(
      {"Service", "Stream Key", "Orientation", "Enabled"});
  destTable->horizontalHeader()->setStretchLastSection(false);
  destTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  destTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  destTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  destTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  destTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  destTable->setSelectionMode(QAbstractItemView::SingleSelection);
  destTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  destTable->setMinimumHeight(150);

  /* Populate destinations table */
  destTable->setRowCount(static_cast<int>(profile->destination_count));
  for (int i = 0; i < static_cast<int>(profile->destination_count); i++) {
    profile_destination_t *dest = &profile->destinations[i];

    /* Service name */
    QTableWidgetItem *serviceItem = new QTableWidgetItem(dest->service_name);
    destTable->setItem(i, 0, serviceItem);

    /* Stream key (masked) */
    QString maskedKey = QString(dest->stream_key);
    if (maskedKey.length() > 8) {
      maskedKey = maskedKey.left(4) + "..." + maskedKey.right(4);
    }
    QTableWidgetItem *keyItem = new QTableWidgetItem(maskedKey);
    destTable->setItem(i, 1, keyItem);

    /* Orientation */
    QString orientation;
    switch (dest->target_orientation) {
    case ORIENTATION_HORIZONTAL:
      orientation = "Horizontal";
      break;
    case ORIENTATION_VERTICAL:
      orientation = "Vertical";
      break;
    case ORIENTATION_SQUARE:
      orientation = "Square";
      break;
    default:
      orientation = "Auto";
      break;
    }
    QTableWidgetItem *orientItem = new QTableWidgetItem(orientation);
    destTable->setItem(i, 2, orientItem);

    /* Enabled checkbox */
    QTableWidgetItem *enabledItem = new QTableWidgetItem();
    enabledItem->setCheckState(dest->enabled ? Qt::Checked : Qt::Unchecked);
    destTable->setItem(i, 3, enabledItem);
  }

  destLayout->addWidget(destTable);

  /* Destination buttons */
  QHBoxLayout *destButtonLayout = new QHBoxLayout();
  destButtonLayout->addStretch();
  QPushButton *addDestButton = new QPushButton("Add Destination");
  addDestButton->setMinimumWidth(140);
  QPushButton *removeDestButton = new QPushButton("Remove Destination");
  removeDestButton->setMinimumWidth(140);
  QPushButton *editDestButton = new QPushButton("Edit Destination");
  editDestButton->setMinimumWidth(140);

  destButtonLayout->addWidget(addDestButton);
  destButtonLayout->addWidget(removeDestButton);
  destButtonLayout->addWidget(editDestButton);
  destButtonLayout->addStretch();

  destLayout->addLayout(destButtonLayout);

  /* Add destination handler */
  connect(
      addDestButton, &QPushButton::clicked, [&, destTable, profile, this]() {
        /* Create enhanced add destination dialog */
        QDialog destDialog(&dialog);
        destDialog.setWindowTitle("Add Streaming Destination");
        destDialog.setMinimumWidth(500);

        QVBoxLayout *destDialogLayout = new QVBoxLayout(&destDialog);

        QGroupBox *destFormGroup = new QGroupBox("Destination Settings");
        QGridLayout *destForm = new QGridLayout();
        destForm->setColumnStretch(1, 1); // Make the widget column stretch
        destForm->setHorizontalSpacing(10);
        destForm->setVerticalSpacing(10);

        /* Service combo with full OBS service list */
        QComboBox *serviceCombo = new QComboBox();
        serviceCombo->setMinimumWidth(300);

        /* Show common services first, then all services */
        QStringList commonServices = serviceLoader->getCommonServiceNames();
        QStringList allServices = serviceLoader->getServiceNames();

        // Add common services
        for (const QString &serviceName : commonServices) {
          serviceCombo->addItem(serviceName, serviceName);
        }

        // Add separator and remaining services
        if (!commonServices.isEmpty() &&
            commonServices.size() < allServices.size()) {
          serviceCombo->insertSeparator(serviceCombo->count());
          serviceCombo->addItem("-- Show All Services --", QString());
          serviceCombo->insertSeparator(serviceCombo->count());

          for (const QString &serviceName : allServices) {
            if (!commonServices.contains(serviceName)) {
              serviceCombo->addItem(serviceName, serviceName);
            }
          }
        }

        // Add Custom RTMP option
        serviceCombo->insertSeparator(serviceCombo->count());
        serviceCombo->addItem("Custom RTMP Server", "custom");

        /* Create labels for form fields */
        QLabel *serviceLabel = new QLabel("Service:");
        serviceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLabel *serverLabel = new QLabel("Server:");
        serverLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLabel *customUrlLabel = new QLabel("RTMP URL:");
        customUrlLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLabel *streamKeyLabel = new QLabel("Stream Key:");
        streamKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QLabel *orientationLabel = new QLabel("Target Orientation:");
        orientationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        /* Server selection combo */
        QComboBox *serverCombo = new QComboBox();
        serverCombo->setMinimumWidth(300);

        /* Custom server URL field */
        QLineEdit *customUrlEdit = new QLineEdit();
        customUrlEdit->setPlaceholderText("rtmp://your-server/live/stream-key");
        customUrlEdit->setMinimumWidth(300);
        customUrlEdit->setVisible(false);

        /* Stream key */
        QLineEdit *keyEdit = new QLineEdit();
        keyEdit->setPlaceholderText("Enter your stream key");
        keyEdit->setMinimumWidth(300);

        /* Stream key help label */
        QLabel *streamKeyHelpLabel = new QLabel();
        streamKeyHelpLabel->setOpenExternalLinks(true);
        streamKeyHelpLabel->setWordWrap(true);
        streamKeyHelpLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: 11px; }")
                .arg(obs_theme_get_info_color().name()));

        /* Target orientation */
        QComboBox *targetOrientCombo = new QComboBox();
        targetOrientCombo->addItem("Horizontal (16:9)",
                                   (int)ORIENTATION_HORIZONTAL);
        targetOrientCombo->addItem("Vertical (9:16)",
                                   (int)ORIENTATION_VERTICAL);
        targetOrientCombo->addItem("Square (1:1)", (int)ORIENTATION_SQUARE);
        targetOrientCombo->setMinimumWidth(300);

        /* Enabled checkbox */
        QCheckBox *enabledCheck = new QCheckBox("Enable this destination");
        enabledCheck->setChecked(true);

        /* Update server list when service changes */
        auto updateServerList =
            [this, serviceCombo, serverCombo, streamKeyHelpLabel, customUrlEdit,
             keyEdit, serverLabel, customUrlLabel, streamKeyLabel]() {
              QString selectedService = serviceCombo->currentData().toString();
              serverCombo->clear();
              streamKeyHelpLabel->clear();

              if (selectedService == "custom") {
                // Custom RTMP mode - show only custom URL field
                serverLabel->setVisible(false);
                serverCombo->setVisible(false);
                streamKeyLabel->setVisible(false);
                keyEdit->setVisible(false);
                customUrlLabel->setVisible(true);
                customUrlEdit->setVisible(true);
                streamKeyHelpLabel->setText(
                    "Enter the full RTMP URL including stream key");
              } else if (!selectedService.isEmpty() &&
                         selectedService != "-- Show All Services --") {
                // Regular service mode - show server and stream key fields
                customUrlLabel->setVisible(false);
                customUrlEdit->setVisible(false);
                serverLabel->setVisible(true);
                serverCombo->setVisible(true);
                streamKeyLabel->setVisible(true);
                keyEdit->setVisible(true);

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

        connect(serviceCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                updateServerList);

        /* Add widgets to grid layout (row, column) */
        int row = 0;
        destForm->addWidget(serviceLabel, row, 0);
        destForm->addWidget(serviceCombo, row, 1);
        row++;

        destForm->addWidget(serverLabel, row, 0);
        destForm->addWidget(serverCombo, row, 1);
        row++;

        destForm->addWidget(customUrlLabel, row, 0);
        destForm->addWidget(customUrlEdit, row, 1);
        row++;

        destForm->addWidget(streamKeyLabel, row, 0);
        destForm->addWidget(keyEdit, row, 1);
        row++;

        destForm->addWidget(streamKeyHelpLabel, row,
                            1); // Help label spans column 1 only
        row++;

        destForm->addWidget(orientationLabel, row, 0);
        destForm->addWidget(targetOrientCombo, row, 1);
        row++;

        destForm->addWidget(enabledCheck, row, 1); // Checkbox in column 1

        /* Initially hide custom URL fields since we start with regular services
         */
        customUrlLabel->setVisible(false);
        customUrlEdit->setVisible(false);

        destFormGroup->setLayout(destForm);
        destDialogLayout->addWidget(destFormGroup);

        /* Info label */
        QLabel *infoLabel = new QLabel(
            "Tip: Select a service and server, then enter your stream key. "
            "For custom RTMP servers, enter the complete URL including the "
            "stream key.");
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: 10px; padding: 10px; }")
                .arg(obs_theme_get_muted_color().name()));
        destDialogLayout->addWidget(infoLabel);

        /* Dialog buttons */
        QDialogButtonBox *destButtonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(destButtonBox, &QDialogButtonBox::accepted, &destDialog,
                &QDialog::accept);
        connect(destButtonBox, &QDialogButtonBox::rejected, &destDialog,
                &QDialog::reject);
        destDialogLayout->addWidget(destButtonBox);

        /* Initialize server list for first service */
        updateServerList();

        /* Show dialog and add destination */
        if (destDialog.exec() == QDialog::Accepted) {
          QString serviceName = serviceCombo->currentText();
          QString streamKey;
          QString rtmpUrl;

          // Determine stream key and RTMP URL based on service type
          if (serviceCombo->currentData().toString() == "custom") {
            // Custom RTMP mode - use the full URL from customUrlEdit
            rtmpUrl = customUrlEdit->text().trimmed();
            if (rtmpUrl.isEmpty()) {
              QMessageBox::warning(&dialog, "Validation Error",
                                   "RTMP URL cannot be empty.");
              return;
            }
            // Extract stream key from URL for display (last path component)
            streamKey = rtmpUrl.section('/', -1);
          } else {
            // Regular service - construct URL from server + stream key
            streamKey = keyEdit->text().trimmed();
            if (streamKey.isEmpty()) {
              QMessageBox::warning(&dialog, "Validation Error",
                                   "Stream key cannot be empty.");
              return;
            }

            QString serverUrl = serverCombo->currentData().toString();
            if (serverUrl.isEmpty()) {
              QMessageBox::warning(&dialog, "Validation Error",
                                   "Please select a server.");
              return;
            }

            // Construct full RTMP URL
            rtmpUrl = serverUrl;
            if (!rtmpUrl.endsWith("/")) {
              rtmpUrl += "/";
            }
            rtmpUrl += streamKey;
          }

          // Map service name to service enum for compatibility
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

          stream_orientation_t targetOrient =
              (stream_orientation_t)targetOrientCombo->currentData().toInt();

          /* Add destination to profile */
          encoding_settings_t defaultEncoding = profile_get_default_encoding();
          if (profile_add_destination(profile, service,
                                      streamKey.toUtf8().constData(),
                                      targetOrient, &defaultEncoding)) {
            /* Update table */
            int row = destTable->rowCount();
            destTable->insertRow(row);

            const char *serviceName =
                restreamer_multistream_get_service_name(service);
            destTable->setItem(row, 0, new QTableWidgetItem(serviceName));

            QString maskedKey = streamKey;
            if (maskedKey.length() > 8) {
              maskedKey = maskedKey.left(4) + "..." + maskedKey.right(4);
            }
            destTable->setItem(row, 1, new QTableWidgetItem(maskedKey));

            QString orientStr;
            switch (targetOrient) {
            case ORIENTATION_HORIZONTAL:
              orientStr = "Horizontal";
              break;
            case ORIENTATION_VERTICAL:
              orientStr = "Vertical";
              break;
            case ORIENTATION_SQUARE:
              orientStr = "Square";
              break;
            default:
              orientStr = "Auto";
              break;
            }
            destTable->setItem(row, 2, new QTableWidgetItem(orientStr));

            QTableWidgetItem *enabledItem = new QTableWidgetItem();
            enabledItem->setCheckState(
                enabledCheck->isChecked() ? Qt::Checked : Qt::Unchecked);
            destTable->setItem(row, 3, enabledItem);

            /* Update enabled state if needed */
            if (!enabledCheck->isChecked()) {
              profile_set_destination_enabled(
                  profile, profile->destination_count - 1, false);
            }
          } else {
            QMessageBox::warning(&dialog, "Error",
                                 "Failed to add destination.");
          }
        }
      });

  /* Remove destination handler */
  connect(removeDestButton, &QPushButton::clicked, [&, destTable, profile]() {
    int currentRow = destTable->currentRow();
    if (currentRow < 0) {
      QMessageBox::information(&dialog, "No Selection",
                               "Please select a destination to remove.");
      return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        &dialog, "Confirm Remove",
        "Are you sure you want to remove this destination?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
      if (profile_remove_destination(profile, currentRow)) {
        destTable->removeRow(currentRow);
      } else {
        QMessageBox::warning(&dialog, "Error", "Failed to remove destination.");
      }
    }
  });

  /* Edit destination handler */
  connect(editDestButton, &QPushButton::clicked, [&, destTable, profile]() {
    int currentRow = destTable->currentRow();
    if (currentRow < 0) {
      QMessageBox::information(&dialog, "No Selection",
                               "Please select a destination to edit.");
      return;
    }

    if ((size_t)currentRow >= profile->destination_count) {
      return;
    }

    profile_destination_t *dest = &profile->destinations[currentRow];

    /* Create edit destination dialog */
    QDialog destDialog(&dialog);
    destDialog.setWindowTitle("Edit Destination");
    destDialog.setMinimumWidth(450);

    QVBoxLayout *destDialogLayout = new QVBoxLayout(&destDialog);

    QGroupBox *destFormGroup = new QGroupBox("Destination Settings");
    QGridLayout *destForm = new QGridLayout();
    destForm->setColumnStretch(1, 1);
    destForm->setHorizontalSpacing(10);
    destForm->setVerticalSpacing(10);

    /* Create labels */
    QLabel *serviceLabel = new QLabel("Service:");
    serviceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel *keyLabel = new QLabel("Stream Key:");
    keyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel *orientLabel = new QLabel("Target Orientation:");
    orientLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    /* Service combo (pre-selected) */
    QComboBox *serviceCombo = new QComboBox();
    serviceCombo->addItem("Custom", (int)SERVICE_CUSTOM);
    serviceCombo->addItem("Twitch", (int)SERVICE_TWITCH);
    serviceCombo->addItem("YouTube", (int)SERVICE_YOUTUBE);
    serviceCombo->addItem("Facebook", (int)SERVICE_FACEBOOK);
    serviceCombo->addItem("Kick", (int)SERVICE_KICK);
    serviceCombo->addItem("TikTok", (int)SERVICE_TIKTOK);
    serviceCombo->addItem("Instagram", (int)SERVICE_INSTAGRAM);
    serviceCombo->addItem("X (Twitter)", (int)SERVICE_X_TWITTER);
    serviceCombo->setCurrentIndex(serviceCombo->findData((int)dest->service));
    serviceCombo->setMinimumWidth(250);

    /* Stream key */
    QLineEdit *keyEdit = new QLineEdit(dest->stream_key);
    keyEdit->setMinimumWidth(250);

    /* Target orientation */
    QComboBox *targetOrientCombo = new QComboBox();
    targetOrientCombo->addItem("Horizontal (16:9)",
                               (int)ORIENTATION_HORIZONTAL);
    targetOrientCombo->addItem("Vertical (9:16)", (int)ORIENTATION_VERTICAL);
    targetOrientCombo->addItem("Square (1:1)", (int)ORIENTATION_SQUARE);
    targetOrientCombo->setCurrentIndex(
        targetOrientCombo->findData((int)dest->target_orientation));
    targetOrientCombo->setMinimumWidth(250);

    /* Enabled checkbox */
    QCheckBox *enabledCheck = new QCheckBox("Enable this destination");
    enabledCheck->setChecked(dest->enabled);

    /* Add widgets to grid layout */
    int row = 0;
    destForm->addWidget(serviceLabel, row, 0);
    destForm->addWidget(serviceCombo, row, 1);
    row++;

    destForm->addWidget(keyLabel, row, 0);
    destForm->addWidget(keyEdit, row, 1);
    row++;

    destForm->addWidget(orientLabel, row, 0);
    destForm->addWidget(targetOrientCombo, row, 1);
    row++;

    destForm->addWidget(enabledCheck, row, 1);

    destFormGroup->setLayout(destForm);
    destDialogLayout->addWidget(destFormGroup);

    /* Dialog buttons */
    QDialogButtonBox *destButtonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(destButtonBox, &QDialogButtonBox::accepted, &destDialog,
            &QDialog::accept);
    connect(destButtonBox, &QDialogButtonBox::rejected, &destDialog,
            &QDialog::reject);
    destDialogLayout->addWidget(destButtonBox);

    /* Show dialog and update destination */
    if (destDialog.exec() == QDialog::Accepted) {
      QString streamKey = keyEdit->text().trimmed();
      if (streamKey.isEmpty()) {
        QMessageBox::warning(&dialog, "Validation Error",
                             "Stream key cannot be empty.");
        return;
      }

      streaming_service_t service =
          (streaming_service_t)serviceCombo->currentData().toInt();
      stream_orientation_t targetOrient =
          (stream_orientation_t)targetOrientCombo->currentData().toInt();

      /* Update destination (remove and re-add with new settings) */
      profile_remove_destination(profile, currentRow);

      encoding_settings_t defaultEncoding = profile_get_default_encoding();
      if (profile_add_destination(profile, service,
                                  streamKey.toUtf8().constData(), targetOrient,
                                  &defaultEncoding)) {
        /* Move the new destination to the correct position */
        if ((size_t)currentRow < profile->destination_count - 1) {
          profile_destination_t temp =
              profile->destinations[profile->destination_count - 1];
          for (size_t i = profile->destination_count - 1;
               i > (size_t)currentRow; i--) {
            profile->destinations[i] = profile->destinations[i - 1];
          }
          profile->destinations[currentRow] = temp;
        }

        /* Update enabled state */
        profile_set_destination_enabled(profile, currentRow,
                                        enabledCheck->isChecked());

        /* Update table */
        const char *serviceName =
            restreamer_multistream_get_service_name(service);
        destTable->item(currentRow, 0)->setText(serviceName);

        QString maskedKey = streamKey;
        if (maskedKey.length() > 8) {
          maskedKey = maskedKey.left(4) + "..." + maskedKey.right(4);
        }
        destTable->item(currentRow, 1)->setText(maskedKey);

        QString orientStr;
        switch (targetOrient) {
        case ORIENTATION_HORIZONTAL:
          orientStr = "Horizontal";
          break;
        case ORIENTATION_VERTICAL:
          orientStr = "Vertical";
          break;
        case ORIENTATION_SQUARE:
          orientStr = "Square";
          break;
        default:
          orientStr = "Auto";
          break;
        }
        destTable->item(currentRow, 2)->setText(orientStr);

        destTable->item(currentRow, 3)
            ->setCheckState(enabledCheck->isChecked() ? Qt::Checked
                                                      : Qt::Unchecked);
      } else {
        QMessageBox::warning(&dialog, "Error", "Failed to update destination.");
      }
    }
  });

  destGroup->setLayout(destLayout);
  mainLayout->addWidget(destGroup);

  /* Notes & Metadata group */
  QGroupBox *notesGroup = new QGroupBox("Notes & Metadata");
  QVBoxLayout *notesLayout = new QVBoxLayout();

  QLabel *notesLabel = new QLabel("Profile Notes (optional):");
  notesLayout->addWidget(notesLabel);

  QTextEdit *notesEdit = new QTextEdit();
  notesEdit->setPlaceholderText(
      "Add notes, tags, or any custom information about this profile...");
  notesEdit->setMaximumHeight(100);

  /* Try to fetch metadata from API if profile has active process */
  if (api && profile->process_reference) {
    char *metadata_value = nullptr;
    if (restreamer_api_get_process_metadata(api, profile->process_reference,
                                            "profile_notes", &metadata_value)) {
      if (metadata_value) {
        notesEdit->setPlainText(QString::fromUtf8(metadata_value));
        bfree(metadata_value);
      }
    }
  }

  notesLayout->addWidget(notesEdit);
  notesGroup->setLayout(notesLayout);
  mainLayout->addWidget(notesGroup);

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
      QMessageBox::warning(this, "Validation Error",
                           "Input URL must include host, application, and "
                           "stream key.\n\nFormat: "
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

    /* Save notes/metadata to API if profile has active process */
    QString notes = notesEdit->toPlainText().trimmed();
    if (api && profile->process_reference && !notes.isEmpty()) {
      restreamer_api_set_process_metadata(api, profile->process_reference,
                                          "profile_notes",
                                          notes.toUtf8().constData());
    }

    updateProfileList();
    updateProfileDetails();
    saveSettings();

    QMessageBox::information(this, "Success", "Profile settings updated.");
  }
}

void RestreamerDock::onProfileListContextMenu(const QPoint &pos) {
  QMenu contextMenu(tr("Profile Actions"), this);

  /* Get the item at the clicked position */
  QListWidgetItem *item = profileListWidget->itemAt(pos);

  if (item) {
    /* Right-clicked on an item - show full menu */
    QString profileId = item->data(Qt::UserRole).toString();
    output_profile_t *profile =
        profileManager ? profile_manager_get_profile(
                             profileManager, profileId.toUtf8().constData())
                       : nullptr;

    /* Create profile */
    QAction *createAction = contextMenu.addAction("Create...");
    connect(createAction, &QAction::triggered, this,
            &RestreamerDock::onCreateProfileClicked);

    contextMenu.addSeparator();

    /* Delete profile (only if inactive) */
    QAction *deleteAction = contextMenu.addAction("Delete");
    deleteAction->setEnabled(profile &&
                             profile->status == PROFILE_STATUS_INACTIVE);
    connect(deleteAction, &QAction::triggered, this,
            &RestreamerDock::onDeleteProfileClicked);

    /* Duplicate profile */
    QAction *duplicateAction = contextMenu.addAction("Duplicate...");
    duplicateAction->setEnabled(profile != nullptr);
    connect(duplicateAction, &QAction::triggered, this,
            &RestreamerDock::onDuplicateProfileClicked);

    /* Configure profile (only if inactive) */
    QAction *configureAction = contextMenu.addAction("Configure...");
    configureAction->setEnabled(profile &&
                                profile->status == PROFILE_STATUS_INACTIVE);
    connect(configureAction, &QAction::triggered, this,
            &RestreamerDock::onConfigureProfileClicked);

    contextMenu.addSeparator();

    /* Start profile (only if inactive) */
    QAction *startAction = contextMenu.addAction("Start");
    startAction->setEnabled(profile &&
                            profile->status == PROFILE_STATUS_INACTIVE);
    connect(startAction, &QAction::triggered, this,
            &RestreamerDock::onStartProfileClicked);

    /* Stop profile (only if active or starting) */
    QAction *stopAction = contextMenu.addAction("Stop");
    stopAction->setEnabled(profile &&
                           (profile->status == PROFILE_STATUS_ACTIVE ||
                            profile->status == PROFILE_STATUS_STARTING));
    connect(stopAction, &QAction::triggered, this,
            &RestreamerDock::onStopProfileClicked);

    contextMenu.addSeparator();

    /* Start all profiles */
    QAction *startAllAction = contextMenu.addAction("Start All");
    startAllAction->setEnabled(profileManager &&
                               profileManager->profile_count > 0);
    connect(startAllAction, &QAction::triggered, this,
            &RestreamerDock::onStartAllProfilesClicked);

    /* Stop all profiles */
    QAction *stopAllAction = contextMenu.addAction("Stop All");
    bool hasActiveProfile = false;
    if (profileManager) {
      for (size_t i = 0; i < profileManager->profile_count; i++) {
        if (profileManager->profiles[i]->status == PROFILE_STATUS_ACTIVE ||
            profileManager->profiles[i]->status == PROFILE_STATUS_STARTING) {
          hasActiveProfile = true;
          break;
        }
      }
    }
    stopAllAction->setEnabled(hasActiveProfile);
    connect(stopAllAction, &QAction::triggered, this,
            &RestreamerDock::onStopAllProfilesClicked);

  } else {
    /* Right-clicked on empty space - show limited menu */
    QAction *createAction = contextMenu.addAction("Create...");
    connect(createAction, &QAction::triggered, this,
            &RestreamerDock::onCreateProfileClicked);

    contextMenu.addSeparator();

    /* Start all profiles */
    QAction *startAllAction = contextMenu.addAction("Start All");
    startAllAction->setEnabled(profileManager &&
                               profileManager->profile_count > 0);
    connect(startAllAction, &QAction::triggered, this,
            &RestreamerDock::onStartAllProfilesClicked);

    /* Stop all profiles */
    QAction *stopAllAction = contextMenu.addAction("Stop All");
    bool hasActiveProfile = false;
    if (profileManager) {
      for (size_t i = 0; i < profileManager->profile_count; i++) {
        if (profileManager->profiles[i]->status == PROFILE_STATUS_ACTIVE ||
            profileManager->profiles[i]->status == PROFILE_STATUS_STARTING) {
          hasActiveProfile = true;
          break;
        }
      }
    }
    stopAllAction->setEnabled(hasActiveProfile);
    connect(stopAllAction, &QAction::triggered, this,
            &RestreamerDock::onStopAllProfilesClicked);
  }

  contextMenu.exec(profileListWidget->mapToGlobal(pos));
}

void RestreamerDock::onProbeInputClicked() {
  if (!api || !selectedProcessId) {
    QMessageBox::warning(this, "No Process Selected",
                         "Please select a process first.");
    return;
  }

  /* Probe the input stream */
  restreamer_probe_info_t info = {0};
  if (!restreamer_api_probe_input(api, selectedProcessId, &info)) {
    QMessageBox::critical(this, "Probe Failed",
                          QString("Failed to probe input: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to display probe information */
  QDialog probeDialog(this);
  probeDialog.setWindowTitle("Input Stream Probe");
  probeDialog.setMinimumWidth(500);

  QVBoxLayout *layout = new QVBoxLayout(&probeDialog);

  /* Format information */
  QGroupBox *formatGroup = new QGroupBox("Format Information");
  QFormLayout *formatLayout = new QFormLayout();
  formatLayout->addRow("Format:",
                       new QLabel(info.format_name ? info.format_name : "-"));
  formatLayout->addRow(
      "Description:",
      new QLabel(info.format_long_name ? info.format_long_name : "-"));
  formatLayout->addRow(
      "Duration:",
      new QLabel(
          QString("%1 seconds").arg(info.duration / 1000000.0, 0, 'f', 2)));
  formatLayout->addRow("Size:", new QLabel(QString("%1 MB").arg(
                                    info.size / 1024.0 / 1024.0, 0, 'f', 2)));
  formatLayout->addRow("Bitrate:",
                       new QLabel(QString("%1 kbps").arg(info.bitrate / 1000)));
  formatGroup->setLayout(formatLayout);
  layout->addWidget(formatGroup);

  /* Stream information table */
  QGroupBox *streamsGroup = new QGroupBox("Streams");
  QVBoxLayout *streamsLayout = new QVBoxLayout();

  QTableWidget *streamsTable = new QTableWidget();
  streamsTable->setColumnCount(5);
  streamsTable->setHorizontalHeaderLabels(
      {"Type", "Codec", "Resolution/Sample Rate", "Bitrate", "Details"});
  streamsTable->horizontalHeader()->setStretchLastSection(true);
  streamsTable->setRowCount(static_cast<int>(info.stream_count));

  for (size_t i = 0; i < info.stream_count; i++) {
    restreamer_stream_info_t *stream = &info.streams[i];
    int row = static_cast<int>(i);

    streamsTable->setItem(
        row, 0,
        new QTableWidgetItem(stream->codec_type ? stream->codec_type : "-"));
    streamsTable->setItem(
        row, 1,
        new QTableWidgetItem(stream->codec_name ? stream->codec_name : "-"));

    /* Resolution for video, sample rate for audio */
    QString resInfo = "-";
    if (stream->codec_type && strcmp(stream->codec_type, "video") == 0 &&
        stream->width > 0) {
      double fps =
          stream->fps_den > 0 ? (double)stream->fps_num / stream->fps_den : 0.0;
      resInfo = QString("%1x%2 @ %3fps")
                    .arg(stream->width)
                    .arg(stream->height)
                    .arg(fps, 0, 'f', 2);
    } else if (stream->codec_type && strcmp(stream->codec_type, "audio") == 0 &&
               stream->sample_rate > 0) {
      resInfo = QString("%1 Hz, %2 ch")
                    .arg(stream->sample_rate)
                    .arg(stream->channels);
    }
    streamsTable->setItem(row, 2, new QTableWidgetItem(resInfo));

    streamsTable->setItem(
        row, 3,
        new QTableWidgetItem(
            stream->bitrate > 0 ? QString("%1 kbps").arg(stream->bitrate / 1000)
                                : "-"));
    streamsTable->setItem(
        row, 4, new QTableWidgetItem(stream->profile ? stream->profile : "-"));
  }

  streamsLayout->addWidget(streamsTable);
  streamsGroup->setLayout(streamsLayout);
  layout->addWidget(streamsGroup);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, &probeDialog,
          &QDialog::accept);
  layout->addWidget(buttonBox);

  probeDialog.exec();

  restreamer_api_free_probe_info(&info);
}

void RestreamerDock::onViewMetricsClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Fetch metrics from API */
  char *metrics_json = nullptr;
  if (!restreamer_api_get_prometheus_metrics(api, &metrics_json)) {
    QMessageBox::critical(this, "Metrics Failed",
                          QString("Failed to fetch metrics: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to display metrics */
  QDialog metricsDialog(this);
  metricsDialog.setWindowTitle("Restreamer Metrics");
  metricsDialog.setMinimumSize(700, 500);

  QVBoxLayout *layout = new QVBoxLayout(&metricsDialog);

  QLabel *infoLabel = new QLabel("Prometheus Metrics (raw format):");
  layout->addWidget(infoLabel);

  QTextEdit *metricsText = new QTextEdit();
  metricsText->setReadOnly(true);
  metricsText->setPlainText(QString::fromUtf8(metrics_json));
  metricsText->setFont(QFont("Courier", 10));
  layout->addWidget(metricsText);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, &metricsDialog,
          &QDialog::accept);
  layout->addWidget(buttonBox);

  metricsDialog.exec();

  bfree(metrics_json);
}

/* Configuration Management */
void RestreamerDock::onViewConfigClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Fetch configuration from API */
  char *config_json = nullptr;
  if (!restreamer_api_get_config(api, &config_json)) {
    QMessageBox::critical(this, "Configuration Failed",
                          QString("Failed to fetch configuration: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to view/edit configuration */
  QDialog configDialog(this);
  configDialog.setWindowTitle("Restreamer Configuration");
  configDialog.setMinimumSize(800, 600);

  QVBoxLayout *layout = new QVBoxLayout(&configDialog);

  QLabel *infoLabel = new QLabel("Restreamer Configuration (JSON format):");
  layout->addWidget(infoLabel);

  QLabel *warningLabel =
      new QLabel("⚠️ Warning: Editing configuration requires careful attention. "
                 "Invalid JSON will be rejected.");
  warningLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
                                  .arg(obs_theme_get_warning_color().name()));
  layout->addWidget(warningLabel);

  QTextEdit *configText = new QTextEdit();
  configText->setPlainText(QString::fromUtf8(config_json));
  configText->setFont(QFont("Courier", 10));
  layout->addWidget(configText);

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
    /* Save configuration back to API */
    QString newConfig = configText->toPlainText();
    if (restreamer_api_set_config(api, newConfig.toUtf8().constData())) {
      QMessageBox::information(&configDialog, "Success",
                               "Configuration updated successfully. You may "
                               "want to reload the configuration.");
      configDialog.accept();
    } else {
      QMessageBox::critical(&configDialog, "Save Failed",
                            QString("Failed to save configuration: %1")
                                .arg(restreamer_api_get_error(api)));
    }
  });
  connect(buttonBox, &QDialogButtonBox::rejected, &configDialog,
          &QDialog::reject);
  layout->addWidget(buttonBox);

  configDialog.exec();

  bfree(config_json);
}

void RestreamerDock::onReloadConfigClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Reload configuration via API */
  if (restreamer_api_reload_config(api)) {
    QMessageBox::information(this, "Success",
                             "Restreamer configuration reloaded successfully.");
  } else {
    QMessageBox::critical(this, "Reload Failed",
                          QString("Failed to reload configuration: %1")
                              .arg(restreamer_api_get_error(api)));
  }
}

/* Advanced Features */
void RestreamerDock::onViewSkillsClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Fetch FFmpeg capabilities from API */
  char *skills_json = nullptr;
  if (!restreamer_api_get_skills(api, &skills_json)) {
    QMessageBox::critical(this, "Skills Failed",
                          QString("Failed to fetch FFmpeg capabilities: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to display skills */
  QDialog skillsDialog(this);
  skillsDialog.setWindowTitle("FFmpeg Capabilities");
  skillsDialog.setMinimumSize(800, 600);

  QVBoxLayout *layout = new QVBoxLayout(&skillsDialog);

  QLabel *infoLabel = new QLabel("FFmpeg Codecs, Formats, and Capabilities:");
  layout->addWidget(infoLabel);

  QTextEdit *skillsText = new QTextEdit();
  skillsText->setReadOnly(true);
  skillsText->setPlainText(QString::fromUtf8(skills_json));
  skillsText->setFont(QFont("Courier", 10));
  layout->addWidget(skillsText);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, &skillsDialog,
          &QDialog::accept);
  layout->addWidget(buttonBox);

  skillsDialog.exec();

  bfree(skills_json);
}

void RestreamerDock::onViewRtmpStreamsClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Fetch RTMP streams from API */
  char *streams_json = nullptr;
  if (!restreamer_api_get_rtmp_streams(api, &streams_json)) {
    QMessageBox::critical(this, "RTMP Streams Failed",
                          QString("Failed to fetch RTMP streams: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to display RTMP streams */
  QDialog streamsDialog(this);
  streamsDialog.setWindowTitle("Active RTMP Streams");
  streamsDialog.setMinimumSize(700, 500);

  QVBoxLayout *layout = new QVBoxLayout(&streamsDialog);

  QLabel *infoLabel = new QLabel("Currently Active RTMP Streams:");
  layout->addWidget(infoLabel);

  QTextEdit *streamsText = new QTextEdit();
  streamsText->setReadOnly(true);
  streamsText->setPlainText(QString::fromUtf8(streams_json));
  streamsText->setFont(QFont("Courier", 10));
  layout->addWidget(streamsText);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, &streamsDialog,
          &QDialog::accept);
  layout->addWidget(buttonBox);

  streamsDialog.exec();

  bfree(streams_json);
}

void RestreamerDock::onViewSrtStreamsClicked() {
  if (!api) {
    QMessageBox::warning(this, "Not Connected",
                         "Please connect to a Restreamer instance first.");
    return;
  }

  /* Fetch SRT streams from API */
  char *streams_json = nullptr;
  if (!restreamer_api_get_srt_streams(api, &streams_json)) {
    QMessageBox::critical(this, "SRT Streams Failed",
                          QString("Failed to fetch SRT streams: %1")
                              .arg(restreamer_api_get_error(api)));
    return;
  }

  /* Create dialog to display SRT streams */
  QDialog streamsDialog(this);
  streamsDialog.setWindowTitle("Active SRT Streams");
  streamsDialog.setMinimumSize(700, 500);

  QVBoxLayout *layout = new QVBoxLayout(&streamsDialog);

  QLabel *infoLabel = new QLabel("Currently Active SRT Streams:");
  layout->addWidget(infoLabel);

  QTextEdit *streamsText = new QTextEdit();
  streamsText->setReadOnly(true);
  streamsText->setPlainText(QString::fromUtf8(streams_json));
  streamsText->setFont(QFont("Courier", 10));
  layout->addWidget(streamsText);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, &streamsDialog,
          &QDialog::accept);
  layout->addWidget(buttonBox);

  streamsDialog.exec();

  bfree(streams_json);
}

/* ===== Section Title Update Helpers ===== */

void RestreamerDock::updateConnectionSectionTitle() {
  if (!connectionSection) {
    return;
  }

  QString status = connectionStatusLabel->text();
  QString title = "Connection";

  if (status == "Connected") {
    title = "Connection ● Connected";
  } else if (status == "Connection failed" ||
             status == "Failed to create API") {
    title = "Connection ● Disconnected";
  }

  connectionSection->setTitle(title);
}

void RestreamerDock::updateBridgeSectionTitle() {
  if (!bridgeSection) {
    return;
  }

  QString status = bridgeStatusLabel->text();
  QString title = "Bridge";

  if (status.contains("Auto-start enabled")) {
    title = "Bridge 🟢 Active";
  } else if (status.contains("Auto-start disabled") ||
             status.contains("idle")) {
    title = "Bridge ⚫ Inactive";
  }

  bridgeSection->setTitle(title);
}

void RestreamerDock::updateProfilesSectionTitle() {
  if (!profilesSection) {
    return;
  }

  QString status = profileStatusLabel->text();
  QString title = "Profiles";

  /* If we have a selected profile, show its name and status */
  if (profileListWidget && profileListWidget->currentItem()) {
    QString profileName = profileListWidget->currentItem()->text();

    if (status.contains("🟢")) {
      title = QString("Profiles - %1 🟢 Active").arg(profileName);
    } else if (status.contains("⚫")) {
      title = QString("Profiles - %1 ⚫ Idle").arg(profileName);
    } else {
      title = QString("Profiles - %1").arg(profileName);
    }
  } else if (profileManager && profileManager->profile_count > 0) {
    title = QString("Profiles (%1)").arg(profileManager->profile_count);
  }

  profilesSection->setTitle(title);
}

void RestreamerDock::updateMonitoringSectionTitle() {
  if (!monitoringSection) {
    return;
  }

  QString state = processStateLabel->text();
  QString title = "Monitoring";

  /* Check if we're monitoring an active process */
  if (state.contains("running") || state.contains("online")) {
    title = "Monitoring 🟢 Active";
  } else if (state.contains("stopped") || state.contains("No process")) {
    title = "Monitoring ⚫ Idle";
  }

  monitoringSection->setTitle(title);
}

void RestreamerDock::updateSystemSectionTitle() {
  if (!systemSection) {
    return;
  }

  /* For now, just show static title - can be enhanced later with server health
   */
  QString title = "System";
  systemSection->setTitle(title);
}

void RestreamerDock::updateAdvancedSectionTitle() {
  if (!advancedSection) {
    return;
  }

  /* For now, just show static title - can be enhanced later with debug mode
   * indicator */
  QString title = "Advanced";
  advancedSection->setTitle(title);
}
