#include "restreamer-outputs-dialog.h"
#include <QInputDialog>
#include <QMessageBox>

RestreamerOutputsDialog::RestreamerOutputsDialog(QWidget *parent,
						 restreamer_api_t *api,
						 const char *process_id)
	: QDialog(parent), api(api), processId(nullptr)
{
	if (process_id) {
		processId = strdup(process_id);
	}

	setWindowTitle("Manage Stream Outputs");
	resize(600, 450);

	setupUI();
	loadOutputs();
}

RestreamerOutputsDialog::~RestreamerOutputsDialog()
{
	free(processId);
}

void RestreamerOutputsDialog::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *infoLabel = new QLabel(
		"<b>Dynamic Output Management</b><br>"
		"Add or remove streaming destinations while the process is running:");
	infoLabel->setWordWrap(true);
	mainLayout->addWidget(infoLabel);

	/* Outputs list */
	outputsListWidget = new QListWidget();
	outputsListWidget->setSelectionMode(
		QAbstractItemView::SingleSelection);
	mainLayout->addWidget(outputsListWidget);

	/* Action buttons */
	QHBoxLayout *buttonLayout = new QHBoxLayout();

	refreshButton = new QPushButton("Refresh");
	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerOutputsDialog::onRefreshClicked);
	buttonLayout->addWidget(refreshButton);

	addButton = new QPushButton("Add Output");
	connect(addButton, &QPushButton::clicked, this,
		&RestreamerOutputsDialog::onAddOutputClicked);
	buttonLayout->addWidget(addButton);

	editButton = new QPushButton("Edit Output");
	connect(editButton, &QPushButton::clicked, this,
		&RestreamerOutputsDialog::onEditOutputClicked);
	buttonLayout->addWidget(editButton);

	removeButton = new QPushButton("Remove Output");
	removeButton->setStyleSheet("QPushButton { color: red; }");
	connect(removeButton, &QPushButton::clicked, this,
		&RestreamerOutputsDialog::onRemoveOutputClicked);
	buttonLayout->addWidget(removeButton);

	buttonLayout->addStretch();
	mainLayout->addLayout(buttonLayout);

	/* Status label */
	statusLabel = new QLabel("");
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel);

	/* Dialog buttons */
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttonBox->button(QDialogButtonBox::Close),
		&QPushButton::clicked, this, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
}

void RestreamerOutputsDialog::loadOutputs()
{
	if (!api || !processId) {
		statusLabel->setText(
			"<span style='color: orange;'>⚠ No process selected</span>");
		return;
	}

	char **output_ids = nullptr;
	size_t output_count = 0;

	if (!restreamer_api_get_process_outputs(api, processId, &output_ids,
						&output_count)) {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to load outputs</span>");
		return;
	}

	outputsListWidget->clear();

	for (size_t i = 0; i < output_count; i++) {
		outputsListWidget->addItem(QString::fromUtf8(output_ids[i]));
	}

	restreamer_api_free_outputs_list(output_ids, output_count);

	statusLabel->setText(
		QString("<span style='color: green;'>✓ Loaded %1 outputs</span>")
			.arg(output_count));
}

void RestreamerOutputsDialog::onRefreshClicked()
{
	loadOutputs();
}

void RestreamerOutputsDialog::onAddOutputClicked()
{
	if (!api || !processId) {
		return;
	}

	/* Get output ID */
	bool ok;
	QString outputId = QInputDialog::getText(
		this, "Add Output", "Enter output ID (e.g., output_youtube):",
		QLineEdit::Normal, "", &ok);

	if (!ok || outputId.trimmed().isEmpty()) {
		return;
	}

	/* Get output URL */
	QString outputUrl = QInputDialog::getText(
		this, "Add Output",
		"Enter stream URL (e.g., rtmp://a.rtmp.youtube.com/live2/YOUR-KEY):",
		QLineEdit::Normal, "", &ok);

	if (!ok || outputUrl.trimmed().isEmpty()) {
		return;
	}

	/* Add output */
	statusLabel->setText("Adding output...");

	if (restreamer_api_add_process_output(
		    api, processId, outputId.toUtf8().constData(),
		    outputUrl.toUtf8().constData(), nullptr)) {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Added output: %1</span>")
				.arg(outputId));
		loadOutputs();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to add output</span>");
	}
}

void RestreamerOutputsDialog::onRemoveOutputClicked()
{
	if (!api || !processId) {
		return;
	}

	QListWidgetItem *currentItem = outputsListWidget->currentItem();
	if (!currentItem) {
		QMessageBox::information(
			this, "Remove Output",
			"Please select an output to remove from the list.");
		return;
	}

	QString outputId = currentItem->text();

	/* Confirm removal */
	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Remove Output",
		QString("Remove output: %1?\n\nThe stream will stop immediately!")
			.arg(outputId),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (reply != QMessageBox::Yes) {
		return;
	}

	/* Remove output */
	if (restreamer_api_remove_process_output(
		    api, processId, outputId.toUtf8().constData())) {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Removed output: %1</span>")
				.arg(outputId));
		loadOutputs();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to remove output</span>");
	}
}

void RestreamerOutputsDialog::onEditOutputClicked()
{
	if (!api || !processId) {
		return;
	}

	QListWidgetItem *currentItem = outputsListWidget->currentItem();
	if (!currentItem) {
		QMessageBox::information(
			this, "Edit Output",
			"Please select an output to edit from the list.");
		return;
	}

	QString outputId = currentItem->text();

	/* Get new URL */
	bool ok;
	QString newUrl = QInputDialog::getText(
		this, "Edit Output",
		QString("Enter new stream URL for: %1").arg(outputId),
		QLineEdit::Normal, "", &ok);

	if (!ok || newUrl.trimmed().isEmpty()) {
		return;
	}

	/* Update output */
	if (restreamer_api_update_process_output(
		    api, processId, outputId.toUtf8().constData(),
		    newUrl.toUtf8().constData(), nullptr)) {
		statusLabel->setText(
			QString("<span style='color: green;'>✓ Updated output: %1</span>")
				.arg(outputId));
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to update output</span>");
	}
}
