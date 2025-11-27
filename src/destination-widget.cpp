/*
 * OBS Polyemesis Plugin - Destination Widget Implementation
 */

#include "destination-widget.h"
#include "obs-theme-utils.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

DestinationWidget::DestinationWidget(profile_destination_t *destination,
				     size_t destIndex, const char *profileId,
				     QWidget *parent)
	: QWidget(parent), m_detailsPanel(nullptr), m_detailsExpanded(false),
	  m_hovered(false)
{
	/* Store profile ID, destination index, and pointer */
	m_profileId = bstrdup(profileId);
	m_destinationIndex = destIndex;
	m_destination = destination; // Store pointer, not copy

	setupUI();
	updateFromDestination();
}

DestinationWidget::~DestinationWidget()
{
	bfree(m_profileId);
	/* m_destination is a pointer to external data, don't free it */
}

void DestinationWidget::setupUI()
{
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setContentsMargins(12, 8, 12, 8);
	m_mainLayout->setSpacing(12);

	/* Status indicator */
	m_statusIndicator = new QLabel(this);
	m_statusIndicator->setStyleSheet("font-size: 16px;");
	m_statusIndicator->setFixedWidth(20);

	/* Info widget */
	m_infoWidget = new QWidget(this);
	m_infoLayout = new QVBoxLayout(m_infoWidget);
	m_infoLayout->setContentsMargins(0, 0, 0, 0);
	m_infoLayout->setSpacing(2);

	m_serviceLabel = new QLabel(this);
	m_serviceLabel->setStyleSheet("font-weight: 600; font-size: 13px;");

	m_detailsLabel = new QLabel(this);
	QColor mutedColor = obs_theme_get_muted_color();
	m_detailsLabel->setStyleSheet(
		QString("font-size: 11px; color: %1;").arg(mutedColor.name()));

	m_infoLayout->addWidget(m_serviceLabel);
	m_infoLayout->addWidget(m_detailsLabel);

	/* Stats widget (only visible when active) */
	m_statsWidget = new QWidget(this);
	m_statsLayout = new QHBoxLayout(m_statsWidget);
	m_statsLayout->setContentsMargins(0, 0, 0, 0);
	m_statsLayout->setSpacing(12);

	m_bitrateLabel = new QLabel(this);
	m_bitrateLabel->setStyleSheet("font-size: 11px;");

	m_droppedLabel = new QLabel(this);
	m_droppedLabel->setStyleSheet("font-size: 11px;");

	m_durationLabel = new QLabel(this);
	m_durationLabel->setStyleSheet("font-size: 11px;");

	m_statsLayout->addWidget(m_bitrateLabel);
	m_statsLayout->addWidget(m_droppedLabel);
	m_statsLayout->addWidget(m_durationLabel);

	/* Actions widget (shown on hover) */
	m_actionsWidget = new QWidget(this);
	m_actionsLayout = new QHBoxLayout(m_actionsWidget);
	m_actionsLayout->setContentsMargins(0, 0, 0, 0);
	m_actionsLayout->setSpacing(4);

	m_startStopButton = new QPushButton(this);
	m_startStopButton->setFixedSize(28, 24);
	m_startStopButton->setStyleSheet("font-size: 14px;");
	connect(m_startStopButton, &QPushButton::clicked, this,
		&DestinationWidget::onStartStopClicked);

	m_settingsButton = new QPushButton("⚙️", this);
	m_settingsButton->setFixedSize(28, 24);
	m_settingsButton->setStyleSheet("font-size: 12px;");
	connect(m_settingsButton, &QPushButton::clicked, this,
		&DestinationWidget::onSettingsClicked);

	m_actionsLayout->addWidget(m_startStopButton);
	m_actionsLayout->addWidget(m_settingsButton);

	/* Initially hide actions */
	m_actionsWidget->setVisible(false);

	/* Add to main layout */
	m_mainLayout->addWidget(m_statusIndicator);
	m_mainLayout->addWidget(m_infoWidget, 1); // Stretch
	m_mainLayout->addWidget(m_statsWidget);
	m_mainLayout->addWidget(m_actionsWidget);

	/* Style */
	setStyleSheet(
		"DestinationWidget { "
		"  background-color: palette(window); "
		"  border-bottom: 1px solid palette(mid); "
		"} "
		"DestinationWidget:hover { "
		"  background-color: palette(button); "
		"}");

	setCursor(Qt::PointingHandCursor);
}

void DestinationWidget::updateFromDestination()
{
	/* Pointer is already updated by caller, just refresh UI */
	updateStatus();
	updateStats();
}

void DestinationWidget::updateStatus()
{
	/* Update status indicator */
	QString statusIcon = getStatusIcon();
	QColor statusColor = getStatusColor();

	m_statusIndicator->setText(statusIcon);
	m_statusIndicator->setStyleSheet(
		QString("font-size: 16px; color: %1;").arg(statusColor.name()));

	/* Update service name */
	m_serviceLabel->setText(m_destination->service_name);

	/* Update details - use encoding settings */
	QString resolution = QString("%1x%2")
				     .arg(m_destination->encoding.width)
				     .arg(m_destination->encoding.height);
	QString bitrate = formatBitrate(m_destination->encoding.bitrate);
	QString fps = m_destination->encoding.fps_num > 0
			      ? QString("%1 FPS").arg(m_destination->encoding.fps_num)
			      : "";

	QStringList details;
	details << resolution << bitrate;
	if (!fps.isEmpty()) {
		details << fps;
	}

	m_detailsLabel->setText(details.join(" • "));

	/* Update start/stop button - status based on connected && enabled */
	bool isActive = (m_destination->connected && m_destination->enabled);
	if (isActive) {
		m_startStopButton->setText("■");
		m_startStopButton->setProperty("danger", true);
	} else {
		m_startStopButton->setText("▶");
		m_startStopButton->setProperty("danger", false);
	}
	m_startStopButton->style()->unpolish(m_startStopButton);
	m_startStopButton->style()->polish(m_startStopButton);
}

void DestinationWidget::updateStats()
{
	/* Show stats only when active (connected and enabled) */
	bool showStats = (m_destination->connected && m_destination->enabled);
	m_statsWidget->setVisible(showStats);

	if (!showStats) {
		return;
	}

	/* Update bitrate from current_bitrate field */
	int currentBitrate = m_destination->current_bitrate;
	QColor bitrateColor = obs_theme_get_success_color();
	m_bitrateLabel->setText(QString("↑ %1").arg(formatBitrate(currentBitrate)));
	m_bitrateLabel->setStyleSheet(
		QString("font-size: 11px; color: %1;").arg(bitrateColor.name()));

	/* Update dropped frames from dropped_frames field */
	uint32_t droppedFrames = m_destination->dropped_frames;
	// TODO: Calculate percentage when we have total frames
	float droppedPercent = 0.0f;
	QColor droppedColor;
	if (droppedPercent > 5.0f) {
		droppedColor = obs_theme_get_error_color();
	} else if (droppedPercent > 1.0f) {
		droppedColor = obs_theme_get_warning_color();
	} else {
		droppedColor = obs_theme_get_success_color();
	}
	m_droppedLabel->setText(QString("%1 dropped").arg(droppedFrames));
	m_droppedLabel->setStyleSheet(
		QString("font-size: 11px; color: %1;").arg(droppedColor.name()));

	/* Update duration */
	// TODO: Get actual duration from statistics
	int duration = 0; // seconds
	m_durationLabel->setText(formatDuration(duration));
	QColor mutedColor = obs_theme_get_muted_color();
	m_durationLabel->setStyleSheet(
		QString("font-size: 11px; color: %1;").arg(mutedColor.name()));
}

QColor DestinationWidget::getStatusColor() const
{
	/* Status based on connected and enabled flags */
	if (m_destination->connected && m_destination->enabled) {
		return obs_theme_get_success_color();
	} else if (m_destination->enabled && !m_destination->connected) {
		/* Enabled but not connected = error/trying to connect */
		return obs_theme_get_error_color();
	}
	return obs_theme_get_muted_color();
}

QString DestinationWidget::getStatusIcon() const
{
	/* Status based on connected and enabled flags */
	if (m_destination->connected && m_destination->enabled) {
		return "🟢"; // Active
	} else if (m_destination->enabled && !m_destination->connected) {
		return "🔴"; // Error/trying to connect
	} else if (!m_destination->enabled) {
		return "⚫"; // Disabled
	}
	return "⚫";
}

QString DestinationWidget::getStatusText() const
{
	/* Status based on connected and enabled flags */
	if (m_destination->connected && m_destination->enabled) {
		return "Active";
	} else if (m_destination->enabled && !m_destination->connected) {
		return "Error";
	} else if (!m_destination->enabled) {
		return "Disabled";
	}
	return "Stopped";
}

QString DestinationWidget::formatBitrate(int kbps) const
{
	if (kbps >= 1000) {
		return QString("%1 Mbps").arg(kbps / 1000.0, 0, 'f', 1);
	}
	return QString("%1 Kbps").arg(kbps);
}

QString DestinationWidget::formatDuration(int seconds) const
{
	int hours = seconds / 3600;
	int minutes = (seconds % 3600) / 60;
	int secs = seconds % 60;

	return QString("%1:%2:%3")
		.arg(hours, 2, 10, QChar('0'))
		.arg(minutes, 2, 10, QChar('0'))
		.arg(secs, 2, 10, QChar('0'));
}

void DestinationWidget::contextMenuEvent(QContextMenuEvent *event)
{
	showContextMenu(event->pos());
	event->accept();
}

void DestinationWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		/* Toggle details on double-click */
		toggleDetailsPanel();
		event->accept();
	} else {
		QWidget::mouseDoubleClickEvent(event);
	}
}

void DestinationWidget::enterEvent(QEnterEvent *event)
{
	m_hovered = true;
	m_actionsWidget->setVisible(true);
	QWidget::enterEvent(event);
}

void DestinationWidget::leaveEvent(QEvent *event)
{
	m_hovered = false;
	m_actionsWidget->setVisible(false);
	QWidget::leaveEvent(event);
}

void DestinationWidget::onStartStopClicked()
{
	bool isActive = (m_destination->connected && m_destination->enabled);
	if (isActive) {
		emit stopRequested(m_destinationIndex);
	} else {
		emit startRequested(m_destinationIndex);
	}
}

void DestinationWidget::onSettingsClicked()
{
	emit editRequested(m_destinationIndex);
}

void DestinationWidget::onDetailsToggled()
{
	toggleDetailsPanel();
}

void DestinationWidget::showContextMenu(const QPoint &pos)
{
	QMenu menu(this);

	/* Start/Stop actions */
	bool isActive = (m_destination->connected && m_destination->enabled);

	QAction *startAction = menu.addAction("▶ Start Stream");
	startAction->setEnabled(!isActive);
	connect(startAction, &QAction::triggered, this,
		[this]() { emit startRequested(m_destinationIndex); });

	QAction *stopAction = menu.addAction("■ Stop Stream");
	stopAction->setEnabled(isActive);
	connect(stopAction, &QAction::triggered, this,
		[this]() { emit stopRequested(m_destinationIndex); });

	QAction *restartAction = menu.addAction("↻ Restart Stream");
	restartAction->setEnabled(isActive);
	connect(restartAction, &QAction::triggered, this,
		[this]() { emit restartRequested(m_destinationIndex); });

	menu.addSeparator();

	/* Edit actions */
	QAction *editAction = menu.addAction("✎ Edit Destination...");
	connect(editAction, &QAction::triggered, this,
		[this]() { emit editRequested(m_destinationIndex); });

	QAction *copyUrlAction = menu.addAction("📋 Copy Stream URL");
	connect(copyUrlAction, &QAction::triggered, this, [this]() {
		// TODO: Copy URL to clipboard
		obs_log(LOG_INFO, "Copy URL for destination: %zu", m_destinationIndex);
	});

	QAction *copyKeyAction = menu.addAction("📋 Copy Stream Key");
	connect(copyKeyAction, &QAction::triggered, this, [this]() {
		// TODO: Copy key to clipboard
		obs_log(LOG_INFO, "Copy key for destination: %zu", m_destinationIndex);
	});

	menu.addSeparator();

	/* Info actions */
	QAction *statsAction = menu.addAction("📊 View Stream Stats");
	connect(statsAction, &QAction::triggered, this,
		[this]() { emit viewStatsRequested(m_destinationIndex); });

	QAction *logsAction = menu.addAction("📝 View Stream Logs");
	connect(logsAction, &QAction::triggered, this,
		[this]() { emit viewLogsRequested(m_destinationIndex); });

	QAction *testAction = menu.addAction("🔍 Test Stream Health");
	connect(testAction, &QAction::triggered, this, [this]() {
		obs_log(LOG_INFO, "Test health for destination: %zu", m_destinationIndex);
		// TODO: Test stream health
	});

	menu.addSeparator();

	QAction *removeAction = menu.addAction("🗑️ Remove Destination");
	connect(removeAction, &QAction::triggered, this,
		[this]() { emit removeRequested(m_destinationIndex); });

	/* Show menu at global position */
	QPoint globalPos = mapToGlobal(pos);
	menu.exec(globalPos);
}

void DestinationWidget::toggleDetailsPanel()
{
	if (!m_detailsPanel) {
		/* Create details panel */
		m_detailsPanel = new QWidget(this);
		QVBoxLayout *detailsLayout = new QVBoxLayout(m_detailsPanel);
		detailsLayout->setContentsMargins(40, 8, 12, 8);
		detailsLayout->setSpacing(8);

		QColor mutedColor = obs_theme_get_muted_color();
		QString mutedStyle = QString("font-size: 11px; color: %1;").arg(mutedColor.name());

		/* Network Statistics */
		QLabel *networkTitle = new QLabel("<b>Network Statistics</b>", this);
		networkTitle->setStyleSheet("font-size: 11px;");
		detailsLayout->addWidget(networkTitle);

		double bytesSentMB = m_destination->bytes_sent / (1024.0 * 1024.0);
		QLabel *bytesLabel = new QLabel(
			QString("  Total Data Sent: %1 MB").arg(bytesSentMB, 0, 'f', 2), this);
		bytesLabel->setStyleSheet(mutedStyle);
		detailsLayout->addWidget(bytesLabel);

		QLabel *currentBitrateLabel = new QLabel(
			QString("  Current Bitrate: %1 kbps").arg(m_destination->current_bitrate), this);
		currentBitrateLabel->setStyleSheet(mutedStyle);
		detailsLayout->addWidget(currentBitrateLabel);

		QLabel *droppedLabel = new QLabel(
			QString("  Dropped Frames: %1").arg(m_destination->dropped_frames), this);
		droppedLabel->setStyleSheet(mutedStyle);
		detailsLayout->addWidget(droppedLabel);

		/* Connection Status */
		detailsLayout->addSpacing(4);
		QLabel *connectionTitle = new QLabel("<b>Connection</b>", this);
		connectionTitle->setStyleSheet("font-size: 11px;");
		detailsLayout->addWidget(connectionTitle);

		QLabel *connectedLabel = new QLabel(
			QString("  Status: %1")
				.arg(m_destination->connected ? "Connected" : "Disconnected"),
			this);
		connectedLabel->setStyleSheet(mutedStyle);
		detailsLayout->addWidget(connectedLabel);

		QLabel *autoReconnectLabel = new QLabel(
			QString("  Auto-Reconnect: %1")
				.arg(m_destination->auto_reconnect_enabled ? "Enabled"
									    : "Disabled"),
			this);
		autoReconnectLabel->setStyleSheet(mutedStyle);
		detailsLayout->addWidget(autoReconnectLabel);

		/* Health Monitoring */
		if (m_destination->last_health_check > 0) {
			detailsLayout->addSpacing(4);
			QLabel *healthTitle = new QLabel("<b>Health Monitoring</b>", this);
			healthTitle->setStyleSheet("font-size: 11px;");
			detailsLayout->addWidget(healthTitle);

			time_t now = time(NULL);
			int secondsSinceCheck = (int)difftime(now, m_destination->last_health_check);
			QLabel *lastCheckLabel = new QLabel(
				QString("  Last Health Check: %1 seconds ago")
					.arg(secondsSinceCheck),
				this);
			lastCheckLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(lastCheckLabel);

			QLabel *failuresLabel = new QLabel(
				QString("  Consecutive Failures: %1")
					.arg(m_destination->consecutive_failures),
				this);
			failuresLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(failuresLabel);
		}

		/* Failover Information */
		if (m_destination->is_backup || m_destination->failover_active) {
			detailsLayout->addSpacing(4);
			QLabel *failoverTitle = new QLabel("<b>Failover</b>", this);
			failoverTitle->setStyleSheet("font-size: 11px;");
			detailsLayout->addWidget(failoverTitle);

			if (m_destination->is_backup) {
				QLabel *backupLabel = new QLabel(
					QString("  Role: Backup for destination #%1")
						.arg(m_destination->primary_index),
					this);
				backupLabel->setStyleSheet(mutedStyle);
				detailsLayout->addWidget(backupLabel);
			} else if (m_destination->backup_index != (size_t)-1) {
				QLabel *primaryLabel = new QLabel(
					QString("  Role: Primary (Backup: #%1)")
						.arg(m_destination->backup_index),
					this);
				primaryLabel->setStyleSheet(mutedStyle);
				detailsLayout->addWidget(primaryLabel);
			}

			if (m_destination->failover_active) {
				time_t now = time(NULL);
				int failoverDuration = (int)difftime(now, m_destination->failover_start_time);
				QLabel *failoverLabel = new QLabel(
					QString("  Failover Active: %1 seconds")
						.arg(failoverDuration),
					this);
				failoverLabel->setStyleSheet(mutedStyle);
				detailsLayout->addWidget(failoverLabel);
			}
		}

		/* Encoding Settings */
		detailsLayout->addSpacing(4);
		QLabel *encodingTitle = new QLabel("<b>Encoding Settings</b>", this);
		encodingTitle->setStyleSheet("font-size: 11px;");
		detailsLayout->addWidget(encodingTitle);

		if (m_destination->encoding.width > 0 && m_destination->encoding.height > 0) {
			QLabel *resolutionLabel = new QLabel(
				QString("  Resolution: %1x%2")
					.arg(m_destination->encoding.width)
					.arg(m_destination->encoding.height),
				this);
			resolutionLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(resolutionLabel);
		}

		if (m_destination->encoding.bitrate > 0) {
			QLabel *targetBitrateLabel = new QLabel(
				QString("  Target Bitrate: %1 kbps")
					.arg(m_destination->encoding.bitrate),
				this);
			targetBitrateLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(targetBitrateLabel);
		}

		if (m_destination->encoding.fps_num > 0) {
			double fps = (double)m_destination->encoding.fps_num /
				     (m_destination->encoding.fps_den > 0
					      ? m_destination->encoding.fps_den
					      : 1);
			QLabel *fpsLabel = new QLabel(
				QString("  Frame Rate: %1 fps").arg(fps, 0, 'f', 2), this);
			fpsLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(fpsLabel);
		}

		if (m_destination->encoding.audio_bitrate > 0) {
			QLabel *audioBitrateLabel = new QLabel(
				QString("  Audio Bitrate: %1 kbps")
					.arg(m_destination->encoding.audio_bitrate),
				this);
			audioBitrateLabel->setStyleSheet(mutedStyle);
			detailsLayout->addWidget(audioBitrateLabel);
		}

		/* Add to parent layout */
		QWidget *parentWidget = qobject_cast<QWidget *>(parent());
		if (parentWidget) {
			QVBoxLayout *parentLayout = qobject_cast<QVBoxLayout *>(parentWidget->layout());
			if (parentLayout) {
				int index = parentLayout->indexOf(this);
				parentLayout->insertWidget(index + 1, m_detailsPanel);
			}
		}

		m_detailsExpanded = true;
	} else {
		/* Remove details panel */
		m_detailsPanel->deleteLater();
		m_detailsPanel = nullptr;
		m_detailsExpanded = false;
	}
}
