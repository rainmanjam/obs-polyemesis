#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "restreamer-api.h"

/* Dynamic Output Management Dialog
 * Add, remove, and update streaming outputs while process is running
 */
class RestreamerOutputsDialog : public QDialog {
	Q_OBJECT

public:
	RestreamerOutputsDialog(QWidget *parent, restreamer_api_t *api,
				const char *process_id);
	~RestreamerOutputsDialog();

private slots:
	void onRefreshClicked();
	void onAddOutputClicked();
	void onRemoveOutputClicked();
	void onEditOutputClicked();

private:
	void setupUI();
	void loadOutputs();

	restreamer_api_t *api;
	char *processId;

	/* UI Components */
	QListWidget *outputsListWidget;
	QPushButton *refreshButton;
	QPushButton *addButton;
	QPushButton *removeButton;
	QPushButton *editButton;
	QLabel *statusLabel;
	QDialogButtonBox *buttonBox;
};
