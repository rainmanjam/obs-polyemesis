#pragma once

#include <QDockWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>

#include "restreamer-api.h"
#include "restreamer-multistream.h"

class RestreamerDock : public QDockWidget {
	Q_OBJECT

public:
	RestreamerDock(QWidget *parent = nullptr);
	~RestreamerDock();

private slots:
	void onRefreshClicked();
	void onTestConnectionClicked();
	void onProcessSelected();
	void onStartProcessClicked();
	void onStopProcessClicked();
	void onRestartProcessClicked();
	void onCreateMultistreamClicked();
	void onAddDestinationClicked();
	void onRemoveDestinationClicked();
	void onSaveSettingsClicked();
	void onUpdateTimer();

private:
	void setupUI();
	void loadSettings();
	void saveSettings();
	void updateProcessList();
	void updateProcessDetails();
	void updateSessionList();
	void updateDestinationList();

	restreamer_api_t *api;
	QTimer *updateTimer;

	/* Connection group */
	QLineEdit *hostEdit;
	QLineEdit *portEdit;
	QCheckBox *httpsCheckbox;
	QLineEdit *usernameEdit;
	QLineEdit *passwordEdit;
	QPushButton *testConnectionButton;
	QLabel *connectionStatusLabel;

	/* Process list group */
	QListWidget *processList;
	QPushButton *refreshButton;
	QPushButton *startButton;
	QPushButton *stopButton;
	QPushButton *restartButton;

	/* Process details */
	QLabel *processIdLabel;
	QLabel *processStateLabel;
	QLabel *processUptimeLabel;
	QLabel *processCpuLabel;
	QLabel *processMemoryLabel;

	/* Sessions */
	QTableWidget *sessionTable;

	/* Multistream configuration */
	QComboBox *orientationCombo;
	QCheckBox *autoDetectOrientationCheck;
	QTableWidget *destinationsTable;
	QPushButton *addDestinationButton;
	QPushButton *removeDestinationButton;
	QPushButton *createMultistreamButton;

	multistream_config_t *multistreamConfig;
	char *selectedProcessId;
};
