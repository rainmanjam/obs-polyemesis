#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "restreamer-api.h"
#include "restreamer-multistream.h"
#include "restreamer-output-profile.h"

class RestreamerDock : public QDockWidget {
  Q_OBJECT

public:
  RestreamerDock(QWidget *parent = nullptr);
  ~RestreamerDock();

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

private:
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

  /* Profile manager */
  profile_manager_t *profileManager;

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

  /* Sessions */
  QTableWidget *sessionTable;

  /* Multistream configuration */
  QComboBox *orientationCombo;
  QCheckBox *autoDetectOrientationCheck;
  QTableWidget *destinationsTable;
  QPushButton *addDestinationButton;
  QPushButton *removeDestinationButton;
  QPushButton *createMultistreamButton;

  multistream_config_t *multistreamConfig;
  char *selectedProcessId;
};
