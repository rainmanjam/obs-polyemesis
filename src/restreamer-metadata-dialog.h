#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "restreamer-api.h"

/* Metadata Management Dialog
 * Allows viewing and editing custom metadata for processes
 */
class RestreamerMetadataDialog : public QDialog {
	Q_OBJECT

public:
	RestreamerMetadataDialog(QWidget *parent, restreamer_api_t *api,
				 const char *process_id);
	~RestreamerMetadataDialog();

private slots:
	void onAddClicked();
	void onRemoveClicked();
	void onSaveClicked();
	void onRefreshClicked();
	void onTableCellChanged(int row, int column);

private:
	void setupUI();
	void loadMetadata();
	void saveMetadata();
	void addMetadataRow(const QString &key, const QString &value);

	restreamer_api_t *api;
	char *processId;

	/* UI Components */
	QTableWidget *metadataTable;
	QPushButton *addButton;
	QPushButton *removeButton;
	QPushButton *refreshButton;
	QDialogButtonBox *buttonBox;
	QLabel *statusLabel;

	/* Quick access metadata fields */
	QLineEdit *notesEdit;
	QLineEdit *tagsEdit;
	QLineEdit *descriptionEdit;
};
