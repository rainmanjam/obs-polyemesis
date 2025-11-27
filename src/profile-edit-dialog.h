/*
 * OBS Polyemesis Plugin - Profile Edit Dialog
 */

#pragma once

#include "restreamer-output-profile.h"
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QLabel>

class ProfileEditDialog : public QDialog {
	Q_OBJECT

public:
	explicit ProfileEditDialog(output_profile_t *profile,
				   QWidget *parent = nullptr);
	~ProfileEditDialog();

	/* Get updated profile settings */
	bool getProfileName(char **name) const;
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
	void profileUpdated();

private slots:
	void onSave();
	void onCancel();
	void onOrientationChanged(int index);
	void onAutoDetectChanged(int state);
	void onAutoReconnectChanged(int state);
	void onHealthMonitoringChanged(int state);

private:
	void setupUI();
	void loadProfileSettings();
	void validateAndSave();

	/* Profile being edited */
	output_profile_t *m_profile;

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

	/* Dialog buttons */
	QPushButton *m_saveButton;
	QPushButton *m_cancelButton;
	QTabWidget *m_tabWidget;
	QLabel *m_statusLabel;
};
