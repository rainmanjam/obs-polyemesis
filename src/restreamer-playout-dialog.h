#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "restreamer-api.h"

/* Playout Management Dialog
 * Monitor and control input sources for streaming
 */
class RestreamerPlayoutDialog : public QDialog {
	Q_OBJECT

public:
	RestreamerPlayoutDialog(QWidget *parent, restreamer_api_t *api,
				const char *process_id, const char *input_id);
	~RestreamerPlayoutDialog();

private slots:
	void onRefreshClicked();
	void onSwitchInputClicked();
	void onReopenClicked();

private:
	void setupUI();
	void loadPlayoutStatus();

	restreamer_api_t *api;
	char *processId;
	char *inputId;

	/* UI Components */
	QLabel *statusLabel;
	QLabel *urlLabel;
	QLabel *bitrateLabel;
	QLabel *bytesLabel;
	QLabel *connectedLabel;
	QPushButton *refreshButton;
	QPushButton *switchInputButton;
	QPushButton *reopenButton;
	QDialogButtonBox *buttonBox;
};
