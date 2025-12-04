/*
 * OBS Polyemesis Plugin - Channel Edit Dialog Implementation
 */

#include "channel-edit-dialog.h"
#include "obs-helpers.hpp"
#include "obs-theme-utils.h"
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
      new QLabel("<small>RTMP input URL for this channel (optional)</small>");
  inputHelpLabel->setWordWrap(true);
  inputHelpLabel->setStyleSheet(
      QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
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
      new QLabel("<small>Automatically activate this channel when you start streaming in OBS</small>");
  autoStartHelp->setWordWrap(true);
  autoStartHelp->setStyleSheet(
      QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
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
      "<small>Automatically reconnect if the stream drops. Set max attempts to 0 for unlimited retries.</small>");
  reconnectHelp->setWordWrap(true);
  reconnectHelp->setStyleSheet(
      QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
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

  QLabel *healthHelp = new QLabel(
      "<small>Monitor stream health and automatically trigger reconnects when issues are detected. "
      "The failure threshold determines how many consecutive health check failures trigger a reconnect.</small>");
  healthHelp->setWordWrap(true);
  healthHelp->setStyleSheet(
      QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
  healthGroupLayout->addWidget(healthHelp);

  healthLayout->addWidget(healthGroup);
  healthLayout->addStretch();

  /* ===== Outputs Tab ===== */
  QWidget *outputsTab = new QWidget();
  QVBoxLayout *outputsLayout = new QVBoxLayout(outputsTab);
  outputsLayout->setSpacing(12);

  QGroupBox *outputsGroup = new QGroupBox("Configured Outputs");
  QVBoxLayout *outputsGroupLayout = new QVBoxLayout(outputsGroup);

  m_outputsList = new QListWidget(this);
  m_outputsList->setMinimumHeight(120);
  m_outputsList->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(m_outputsList, &QListWidget::currentRowChanged, this,
          &ChannelEditDialog::onOutputSelectionChanged);
  outputsGroupLayout->addWidget(m_outputsList);

  /* Output action buttons */
  QHBoxLayout *outputButtonLayout = new QHBoxLayout();

  m_addOutputButton = new QPushButton("Add Output...", this);
  m_addOutputButton->setToolTip("Add a new streaming output to this channel");
  connect(m_addOutputButton, &QPushButton::clicked, this,
          &ChannelEditDialog::onAddOutput);
  outputButtonLayout->addWidget(m_addOutputButton);

  m_editOutputButton = new QPushButton("Edit...", this);
  m_editOutputButton->setToolTip("Edit the selected output settings");
  m_editOutputButton->setEnabled(false);
  connect(m_editOutputButton, &QPushButton::clicked, this,
          &ChannelEditDialog::onEditOutput);
  outputButtonLayout->addWidget(m_editOutputButton);

  m_removeOutputButton = new QPushButton("Remove", this);
  m_removeOutputButton->setToolTip("Remove the selected output");
  m_removeOutputButton->setEnabled(false);
  connect(m_removeOutputButton, &QPushButton::clicked, this,
          &ChannelEditDialog::onRemoveOutput);
  outputButtonLayout->addWidget(m_removeOutputButton);

  outputButtonLayout->addStretch();
  outputsGroupLayout->addLayout(outputButtonLayout);

  /* Bulk action buttons */
  QHBoxLayout *bulkButtonLayout = new QHBoxLayout();

  QPushButton *enableAllButton = new QPushButton("Enable All", this);
  enableAllButton->setToolTip("Enable all outputs");
  connect(enableAllButton, &QPushButton::clicked, this, [this]() {
    /* NULL safety check */
    if (!m_channel) {
      QMessageBox::warning(this, "Error", "Channel data is not available.");
      obs_log(LOG_ERROR, "Enable All: m_channel is NULL");
      return;
    }
    if (!m_channel->outputs) {
      obs_log(LOG_WARNING, "Enable All: No outputs available");
      return;
    }
    for (size_t i = 0; i < m_channel->output_count; i++) {
      m_channel->outputs[i].enabled = true;
    }
    populateOutputsList();
    obs_log(LOG_INFO, "Enabled all outputs for channel %s",
            m_channel->channel_name ? m_channel->channel_name : "Unknown");
  });
  bulkButtonLayout->addWidget(enableAllButton);

  QPushButton *disableAllButton = new QPushButton("Disable All", this);
  disableAllButton->setToolTip("Disable all outputs");
  connect(disableAllButton, &QPushButton::clicked, this, [this]() {
    /* NULL safety check */
    if (!m_channel) {
      QMessageBox::warning(this, "Error", "Channel data is not available.");
      obs_log(LOG_ERROR, "Disable All: m_channel is NULL");
      return;
    }
    if (!m_channel->outputs) {
      obs_log(LOG_WARNING, "Disable All: No outputs available");
      return;
    }
    for (size_t i = 0; i < m_channel->output_count; i++) {
      m_channel->outputs[i].enabled = false;
    }
    populateOutputsList();
    obs_log(LOG_INFO, "Disabled all outputs for channel %s",
            m_channel->channel_name ? m_channel->channel_name : "Unknown");
  });
  bulkButtonLayout->addWidget(disableAllButton);

  bulkButtonLayout->addStretch();
  outputsGroupLayout->addLayout(bulkButtonLayout);

  /* Output details panel */
  QGroupBox *detailsGroup = new QGroupBox("Output Details");
  QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);

  m_outputDetailsLabel = new QLabel("Select an output to view details");
  m_outputDetailsLabel->setWordWrap(true);
  m_outputDetailsLabel->setStyleSheet(
      QString("color: %1;").arg(obs_theme_get_muted_color().name()));
  detailsLayout->addWidget(m_outputDetailsLabel);

  outputsLayout->addWidget(outputsGroup);
  outputsLayout->addWidget(detailsGroup);
  outputsLayout->addStretch();

  /* Add tabs */
  m_tabWidget->addTab(generalTab, "General");
  m_tabWidget->addTab(outputsTab, "Outputs");
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

  /* Load outputs */
  populateOutputsList();
}

void ChannelEditDialog::validateAndSave() {
  /* NULL safety check */
  if (!m_channel) {
    obs_log(LOG_ERROR, "ChannelEditDialog::validateAndSave: m_channel is NULL");
    reject();
    return;
  }

  /* Disable save button during operation to prevent double-clicks */
  if (m_saveButton) {
    m_saveButton->setEnabled(false);
  }

  QString name = m_nameEdit->text().trimmed();

  if (name.isEmpty()) {
    if (m_statusLabel) {
      m_statusLabel->setText("⚠️ Channel name cannot be empty");
      m_statusLabel->setStyleSheet("background-color: #5a3a00; color: #ffcc00; "
                                   "padding: 8px; border-radius: 4px;");
      m_statusLabel->show();
    }
    if (m_tabWidget) {
      m_tabWidget->setCurrentIndex(0); /* Switch to General tab */
    }
    if (m_nameEdit) {
      m_nameEdit->setFocus();
    }
    /* Re-enable save button */
    if (m_saveButton) {
      m_saveButton->setEnabled(true);
    }
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

  /* Validate input URL if provided */
  QString inputUrl = m_inputUrlEdit->text().trimmed();
  if (!inputUrl.isEmpty() && !isValidRtmpUrl(inputUrl)) {
    if (m_statusLabel) {
      m_statusLabel->setText(
          "⚠️ Invalid RTMP URL format. Must start with rtmp:// or rtmps:// "
          "and contain a valid host");
      m_statusLabel->setStyleSheet(
          "background-color: #5a3a00; color: #ffcc00; padding: 8px; "
          "border-radius: 4px;");
      m_statusLabel->show();
    }
    if (m_tabWidget) {
      m_tabWidget->setCurrentIndex(0); /* Switch to General tab */
    }
    if (m_inputUrlEdit) {
      m_inputUrlEdit->setFocus();
    }
    /* Re-enable save button */
    if (m_saveButton) {
      m_saveButton->setEnabled(true);
    }
    return;
  }

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

bool ChannelEditDialog::isValidRtmpUrl(const QString &url) {
  /* Check if URL starts with rtmp:// or rtmps:// */
  if (!url.startsWith("rtmp://") && !url.startsWith("rtmps://")) {
    return false;
  }

  /* Extract the part after the protocol */
  int protocolEnd = url.startsWith("rtmps://") ? 8 : 7;
  QString hostPath = url.mid(protocolEnd);

  /* Host/path must not be empty */
  if (hostPath.isEmpty()) {
    return false;
  }

  /* Extract host part (everything before first / or : for port) */
  int hostEnd = hostPath.indexOf('/');
  int portStart = hostPath.indexOf(':');

  /* Determine where host ends */
  int hostLength = hostPath.length();
  if (hostEnd >= 0)
    hostLength = hostEnd;
  if (portStart >= 0 && portStart < hostLength)
    hostLength = portStart;

  QString host = hostPath.left(hostLength);

  /* Host must contain at least one character and should be a valid hostname */
  if (host.isEmpty()) {
    return false;
  }

  /* Basic hostname validation: should contain alphanumeric, dots, hyphens */
  bool hasValidChar = false;
  for (const QChar &c : host) {
    if (c.isLetterOrNumber() || c == '.' || c == '-' || c == '_') {
      hasValidChar = true;
      break;
    }
  }

  return hasValidChar;
}

void ChannelEditDialog::populateOutputsList() {
  m_outputsList->clear();

  if (!m_channel || !m_channel->outputs) {
    return;
  }

  for (size_t i = 0; i < m_channel->output_count; i++) {
    channel_output_t *output = &m_channel->outputs[i];
    QString itemText = QString("%1. %2")
                           .arg(i + 1)
                           .arg(getServiceName(output->service));

    if (output->service_name && strlen(output->service_name) > 0) {
      itemText += QString(" (%1)").arg(output->service_name);
    }

    if (!output->enabled) {
      itemText += " [Disabled]";
    }

    if (output->is_backup) {
      itemText += " [Backup]";
    }

    QListWidgetItem *item = new QListWidgetItem(itemText);
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<int>(i)));
    m_outputsList->addItem(item);
  }

  /* Update button states */
  onOutputSelectionChanged();
}

void ChannelEditDialog::updateOutputDetails(int index) {
  if (!m_channel || !m_channel->outputs || index < 0 ||
      static_cast<size_t>(index) >= m_channel->output_count) {
    m_outputDetailsLabel->setText("Select an output to view details");
    m_outputDetailsLabel->setStyleSheet(
        QString("color: %1;").arg(obs_theme_get_muted_color().name()));
    return;
  }

  channel_output_t *output = &m_channel->outputs[index];

  QString details;
  details += QString("<b>Service:</b> %1<br>").arg(getServiceName(output->service));

  if (output->rtmp_url && strlen(output->rtmp_url) > 0) {
    /* Mask stream key in URL for display */
    QString url = QString(output->rtmp_url);
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash > 0 && lastSlash < url.length() - 1) {
      url = url.left(lastSlash + 1) + "********";
    }
    details += QString("<b>URL:</b> %1<br>").arg(url);
  }

  details += QString("<b>Orientation:</b> %1<br>")
                 .arg(getOrientationName(output->target_orientation));

  details += QString("<b>Status:</b> %1<br>")
                 .arg(output->enabled ? "Enabled" : "Disabled");

  if (output->encoding.width > 0 && output->encoding.height > 0) {
    details += QString("<b>Resolution:</b> %1x%2<br>")
                   .arg(output->encoding.width)
                   .arg(output->encoding.height);
  }

  if (output->encoding.bitrate > 0) {
    details += QString("<b>Bitrate:</b> %1 kbps<br>").arg(output->encoding.bitrate);
  }

  if (output->is_backup) {
    details += QString("<b>Backup for output:</b> #%1<br>")
                   .arg(output->primary_index + 1);
  }

  m_outputDetailsLabel->setText(details);
  m_outputDetailsLabel->setStyleSheet("");
}

QString ChannelEditDialog::getServiceName(int service) const {
  switch (service) {
  case SERVICE_TWITCH:
    return "Twitch";
  case SERVICE_YOUTUBE:
    return "YouTube";
  case SERVICE_FACEBOOK:
    return "Facebook";
  case SERVICE_KICK:
    return "Kick";
  case SERVICE_TIKTOK:
    return "TikTok";
  case SERVICE_INSTAGRAM:
    return "Instagram";
  case SERVICE_X_TWITTER:
    return "X/Twitter";
  case SERVICE_CUSTOM:
    return "Custom RTMP";
  default:
    return "Unknown";
  }
}

QString ChannelEditDialog::getOrientationName(int orientation) const {
  switch (orientation) {
  case ORIENTATION_AUTO:
    return "Auto";
  case ORIENTATION_HORIZONTAL:
    return "Horizontal (16:9)";
  case ORIENTATION_VERTICAL:
    return "Vertical (9:16)";
  case ORIENTATION_SQUARE:
    return "Square (1:1)";
  default:
    return "Unknown";
  }
}

void ChannelEditDialog::onOutputSelectionChanged() {
  int currentRow = m_outputsList->currentRow();
  bool hasSelection = currentRow >= 0;

  m_editOutputButton->setEnabled(hasSelection);
  m_removeOutputButton->setEnabled(hasSelection);

  updateOutputDetails(currentRow);
}

void ChannelEditDialog::onAddOutput() {
  /* NULL safety check */
  if (!m_channel) {
    QMessageBox::warning(this, "Error", "Channel data is not available.");
    obs_log(LOG_ERROR, "ChannelEditDialog::onAddOutput: m_channel is NULL");
    return;
  }

  /* Create a simple dialog to add an output */
  QDialog addDialog(this);
  addDialog.setWindowTitle("Add Output");
  addDialog.setModal(true);
  addDialog.setMinimumWidth(450);

  QVBoxLayout *layout = new QVBoxLayout(&addDialog);
  QFormLayout *form = new QFormLayout();

  /* Service selection */
  QComboBox *serviceCombo = new QComboBox();
  serviceCombo->addItem("Twitch", SERVICE_TWITCH);
  serviceCombo->addItem("YouTube", SERVICE_YOUTUBE);
  serviceCombo->addItem("Facebook", SERVICE_FACEBOOK);
  serviceCombo->addItem("Kick", SERVICE_KICK);
  serviceCombo->addItem("TikTok", SERVICE_TIKTOK);
  serviceCombo->addItem("Instagram", SERVICE_INSTAGRAM);
  serviceCombo->addItem("X/Twitter", SERVICE_X_TWITTER);
  serviceCombo->addItem("Custom RTMP", SERVICE_CUSTOM);
  form->addRow("Service:", serviceCombo);

  /* Stream key */
  QLineEdit *streamKeyEdit = new QLineEdit();
  streamKeyEdit->setPlaceholderText("Enter stream key");
  streamKeyEdit->setEchoMode(QLineEdit::Password);
  form->addRow("Stream Key:", streamKeyEdit);

  /* Custom RTMP URL (shown only for custom RTMP) */
  QLabel *rtmpUrlLabel = new QLabel("RTMP URL:");
  QLineEdit *rtmpUrlEdit = new QLineEdit();
  rtmpUrlEdit->setPlaceholderText("rtmp://server/app");
  rtmpUrlLabel->hide();
  rtmpUrlEdit->hide();
  form->addRow(rtmpUrlLabel, rtmpUrlEdit);

  /* Show/hide RTMP URL based on service */
  connect(serviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [serviceCombo, rtmpUrlLabel, rtmpUrlEdit]() {
            bool isCustom =
                serviceCombo->currentData().toInt() == SERVICE_CUSTOM;
            rtmpUrlLabel->setVisible(isCustom);
            rtmpUrlEdit->setVisible(isCustom);
          });

  /* Orientation */
  QComboBox *orientationCombo = new QComboBox();
  orientationCombo->addItem("Auto", ORIENTATION_AUTO);
  orientationCombo->addItem("Horizontal (16:9)", ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (9:16)", ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square (1:1)", ORIENTATION_SQUARE);
  form->addRow("Orientation:", orientationCombo);

  /* Quality Preset - simplified template feature */
  QComboBox *qualityPresetCombo = new QComboBox();
  qualityPresetCombo->addItem("Auto (Use Source)", 0);
  qualityPresetCombo->addItem("1080p High Quality (6000 kbps)", 6000);
  qualityPresetCombo->addItem("1080p Standard (4500 kbps)", 4500);
  qualityPresetCombo->addItem("720p High Quality (4000 kbps)", 4000);
  qualityPresetCombo->addItem("720p Standard (2500 kbps)", 2500);
  qualityPresetCombo->addItem("480p (1500 kbps)", 1500);
  qualityPresetCombo->addItem("Low Bandwidth (800 kbps)", 800);
  qualityPresetCombo->setToolTip(
      "Select a quality preset or 'Auto' to use source settings");
  form->addRow("Quality Preset:", qualityPresetCombo);

  layout->addLayout(form);

  /* Buttons */
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  QPushButton *cancelBtn = new QPushButton("Cancel");
  QPushButton *addBtn = new QPushButton("Add");
  addBtn->setDefault(true);

  buttonLayout->addStretch();
  buttonLayout->addWidget(cancelBtn);
  buttonLayout->addWidget(addBtn);
  layout->addLayout(buttonLayout);

  connect(cancelBtn, &QPushButton::clicked, &addDialog, &QDialog::reject);
  connect(addBtn, &QPushButton::clicked, [&]() {
    QString streamKey = streamKeyEdit->text().trimmed();
    if (streamKey.isEmpty()) {
      QMessageBox::warning(&addDialog, "Validation Error",
                           "Stream key is required.");
      return;
    }

    int service = serviceCombo->currentData().toInt();
    if (service == SERVICE_CUSTOM) {
      QString rtmpUrl = rtmpUrlEdit->text().trimmed();
      if (rtmpUrl.isEmpty() || !isValidRtmpUrl(rtmpUrl)) {
        QMessageBox::warning(&addDialog, "Validation Error",
                             "Valid RTMP URL is required for custom service.");
        return;
      }
    }

    addDialog.accept();
  });

  if (addDialog.exec() == QDialog::Accepted) {
    /* Add the output to the channel */
    streaming_service_t service =
        static_cast<streaming_service_t>(serviceCombo->currentData().toInt());
    stream_orientation_t orientation = static_cast<stream_orientation_t>(
        orientationCombo->currentData().toInt());
    QString streamKey = streamKeyEdit->text().trimmed();

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Apply quality preset */
    uint32_t presetBitrate = qualityPresetCombo->currentData().toUInt();
    if (presetBitrate > 0) {
      encoding.bitrate = presetBitrate;
      /* Set resolution based on bitrate tier */
      if (presetBitrate >= 4500) {
        encoding.width = 1920;
        encoding.height = 1080;
      } else if (presetBitrate >= 2500) {
        encoding.width = 1280;
        encoding.height = 720;
      } else {
        encoding.width = 854;
        encoding.height = 480;
      }
    }

    if (channel_add_output(m_channel, service, streamKey.toUtf8().constData(),
                           orientation, &encoding)) {
      /* Set custom RTMP URL if needed */
      if (service == SERVICE_CUSTOM && m_channel->output_count > 0) {
        channel_output_t *newOutput =
            &m_channel->outputs[m_channel->output_count - 1];
        if (newOutput->rtmp_url) {
          bfree(newOutput->rtmp_url);
        }
        QString fullUrl = rtmpUrlEdit->text().trimmed() + "/" + streamKey;
        newOutput->rtmp_url = bstrdup(fullUrl.toUtf8().constData());
      }

      populateOutputsList();
      obs_log(LOG_INFO, "Output added to channel %s",
              m_channel->channel_name ? m_channel->channel_name : "Unknown");
    } else {
      QMessageBox::critical(this, "Error",
          "Failed to add output to channel. Please check the configuration and try again.");
      obs_log(LOG_ERROR, "Failed to add output to channel");
    }
  }
}

void ChannelEditDialog::onEditOutput() {
  int currentRow = m_outputsList->currentRow();

  /* NULL safety and bounds check */
  if (currentRow < 0) {
    QMessageBox::warning(this, "No Selection", "Please select an output to edit.");
    return;
  }

  if (!m_channel) {
    QMessageBox::warning(this, "Error", "Channel data is not available.");
    obs_log(LOG_ERROR, "ChannelEditDialog::onEditOutput: m_channel is NULL");
    return;
  }

  if (!m_channel->outputs || static_cast<size_t>(currentRow) >= m_channel->output_count) {
    QMessageBox::warning(this, "Error", "Selected output is no longer available.");
    obs_log(LOG_ERROR, "ChannelEditDialog::onEditOutput: Invalid output index %d", currentRow);
    return;
  }

  channel_output_t *output = &m_channel->outputs[currentRow];

  /* Create edit dialog */
  QDialog editDialog(this);
  editDialog.setWindowTitle("Edit Output");
  editDialog.setModal(true);
  editDialog.setMinimumWidth(450);

  QVBoxLayout *layout = new QVBoxLayout(&editDialog);
  QFormLayout *form = new QFormLayout();

  /* Service (read-only) */
  QLabel *serviceLabel = new QLabel(getServiceName(output->service));
  form->addRow("Service:", serviceLabel);

  /* Stream key */
  QLineEdit *streamKeyEdit = new QLineEdit();
  if (output->stream_key) {
    streamKeyEdit->setText(output->stream_key);
  }
  streamKeyEdit->setEchoMode(QLineEdit::Password);
  form->addRow("Stream Key:", streamKeyEdit);

  /* Orientation */
  QComboBox *orientationCombo = new QComboBox();
  orientationCombo->addItem("Auto", ORIENTATION_AUTO);
  orientationCombo->addItem("Horizontal (16:9)", ORIENTATION_HORIZONTAL);
  orientationCombo->addItem("Vertical (9:16)", ORIENTATION_VERTICAL);
  orientationCombo->addItem("Square (1:1)", ORIENTATION_SQUARE);
  orientationCombo->setCurrentIndex(
      orientationCombo->findData(output->target_orientation));
  form->addRow("Orientation:", orientationCombo);

  /* Enabled checkbox */
  QCheckBox *enabledCheckBox = new QCheckBox("Output Enabled");
  enabledCheckBox->setChecked(output->enabled);
  form->addRow("", enabledCheckBox);

  /* Encoding settings group */
  QGroupBox *encodingGroup = new QGroupBox("Encoding Settings");
  QFormLayout *encodingForm = new QFormLayout(encodingGroup);

  QSpinBox *widthSpin = new QSpinBox();
  widthSpin->setRange(0, 7680);
  widthSpin->setValue(output->encoding.width);
  widthSpin->setSpecialValueText("Auto");
  encodingForm->addRow("Width:", widthSpin);

  QSpinBox *heightSpin = new QSpinBox();
  heightSpin->setRange(0, 4320);
  heightSpin->setValue(output->encoding.height);
  heightSpin->setSpecialValueText("Auto");
  encodingForm->addRow("Height:", heightSpin);

  QSpinBox *bitrateSpin = new QSpinBox();
  bitrateSpin->setRange(0, 50000);
  bitrateSpin->setValue(output->encoding.bitrate);
  bitrateSpin->setSuffix(" kbps");
  bitrateSpin->setSpecialValueText("Default");
  encodingForm->addRow("Video Bitrate:", bitrateSpin);

  QSpinBox *audioBitrateSpin = new QSpinBox();
  audioBitrateSpin->setRange(0, 512);
  audioBitrateSpin->setValue(output->encoding.audio_bitrate);
  audioBitrateSpin->setSuffix(" kbps");
  audioBitrateSpin->setSpecialValueText("Default");
  encodingForm->addRow("Audio Bitrate:", audioBitrateSpin);

  layout->addLayout(form);
  layout->addWidget(encodingGroup);

  /* Backup/Failover settings group */
  QGroupBox *backupGroup = new QGroupBox("Backup/Failover Settings");
  QVBoxLayout *backupLayout = new QVBoxLayout(backupGroup);

  QCheckBox *isBackupCheckBox = new QCheckBox("This is a backup output");
  isBackupCheckBox->setChecked(output->is_backup);
  isBackupCheckBox->setToolTip(
      "Enable to use this output as a backup when a primary output fails");
  backupLayout->addWidget(isBackupCheckBox);

  QHBoxLayout *primaryLayout = new QHBoxLayout();
  QLabel *primaryLabel = new QLabel("Primary Output:");
  QComboBox *primaryCombo = new QComboBox();
  primaryCombo->addItem("None", -1);

  /* Populate with other outputs (excluding this one) */
  for (size_t i = 0; i < m_channel->output_count; i++) {
    if (i != static_cast<size_t>(currentRow)) {
      channel_output_t *otherOutput = &m_channel->outputs[i];
      QString name = QString("#%1 - %2")
                         .arg(i + 1)
                         .arg(getServiceName(otherOutput->service));
      primaryCombo->addItem(name, static_cast<int>(i));
    }
  }

  if (output->is_backup) {
    int idx = primaryCombo->findData(static_cast<int>(output->primary_index));
    if (idx >= 0)
      primaryCombo->setCurrentIndex(idx);
  }

  primaryLabel->setEnabled(output->is_backup);
  primaryCombo->setEnabled(output->is_backup);

  primaryLayout->addWidget(primaryLabel);
  primaryLayout->addWidget(primaryCombo);
  primaryLayout->addStretch();
  backupLayout->addLayout(primaryLayout);

  /* Connect checkbox to enable/disable primary selection */
  connect(isBackupCheckBox, &QCheckBox::toggled,
          [primaryLabel, primaryCombo](bool checked) {
            primaryLabel->setEnabled(checked);
            primaryCombo->setEnabled(checked);
          });

  QCheckBox *autoReconnectCheckBox =
      new QCheckBox("Auto-reconnect on failure");
  autoReconnectCheckBox->setChecked(output->auto_reconnect_enabled);
  autoReconnectCheckBox->setToolTip(
      "Automatically attempt to reconnect if this output disconnects");
  backupLayout->addWidget(autoReconnectCheckBox);

  QLabel *backupHelp = new QLabel(
      "<small>Backup outputs activate automatically when the primary fails. "
      "Auto-reconnect tries to restore the connection before triggering failover.</small>");
  backupHelp->setWordWrap(true);
  backupHelp->setStyleSheet(
      QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
  backupLayout->addWidget(backupHelp);

  layout->addWidget(backupGroup);

  /* Buttons */
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  QPushButton *cancelBtn = new QPushButton("Cancel");
  QPushButton *saveBtn = new QPushButton("Save");
  saveBtn->setDefault(true);

  buttonLayout->addStretch();
  buttonLayout->addWidget(cancelBtn);
  buttonLayout->addWidget(saveBtn);
  layout->addLayout(buttonLayout);

  connect(cancelBtn, &QPushButton::clicked, &editDialog, &QDialog::reject);
  connect(saveBtn, &QPushButton::clicked, &editDialog, &QDialog::accept);

  if (editDialog.exec() == QDialog::Accepted) {
    /* Update output settings */
    if (output->stream_key) {
      bfree(output->stream_key);
    }
    output->stream_key =
        bstrdup(streamKeyEdit->text().trimmed().toUtf8().constData());

    output->target_orientation = static_cast<stream_orientation_t>(
        orientationCombo->currentData().toInt());
    output->enabled = enabledCheckBox->isChecked();

    output->encoding.width = widthSpin->value();
    output->encoding.height = heightSpin->value();
    output->encoding.bitrate = bitrateSpin->value();
    output->encoding.audio_bitrate = audioBitrateSpin->value();

    /* Update backup settings */
    bool wasBackup = output->is_backup;
    bool nowBackup = isBackupCheckBox->isChecked();
    output->auto_reconnect_enabled = autoReconnectCheckBox->isChecked();

    if (nowBackup && primaryCombo->currentData().toInt() >= 0) {
      size_t primaryIdx = static_cast<size_t>(primaryCombo->currentData().toInt());
      if (!wasBackup || output->primary_index != primaryIdx) {
        /* Set up new backup relationship */
        channel_set_output_backup(m_channel, primaryIdx,
                                  static_cast<size_t>(currentRow));
      }
    } else if (wasBackup && !nowBackup) {
      /* Remove backup relationship */
      channel_remove_output_backup(m_channel, output->primary_index);
    }

    populateOutputsList();
    obs_log(LOG_INFO, "Output %zu updated in channel %s",
            static_cast<size_t>(currentRow),
            m_channel->channel_name ? m_channel->channel_name : "Unknown");
  }
}

void ChannelEditDialog::onRemoveOutput() {
  int currentRow = m_outputsList->currentRow();

  /* NULL safety and bounds check */
  if (currentRow < 0) {
    QMessageBox::warning(this, "No Selection", "Please select an output to remove.");
    return;
  }

  if (!m_channel) {
    QMessageBox::warning(this, "Error", "Channel data is not available.");
    obs_log(LOG_ERROR, "ChannelEditDialog::onRemoveOutput: m_channel is NULL");
    return;
  }

  QMessageBox::StandardButton reply = QMessageBox::question(
      this, "Remove Output",
      QString("Are you sure you want to remove output #%1?").arg(currentRow + 1),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    if (channel_remove_output(m_channel, static_cast<size_t>(currentRow))) {
      populateOutputsList();
      obs_log(LOG_INFO, "Output %d removed from channel %s", currentRow,
              m_channel->channel_name ? m_channel->channel_name : "Unknown");
    } else {
      QMessageBox::critical(this, "Error",
          "Failed to remove output. The output may be in use or already removed.");
      obs_log(LOG_ERROR, "Failed to remove output %d from channel", currentRow);
    }
  }
}
