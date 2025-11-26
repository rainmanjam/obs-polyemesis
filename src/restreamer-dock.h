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

#include "obs-service-loader.h"
#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include "restreamer-output-profile.h"

/* Forward declare C types */
extern "C" {
typedef struct obs_bridge obs_bridge_t;
}

/* Forward declare Qt classes */
class CollapsibleSection;
class ProfileWidget;
class ConnectionConfigDialog;

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
  void onConfigureConnectionClicked();
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
  void onStartAllProfilesClicked();
  void onStopAllProfilesClicked();

  /* ProfileWidget signal handlers */
  void onProfileStartRequested(const char *profileId);
  void onProfileStopRequested(const char *profileId);
  void onProfileEditRequested(const char *profileId);
  void onProfileDeleteRequested(const char *profileId);
  void onProfileDuplicateRequested(const char *profileId);

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
  static void frontend_save_callback(obs_data_t *save_data, bool saving,
                                     void *private_data);
  void onFrontendSave(obs_data_t *save_data, bool saving);
  void setupUI();
  void loadSettings();
  void saveSettings();
  void updateProcessList();
  void updateProcessDetails();
  void updateSessionList();
  void updateDestinationList();
  void updateProfileList();

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
  /* Connection status bar */
  QLabel *connectionStatusLabel;
  QPushButton *configureConnectionButton;

  /* Output Profiles group */
  QWidget *profileListContainer;
  QVBoxLayout *profileListLayout;
  QList<ProfileWidget *> profileWidgets;
  QPushButton *createProfileButton;
  QPushButton *startAllProfilesButton;
  QPushButton *stopAllProfilesButton;
  QLabel *profileStatusLabel;

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
};
