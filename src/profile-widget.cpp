/*
 * OBS Polyemesis Plugin - Profile Widget Implementation
 */

#include "profile-widget.h"
#include "destination-widget.h"
#include "obs-theme-utils.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStyle>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

ProfileWidget::ProfileWidget(output_profile_t *profile, QWidget *parent)
	: QWidget(parent), m_profile(profile), m_expanded(false), m_hovered(false)
{
	obs_log(LOG_INFO, "[ProfileWidget] Creating ProfileWidget for profile: %s",
	        profile ? profile->profile_name : "NULL");
	setupUI();
	updateFromProfile();
	obs_log(LOG_INFO, "[ProfileWidget] ProfileWidget created successfully");
}

ProfileWidget::~ProfileWidget()
{
	/* Widgets are deleted automatically by Qt parent/child relationship */
}

void ProfileWidget::setupUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

	/* === Header Widget === */
	m_headerWidget = new QWidget(this);
	m_headerWidget->setObjectName("profileHeader");
	m_headerWidget->setCursor(Qt::PointingHandCursor);

	m_headerLayout = new QHBoxLayout(m_headerWidget);
	m_headerLayout->setContentsMargins(12, 12, 12, 12);
	m_headerLayout->setSpacing(12);

	/* Status indicator */
	m_statusIndicator = new QLabel(this);
	m_statusIndicator->setStyleSheet("font-size: 18px;");

	/* Profile info */
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
		&ProfileWidget::onStartStopClicked);

	m_editButton = new QPushButton("Edit", this);
	m_editButton->setFixedSize(60, 28);
	connect(m_editButton, &QPushButton::clicked, this, &ProfileWidget::onEditClicked);

	m_menuButton = new QPushButton("⋮", this);
	m_menuButton->setFixedSize(28, 28);
	m_menuButton->setStyleSheet("font-size: 16px;");
	connect(m_menuButton, &QPushButton::clicked, this, &ProfileWidget::onMenuClicked);

	/* Add to header layout */
	m_headerLayout->addWidget(m_statusIndicator);
	m_headerLayout->addWidget(infoWidget, 1); // Stretch
	m_headerLayout->addWidget(m_startStopButton);
	m_headerLayout->addWidget(m_editButton);
	m_headerLayout->addWidget(m_menuButton);

	/* Make header clickable */
	m_headerWidget->installEventFilter(this);

	m_mainLayout->addWidget(m_headerWidget);

	/* === Content Widget (Destinations) === */
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
	setStyleSheet(
		"ProfileWidget { "
		"  background-color: #2d2d30; "
		"  border: 5px solid #00ff00; "
		"  border-radius: 8px; "
		"  margin: 8px; "
		"  padding: 4px; "
		"} "
		"#profileHeader { "
		"  background-color: #3d3d40; "
		"  border-bottom: 2px solid #00ff00; "
		"  padding: 8px; "
		"} "
		"#profileHeader:hover { "
		"  background-color: #4d4d50; "
		"}");
}

void ProfileWidget::updateFromProfile()
{
	if (!m_profile) {
		return;
	}

	updateHeader();
	updateDestinations();
}

void ProfileWidget::updateHeader()
{
	if (!m_profile) {
		return;
	}

	/* Update name */
	m_nameLabel->setText(m_profile->profile_name);

	/* Update status indicator */
	QString statusIcon = getStatusIcon();
	QColor statusColor = getStatusColor();

	m_statusIndicator->setText(statusIcon);
	m_statusIndicator->setStyleSheet(
		QString("font-size: 18px; color: %1;").arg(statusColor.name()));

	/* Update summary */
	m_summaryLabel->setText(getSummaryText());

	/* Update start/stop button */
	if (m_profile->status == PROFILE_STATUS_ACTIVE ||
	    m_profile->status == PROFILE_STATUS_STARTING) {
		m_startStopButton->setText("■ Stop");
		m_startStopButton->setProperty("danger", true);
	} else {
		m_startStopButton->setText("▶ Start");
		m_startStopButton->setProperty("danger", false);
	}
	m_startStopButton->style()->unpolish(m_startStopButton);
	m_startStopButton->style()->polish(m_startStopButton);
}

void ProfileWidget::updateDestinations()
{
	if (!m_profile) {
		return;
	}

	/* Clear existing destination widgets */
	qDeleteAll(m_destinationWidgets);
	m_destinationWidgets.clear();

	/* Create widget for each destination */
	for (size_t i = 0; i < m_profile->destination_count; i++) {
		profile_destination_t *dest = &m_profile->destinations[i];

		DestinationWidget *destWidget =
			new DestinationWidget(dest, i, m_profile->profile_id, this);

		/* Connect signals */
		connect(destWidget, &DestinationWidget::startRequested, this,
			&ProfileWidget::onDestinationStartRequested);
		connect(destWidget, &DestinationWidget::stopRequested, this,
			&ProfileWidget::onDestinationStopRequested);
		connect(destWidget, &DestinationWidget::editRequested, this,
			&ProfileWidget::onDestinationEditRequested);

		m_contentLayout->addWidget(destWidget);
		m_destinationWidgets.append(destWidget);
	}
}

void ProfileWidget::setExpanded(bool expanded)
{
	if (m_expanded == expanded) {
		return;
	}

	m_expanded = expanded;
	m_contentWidget->setVisible(m_expanded);

	/* Update header border */
	if (m_expanded) {
		m_headerWidget->setStyleSheet(
			"#profileHeader { "
			"  border-bottom: 1px solid palette(mid); "
			"}");
	} else {
		m_headerWidget->setStyleSheet(
			"#profileHeader { "
			"  border-bottom: none; "
			"}");
	}

	emit expandedChanged(m_expanded);
}

const char *ProfileWidget::getProfileId() const
{
	return m_profile ? m_profile->profile_id : nullptr;
}

QString ProfileWidget::getAggregateStatus() const
{
	if (!m_profile) {
		return "inactive";
	}

	if (m_profile->status == PROFILE_STATUS_ACTIVE) {
		/* Check for errors in destinations (enabled but not connected) */
		for (size_t i = 0; i < m_profile->destination_count; i++) {
			if (m_profile->destinations[i].enabled &&
			    !m_profile->destinations[i].connected) {
				return "error";
			}
		}

		return "active";
	} else if (m_profile->status == PROFILE_STATUS_STARTING) {
		return "starting";
	} else if (m_profile->status == PROFILE_STATUS_ERROR) {
		return "error";
	}

	return "inactive";
}

QString ProfileWidget::getSummaryText() const
{
	if (!m_profile) {
		return "";
	}

	int activeCount = 0;
	int errorCount = 0;
	int totalCount = (int)m_profile->destination_count;

	for (size_t i = 0; i < m_profile->destination_count; i++) {
		/* Status based on connected and enabled flags */
		if (m_profile->destinations[i].connected &&
		    m_profile->destinations[i].enabled) {
			activeCount++;
		} else if (m_profile->destinations[i].enabled &&
			   !m_profile->destinations[i].connected) {
			errorCount++;
		}
	}

	if (m_profile->status == PROFILE_STATUS_INACTIVE) {
		if (totalCount == 1) {
			return "1 destination";
		}
		return QString("%1 destinations").arg(totalCount);
	} else if (m_profile->status == PROFILE_STATUS_STARTING) {
		return QString("Starting %1 destination%2...")
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
		return QString("%1 destinations").arg(totalCount);
	}
}

QColor ProfileWidget::getStatusColor() const
{
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

QString ProfileWidget::getStatusIcon() const
{
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

void ProfileWidget::contextMenuEvent(QContextMenuEvent *event)
{
	showContextMenu(event->pos());
	event->accept();
}

void ProfileWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		/* Toggle expansion on double-click */
		setExpanded(!m_expanded);
		event->accept();
	} else {
		QWidget::mouseDoubleClickEvent(event);
	}
}

void ProfileWidget::enterEvent(QEnterEvent *event)
{
	m_hovered = true;
	QWidget::enterEvent(event);
}

void ProfileWidget::leaveEvent(QEvent *event)
{
	m_hovered = false;
	QWidget::leaveEvent(event);
}

void ProfileWidget::onHeaderClicked()
{
	/* Toggle expansion */
	setExpanded(!m_expanded);
}

void ProfileWidget::onStartStopClicked()
{
	if (!m_profile) {
		return;
	}

	if (m_profile->status == PROFILE_STATUS_ACTIVE ||
	    m_profile->status == PROFILE_STATUS_STARTING) {
		emit stopRequested(m_profile->profile_id);
	} else {
		emit startRequested(m_profile->profile_id);
	}
}

void ProfileWidget::onEditClicked()
{
	if (!m_profile) {
		return;
	}

	emit editRequested(m_profile->profile_id);
}

void ProfileWidget::onMenuClicked()
{
	showContextMenu(m_menuButton->geometry().bottomLeft());
}

void ProfileWidget::onDestinationStartRequested(size_t destIndex)
{
	obs_log(LOG_INFO, "Start destination requested: profile=%s, index=%zu",
		m_profile->profile_id, destIndex);
	// TODO: Implement destination start
}

void ProfileWidget::onDestinationStopRequested(size_t destIndex)
{
	obs_log(LOG_INFO, "Stop destination requested: profile=%s, index=%zu",
		m_profile->profile_id, destIndex);
	// TODO: Implement destination stop
}

void ProfileWidget::onDestinationEditRequested(size_t destIndex)
{
	obs_log(LOG_INFO, "Edit destination requested: profile=%s, index=%zu",
		m_profile->profile_id, destIndex);
	// TODO: Implement destination edit
}

void ProfileWidget::showContextMenu(const QPoint &pos)
{
	if (!m_profile) {
		return;
	}

	QMenu menu(this);

	/* Start/Stop actions */
	bool isActive = (m_profile->status == PROFILE_STATUS_ACTIVE ||
			 m_profile->status == PROFILE_STATUS_STARTING);

	QAction *startAction = menu.addAction("▶ Start Profile");
	startAction->setEnabled(!isActive);
	connect(startAction, &QAction::triggered, this, [this]() {
		emit startRequested(m_profile->profile_id);
	});

	QAction *stopAction = menu.addAction("■ Stop Profile");
	stopAction->setEnabled(isActive);
	connect(stopAction, &QAction::triggered, this, [this]() {
		emit stopRequested(m_profile->profile_id);
	});

	QAction *restartAction = menu.addAction("↻ Restart Profile");
	restartAction->setEnabled(isActive);
	connect(restartAction, &QAction::triggered, this, [this]() {
		emit stopRequested(m_profile->profile_id);
		// TODO: Start after a delay
	});

	menu.addSeparator();

	/* Edit actions */
	QAction *editAction = menu.addAction("✎ Edit Profile...");
	connect(editAction, &QAction::triggered, this, [this]() {
		emit editRequested(m_profile->profile_id);
	});

	QAction *duplicateAction = menu.addAction("📋 Duplicate Profile");
	connect(duplicateAction, &QAction::triggered, this, [this]() {
		emit duplicateRequested(m_profile->profile_id);
	});

	QAction *deleteAction = menu.addAction("🗑️ Delete Profile");
	connect(deleteAction, &QAction::triggered, this, [this]() {
		emit deleteRequested(m_profile->profile_id);
	});

	menu.addSeparator();

	/* Info actions */
	QAction *statsAction = menu.addAction("📊 View Statistics");
	connect(statsAction, &QAction::triggered, this, [this]() {
		obs_log(LOG_INFO, "View stats for profile: %s",
			m_profile->profile_id);

		/* Build comprehensive statistics message */
		QString stats;
		stats += QString("<b>Profile: %1</b><br><br>").arg(m_profile->profile_name);

		/* Profile Status */
		stats += "<b>Status:</b> ";
		switch (m_profile->status) {
		case PROFILE_STATUS_INACTIVE:
			stats += "Inactive";
			break;
		case PROFILE_STATUS_STARTING:
			stats += "Starting";
			break;
		case PROFILE_STATUS_ACTIVE:
			stats += "Active";
			break;
		case PROFILE_STATUS_STOPPING:
			stats += "Stopping";
			break;
		case PROFILE_STATUS_PREVIEW:
			stats += "Preview Mode";
			break;
		case PROFILE_STATUS_ERROR:
			stats += "Error";
			break;
		}
		stats += "<br><br>";

		/* Source Configuration */
		stats += "<b>Source Configuration:</b><br>";
		stats += QString("  Orientation: ");
		switch (m_profile->source_orientation) {
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

		if (m_profile->source_width > 0 && m_profile->source_height > 0) {
			stats += QString("  Resolution: %1x%2<br>")
				.arg(m_profile->source_width)
				.arg(m_profile->source_height);
		}

		if (m_profile->input_url) {
			stats += QString("  Input URL: %1<br>").arg(m_profile->input_url);
		}
		stats += "<br>";

		/* Destinations */
		stats += QString("<b>Destinations: %1</b><br>").arg(m_profile->destination_count);
		size_t active_count = 0;
		uint64_t total_bytes = 0;
		uint32_t total_dropped = 0;

		for (size_t i = 0; i < m_profile->destination_count; i++) {
			profile_destination_t *dest = &m_profile->destinations[i];
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
			.arg(m_profile->auto_start ? "Yes" : "No");
		stats += QString("  Auto-Reconnect: %1<br>")
			.arg(m_profile->auto_reconnect ? "Yes" : "No");

		if (m_profile->auto_reconnect) {
			stats += QString("  Reconnect Delay: %1 seconds<br>")
				.arg(m_profile->reconnect_delay_sec);
			stats += QString("  Max Reconnect Attempts: %1<br>")
				.arg(m_profile->max_reconnect_attempts == 0 ? "Unlimited" :
				     QString::number(m_profile->max_reconnect_attempts));
		}

		stats += QString("  Health Monitoring: %1<br>")
			.arg(m_profile->health_monitoring_enabled ? "Enabled" : "Disabled");

		QMessageBox::information(this, "Profile Statistics", stats);
	});

	QAction *exportAction = menu.addAction("📝 Export Configuration");
	connect(exportAction, &QAction::triggered, this, [this]() {
		obs_log(LOG_INFO, "Export config for profile: %s",
			m_profile->profile_id);

		/* Build JSON configuration */
		QString config = "{\n";
		config += QString("  \"profile_name\": \"%1\",\n").arg(m_profile->profile_name);
		config += QString("  \"profile_id\": \"%1\",\n").arg(m_profile->profile_id);

		/* Source configuration */
		config += "  \"source\": {\n";
		config += QString("    \"orientation\": \"%1\",\n")
			.arg(m_profile->source_orientation == ORIENTATION_AUTO ? "auto" :
			     m_profile->source_orientation == ORIENTATION_HORIZONTAL ? "horizontal" :
			     m_profile->source_orientation == ORIENTATION_VERTICAL ? "vertical" : "square");
		config += QString("    \"auto_detect\": %1,\n")
			.arg(m_profile->auto_detect_orientation ? "true" : "false");
		config += QString("    \"width\": %1,\n").arg(m_profile->source_width);
		config += QString("    \"height\": %1").arg(m_profile->source_height);
		if (m_profile->input_url) {
			config += QString(",\n    \"input_url\": \"%1\"\n").arg(m_profile->input_url);
		} else {
			config += "\n";
		}
		config += "  },\n";

		/* Settings */
		config += "  \"settings\": {\n";
		config += QString("    \"auto_start\": %1,\n")
			.arg(m_profile->auto_start ? "true" : "false");
		config += QString("    \"auto_reconnect\": %1,\n")
			.arg(m_profile->auto_reconnect ? "true" : "false");
		config += QString("    \"reconnect_delay_sec\": %1,\n")
			.arg(m_profile->reconnect_delay_sec);
		config += QString("    \"max_reconnect_attempts\": %1,\n")
			.arg(m_profile->max_reconnect_attempts);
		config += QString("    \"health_monitoring_enabled\": %1,\n")
			.arg(m_profile->health_monitoring_enabled ? "true" : "false");
		config += QString("    \"health_check_interval_sec\": %1,\n")
			.arg(m_profile->health_check_interval_sec);
		config += QString("    \"failure_threshold\": %1\n")
			.arg(m_profile->failure_threshold);
		config += "  },\n";

		/* Destinations */
		config += QString("  \"destination_count\": %1\n").arg(m_profile->destination_count);
		config += "}\n";

		/* Save to file */
		QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
		QString fileName = QString("%1_profile.json").arg(m_profile->profile_name);
		QString filePath = QFileDialog::getSaveFileName(
			this,
			"Export Profile Configuration",
			defaultPath + "/" + fileName,
			"JSON Files (*.json)");

		if (!filePath.isEmpty()) {
			QFile file(filePath);
			if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QTextStream out(&file);
				out << config;
				file.close();

				QMessageBox::information(this, "Export Successful",
					QString("Profile configuration exported to:\n%1").arg(filePath));
				obs_log(LOG_INFO, "Profile configuration exported to: %s",
					filePath.toUtf8().constData());
			} else {
				QMessageBox::warning(this, "Export Failed",
					QString("Failed to write to file:\n%1").arg(filePath));
			}
		}
	});

	menu.addSeparator();

	QAction *settingsAction = menu.addAction("⚙️ Profile Settings...");
	connect(settingsAction, &QAction::triggered, this, [this]() {
		emit editRequested(m_profile->profile_id);
	});

	/* Show menu at global position */
	QPoint globalPos = mapToGlobal(pos);
	menu.exec(globalPos);
}
