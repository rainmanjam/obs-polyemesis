#include "restreamer-filebrowser-dialog.h"
#include <QDateTime>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QScrollArea>

RestreamerFileBrowserDialog::RestreamerFileBrowserDialog(QWidget *parent,
							 restreamer_api_t *api)
	: QDialog(parent), api(api)
{
	setWindowTitle("File System Browser");
	resize(900, 600);

	setupUI();
	loadStorages();
}

RestreamerFileBrowserDialog::~RestreamerFileBrowserDialog()
{
}

void RestreamerFileBrowserDialog::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	/* Storage selection and filter */
	QHBoxLayout *topLayout = new QHBoxLayout();

	QLabel *storageLabel = new QLabel("Storage:");
	topLayout->addWidget(storageLabel);

	storageCombo = new QComboBox();
	storageCombo->setMinimumWidth(200);
	connect(storageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &RestreamerFileBrowserDialog::onStorageChanged);
	topLayout->addWidget(storageCombo);

	topLayout->addSpacing(20);

	QLabel *filterLabel = new QLabel("Filter:");
	topLayout->addWidget(filterLabel);

	filterEdit = new QLineEdit();
	filterEdit->setPlaceholderText("*.mp4, *.ts, etc. (glob pattern)");
	filterEdit->setText("*");
	connect(filterEdit, &QLineEdit::textChanged, this,
		&RestreamerFileBrowserDialog::onFilterChanged);
	topLayout->addWidget(filterEdit);

	refreshButton = new QPushButton("Refresh");
	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerFileBrowserDialog::onRefreshClicked);
	topLayout->addWidget(refreshButton);

	topLayout->addStretch();
	mainLayout->addLayout(topLayout);

	/* Files table */
	filesTable = new QTableWidget(0, 4);
	filesTable->setHorizontalHeaderLabels(
		{"Name", "Size", "Modified", "Path"});
	filesTable->horizontalHeader()->setStretchLastSection(true);
	filesTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Interactive);
	filesTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::ResizeToContents);
	filesTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::ResizeToContents);
	filesTable->setColumnWidth(0, 250);
	filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	filesTable->setSortingEnabled(true);

	connect(filesTable, &QTableWidget::cellDoubleClicked, this,
		&RestreamerFileBrowserDialog::onFileDoubleClicked);

	mainLayout->addWidget(filesTable);

	/* Progress bar */
	progressBar = new QProgressBar();
	progressBar->setVisible(false);
	mainLayout->addWidget(progressBar);

	/* Status label */
	statusLabel = new QLabel("");
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel);

	/* Action buttons */
	QHBoxLayout *buttonLayout = new QHBoxLayout();

	downloadButton = new QPushButton("Download");
	downloadButton->setToolTip("Download selected file to local computer");
	connect(downloadButton, &QPushButton::clicked, this,
		&RestreamerFileBrowserDialog::onDownloadClicked);
	buttonLayout->addWidget(downloadButton);

	uploadButton = new QPushButton("Upload");
	uploadButton->setToolTip("Upload file from local computer to storage");
	connect(uploadButton, &QPushButton::clicked, this,
		&RestreamerFileBrowserDialog::onUploadClicked);
	buttonLayout->addWidget(uploadButton);

	deleteButton = new QPushButton("Delete");
	deleteButton->setToolTip("Delete selected file from storage");
	deleteButton->setStyleSheet("QPushButton { color: red; }");
	connect(deleteButton, &QPushButton::clicked, this,
		&RestreamerFileBrowserDialog::onDeleteClicked);
	buttonLayout->addWidget(deleteButton);

	buttonLayout->addStretch();
	mainLayout->addLayout(buttonLayout);

	/* Dialog buttons */
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttonBox->button(QDialogButtonBox::Close),
		&QPushButton::clicked, this, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
}

void RestreamerFileBrowserDialog::loadStorages()
{
	if (!api) {
		statusLabel->setText(
			"<span style='color: orange;'>⚠ Not connected to Restreamer</span>");
		return;
	}

	char *storages_json = nullptr;
	if (!restreamer_api_list_filesystems(api, &storages_json)) {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to load filesystems</span>");
		return;
	}

	/* Parse JSON response */
	QJsonDocument doc = QJsonDocument::fromJson(storages_json);
	free(storages_json);

	if (doc.isArray()) {
		QJsonArray storages = doc.array();
		availableStorages.clear();
		storageCombo->clear();

		for (QJsonValue value : storages) {
			QString storage = value.toString();
			availableStorages.append(storage);
			storageCombo->addItem(storage);
		}

		if (!availableStorages.isEmpty()) {
			currentStorage = availableStorages.first();
			refreshFileList();
		}
	} else {
		statusLabel->setText(
			"<span style='color: orange;'>⚠ No filesystems available</span>");
	}
}

void RestreamerFileBrowserDialog::loadFiles()
{
	if (!api || currentStorage.isEmpty()) {
		return;
	}

	QString filter = filterEdit->text().trimmed();
	if (filter.isEmpty()) {
		filter = "*";
	}

	restreamer_fs_list_t file_list;
	memset(&file_list, 0, sizeof(file_list));

	statusLabel->setText("Loading files...");

	if (!restreamer_api_list_files(api, currentStorage.toUtf8().constData(),
				       filter.toUtf8().constData(),
				       &file_list)) {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to load files</span>");
		return;
	}

	/* Clear table and populate with files */
	filesTable->setRowCount(0);
	filesTable->setSortingEnabled(false);

	for (size_t i = 0; i < file_list.count; i++) {
		const restreamer_fs_entry_t *entry = &file_list.entries[i];

		int row = filesTable->rowCount();
		filesTable->insertRow(row);

		/* Name */
		QTableWidgetItem *nameItem = new QTableWidgetItem(
			QString::fromUtf8(entry->name));
		if (entry->is_directory) {
			nameItem->setText(QString::fromUtf8(entry->name) +
					  "/");
			QFont font = nameItem->font();
			font.setBold(true);
			nameItem->setFont(font);
		}
		filesTable->setItem(row, 0, nameItem);

		/* Size */
		QString sizeStr = entry->is_directory
					  ? "<DIR>"
					  : formatFileSize(entry->size);
		QTableWidgetItem *sizeItem = new QTableWidgetItem(sizeStr);
		sizeItem->setTextAlignment(Qt::AlignRight |
					   Qt::AlignVCenter);
		filesTable->setItem(row, 1, sizeItem);

		/* Modified */
		QString modifiedStr = formatTimestamp(entry->modified);
		QTableWidgetItem *modifiedItem =
			new QTableWidgetItem(modifiedStr);
		filesTable->setItem(row, 2, modifiedItem);

		/* Path */
		QTableWidgetItem *pathItem =
			new QTableWidgetItem(QString::fromUtf8(entry->path));
		filesTable->setItem(row, 3, pathItem);
	}

	filesTable->setSortingEnabled(true);
	filesTable->sortByColumn(0, Qt::AscendingOrder);

	restreamer_api_free_fs_list(&file_list);

	statusLabel->setText(
		QString("<span style='color: green;'>✓ Loaded %1 items</span>")
			.arg(filesTable->rowCount()));
}

void RestreamerFileBrowserDialog::refreshFileList()
{
	loadFiles();
}

QString RestreamerFileBrowserDialog::formatFileSize(uint64_t bytes)
{
	const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	int unitIndex = 0;
	double size = static_cast<double>(bytes);

	while (size >= 1024.0 && unitIndex < 4) {
		size /= 1024.0;
		unitIndex++;
	}

	return QString::number(size, 'f', 2) + " " + units[unitIndex];
}

QString RestreamerFileBrowserDialog::formatTimestamp(int64_t timestamp)
{
	if (timestamp <= 0) {
		return "Unknown";
	}

	QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp);
	return dateTime.toString("yyyy-MM-dd HH:mm:ss");
}

void RestreamerFileBrowserDialog::onStorageChanged(int index)
{
	(void)index;
	if (storageCombo->currentIndex() >= 0) {
		currentStorage = storageCombo->currentText();
		refreshFileList();
	}
}

void RestreamerFileBrowserDialog::onRefreshClicked()
{
	refreshFileList();
}

void RestreamerFileBrowserDialog::onDownloadClicked()
{
	int currentRow = filesTable->currentRow();
	if (currentRow < 0) {
		QMessageBox::information(
			this, "Download File",
			"Please select a file to download from the list.");
		return;
	}

	QTableWidgetItem *nameItem = filesTable->item(currentRow, 0);
	QTableWidgetItem *pathItem = filesTable->item(currentRow, 3);

	if (!nameItem || !pathItem) {
		return;
	}

	QString fileName = nameItem->text();
	QString filePath = pathItem->text();

	/* Don't download directories */
	if (fileName.endsWith("/")) {
		QMessageBox::information(this, "Download File",
					 "Cannot download directories.");
		return;
	}

	/* Sanitize filename to prevent path traversal */
	QString sanitizedFileName = QFileInfo(fileName).fileName();

	/* Validate that path doesn't contain traversal sequences */
	if (filePath.contains("..") || filePath.startsWith("/") ||
	    filePath.contains("\\") || sanitizedFileName.isEmpty()) {
		QMessageBox::warning(this, "Download File",
				     "Invalid file path detected.");
		return;
	}

	/* Ask user where to save - use sanitized filename */
	QString saveFilePath = QFileDialog::getSaveFileName(
		this, "Save File As", sanitizedFileName, "All Files (*)");

	if (saveFilePath.isEmpty()) {
		return;
	}

	/* Validate that the save path is absolute and normalized */
	QFileInfo saveFileInfo(saveFilePath);
	QString canonicalSavePath = saveFileInfo.canonicalFilePath();
	if (canonicalSavePath.isEmpty()) {
		/* File doesn't exist yet, so use absoluteFilePath instead */
		canonicalSavePath = saveFileInfo.absoluteFilePath();
	}

	/* Download file */
	unsigned char *data = nullptr;
	size_t size = 0;

	progressBar->setVisible(true);
	progressBar->setRange(0, 0); /* Indeterminate progress */
	statusLabel->setText("Downloading file...");

	if (restreamer_api_download_file(api,
					 currentStorage.toUtf8().constData(),
					 filePath.toUtf8().constData(), &data,
					 &size)) {
		/* Save to disk using validated path */
		QFile file(canonicalSavePath);
		if (file.open(QIODevice::WriteOnly)) {
			file.write(reinterpret_cast<const char *>(data),
				   size);
			file.close();
			free(data);

			statusLabel->setText(
				QString("<span style='color: green;'>✓ Downloaded %1 (%2)</span>")
					.arg(fileName)
					.arg(formatFileSize(size)));
		} else {
			free(data);
			statusLabel->setText(
				QString("<span style='color: red;'>✗ Failed to save file: %1</span>")
					.arg(file.errorString()));
		}
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to download file</span>");
	}

	progressBar->setVisible(false);
}

void RestreamerFileBrowserDialog::onUploadClicked()
{
	/* Ask user to select file */
	QString sourceFilePath = QFileDialog::getOpenFileName(
		this, "Select File to Upload", "", "All Files (*)");

	if (sourceFilePath.isEmpty()) {
		return;
	}

	QFile file(sourceFilePath);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(
			this, "Upload File",
			QString("Failed to open file: %1")
				.arg(file.errorString()));
		return;
	}

	QByteArray fileData = file.readAll();
	file.close();

	QString fileName = QFileInfo(sourceFilePath).fileName();

	/* Upload to current storage */
	progressBar->setVisible(true);
	progressBar->setRange(0, 0);
	statusLabel->setText("Uploading file...");

	if (restreamer_api_upload_file(
		    api, currentStorage.toUtf8().constData(),
		    fileName.toUtf8().constData(),
		    reinterpret_cast<const unsigned char *>(fileData.constData()),
		    fileData.size())) {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Uploaded %1 (%2)</span>")
				.arg(fileName)
				.arg(formatFileSize(fileData.size())));
		refreshFileList();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to upload file</span>");
	}

	progressBar->setVisible(false);
}

void RestreamerFileBrowserDialog::onDeleteClicked()
{
	int currentRow = filesTable->currentRow();
	if (currentRow < 0) {
		QMessageBox::information(
			this, "Delete File",
			"Please select a file to delete from the list.");
		return;
	}

	QTableWidgetItem *nameItem = filesTable->item(currentRow, 0);
	QTableWidgetItem *pathItem = filesTable->item(currentRow, 3);

	if (!nameItem || !pathItem) {
		return;
	}

	QString fileName = nameItem->text();
	QString filePath = pathItem->text();

	/* Confirm deletion */
	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Delete File",
		QString("Are you sure you want to delete:\n\n%1\n\nThis action cannot be undone!")
			.arg(fileName),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (reply != QMessageBox::Yes) {
		return;
	}

	/* Delete file */
	if (restreamer_api_delete_file(api,
				       currentStorage.toUtf8().constData(),
				       filePath.toUtf8().constData())) {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Deleted %1</span>")
				.arg(fileName));
		refreshFileList();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to delete file</span>");
	}
}

void RestreamerFileBrowserDialog::onFileDoubleClicked(int row, int column)
{
	(void)column;

	QTableWidgetItem *nameItem = filesTable->item(row, 0);
	if (!nameItem) {
		return;
	}

	QString fileName = nameItem->text();

	/* If directory, could implement navigation here in future */
	if (fileName.endsWith("/")) {
		QMessageBox::information(
			this, "Directory",
			"Directory navigation is not yet implemented.");
	} else {
		/* Double-click downloads the file */
		onDownloadClicked();
	}
}

void RestreamerFileBrowserDialog::onFilterChanged(const QString &text)
{
	(void)text;
	/* Debounce could be added here, but for now refresh immediately */
	refreshFileList();
}
