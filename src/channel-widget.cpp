/*
 * OBS Polyemesis Plugin - Channel Widget Implementation
 */

#include "channel-widget.h"
#include "output-widget.h"
#include "obs-theme-utils.h"

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

ChannelWidget::ChannelWidget(stream_channel_t *channel, QWidget *parent)
    : QWidget(parent), m_channel(channel), m_expanded(false), m_hovered(false) {
  obs_log(LOG_INFO, "[ChannelWidget] Creating ChannelWidget for channel: %s",
          channel ? channel->channel_name : "NULL");
  setupUI();
  updateFromChannel();
  obs_log(LOG_INFO, "[ChannelWidget] ChannelWidget created successfully");
}

ChannelWidget::~ChannelWidget() {
  /* Widgets are deleted automatically by Qt parent/child relationship */
}

void ChannelWidget::setupUI() {
  m_mainLayout = new QVBoxLayout(this);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_mainLayout->setSpacing(0);

  /* === Header Widget === */
  m_headerWidget = new QWidget(this);
  m_headerWidget->setObjectName("channelHeader");
  m_headerWidget->setCursor(Qt::PointingHandCursor);

  m_headerLayout = new QHBoxLayout(m_headerWidget);
  m_headerLayout->setContentsMargins(12, 12, 12, 12);
  m_headerLayout->setSpacing(12);

  /* Status indicator */
  m_statusIndicator = new QLabel(this);
  m_statusIndicator->setStyleSheet("font-size: 18px;");

  /* Channel info */
  QWidget *infoWidget = new QWidget(this);
  QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
  infoLayout->setContentsMargins(0, 0, 0, 0);
  infoLayout->setSpacing(2);

  m_nameLabel = new QLabel(this);
  m_nameLabel->setStyleSheet("font-weight: 600; font-size: 14px;");

  m_summaryLabel = new QLabel(this);
  QColor mutedColor = obs_theme_get_muted_color();
  m_summaryLabel->setStyleSheet(
      QString("font-size: 11px; color: %1;").arg(mutedColor.name()));

  infoLayout->addWidget(m_nameLabel);
  infoLayout->addWidget(m_summaryLabel);

  /* Header actions */
  m_startStopButton = new QPushButton(this);
  m_startStopButton->setFixedSize(70, 28);
  connect(m_startStopButton, &QPushButton::clicked, this,
          &ChannelWidget::onStartStopClicked);

  m_editButton = new QPushButton("Edit", this);
  m_editButton->setFixedSize(60, 28);
  connect(m_editButton, &QPushButton::clicked, this,
          &ChannelWidget::onEditClicked);

  m_menuButton = new QPushButton("⋮", this);
  m_menuButton->setFixedSize(28, 28);
  m_menuButton->setStyleSheet("font-size: 16px;");
  connect(m_menuButton, &QPushButton::clicked, this,
          &ChannelWidget::onMenuClicked);

  /* Add to header layout */
  m_headerLayout->addWidget(m_statusIndicator);
  m_headerLayout->addWidget(infoWidget, 1); // Stretch
  m_headerLayout->addWidget(m_startStopButton);
  m_headerLayout->addWidget(m_editButton);
  m_headerLayout->addWidget(m_menuButton);

  /* Make header clickable */
  m_headerWidget->installEventFilter(this);

  m_mainLayout->addWidget(m_headerWidget);

  /* === Content Widget (Outputs) === */
  m_contentWidget = new QWidget(this);
  m_contentWidget->setVisible(false);

  m_contentLayout = new QVBoxLayout(m_contentWidget);
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  m_contentLayout->setSpacing(0);

  m_mainLayout->addWidget(m_contentWidget);

  /* Set minimum size to ensure widget is visible */
  setMinimumHeight(80);
  m_headerWidget->setMinimumHeight(60);

  /* Style the widget - BRIGHT GREEN BORDER FOR TESTING */
  setStyleSheet("ChannelWidget { "
                "  background-color: #2d2d30; "
                "  border: 5px solid #00ff00; "
                "  border-radius: 8px; "
                "  margin: 8px; "
                "  padding: 4px; "
                "} "
                "#channelHeader { "
                "  background-color: #3d3d40; "
                "  border-bottom: 2px solid #00ff00; "
                "  padding: 8px; "
                "} "
                "#channelHeader:hover { "
                "  background-color: #4d4d50; "
                "}");
}

void ChannelWidget::updateFromChannel() {
  if (!m_channel) {
    return;
  }

  updateHeader();
  updateOutputs();
}

void ChannelWidget::updateHeader() {
  if (!m_channel) {
    return;
  }

  /* Update name */
  m_nameLabel->setText(m_channel->channel_name);

  /* Update status indicator */
  QString statusIcon = getStatusIcon();
  QColor statusColor = getStatusColor();

  m_statusIndicator->setText(statusIcon);
  m_statusIndicator->setStyleSheet(
      QString("font-size: 18px; color: %1;").arg(statusColor.name()));

  /* Update summary */
  m_summaryLabel->setText(getSummaryText());

  /* Update start/stop button */
  if (m_channel->status == CHANNEL_STATUS_ACTIVE ||
      m_channel->status == CHANNEL_STATUS_STARTING) {
    m_startStopButton->setText("■ Stop");
    m_startStopButton->setProperty("danger", true);
  } else {
    m_startStopButton->setText("▶ Start");
    m_startStopButton->setProperty("danger", false);
  }
  m_startStopButton->style()->unpolish(m_startStopButton);
  m_startStopButton->style()->polish(m_startStopButton);
}

void ChannelWidget::updateOutputs() {
  if (!m_channel) {
    return;
  }

  /* Clear existing output widgets */
  qDeleteAll(m_outputWidgets);
  m_outputWidgets.clear();

  /* Create widget for each output */
  for (size_t i = 0; i < m_channel->output_count; i++) {
    channel_output_t *dest = &m_channel->outputs[i];

    OutputWidget *outputWidget =
        new OutputWidget(dest, i, m_channel->channel_id, this);

    /* Connect signals */
    connect(outputWidget, &OutputWidget::startRequested, this,
            &ChannelWidget::onOutputStartRequested);
    connect(outputWidget, &OutputWidget::stopRequested, this,
            &ChannelWidget::onOutputStopRequested);
    connect(outputWidget, &OutputWidget::editRequested, this,
            &ChannelWidget::onOutputEditRequested);

    m_contentLayout->addWidget(outputWidget);
    m_outputWidgets.append(outputWidget);
  }
}

void ChannelWidget::setExpanded(bool expanded) {
  if (m_expanded == expanded) {
    return;
  }

  m_expanded = expanded;
  m_contentWidget->setVisible(m_expanded);

  /* Update header border */
  if (m_expanded) {
    m_headerWidget->setStyleSheet("#channelHeader { "
                                  "  border-bottom: 1px solid palette(mid); "
                                  "}");
  } else {
    m_headerWidget->setStyleSheet("#channelHeader { "
                                  "  border-bottom: none; "
                                  "}");
  }

  emit expandedChanged(m_expanded);
}

const char *ChannelWidget::getChannelId() const {
  return m_channel ? m_channel->channel_id : nullptr;
}

QString ChannelWidget::getAggregateStatus() const {
  if (!m_channel) {
    return "inactive";
  }

  if (m_channel->status == CHANNEL_STATUS_ACTIVE) {
    /* Check for errors in outputs (enabled but not connected) */
    for (size_t i = 0; i < m_channel->output_count; i++) {
      if (m_channel->outputs[i].enabled &&
          !m_channel->outputs[i].connected) {
        return "error";
      }
    }

    return "active";
  } else if (m_channel->status == CHANNEL_STATUS_STARTING) {
    return "starting";
  } else if (m_channel->status == CHANNEL_STATUS_ERROR) {
    return "error";
  }

  return "inactive";
}

QString ChannelWidget::getSummaryText() const {
  if (!m_channel) {
    return "";
  }

  int activeCount = 0;
  int errorCount = 0;
  int totalCount = (int)m_channel->output_count;

  for (size_t i = 0; i < m_channel->output_count; i++) {
    /* Status based on connected and enabled flags */
    if (m_channel->outputs[i].connected &&
        m_channel->outputs[i].enabled) {
      activeCount++;
    } else if (m_channel->outputs[i].enabled &&
               !m_channel->outputs[i].connected) {
      errorCount++;
    }
  }

  if (m_channel->status == CHANNEL_STATUS_INACTIVE) {
    if (totalCount == 1) {
      return "1 output";
    }
    return QString("%1 outputs").arg(totalCount);
  } else if (m_channel->status == CHANNEL_STATUS_STARTING) {
    return QString("Starting %1 output%2...")
        .arg(totalCount)
        .arg(totalCount != 1 ? "s" : "");
  } else {
    QStringList parts;
    if (activeCount > 0) {
      parts.append(QString("%1 active").arg(activeCount));
    }
    if (errorCount > 0) {
      parts.append(QString("%1 error%2")
                       .arg(errorCount)
                       .arg(errorCount != 1 ? "s" : ""));
    }
    if (!parts.isEmpty()) {
      return parts.join(", ");
    }
    return QString("%1 outputs").arg(totalCount);
  }
}

QColor ChannelWidget::getStatusColor() const {
  QString status = getAggregateStatus();

  if (status == "active") {
    return obs_theme_get_success_color();
  } else if (status == "starting") {
    return obs_theme_get_warning_color();
  } else if (status == "error") {
    return obs_theme_get_error_color();
  }

  return obs_theme_get_muted_color();
}

QString ChannelWidget::getStatusIcon() const {
  QString status = getAggregateStatus();

  if (status == "active") {
    return "🟢";
  } else if (status == "starting") {
    return "🟡";
  } else if (status == "error") {
    return "🔴";
  }

  return "⚫";
}

void ChannelWidget::contextMenuEvent(QContextMenuEvent *event) {
  showContextMenu(event->pos());
  event->accept();
}

void ChannelWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    /* Toggle expansion on double-click */
    setExpanded(!m_expanded);
    event->accept();
  } else {
    QWidget::mouseDoubleClickEvent(event);
  }
}

void ChannelWidget::enterEvent(QEnterEvent *event) {
  m_hovered = true;
  QWidget::enterEvent(event);
}

void ChannelWidget::leaveEvent(QEvent *event) {
  m_hovered = false;
  QWidget::leaveEvent(event);
}

void ChannelWidget::onHeaderClicked() {
  /* Toggle expansion */
  setExpanded(!m_expanded);
}

void ChannelWidget::onStartStopClicked() {
  if (!m_channel) {
    return;
  }

  if (m_channel->status == CHANNEL_STATUS_ACTIVE ||
      m_channel->status == CHANNEL_STATUS_STARTING) {
    emit stopRequested(m_channel->channel_id);
  } else {
    emit startRequested(m_channel->channel_id);
  }
}

void ChannelWidget::onEditClicked() {
  if (!m_channel) {
    return;
  }

  emit editRequested(m_channel->channel_id);
}

void ChannelWidget::onMenuClicked() {
  showContextMenu(m_menuButton->geometry().bottomLeft());
}

void ChannelWidget::onOutputStartRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Start output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  /* Emit signal for dock to handle (dock has access to API and channel manager)
   */
  emit outputStartRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputStopRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Stop output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  /* Emit signal for dock to handle (dock has access to API and channel manager)
   */
  emit outputStopRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputEditRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Edit output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  /* Emit signal for dock to handle (dock has access to API and channel manager)
   */
  emit outputEditRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::showContextMenu(const QPoint &pos) {
  if (!m_channel) {
    return;
  }

  QMenu menu(this);

  /* Start/Stop actions */
  bool isActive = (m_channel->status == CHANNEL_STATUS_ACTIVE ||
                   m_channel->status == CHANNEL_STATUS_STARTING);

  QAction *startAction = menu.addAction("▶ Start Channel");
  startAction->setEnabled(!isActive);
  connect(startAction, &QAction::triggered, this,
          [this]() { emit startRequested(m_channel->channel_id); });

  QAction *stopAction = menu.addAction("■ Stop Channel");
  stopAction->setEnabled(isActive);
  connect(stopAction, &QAction::triggered, this,
          [this]() { emit stopRequested(m_channel->channel_id); });

  QAction *restartAction = menu.addAction("↻ Restart Channel");
  restartAction->setEnabled(isActive);
  connect(restartAction, &QAction::triggered, this, [this]() {
    emit stopRequested(m_channel->channel_id);

    // Store channel ID for lambda capture (m_channel may change)
    QString channelId = QString::fromUtf8(m_channel->channel_id);

    // Start after a 2-second delay to ensure clean stop
    QTimer::singleShot(2000, this, [this, channelId]() {
      // Verify channel still exists and widget is valid
      if (m_channel && QString::fromUtf8(m_channel->channel_id) == channelId) {
        emit startRequested(m_channel->channel_id);
        obs_log(LOG_INFO, "Channel restart: starting %s after delay",
                channelId.toUtf8().constData());
      }
    });

    obs_log(LOG_INFO, "Channel restart initiated: %s", m_channel->channel_id);
  });

  menu.addSeparator();

  /* Edit actions */
  QAction *editAction = menu.addAction("✎ Edit Channel...");
  connect(editAction, &QAction::triggered, this,
          [this]() { emit editRequested(m_channel->channel_id); });

  QAction *duplicateAction = menu.addAction("📋 Duplicate Channel");
  connect(duplicateAction, &QAction::triggered, this,
          [this]() { emit duplicateRequested(m_channel->channel_id); });

  QAction *deleteAction = menu.addAction("🗑️ Delete Channel");
  connect(deleteAction, &QAction::triggered, this,
          [this]() { emit deleteRequested(m_channel->channel_id); });

  menu.addSeparator();

  /* Info actions */
  QAction *statsAction = menu.addAction("📊 View Statistics");
  connect(statsAction, &QAction::triggered, this, [this]() {
    obs_log(LOG_INFO, "View stats for channel: %s", m_channel->channel_id);

    /* Build comprehensive statistics message */
    QString stats;
    stats += QString("<b>Channel: %1</b><br><br>").arg(m_channel->channel_name);

    /* Channel Status */
    stats += "<b>Status:</b> ";
    switch (m_channel->status) {
    case CHANNEL_STATUS_INACTIVE:
      stats += "Inactive";
      break;
    case CHANNEL_STATUS_STARTING:
      stats += "Starting";
      break;
    case CHANNEL_STATUS_ACTIVE:
      stats += "Active";
      break;
    case CHANNEL_STATUS_STOPPING:
      stats += "Stopping";
      break;
    case CHANNEL_STATUS_PREVIEW:
      stats += "Preview Mode";
      break;
    case CHANNEL_STATUS_ERROR:
      stats += "Error";
      break;
    }
    stats += "<br><br>";

    /* Source Configuration */
    stats += "<b>Source Configuration:</b><br>";
    stats += QString("  Orientation: ");
    switch (m_channel->source_orientation) {
    case ORIENTATION_AUTO:
      stats += "Auto-Detect";
      break;
    case ORIENTATION_HORIZONTAL:
      stats += "Horizontal (16:9)";
      break;
    case ORIENTATION_VERTICAL:
      stats += "Vertical (9:16)";
      break;
    case ORIENTATION_SQUARE:
      stats += "Square (1:1)";
      break;
    }
    stats += "<br>";

    if (m_channel->source_width > 0 && m_channel->source_height > 0) {
      stats += QString("  Resolution: %1x%2<br>")
                   .arg(m_channel->source_width)
                   .arg(m_channel->source_height);
    }

    if (m_channel->input_url) {
      stats += QString("  Input URL: %1<br>").arg(m_channel->input_url);
    }
    stats += "<br>";

    /* Outputs */
    stats += QString("<b>Outputs: %1</b><br>")
                 .arg(m_channel->output_count);
    size_t active_count = 0;
    uint64_t total_bytes = 0;
    uint32_t total_dropped = 0;

    for (size_t i = 0; i < m_channel->output_count; i++) {
      channel_output_t *dest = &m_channel->outputs[i];
      if (dest->connected) {
        active_count++;
      }
      total_bytes += dest->bytes_sent;
      total_dropped += dest->dropped_frames;
    }

    stats += QString("  Active: %1<br>").arg(active_count);
    stats += QString("  Total Data Sent: %1 MB<br>")
                 .arg(total_bytes / (1024.0 * 1024.0), 0, 'f', 2);
    stats += QString("  Total Dropped Frames: %1<br><br>").arg(total_dropped);

    /* Settings */
    stats += "<b>Settings:</b><br>";
    stats += QString("  Auto-Start: %1<br>")
                 .arg(m_channel->auto_start ? "Yes" : "No");
    stats += QString("  Auto-Reconnect: %1<br>")
                 .arg(m_channel->auto_reconnect ? "Yes" : "No");

    if (m_channel->auto_reconnect) {
      stats += QString("  Reconnect Delay: %1 seconds<br>")
                   .arg(m_channel->reconnect_delay_sec);
      stats +=
          QString("  Max Reconnect Attempts: %1<br>")
              .arg(m_channel->max_reconnect_attempts == 0
                       ? "Unlimited"
                       : QString::number(m_channel->max_reconnect_attempts));
    }

    stats +=
        QString("  Health Monitoring: %1<br>")
            .arg(m_channel->health_monitoring_enabled ? "Enabled" : "Disabled");

    QMessageBox::information(this, "Channel Statistics", stats);
  });

  QAction *exportAction = menu.addAction("📝 Export Configuration");
  connect(exportAction, &QAction::triggered, this, [this]() {
    obs_log(LOG_INFO, "Export config for channel: %s", m_channel->channel_id);

    /* Build JSON configuration */
    QString config = "{\n";
    config +=
        QString("  \"channel_name\": \"%1\",\n").arg(m_channel->channel_name);
    config += QString("  \"channel_id\": \"%1\",\n").arg(m_channel->channel_id);

    /* Source configuration */
    config += "  \"source\": {\n";
    config +=
        QString("    \"orientation\": \"%1\",\n")
            .arg(m_channel->source_orientation == ORIENTATION_AUTO ? "auto"
                 : m_channel->source_orientation == ORIENTATION_HORIZONTAL
                     ? "horizontal"
                 : m_channel->source_orientation == ORIENTATION_VERTICAL
                     ? "vertical"
                     : "square");
    config += QString("    \"auto_detect\": %1,\n")
                  .arg(m_channel->auto_detect_orientation ? "true" : "false");
    config += QString("    \"width\": %1,\n").arg(m_channel->source_width);
    config += QString("    \"height\": %1").arg(m_channel->source_height);
    if (m_channel->input_url) {
      config +=
          QString(",\n    \"input_url\": \"%1\"\n").arg(m_channel->input_url);
    } else {
      config += "\n";
    }
    config += "  },\n";

    /* Settings */
    config += "  \"settings\": {\n";
    config += QString("    \"auto_start\": %1,\n")
                  .arg(m_channel->auto_start ? "true" : "false");
    config += QString("    \"auto_reconnect\": %1,\n")
                  .arg(m_channel->auto_reconnect ? "true" : "false");
    config += QString("    \"reconnect_delay_sec\": %1,\n")
                  .arg(m_channel->reconnect_delay_sec);
    config += QString("    \"max_reconnect_attempts\": %1,\n")
                  .arg(m_channel->max_reconnect_attempts);
    config += QString("    \"health_monitoring_enabled\": %1,\n")
                  .arg(m_channel->health_monitoring_enabled ? "true" : "false");
    config += QString("    \"health_check_interval_sec\": %1,\n")
                  .arg(m_channel->health_check_interval_sec);
    config += QString("    \"failure_threshold\": %1\n")
                  .arg(m_channel->failure_threshold);
    config += "  },\n";

    /* Outputs */
    config += QString("  \"output_count\": %1\n")
                  .arg(m_channel->output_count);
    config += "}\n";

    /* Save to file */
    QString defaultPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QString("%1_channel.json").arg(m_channel->channel_name);
    QString filePath = QFileDialog::getSaveFileName(
        this, "Export Channel Configuration", defaultPath + "/" + fileName,
        "JSON Files (*.json)");

    if (!filePath.isEmpty()) {
      QFile file(filePath);
      if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << config;
        file.close();

        QMessageBox::information(
            this, "Export Successful",
            QString("Channel configuration exported to:\n%1").arg(filePath));
        obs_log(LOG_INFO, "Channel configuration exported to: %s",
                filePath.toUtf8().constData());
      } else {
        QMessageBox::warning(
            this, "Export Failed",
            QString("Failed to write to file:\n%1").arg(filePath));
      }
    }
  });

  menu.addSeparator();

  QAction *settingsAction = menu.addAction("⚙️ Channel Settings...");
  connect(settingsAction, &QAction::triggered, this,
          [this]() { emit editRequested(m_channel->channel_id); });

  /* Show menu at global position */
  QPoint globalPos = mapToGlobal(pos);
  menu.exec(globalPos);
}
