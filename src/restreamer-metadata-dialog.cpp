#include "restreamer-metadata-dialog.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QSplitter>

RestreamerMetadataDialog::RestreamerMetadataDialog(QWidget *parent,
						   restreamer_api_t *api,
						   const char *process_id)
	: QDialog(parent), api(api), processId(nullptr)
{
	if (process_id) {
		processId = strdup(process_id);
	}

	setWindowTitle("Process Metadata");
	resize(700, 500);

	setupUI();
	loadMetadata();
}

RestreamerMetadataDialog::~RestreamerMetadataDialog()
{
	free(processId);
}

void RestreamerMetadataDialog::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	/* Quick access fields for common metadata */
	QGroupBox *quickAccessGroup = new QGroupBox("Quick Access");
	QFormLayout *quickForm = new QFormLayout();

	notesEdit = new QLineEdit();
	notesEdit->setPlaceholderText("Add notes about this process...");
	quickForm->addRow("Notes:", notesEdit);

	tagsEdit = new QLineEdit();
	tagsEdit->setPlaceholderText("e.g., production, backup, test");
	quickForm->addRow("Tags:", tagsEdit);

	descriptionEdit = new QLineEdit();
	descriptionEdit->setPlaceholderText("Brief description");
	quickForm->addRow("Description:", descriptionEdit);

	quickAccessGroup->setLayout(quickForm);
	mainLayout->addWidget(quickAccessGroup);

	/* Custom metadata table */
	QGroupBox *customGroup = new QGroupBox("Custom Metadata");
	QVBoxLayout *customLayout = new QVBoxLayout();

	QLabel *infoLabel = new QLabel(
		"Add custom key-value metadata to organize and track this process:");
	infoLabel->setWordWrap(true);
	customLayout->addWidget(infoLabel);

	metadataTable = new QTableWidget(0, 2);
	metadataTable->setHorizontalHeaderLabels({"Key", "Value"});
	metadataTable->horizontalHeader()->setStretchLastSection(true);
	metadataTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Interactive);
	metadataTable->setColumnWidth(0, 200);
	metadataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	metadataTable->setEditTriggers(QAbstractItemView::DoubleClicked |
				       QAbstractItemView::EditKeyPressed);

	connect(metadataTable, &QTableWidget::cellChanged, this,
		&RestreamerMetadataDialog::onTableCellChanged);

	customLayout->addWidget(metadataTable);

	/* Table control buttons */
	QHBoxLayout *tableButtonLayout = new QHBoxLayout();

	addButton = new QPushButton("Add Row");
	removeButton = new QPushButton("Remove Row");
	refreshButton = new QPushButton("Refresh");

	connect(addButton, &QPushButton::clicked, this,
		&RestreamerMetadataDialog::onAddClicked);
	connect(removeButton, &QPushButton::clicked, this,
		&RestreamerMetadataDialog::onRemoveClicked);
	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerMetadataDialog::onRefreshClicked);

	tableButtonLayout->addWidget(addButton);
	tableButtonLayout->addWidget(removeButton);
	tableButtonLayout->addWidget(refreshButton);
	tableButtonLayout->addStretch();

	customLayout->addLayout(tableButtonLayout);
	customGroup->setLayout(customLayout);
	mainLayout->addWidget(customGroup);

	/* Status label */
	statusLabel = new QLabel("");
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel);

	/* Dialog buttons */
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Save |
					 QDialogButtonBox::Close);

	connect(buttonBox->button(QDialogButtonBox::Save),
		&QPushButton::clicked, this,
		&RestreamerMetadataDialog::onSaveClicked);
	connect(buttonBox->button(QDialogButtonBox::Close),
		&QPushButton::clicked, this, &QDialog::reject);

	mainLayout->addWidget(buttonBox);
}

void RestreamerMetadataDialog::loadMetadata()
{
	if (!api || !processId) {
		statusLabel->setText(
			"<span style='color: orange;'>⚠ No process selected</span>");
		return;
	}

	/* Clear existing table */
	metadataTable->setRowCount(0);

	/* Load quick access fields */
	char *notes = nullptr;
	char *tags = nullptr;
	char *description = nullptr;

	if (restreamer_api_get_process_metadata(api, processId, "notes",
						&notes)) {
		notesEdit->setText(QString::fromUtf8(notes));
		free(notes);
	}

	if (restreamer_api_get_process_metadata(api, processId, "tags",
						&tags)) {
		tagsEdit->setText(QString::fromUtf8(tags));
		free(tags);
	}

	if (restreamer_api_get_process_metadata(api, processId, "description",
						&description)) {
		descriptionEdit->setText(QString::fromUtf8(description));
		free(description);
	}

	/* Note: The API doesn't provide a way to list all metadata keys,
	 * so we can only show what we know exists. In practice, users will
	 * add custom rows as needed. */

	statusLabel->setText(
		"<span style='color: green;'>✓ Metadata loaded</span>");
}

void RestreamerMetadataDialog::saveMetadata()
{
	if (!api || !processId) {
		QMessageBox::warning(
			this, "Error",
			"Cannot save metadata: No process selected");
		return;
	}

	int savedCount = 0;
	int errorCount = 0;

	/* Save quick access fields */
	if (!notesEdit->text().isEmpty()) {
		if (restreamer_api_set_process_metadata(
			    api, processId, "notes",
			    notesEdit->text().toUtf8().constData())) {
			savedCount++;
		} else {
			errorCount++;
		}
	}

	if (!tagsEdit->text().isEmpty()) {
		if (restreamer_api_set_process_metadata(
			    api, processId, "tags",
			    tagsEdit->text().toUtf8().constData())) {
			savedCount++;
		} else {
			errorCount++;
		}
	}

	if (!descriptionEdit->text().isEmpty()) {
		if (restreamer_api_set_process_metadata(
			    api, processId, "description",
			    descriptionEdit->text().toUtf8().constData())) {
			savedCount++;
		} else {
			errorCount++;
		}
	}

	/* Save custom metadata from table */
	for (int row = 0; row < metadataTable->rowCount(); row++) {
		QTableWidgetItem *keyItem = metadataTable->item(row, 0);
		QTableWidgetItem *valueItem = metadataTable->item(row, 1);

		if (keyItem && valueItem) {
			QString key = keyItem->text().trimmed();
			QString value = valueItem->text().trimmed();

			if (!key.isEmpty() && !value.isEmpty()) {
				if (restreamer_api_set_process_metadata(
					    api, processId,
					    key.toUtf8().constData(),
					    value.toUtf8().constData())) {
					savedCount++;
				} else {
					errorCount++;
				}
			}
		}
	}

	if (errorCount > 0) {
		statusLabel->setText(QString(
			"<span style='color: orange;'>⚠ Saved %1 items, %2 errors</span>")
					     .arg(savedCount)
					     .arg(errorCount));
	} else {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Saved %1 metadata items</span>")
				.arg(savedCount));
	}
}

void RestreamerMetadataDialog::addMetadataRow(const QString &key,
					      const QString &value)
{
	int row = metadataTable->rowCount();
	metadataTable->insertRow(row);

	QTableWidgetItem *keyItem = new QTableWidgetItem(key);
	QTableWidgetItem *valueItem = new QTableWidgetItem(value);

	metadataTable->setItem(row, 0, keyItem);
	metadataTable->setItem(row, 1, valueItem);
}

void RestreamerMetadataDialog::onAddClicked()
{
	addMetadataRow("", "");

	/* Focus on the new row */
	int newRow = metadataTable->rowCount() - 1;
	metadataTable->setCurrentCell(newRow, 0);
	metadataTable->editItem(metadataTable->item(newRow, 0));
}

void RestreamerMetadataDialog::onRemoveClicked()
{
	int currentRow = metadataTable->currentRow();
	if (currentRow >= 0) {
		metadataTable->removeRow(currentRow);
	} else {
		QMessageBox::information(
			this, "Remove Row",
			"Please select a row to remove from the table.");
	}
}

void RestreamerMetadataDialog::onSaveClicked()
{
	saveMetadata();
}

void RestreamerMetadataDialog::onRefreshClicked()
{
	loadMetadata();
}

void RestreamerMetadataDialog::onTableCellChanged(int row, int column)
{
	/* Auto-save is disabled to avoid API spam
	 * Users must click Save button */
	(void)row;
	(void)column;
}
