#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <mutex>

#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include "restreamer-output-profile.h"
#include "obs-service-loader.h"

/* Forward declare C types */
extern "C" {
  typedef struct obs_bridge obs_bridge_t;
}

/* Forward declare Qt classes */
class CollapsibleSection;

class RestreamerDock : public QWidget {
  Q_OBJECT

public:
  RestreamerDock(QWidget *parent = nullptr);
  ~RestreamerDock();

  /* Public accessors for WebSocket API */
  profile_manager_t *getProfileManager() { return profileManager; }
  restreamer_api_t *getApiClient() { return api; }
  obs_bridge_t *getBridge() { return bridge; }

  // cppcheck-suppress unknownMacro
private slots:
  void onRefreshClicked();
  void onTestConnectionClicked();
  void onProcessSelected();
  void onStartProcessClicked();
  void onStopProcessClicked();
  void onRestartProcessClicked();
  void onCreateMultistreamClicked();
  void onAddDestinationClicked();
  void onRemoveDestinationClicked();
  void onSaveSettingsClicked();
  void onUpdateTimer();

  /* Profile management slots */
  void onCreateProfileClicked();
  void onDeleteProfileClicked();
  void onProfileSelected();
  void onStartProfileClicked();
  void onStopProfileClicked();
  void onStartAllProfilesClicked();
  void onStopAllProfilesClicked();
  void onConfigureProfileClicked();
  void onDuplicateProfileClicked();
  void onProfileListContextMenu(const QPoint &pos);

  /* Extended API slots */
  void onProbeInputClicked();
  void onViewMetricsClicked();
  void onViewConfigClicked();
  void onReloadConfigClicked();
  void onViewSkillsClicked();
  void onViewRtmpStreamsClicked();
  void onViewSrtStreamsClicked();

  /* Bridge settings slots */
  void onSaveBridgeSettingsClicked();

private slots:
  /* Dock state change handler */
  void onDockTopLevelChanged(bool floating);

private:
  /* Frontend callbacks for scene collection save/load integration */
  static void frontend_save_callback(obs_data_t *save_data, bool saving, void *private_data);
  void onFrontendSave(obs_data_t *save_data, bool saving);
  void setupUI();
  void loadSettings();
  void saveSettings();
  void updateProcessList();
  void updateProcessDetails();
  void updateSessionList();
  void updateDestinationList();
  void updateProfileList();
  void updateProfileDetails();

  restreamer_api_t *api;
  QTimer *updateTimer;

  /* Thread safety */
  std::recursive_mutex apiMutex;
  std::recursive_mutex profileMutex;

  /* Profile manager */
  profile_manager_t *profileManager;

  /* OBS Bridge */
  obs_bridge_t *bridge;

  /* Size restoration for undocking */
  QSize originalSize;
  bool sizeInitialized;

  /* Connection group */
  QLineEdit *hostEdit;
  QLineEdit *portEdit;
  QCheckBox *httpsCheckbox;
  QLineEdit *usernameEdit;
  QLineEdit *passwordEdit;
  QPushButton *testConnectionButton;
  QLabel *connectionStatusLabel;

  /* Output Profiles group */
  QListWidget *profileListWidget;
  QPushButton *createProfileButton;
  QPushButton *deleteProfileButton;
  QPushButton *duplicateProfileButton;
  QPushButton *configureProfileButton;
  QPushButton *startProfileButton;
  QPushButton *stopProfileButton;
  QPushButton *startAllProfilesButton;
  QPushButton *stopAllProfilesButton;
  QLabel *profileStatusLabel;
  QTableWidget *profileDestinationsTable;

  /* Process list group */
  QListWidget *processList;
  QPushButton *refreshButton;
  QPushButton *startButton;
  QPushButton *stopButton;
  QPushButton *restartButton;

  /* Process details */
  QLabel *processIdLabel;
  QLabel *processStateLabel;
  QLabel *processUptimeLabel;
  QLabel *processCpuLabel;
  QLabel *processMemoryLabel;

  /* Extended process state (from API) */
  QLabel *processFramesLabel;
  QLabel *processDroppedFramesLabel;
  QLabel *processFpsLabel;
  QLabel *processBitrateLabel;
  QLabel *processProgressLabel;
  QPushButton *probeInputButton;
  QPushButton *viewMetricsButton;

  /* Sessions */
  QTableWidget *sessionTable;

  /* Multistream configuration */
  QComboBox *orientationCombo;
  QCheckBox *autoDetectOrientationCheck;
  QTableWidget *destinationsTable;
  QPushButton *addDestinationButton;
  QPushButton *removeDestinationButton;
  QPushButton *createMultistreamButton;

  /* Bridge settings */
  QLineEdit *bridgeHorizontalUrlEdit;
  QLineEdit *bridgeVerticalUrlEdit;
  QCheckBox *bridgeAutoStartCheckbox;
  QPushButton *saveBridgeSettingsButton;
  QLabel *bridgeStatusLabel;

  multistream_config_t *multistreamConfig;
  char *selectedProcessId;

  /* OBS Service Loader */
  OBSServiceLoader *serviceLoader;

  /* Collapsible Section References */
  CollapsibleSection *connectionSection;
  CollapsibleSection *bridgeSection;
  CollapsibleSection *profilesSection;
  CollapsibleSection *monitoringSection;
  CollapsibleSection *systemSection;
  CollapsibleSection *advancedSection;

  /* Quick Action Button References */
  QPushButton *quickProfileToggleButton;

  /* Helper methods for section titles */
  void updateConnectionSectionTitle();
  void updateBridgeSectionTitle();
  void updateProfilesSectionTitle();
  void updateMonitoringSectionTitle();
  void updateSystemSectionTitle();
  void updateAdvancedSectionTitle();
};
