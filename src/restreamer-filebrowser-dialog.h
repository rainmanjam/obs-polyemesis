#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "restreamer-api.h"

/* File System Browser Dialog
 * Browse, download, upload, and delete files from Restreamer storage
 */
class RestreamerFileBrowserDialog : public QDialog {
	Q_OBJECT

public:
	RestreamerFileBrowserDialog(QWidget *parent, restreamer_api_t *api);
	~RestreamerFileBrowserDialog();

private slots:
	void onStorageChanged(int index);
	void onRefreshClicked();
	void onDownloadClicked();
	void onUploadClicked();
	void onDeleteClicked();
	void onFileDoubleClicked(int row, int column);
	void onFilterChanged(const QString &text);

private:
	void setupUI();
	void loadStorages();
	void loadFiles();
	void refreshFileList();
	QString formatFileSize(uint64_t bytes);
	QString formatTimestamp(int64_t timestamp);

	restreamer_api_t *api;

	/* UI Components */
	QComboBox *storageCombo;
	QLineEdit *filterEdit;
	QPushButton *refreshButton;
	QTableWidget *filesTable;
	QPushButton *downloadButton;
	QPushButton *uploadButton;
	QPushButton *deleteButton;
	QLabel *statusLabel;
	QProgressBar *progressBar;
	QDialogButtonBox *buttonBox;

	/* Current state */
	QString currentStorage;
	QStringList availableStorages;
};
