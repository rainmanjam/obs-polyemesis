/*
 * OBS Polyemesis Plugin - Channel Widget Implementation
 */

#include "channel-widget.h"
#include "output-widget.h"
#include "obs-theme-utils.h"

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
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
  m_startStopButton->setToolTip("Start or stop streaming on this channel");
  connect(m_startStopButton, &QPushButton::clicked, this,
          &ChannelWidget::onStartStopClicked);

  m_editButton = new QPushButton("Edit", this);
  m_editButton->setFixedSize(60, 28);
  m_editButton->setToolTip("Edit channel settings");
  connect(m_editButton, &QPushButton::clicked, this,
          &ChannelWidget::onEditClicked);

  m_menuButton = new QPushButton("⋮", this);
  m_menuButton->setFixedSize(28, 28);
  m_menuButton->setStyleSheet("font-size: 16px;");
  m_menuButton->setToolTip("More options");
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

  /* Style the widget using Qt palette for theme compatibility */
  setStyleSheet("ChannelWidget { "
                "  background-color: palette(base); "
                "  border: 1px solid palette(mid); "
                "  border-radius: 8px; "
                "  margin: 4px; "
                "} "
                "#channelHeader { "
                "  background-color: palette(window); "
                "  border-bottom: 1px solid palette(mid); "
                "  border-radius: 8px 8px 0 0; "
                "} "
                "#channelHeader:hover { "
                "  background-color: palette(button); "
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
  if (m_nameLabel && m_channel->channel_name) {
    m_nameLabel->setText(m_channel->channel_name);
  }

  /* Update status indicator */
  QString statusIcon = getStatusIcon();
  QColor statusColor = getStatusColor();

  if (m_statusIndicator) {
    m_statusIndicator->setText(statusIcon);
    m_statusIndicator->setStyleSheet(
        QString("font-size: 18px; color: %1;").arg(statusColor.name()));
  }

  /* Update summary */
  if (m_summaryLabel) {
    m_summaryLabel->setText(getSummaryText());
  }

  /* Update start/stop button */
  if (m_startStopButton) {
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
    m_startStopButton->setEnabled(true); /* Re-enable after operation */
  }
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
    connect(outputWidget, &OutputWidget::restartRequested, this,
            &ChannelWidget::onOutputRestartRequested);
    connect(outputWidget, &OutputWidget::editRequested, this,
            &ChannelWidget::onOutputEditRequested);
    connect(outputWidget, &OutputWidget::removeRequested, this,
            &ChannelWidget::onOutputRemoveRequested);
    connect(outputWidget, &OutputWidget::viewStatsRequested, this,
            &ChannelWidget::onOutputViewStatsRequested);
    connect(outputWidget, &OutputWidget::viewLogsRequested, this,
            &ChannelWidget::onOutputViewLogsRequested);

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

bool ChannelWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == m_headerWidget && event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (mouseEvent->button() == Qt::LeftButton) {
      onHeaderClicked();
      return true;
    }
  }
  return QWidget::eventFilter(obj, event);
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
    obs_log(LOG_ERROR, "ChannelWidget::onStartStopClicked: m_channel is NULL");
    return;
  }

  /* Disable button to prevent double-clicks */
  if (m_startStopButton) {
    m_startStopButton->setEnabled(false);
  }

  if (m_channel->status == CHANNEL_STATUS_ACTIVE ||
      m_channel->status == CHANNEL_STATUS_STARTING) {
    emit stopRequested(m_channel->channel_id);
  } else {
    emit startRequested(m_channel->channel_id);
  }

  /* Button will be re-enabled in updateHeader() when state changes */
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

void ChannelWidget::onOutputRestartRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Restart output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  emit outputRestartRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputEditRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Edit output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  emit outputEditRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputRemoveRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "Remove output requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  emit outputRemoveRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputViewStatsRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "View stats requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  emit outputViewStatsRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::onOutputViewLogsRequested(size_t outputIndex) {
  if (!m_channel || outputIndex >= m_channel->output_count) {
    obs_log(LOG_ERROR, "Invalid output index: %zu", outputIndex);
    return;
  }

  obs_log(LOG_INFO, "View logs requested: channel=%s, index=%zu",
          m_channel->channel_id, outputIndex);

  emit outputViewLogsRequested(m_channel->channel_id, outputIndex);
}

void ChannelWidget::showContextMenu(const QPoint &pos) {
  if (!m_channel) {
    return;
  }

  /* Capture channel_id early for safe access in lambdas - m_channel pointer
   * could theoretically become invalid during event processing in menu.exec() */
  const QString channelId = QString::fromUtf8(m_channel->channel_id);

  QMenu menu(this);

  /* Start/Stop actions */
  bool isActive = (m_channel->status == CHANNEL_STATUS_ACTIVE ||
                   m_channel->status == CHANNEL_STATUS_STARTING);

  QAction *startAction = menu.addAction("▶ Start Channel");
  startAction->setEnabled(!isActive);
  connect(startAction, &QAction::triggered, this,
          [this, channelId]() { emit startRequested(channelId.toUtf8().constData()); });

  QAction *stopAction = menu.addAction("■ Stop Channel");
  stopAction->setEnabled(isActive);
  connect(stopAction, &QAction::triggered, this,
          [this, channelId]() { emit stopRequested(channelId.toUtf8().constData()); });

  QAction *restartAction = menu.addAction("↻ Restart Channel");
  restartAction->setEnabled(isActive);
  connect(restartAction, &QAction::triggered, this, [this, channelId]() {
    emit stopRequested(channelId.toUtf8().constData());

    // Use QPointer to safely guard 'this' - widget may be deleted before timer fires
    QPointer<ChannelWidget> guard = this;

    // Start after a 2-second delay to ensure clean stop
    QTimer::singleShot(2000, this, [guard, channelId]() {
      // Verify widget still exists (guard becomes null if deleted)
      if (!guard) {
        obs_log(LOG_DEBUG, "Channel restart: widget deleted, skipping start for %s",
                channelId.toUtf8().constData());
        return;
      }
      // Verify channel still exists and matches
      if (guard->m_channel &&
          QString::fromUtf8(guard->m_channel->channel_id) == channelId) {
        emit guard->startRequested(channelId.toUtf8().constData());
        obs_log(LOG_INFO, "Channel restart: starting %s after delay",
                channelId.toUtf8().constData());
      }
    });

    obs_log(LOG_INFO, "Channel restart initiated: %s", channelId.toUtf8().constData());
  });

  menu.addSeparator();

  /* Preview mode actions */
  bool isInPreview = (m_channel->status == CHANNEL_STATUS_PREVIEW);
  bool canStartPreview = !isActive && !isInPreview;

  QAction *previewAction = menu.addAction("👁 Start Preview");
  previewAction->setEnabled(canStartPreview);
  connect(previewAction, &QAction::triggered, this, [this, channelId]() {
    /* Show duration selection dialog - stack-allocated modal is safe */
    QDialog durationDialog(this);
    durationDialog.setWindowTitle("Preview Duration");
    durationDialog.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&durationDialog);
    QLabel *label = new QLabel("Select preview duration:");
    layout->addWidget(label);

    QComboBox *durationCombo = new QComboBox();
    durationCombo->addItem("30 seconds", 30);
    durationCombo->addItem("1 minute", 60);
    durationCombo->addItem("2 minutes", 120);
    durationCombo->addItem("5 minutes", 300);
    durationCombo->addItem("10 minutes", 600);
    durationCombo->addItem("Unlimited", 0);
    durationCombo->setCurrentIndex(1); /* Default: 1 minute */
    layout->addWidget(durationCombo);

    QLabel *helpLabel = new QLabel(
        "<small>Preview mode allows you to test your stream without going live. "
        "Select 'Go Live' when ready.</small>");
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet(
        QString("color: %1; font-size: 11px;").arg(obs_theme_get_muted_color().name()));
    layout->addWidget(helpLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("Cancel");
    QPushButton *startBtn = new QPushButton("Start Preview");
    startBtn->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(startBtn);
    layout->addLayout(buttonLayout);

    connect(cancelBtn, &QPushButton::clicked, &durationDialog, &QDialog::reject);
    connect(startBtn, &QPushButton::clicked, &durationDialog, &QDialog::accept);

    if (durationDialog.exec() == QDialog::Accepted) {
      uint32_t duration = durationCombo->currentData().toUInt();
      emit previewStartRequested(channelId.toUtf8().constData(), duration);
      obs_log(LOG_INFO, "Preview requested for channel %s (duration: %u sec)",
              channelId.toUtf8().constData(), duration);
    }
  });

  QAction *goLiveAction = menu.addAction("🎬 Go Live");
  goLiveAction->setEnabled(isInPreview);
  connect(goLiveAction, &QAction::triggered, this, [this, channelId]() {
    emit previewGoLiveRequested(channelId.toUtf8().constData());
    obs_log(LOG_INFO, "Go live requested for channel: %s", channelId.toUtf8().constData());
  });

  QAction *cancelPreviewAction = menu.addAction("✖ Cancel Preview");
  cancelPreviewAction->setEnabled(isInPreview);
  connect(cancelPreviewAction, &QAction::triggered, this, [this, channelId]() {
    emit previewCancelRequested(channelId.toUtf8().constData());
    obs_log(LOG_INFO, "Cancel preview requested for channel: %s",
            channelId.toUtf8().constData());
  });

  menu.addSeparator();

  /* Output management */
  QAction *addOutputAction = menu.addAction("+ Add Output...");
  connect(addOutputAction, &QAction::triggered, this,
          [this, channelId]() { emit outputAddRequested(channelId.toUtf8().constData()); });

  menu.addSeparator();

  /* Edit actions */
  QAction *editAction = menu.addAction("✎ Edit Channel...");
  connect(editAction, &QAction::triggered, this,
          [this, channelId]() { emit editRequested(channelId.toUtf8().constData()); });

  QAction *duplicateAction = menu.addAction("📋 Duplicate Channel");
  connect(duplicateAction, &QAction::triggered, this,
          [this, channelId]() { emit duplicateRequested(channelId.toUtf8().constData()); });

  QAction *deleteAction = menu.addAction("🗑️ Delete Channel");
  connect(deleteAction, &QAction::triggered, this,
          [this, channelId]() { emit deleteRequested(channelId.toUtf8().constData()); });

  menu.addSeparator();

  /* Info actions */
  QAction *statsAction = menu.addAction("📊 View Statistics");
  connect(statsAction, &QAction::triggered, this, [this, channelId]() {
    /* Validate m_channel still exists before accessing detailed data */
    if (!m_channel) {
      obs_log(LOG_WARNING, "Channel data no longer available for stats: %s",
              channelId.toUtf8().constData());
      return;
    }
    obs_log(LOG_INFO, "View stats for channel: %s", channelId.toUtf8().constData());

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
  connect(exportAction, &QAction::triggered, this, [this, channelId]() {
    /* Validate m_channel still exists before accessing detailed data */
    if (!m_channel) {
      obs_log(LOG_WARNING, "Channel data no longer available for export: %s",
              channelId.toUtf8().constData());
      return;
    }
    obs_log(LOG_INFO, "Export config for channel: %s", channelId.toUtf8().constData());

    /* Build JSON configuration */
    /* Helper lambda for JSON escaping */
    auto escapeJson = [](const QString &str) {
      return QString(str).replace("\\", "\\\\").replace("\"", "\\\"");
    };

    QString config = "{\n";
    config +=
        QString("  \"channel_name\": \"%1\",\n").arg(escapeJson(m_channel->channel_name));
    config += QString("  \"channel_id\": \"%1\",\n").arg(escapeJson(m_channel->channel_id));

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
          QString(",\n    \"input_url\": \"%1\"\n").arg(escapeJson(m_channel->input_url));
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

    /* Outputs - export full configuration for each output */
    config += QString("  \"output_count\": %1,\n")
                  .arg(m_channel->output_count);
    config += "  \"outputs\": [\n";

    for (size_t i = 0; i < m_channel->output_count; i++) {
      channel_output_t *output = &m_channel->outputs[i];
      config += "    {\n";

      /* Service name */
      QString serviceName;
      switch (output->service) {
        case SERVICE_CUSTOM: serviceName = "custom"; break;
        case SERVICE_TWITCH: serviceName = "twitch"; break;
        case SERVICE_YOUTUBE: serviceName = "youtube"; break;
        case SERVICE_FACEBOOK: serviceName = "facebook"; break;
        case SERVICE_KICK: serviceName = "kick"; break;
        case SERVICE_TIKTOK: serviceName = "tiktok"; break;
        case SERVICE_INSTAGRAM: serviceName = "instagram"; break;
        case SERVICE_X_TWITTER: serviceName = "x_twitter"; break;
        default: serviceName = QString("unknown_%1").arg(output->service); break;
      }
      config += QString("      \"service\": \"%1\",\n").arg(serviceName);
      config += QString("      \"service_id\": %1,\n").arg(output->service);

      /* Stream key - mask for security but indicate if present */
      bool hasStreamKey = output->stream_key && strlen(output->stream_key) > 0;
      config += QString("      \"has_stream_key\": %1,\n")
                    .arg(hasStreamKey ? "true" : "false");

      /* Target orientation */
      QString targetOrientation;
      switch (output->target_orientation) {
        case ORIENTATION_AUTO: targetOrientation = "auto"; break;
        case ORIENTATION_HORIZONTAL: targetOrientation = "horizontal"; break;
        case ORIENTATION_VERTICAL: targetOrientation = "vertical"; break;
        case ORIENTATION_SQUARE: targetOrientation = "square"; break;
        default: targetOrientation = "unknown"; break;
      }
      config += QString("      \"target_orientation\": \"%1\",\n").arg(targetOrientation);
      config += QString("      \"enabled\": %1,\n")
                    .arg(output->enabled ? "true" : "false");

      /* Encoding settings */
      config += "      \"encoding\": {\n";
      config += QString("        \"width\": %1,\n").arg(output->encoding.width);
      config += QString("        \"height\": %1,\n").arg(output->encoding.height);
      config += QString("        \"bitrate\": %1,\n").arg(output->encoding.bitrate);
      config += QString("        \"audio_bitrate\": %1,\n").arg(output->encoding.audio_bitrate);
      config += QString("        \"audio_track\": %1\n").arg(output->encoding.audio_track);
      config += "      }\n";

      config += QString("    }%1\n").arg(i < m_channel->output_count - 1 ? "," : "");
    }

    config += "  ]\n";
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

  /* Show menu at global position */
  QPoint globalPos = mapToGlobal(pos);
  menu.exec(globalPos);
}
