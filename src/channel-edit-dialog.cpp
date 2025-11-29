/*
 * OBS Polyemesis Plugin - Channel Edit Dialog Implementation
 */

#include "channel-edit-dialog.h"
#include "obs-helpers.hpp"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

ChannelEditDialog::ChannelEditDialog(stream_channel_t *channel, QWidget *parent)
    : QDialog(parent), m_channel(channel) {
  if (!m_channel) {
    obs_log(LOG_ERROR, "ChannelEditDialog created with null channel");
    reject();
    return;
  }

  setupUI();
  loadChannelSettings();
}

ChannelEditDialog::~ChannelEditDialog() {
  /* Widgets are deleted automatically by Qt parent/child relationship */
}

void ChannelEditDialog::setupUI() {
  setWindowTitle("Edit Channel");
  setModal(true);
  setMinimumWidth(600);
  setMinimumHeight(500);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(16);
  mainLayout->setContentsMargins(20, 20, 20, 20);

  /* Create tab widget */
  m_tabWidget = new QTabWidget(this);

  /* ===== General Tab ===== */
  QWidget *generalTab = new QWidget();
  QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
  generalLayout->setSpacing(16);

  QGroupBox *basicGroup = new QGroupBox("Basic Information");
  QFormLayout *basicForm = new QFormLayout(basicGroup);

  m_nameEdit = new QLineEdit(this);
  m_nameEdit->setPlaceholderText("Channel Name");
  basicForm->addRow("Channel Name:", m_nameEdit);

  QGroupBox *sourceGroup = new QGroupBox("Source Configuration");
  QFormLayout *sourceForm = new QFormLayout(sourceGroup);

  m_orientationCombo = new QComboBox(this);
  m_orientationCombo->addItem("Auto-Detect", ORIENTATION_AUTO);
  m_orientationCombo->addItem("Horizontal (16:9)", ORIENTATION_HORIZONTAL);
  m_orientationCombo->addItem("Vertical (9:16)", ORIENTATION_VERTICAL);
  m_orientationCombo->addItem("Square (1:1)", ORIENTATION_SQUARE);
  connect(m_orientationCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ChannelEditDialog::onOrientationChanged);
  sourceForm->addRow("Orientation:", m_orientationCombo);

  m_autoDetectCheckBox = new QCheckBox("Auto-detect orientation from source");
  connect(m_autoDetectCheckBox, &QCheckBox::toggled, this,
          &ChannelEditDialog::onAutoDetectChanged);
  sourceForm->addRow("", m_autoDetectCheckBox);

  QHBoxLayout *dimensionsLayout = new QHBoxLayout();
  m_sourceWidthSpin = new QSpinBox(this);
  m_sourceWidthSpin->setRange(0, 7680);
  m_sourceWidthSpin->setSingleStep(2);
  m_sourceWidthSpin->setSpecialValueText("Auto");
  m_sourceWidthSpin->setSuffix(" px");

  m_sourceHeightSpin = new QSpinBox(this);
  m_sourceHeightSpin->setRange(0, 4320);
  m_sourceHeightSpin->setSingleStep(2);
  m_sourceHeightSpin->setSpecialValueText("Auto");
  m_sourceHeightSpin->setSuffix(" px");

  dimensionsLayout->addWidget(new QLabel("Width:"));
  dimensionsLayout->addWidget(m_sourceWidthSpin);
  dimensionsLayout->addWidget(new QLabel("Height:"));
  dimensionsLayout->addWidget(m_sourceHeightSpin);
  dimensionsLayout->addStretch();

  sourceForm->addRow("Source Dimensions:", dimensionsLayout);

  m_inputUrlEdit = new QLineEdit(this);
  m_inputUrlEdit->setPlaceholderText("rtmp://host/app/key");
  sourceForm->addRow("Input URL:", m_inputUrlEdit);

  QLabel *inputHelpLabel =
      new QLabel("<small style='color: #888;'>RTMP input URL for this channel "
                 "(optional)</small>");
  inputHelpLabel->setWordWrap(true);
  sourceForm->addRow("", inputHelpLabel);

  generalLayout->addWidget(basicGroup);
  generalLayout->addWidget(sourceGroup);
  generalLayout->addStretch();

  /* ===== Streaming Tab ===== */
  QWidget *streamingTab = new QWidget();
  QVBoxLayout *streamingLayout = new QVBoxLayout(streamingTab);
  streamingLayout->setSpacing(16);

  QGroupBox *autoStartGroup = new QGroupBox("Auto-Start Settings");
  QVBoxLayout *autoStartLayout = new QVBoxLayout(autoStartGroup);

  m_autoStartCheckBox =
      new QCheckBox("Auto-start channel when OBS streaming starts");
  autoStartLayout->addWidget(m_autoStartCheckBox);

  QLabel *autoStartHelp =
      new QLabel("<small style='color: #888;'>Automatically activate this "
                 "channel when you start streaming in OBS</small>");
  autoStartHelp->setWordWrap(true);
  autoStartLayout->addWidget(autoStartHelp);

  QGroupBox *reconnectGroup = new QGroupBox("Auto-Reconnect Settings");
  QVBoxLayout *reconnectLayout = new QVBoxLayout(reconnectGroup);

  m_autoReconnectCheckBox =
      new QCheckBox("Enable auto-reconnect on disconnect");
  connect(m_autoReconnectCheckBox, &QCheckBox::toggled, this,
          &ChannelEditDialog::onAutoReconnectChanged);
  reconnectLayout->addWidget(m_autoReconnectCheckBox);

  QFormLayout *reconnectForm = new QFormLayout();

  m_reconnectDelaySpin = new QSpinBox(this);
  m_reconnectDelaySpin->setRange(1, 300);
  m_reconnectDelaySpin->setValue(5);
  m_reconnectDelaySpin->setSuffix(" seconds");
  reconnectForm->addRow("Reconnect Delay:", m_reconnectDelaySpin);

  m_maxReconnectAttemptsSpin = new QSpinBox(this);
  m_maxReconnectAttemptsSpin->setRange(0, 999);
  m_maxReconnectAttemptsSpin->setValue(0);
  m_maxReconnectAttemptsSpin->setSpecialValueText("Unlimited");
  reconnectForm->addRow("Max Attempts:", m_maxReconnectAttemptsSpin);

  reconnectLayout->addLayout(reconnectForm);

  QLabel *reconnectHelp = new QLabel(
      "<small style='color: #888;'>Automatically reconnect if the stream "
      "drops. Set max attempts to 0 for unlimited retries.</small>");
  reconnectHelp->setWordWrap(true);
  reconnectLayout->addWidget(reconnectHelp);

  streamingLayout->addWidget(autoStartGroup);
  streamingLayout->addWidget(reconnectGroup);
  streamingLayout->addStretch();

  /* ===== Health Monitoring Tab ===== */
  QWidget *healthTab = new QWidget();
  QVBoxLayout *healthLayout = new QVBoxLayout(healthTab);
  healthLayout->setSpacing(16);

  QGroupBox *healthGroup = new QGroupBox("Health Monitoring");
  QVBoxLayout *healthGroupLayout = new QVBoxLayout(healthGroup);

  m_healthMonitoringCheckBox = new QCheckBox("Enable stream health monitoring");
  connect(m_healthMonitoringCheckBox, &QCheckBox::toggled, this,
          &ChannelEditDialog::onHealthMonitoringChanged);
  healthGroupLayout->addWidget(m_healthMonitoringCheckBox);

  QFormLayout *healthForm = new QFormLayout();

  m_healthCheckIntervalSpin = new QSpinBox(this);
  m_healthCheckIntervalSpin->setRange(5, 300);
  m_healthCheckIntervalSpin->setValue(30);
  m_healthCheckIntervalSpin->setSuffix(" seconds");
  healthForm->addRow("Health Check Interval:", m_healthCheckIntervalSpin);

  m_failureThresholdSpin = new QSpinBox(this);
  m_failureThresholdSpin->setRange(1, 20);
  m_failureThresholdSpin->setValue(3);
  m_failureThresholdSpin->setSuffix(" failures");
  healthForm->addRow("Failure Threshold:", m_failureThresholdSpin);

  healthGroupLayout->addLayout(healthForm);

  QLabel *healthHelp =
      new QLabel("<small style='color: #888;'>Monitor stream health and "
                 "automatically trigger reconnects when issues are detected. "
                 "The failure threshold determines how many consecutive health "
                 "check failures trigger a reconnect.</small>");
  healthHelp->setWordWrap(true);
  healthGroupLayout->addWidget(healthHelp);

  healthLayout->addWidget(healthGroup);
  healthLayout->addStretch();

  /* Add tabs */
  m_tabWidget->addTab(generalTab, "General");
  m_tabWidget->addTab(streamingTab, "Streaming");
  m_tabWidget->addTab(healthTab, "Health Monitoring");

  mainLayout->addWidget(m_tabWidget);

  /* Status Label */
  m_statusLabel = new QLabel(this);
  m_statusLabel->setWordWrap(true);
  m_statusLabel->setStyleSheet("padding: 8px; border-radius: 4px;");
  m_statusLabel->hide();
  mainLayout->addWidget(m_statusLabel);

  /* Dialog Buttons */
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(8);

  m_cancelButton = new QPushButton("Cancel", this);
  m_cancelButton->setMinimumHeight(32);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &ChannelEditDialog::onCancel);

  m_saveButton = new QPushButton("Save", this);
  m_saveButton->setMinimumHeight(32);
  m_saveButton->setDefault(true);
  connect(m_saveButton, &QPushButton::clicked, this,
          &ChannelEditDialog::onSave);

  buttonLayout->addStretch();
  buttonLayout->addWidget(m_cancelButton);
  buttonLayout->addWidget(m_saveButton);

  mainLayout->addLayout(buttonLayout);

  setLayout(mainLayout);
}

void ChannelEditDialog::loadChannelSettings() {
  if (!m_channel) {
    return;
  }

  /* Load basic info */
  if (m_channel->channel_name) {
    m_nameEdit->setText(m_channel->channel_name);
  }

  /* Load source configuration */
  m_orientationCombo->setCurrentIndex(
      m_orientationCombo->findData(m_channel->source_orientation));
  m_autoDetectCheckBox->setChecked(m_channel->auto_detect_orientation);
  m_sourceWidthSpin->setValue(m_channel->source_width);
  m_sourceHeightSpin->setValue(m_channel->source_height);

  if (m_channel->input_url) {
    m_inputUrlEdit->setText(m_channel->input_url);
  }

  /* Load streaming settings */
  m_autoStartCheckBox->setChecked(m_channel->auto_start);
  m_autoReconnectCheckBox->setChecked(m_channel->auto_reconnect);
  m_reconnectDelaySpin->setValue(m_channel->reconnect_delay_sec);
  m_maxReconnectAttemptsSpin->setValue(m_channel->max_reconnect_attempts);

  /* Load health monitoring settings */
  m_healthMonitoringCheckBox->setChecked(m_channel->health_monitoring_enabled);
  m_healthCheckIntervalSpin->setValue(m_channel->health_check_interval_sec);
  m_failureThresholdSpin->setValue(m_channel->failure_threshold);

  /* Update UI state */
  onAutoDetectChanged(m_autoDetectCheckBox->isChecked());
  onAutoReconnectChanged(m_autoReconnectCheckBox->isChecked());
  onHealthMonitoringChanged(m_healthMonitoringCheckBox->isChecked());
}

void ChannelEditDialog::validateAndSave() {
  QString name = m_nameEdit->text().trimmed();

  if (name.isEmpty()) {
    m_statusLabel->setText("⚠️ Channel name cannot be empty");
    m_statusLabel->setStyleSheet("background-color: #5a3a00; color: #ffcc00; "
                                 "padding: 8px; border-radius: 4px;");
    m_statusLabel->show();
    m_tabWidget->setCurrentIndex(0); /* Switch to General tab */
    m_nameEdit->setFocus();
    return;
  }

  /* Update channel settings */
  bfree(m_channel->channel_name);
  m_channel->channel_name = bstrdup(name.toUtf8().constData());

  m_channel->source_orientation = static_cast<stream_orientation_t>(
      m_orientationCombo->currentData().toInt());
  m_channel->auto_detect_orientation = m_autoDetectCheckBox->isChecked();
  m_channel->source_width = m_sourceWidthSpin->value();
  m_channel->source_height = m_sourceHeightSpin->value();

  QString inputUrl = m_inputUrlEdit->text().trimmed();
  bfree(m_channel->input_url);
  m_channel->input_url =
      inputUrl.isEmpty() ? nullptr : bstrdup(inputUrl.toUtf8().constData());

  m_channel->auto_start = m_autoStartCheckBox->isChecked();
  m_channel->auto_reconnect = m_autoReconnectCheckBox->isChecked();
  m_channel->reconnect_delay_sec = m_reconnectDelaySpin->value();
  m_channel->max_reconnect_attempts = m_maxReconnectAttemptsSpin->value();

  m_channel->health_monitoring_enabled =
      m_healthMonitoringCheckBox->isChecked();
  m_channel->health_check_interval_sec = m_healthCheckIntervalSpin->value();
  m_channel->failure_threshold = m_failureThresholdSpin->value();

  obs_log(LOG_INFO, "Channel updated: %s", m_channel->channel_name);

  emit channelUpdated();
  accept();
}

/* Getters */
bool ChannelEditDialog::getChannelName(char **name) const {
  QString text = m_nameEdit->text().trimmed();
  if (text.isEmpty()) {
    return false;
  }
  *name = bstrdup(text.toUtf8().constData());
  return true;
}

stream_orientation_t ChannelEditDialog::getSourceOrientation() const {
  return static_cast<stream_orientation_t>(
      m_orientationCombo->currentData().toInt());
}

bool ChannelEditDialog::getAutoDetectOrientation() const {
  return m_autoDetectCheckBox->isChecked();
}

uint32_t ChannelEditDialog::getSourceWidth() const {
  return m_sourceWidthSpin->value();
}

uint32_t ChannelEditDialog::getSourceHeight() const {
  return m_sourceHeightSpin->value();
}

bool ChannelEditDialog::getInputUrl(char **url) const {
  QString text = m_inputUrlEdit->text().trimmed();
  if (text.isEmpty()) {
    *url = nullptr;
    return false;
  }
  *url = bstrdup(text.toUtf8().constData());
  return true;
}

bool ChannelEditDialog::getAutoStart() const {
  return m_autoStartCheckBox->isChecked();
}

bool ChannelEditDialog::getAutoReconnect() const {
  return m_autoReconnectCheckBox->isChecked();
}

uint32_t ChannelEditDialog::getReconnectDelay() const {
  return m_reconnectDelaySpin->value();
}

uint32_t ChannelEditDialog::getMaxReconnectAttempts() const {
  return m_maxReconnectAttemptsSpin->value();
}

bool ChannelEditDialog::getHealthMonitoringEnabled() const {
  return m_healthMonitoringCheckBox->isChecked();
}

uint32_t ChannelEditDialog::getHealthCheckInterval() const {
  return m_healthCheckIntervalSpin->value();
}

uint32_t ChannelEditDialog::getFailureThreshold() const {
  return m_failureThresholdSpin->value();
}

/* Slots */
void ChannelEditDialog::onSave() { validateAndSave(); }

void ChannelEditDialog::onCancel() { reject(); }

void ChannelEditDialog::onOrientationChanged(int index) {
  stream_orientation_t orientation = static_cast<stream_orientation_t>(
      m_orientationCombo->itemData(index).toInt());

  /* Auto-enable auto-detect if orientation is set to AUTO */
  if (orientation == ORIENTATION_AUTO) {
    m_autoDetectCheckBox->setChecked(true);
  }
}

void ChannelEditDialog::onAutoDetectChanged(bool checked) {
  /* Disable manual dimension inputs when auto-detect is enabled */
  m_sourceWidthSpin->setEnabled(!checked);
  m_sourceHeightSpin->setEnabled(!checked);

  if (checked) {
    m_sourceWidthSpin->setValue(0);
    m_sourceHeightSpin->setValue(0);
  }
}

void ChannelEditDialog::onAutoReconnectChanged(bool checked) {
  m_reconnectDelaySpin->setEnabled(checked);
  m_maxReconnectAttemptsSpin->setEnabled(checked);
}

void ChannelEditDialog::onHealthMonitoringChanged(bool checked) {
  m_healthCheckIntervalSpin->setEnabled(checked);
  m_failureThresholdSpin->setEnabled(checked);
}
