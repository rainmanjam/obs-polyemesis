/*
 * OBS Polyemesis Plugin - Channel Edit Dialog
 */

#pragma once

#include "restreamer-channel.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

class ChannelEditDialog : public QDialog {
  Q_OBJECT

public:
  explicit ChannelEditDialog(stream_channel_t *channel,
                             QWidget *parent = nullptr);
  ~ChannelEditDialog();

  /* Get updated channel settings */
  bool getChannelName(char **name) const;
  stream_orientation_t getSourceOrientation() const;
  bool getAutoDetectOrientation() const;
  uint32_t getSourceWidth() const;
  uint32_t getSourceHeight() const;
  bool getInputUrl(char **url) const;
  bool getAutoStart() const;
  bool getAutoReconnect() const;
  uint32_t getReconnectDelay() const;
  uint32_t getMaxReconnectAttempts() const;
  bool getHealthMonitoringEnabled() const;
  uint32_t getHealthCheckInterval() const;
  uint32_t getFailureThreshold() const;

signals:
  void channelUpdated();

private slots:
  void onSave();
  void onCancel();
  void onOrientationChanged(int index);
  void onAutoDetectChanged(bool checked);
  void onAutoReconnectChanged(bool checked);
  void onHealthMonitoringChanged(bool checked);
  void onOutputSelectionChanged();
  void onAddOutput();
  void onEditOutput();
  void onRemoveOutput();

private:
  void setupUI();
  void loadChannelSettings();
  void validateAndSave();
  bool isValidRtmpUrl(const QString &url);
  void populateOutputsList();
  void updateOutputDetails(int index);
  QString getServiceName(int service) const;
  QString getOrientationName(int orientation) const;

  /* Channel being edited */
  stream_channel_t *m_channel;

  /* UI Elements - General Tab */
  QLineEdit *m_nameEdit;
  QComboBox *m_orientationCombo;
  QCheckBox *m_autoDetectCheckBox;
  QSpinBox *m_sourceWidthSpin;
  QSpinBox *m_sourceHeightSpin;
  QLineEdit *m_inputUrlEdit;

  /* UI Elements - Streaming Tab */
  QCheckBox *m_autoStartCheckBox;
  QCheckBox *m_autoReconnectCheckBox;
  QSpinBox *m_reconnectDelaySpin;
  QSpinBox *m_maxReconnectAttemptsSpin;

  /* UI Elements - Health Monitoring Tab */
  QCheckBox *m_healthMonitoringCheckBox;
  QSpinBox *m_healthCheckIntervalSpin;
  QSpinBox *m_failureThresholdSpin;

  /* UI Elements - Outputs Tab */
  QListWidget *m_outputsList;
  QPushButton *m_addOutputButton;
  QPushButton *m_editOutputButton;
  QPushButton *m_removeOutputButton;
  QLabel *m_outputDetailsLabel;

  /* Dialog buttons */
  QPushButton *m_saveButton;
  QPushButton *m_cancelButton;
  QTabWidget *m_tabWidget;
  QLabel *m_statusLabel;
};
