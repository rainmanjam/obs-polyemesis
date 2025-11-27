/*
 * OBS Polyemesis Plugin - Profile Edit Dialog Implementation
 */

#include "profile-edit-dialog.h"
#include "obs-helpers.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <obs-module.h>

extern "C" {
#include <plugin-support.h>
}

ProfileEditDialog::ProfileEditDialog(output_profile_t *profile,
				     QWidget *parent)
	: QDialog(parent), m_profile(profile)
{
	if (!m_profile) {
		obs_log(LOG_ERROR, "ProfileEditDialog created with null profile");
		reject();
		return;
	}

	setupUI();
	loadProfileSettings();
}

ProfileEditDialog::~ProfileEditDialog()
{
	/* Widgets are deleted automatically by Qt parent/child relationship */
}

void ProfileEditDialog::setupUI()
{
	setWindowTitle("Edit Profile");
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
	m_nameEdit->setPlaceholderText("Profile Name");
	basicForm->addRow("Profile Name:", m_nameEdit);

	QGroupBox *sourceGroup = new QGroupBox("Source Configuration");
	QFormLayout *sourceForm = new QFormLayout(sourceGroup);

	m_orientationCombo = new QComboBox(this);
	m_orientationCombo->addItem("Auto-Detect", ORIENTATION_AUTO);
	m_orientationCombo->addItem("Horizontal (16:9)",
				    ORIENTATION_HORIZONTAL);
	m_orientationCombo->addItem("Vertical (9:16)",
				    ORIENTATION_VERTICAL);
	m_orientationCombo->addItem("Square (1:1)",
				    ORIENTATION_SQUARE);
	connect(m_orientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&ProfileEditDialog::onOrientationChanged);
	sourceForm->addRow("Orientation:", m_orientationCombo);

	m_autoDetectCheckBox = new QCheckBox("Auto-detect orientation from source");
	connect(m_autoDetectCheckBox, &QCheckBox::stateChanged, this,
		&ProfileEditDialog::onAutoDetectChanged);
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

	QLabel *inputHelpLabel = new QLabel(
		"<small style='color: #888;'>RTMP input URL for this profile (optional)</small>");
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

	m_autoStartCheckBox = new QCheckBox("Auto-start profile when OBS streaming starts");
	autoStartLayout->addWidget(m_autoStartCheckBox);

	QLabel *autoStartHelp = new QLabel(
		"<small style='color: #888;'>Automatically activate this profile when you start streaming in OBS</small>");
	autoStartHelp->setWordWrap(true);
	autoStartLayout->addWidget(autoStartHelp);

	QGroupBox *reconnectGroup = new QGroupBox("Auto-Reconnect Settings");
	QVBoxLayout *reconnectLayout = new QVBoxLayout(reconnectGroup);

	m_autoReconnectCheckBox = new QCheckBox("Enable auto-reconnect on disconnect");
	connect(m_autoReconnectCheckBox, &QCheckBox::stateChanged, this,
		&ProfileEditDialog::onAutoReconnectChanged);
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
		"<small style='color: #888;'>Automatically reconnect if the stream drops. Set max attempts to 0 for unlimited retries.</small>");
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
	connect(m_healthMonitoringCheckBox, &QCheckBox::stateChanged, this,
		&ProfileEditDialog::onHealthMonitoringChanged);
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
		"<small style='color: #888;'>Monitor stream health and automatically trigger reconnects when issues are detected. "
		"The failure threshold determines how many consecutive health check failures trigger a reconnect.</small>");
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
		&ProfileEditDialog::onCancel);

	m_saveButton = new QPushButton("Save", this);
	m_saveButton->setMinimumHeight(32);
	m_saveButton->setDefault(true);
	connect(m_saveButton, &QPushButton::clicked, this,
		&ProfileEditDialog::onSave);

	buttonLayout->addStretch();
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addWidget(m_saveButton);

	mainLayout->addLayout(buttonLayout);

	setLayout(mainLayout);
}

void ProfileEditDialog::loadProfileSettings()
{
	if (!m_profile) {
		return;
	}

	/* Load basic info */
	if (m_profile->profile_name) {
		m_nameEdit->setText(m_profile->profile_name);
	}

	/* Load source configuration */
	m_orientationCombo->setCurrentIndex(
		m_orientationCombo->findData(m_profile->source_orientation));
	m_autoDetectCheckBox->setChecked(
		m_profile->auto_detect_orientation);
	m_sourceWidthSpin->setValue(m_profile->source_width);
	m_sourceHeightSpin->setValue(m_profile->source_height);

	if (m_profile->input_url) {
		m_inputUrlEdit->setText(m_profile->input_url);
	}

	/* Load streaming settings */
	m_autoStartCheckBox->setChecked(m_profile->auto_start);
	m_autoReconnectCheckBox->setChecked(m_profile->auto_reconnect);
	m_reconnectDelaySpin->setValue(m_profile->reconnect_delay_sec);
	m_maxReconnectAttemptsSpin->setValue(
		m_profile->max_reconnect_attempts);

	/* Load health monitoring settings */
	m_healthMonitoringCheckBox->setChecked(
		m_profile->health_monitoring_enabled);
	m_healthCheckIntervalSpin->setValue(
		m_profile->health_check_interval_sec);
	m_failureThresholdSpin->setValue(m_profile->failure_threshold);

	/* Update UI state */
	onAutoDetectChanged(m_autoDetectCheckBox->isChecked() ? Qt::Checked
							      : Qt::Unchecked);
	onAutoReconnectChanged(m_autoReconnectCheckBox->isChecked()
				       ? Qt::Checked
				       : Qt::Unchecked);
	onHealthMonitoringChanged(m_healthMonitoringCheckBox->isChecked()
					  ? Qt::Checked
					  : Qt::Unchecked);
}

void ProfileEditDialog::validateAndSave()
{
	QString name = m_nameEdit->text().trimmed();

	if (name.isEmpty()) {
		m_statusLabel->setText("⚠️ Profile name cannot be empty");
		m_statusLabel->setStyleSheet(
			"background-color: #5a3a00; color: #ffcc00; padding: 8px; border-radius: 4px;");
		m_statusLabel->show();
		m_tabWidget->setCurrentIndex(0); /* Switch to General tab */
		m_nameEdit->setFocus();
		return;
	}

	/* Update profile settings */
	bfree(m_profile->profile_name);
	m_profile->profile_name = bstrdup(name.toUtf8().constData());

	m_profile->source_orientation = static_cast<stream_orientation_t>(
		m_orientationCombo->currentData().toInt());
	m_profile->auto_detect_orientation =
		m_autoDetectCheckBox->isChecked();
	m_profile->source_width = m_sourceWidthSpin->value();
	m_profile->source_height = m_sourceHeightSpin->value();

	QString inputUrl = m_inputUrlEdit->text().trimmed();
	bfree(m_profile->input_url);
	m_profile->input_url = inputUrl.isEmpty()
				       ? nullptr
				       : bstrdup(inputUrl.toUtf8().constData());

	m_profile->auto_start = m_autoStartCheckBox->isChecked();
	m_profile->auto_reconnect = m_autoReconnectCheckBox->isChecked();
	m_profile->reconnect_delay_sec = m_reconnectDelaySpin->value();
	m_profile->max_reconnect_attempts =
		m_maxReconnectAttemptsSpin->value();

	m_profile->health_monitoring_enabled =
		m_healthMonitoringCheckBox->isChecked();
	m_profile->health_check_interval_sec =
		m_healthCheckIntervalSpin->value();
	m_profile->failure_threshold = m_failureThresholdSpin->value();

	obs_log(LOG_INFO, "Profile updated: %s", m_profile->profile_name);

	emit profileUpdated();
	accept();
}

/* Getters */
bool ProfileEditDialog::getProfileName(char **name) const
{
	QString text = m_nameEdit->text().trimmed();
	if (text.isEmpty()) {
		return false;
	}
	*name = bstrdup(text.toUtf8().constData());
	return true;
}

stream_orientation_t ProfileEditDialog::getSourceOrientation() const
{
	return static_cast<stream_orientation_t>(
		m_orientationCombo->currentData().toInt());
}

bool ProfileEditDialog::getAutoDetectOrientation() const
{
	return m_autoDetectCheckBox->isChecked();
}

uint32_t ProfileEditDialog::getSourceWidth() const
{
	return m_sourceWidthSpin->value();
}

uint32_t ProfileEditDialog::getSourceHeight() const
{
	return m_sourceHeightSpin->value();
}

bool ProfileEditDialog::getInputUrl(char **url) const
{
	QString text = m_inputUrlEdit->text().trimmed();
	if (text.isEmpty()) {
		*url = nullptr;
		return false;
	}
	*url = bstrdup(text.toUtf8().constData());
	return true;
}

bool ProfileEditDialog::getAutoStart() const
{
	return m_autoStartCheckBox->isChecked();
}

bool ProfileEditDialog::getAutoReconnect() const
{
	return m_autoReconnectCheckBox->isChecked();
}

uint32_t ProfileEditDialog::getReconnectDelay() const
{
	return m_reconnectDelaySpin->value();
}

uint32_t ProfileEditDialog::getMaxReconnectAttempts() const
{
	return m_maxReconnectAttemptsSpin->value();
}

bool ProfileEditDialog::getHealthMonitoringEnabled() const
{
	return m_healthMonitoringCheckBox->isChecked();
}

uint32_t ProfileEditDialog::getHealthCheckInterval() const
{
	return m_healthCheckIntervalSpin->value();
}

uint32_t ProfileEditDialog::getFailureThreshold() const
{
	return m_failureThresholdSpin->value();
}

/* Slots */
void ProfileEditDialog::onSave()
{
	validateAndSave();
}

void ProfileEditDialog::onCancel()
{
	reject();
}

void ProfileEditDialog::onOrientationChanged(int index)
{
	stream_orientation_t orientation =
		static_cast<stream_orientation_t>(
			m_orientationCombo->itemData(index).toInt());

	/* Auto-enable auto-detect if orientation is set to AUTO */
	if (orientation == ORIENTATION_AUTO) {
		m_autoDetectCheckBox->setChecked(true);
	}
}

void ProfileEditDialog::onAutoDetectChanged(int state)
{
	bool autoDetect = (state == Qt::Checked);

	/* Disable manual dimension inputs when auto-detect is enabled */
	m_sourceWidthSpin->setEnabled(!autoDetect);
	m_sourceHeightSpin->setEnabled(!autoDetect);

	if (autoDetect) {
		m_sourceWidthSpin->setValue(0);
		m_sourceHeightSpin->setValue(0);
	}
}

void ProfileEditDialog::onAutoReconnectChanged(int state)
{
	bool enabled = (state == Qt::Checked);
	m_reconnectDelaySpin->setEnabled(enabled);
	m_maxReconnectAttemptsSpin->setEnabled(enabled);
}

void ProfileEditDialog::onHealthMonitoringChanged(int state)
{
	bool enabled = (state == Qt::Checked);
	m_healthCheckIntervalSpin->setEnabled(enabled);
	m_failureThresholdSpin->setEnabled(enabled);
}
