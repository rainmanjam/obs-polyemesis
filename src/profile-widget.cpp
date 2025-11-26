/*
 * OBS Polyemesis Plugin - Profile Widget Implementation
 */

#include "profile-widget.h"
#include "destination-widget.h"
#include "obs-theme-utils.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStyle>
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
		// TODO: Show stats dialog
	});

	QAction *exportAction = menu.addAction("📝 Export Configuration");
	connect(exportAction, &QAction::triggered, this, [this]() {
		obs_log(LOG_INFO, "Export config for profile: %s",
			m_profile->profile_id);
		// TODO: Export config
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
