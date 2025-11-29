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
#include "restreamer-channel.h"

/* Forward declare C types */
extern "C" {
typedef struct obs_bridge obs_bridge_t;
}

/* Forward declare Qt classes */
class CollapsibleSection;
class ChannelWidget;
class ConnectionConfigDialog;

class RestreamerDock : public QWidget {
  Q_OBJECT

public:
  RestreamerDock(QWidget *parent = nullptr);
  ~RestreamerDock();

  /* Public accessors for WebSocket API */
  channel_manager_t *getChannelManager() { return channelManager; }
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

  /* Channel management slots */
  void onCreateChannelClicked();
  void onStartAllChannelsClicked();
  void onStopAllChannelsClicked();

  /* ChannelWidget signal handlers */
  void onChannelStartRequested(const char *channelId);
  void onChannelStopRequested(const char *channelId);
  void onChannelEditRequested(const char *channelId);
  void onChannelDeleteRequested(const char *channelId);
  void onChannelDuplicateRequested(const char *channelId);

  /* Output control signal handlers */
  void onOutputStartRequested(const char *channelId, size_t outputIndex);
  void onOutputStopRequested(const char *channelId, size_t outputIndex);
  void onOutputEditRequested(const char *channelId, size_t outputIndex);

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

  /* Monitoring and Log viewer slots */
  void showMonitoringDialog();
  void showLogViewer();

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
  void updateChannelList();
  void updateConnectionStatus();

  restreamer_api_t *api;
  QTimer *updateTimer;

  /* Thread safety */
  std::recursive_mutex apiMutex;
  std::recursive_mutex channelMutex;

  /* Channel manager */
  channel_manager_t *channelManager;

  /* OBS Bridge */
  obs_bridge_t *bridge;

  /* Size restoration for undocking */
  QSize originalSize;
  bool sizeInitialized;

  /* Connection group */
  /* Connection status bar */
  QLabel *connectionIndicator;
  QLabel *connectionStatusLabel;
  QPushButton *configureConnectionButton;

  /* Output Channels group */
  QWidget *channelListContainer;
  QVBoxLayout *channelListLayout;
  QList<ChannelWidget *> channelWidgets;
  QPushButton *createChannelButton;
  QPushButton *startAllChannelsButton;
  QPushButton *stopAllChannelsButton;
  QLabel *channelStatusLabel;

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
