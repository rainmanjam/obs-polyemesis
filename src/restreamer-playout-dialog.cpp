#include "restreamer-playout-dialog.h"
#include <QFormLayout>
#include <QInputDialog>
#include <QMessageBox>

RestreamerPlayoutDialog::RestreamerPlayoutDialog(QWidget *parent,
						 restreamer_api_t *api,
						 const char *process_id,
						 const char *input_id)
	: QDialog(parent), api(api), processId(nullptr), inputId(nullptr)
{
	if (process_id) {
		processId = strdup(process_id);
	}
	if (input_id) {
		inputId = strdup(input_id);
	}

	setWindowTitle("Input Source Management");
	resize(500, 350);

	setupUI();
	loadPlayoutStatus();
}

RestreamerPlayoutDialog::~RestreamerPlayoutDialog()
{
	free(processId);
	free(inputId);
}

void RestreamerPlayoutDialog::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *headerLabel = new QLabel(
		"<b>Input Source Control</b><br>"
		"Monitor and manage the input stream for this process:");
	headerLabel->setWordWrap(true);
	mainLayout->addWidget(headerLabel);

	/* Status group */
	QGroupBox *statusGroup = new QGroupBox("Current Status");
	QFormLayout *statusForm = new QFormLayout();

	connectedLabel = new QLabel("Unknown");
	statusForm->addRow("Connection:", connectedLabel);

	urlLabel = new QLabel("N/A");
	urlLabel->setWordWrap(true);
	statusForm->addRow("Stream URL:", urlLabel);

	bitrateLabel = new QLabel("0 kbps");
	statusForm->addRow("Bitrate:", bitrateLabel);

	bytesLabel = new QLabel("0 bytes");
	statusForm->addRow("Received:", bytesLabel);

	statusGroup->setLayout(statusForm);
	mainLayout->addWidget(statusGroup);

	/* Control buttons */
	QGroupBox *controlGroup = new QGroupBox("Actions");
	QVBoxLayout *controlLayout = new QVBoxLayout();

	refreshButton = new QPushButton("Refresh Status");
	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerPlayoutDialog::onRefreshClicked);
	controlLayout->addWidget(refreshButton);

	switchInputButton = new QPushButton("Switch Input URL");
	switchInputButton->setToolTip(
		"Change the input stream URL without stopping the process");
	connect(switchInputButton, &QPushButton::clicked, this,
		&RestreamerPlayoutDialog::onSwitchInputClicked);
	controlLayout->addWidget(switchInputButton);

	reopenButton = new QPushButton("Reconnect Input");
	reopenButton->setToolTip(
		"Close and reopen the input connection");
	connect(reopenButton, &QPushButton::clicked, this,
		&RestreamerPlayoutDialog::onReopenClicked);
	controlLayout->addWidget(reopenButton);

	controlGroup->setLayout(controlLayout);
	mainLayout->addWidget(controlGroup);

	/* Status label */
	statusLabel = new QLabel("");
	statusLabel->setWordWrap(true);
	mainLayout->addWidget(statusLabel);

	mainLayout->addStretch();

	/* Dialog buttons */
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(buttonBox->button(QDialogButtonBox::Close),
		&QPushButton::clicked, this, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
}

void RestreamerPlayoutDialog::loadPlayoutStatus()
{
	if (!api || !processId || !inputId) {
		statusLabel->setText(
			"<span style='color: orange;'>⚠ Invalid parameters</span>");
		return;
	}

	restreamer_playout_status_t status;
	memset(&status, 0, sizeof(status));

	if (!restreamer_api_get_playout_status(api, processId, inputId,
					       &status)) {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to get playout status</span>");
		return;
	}

	/* Update UI with status */
	if (status.url) {
		urlLabel->setText(QString::fromUtf8(status.url));
	}

	connectedLabel->setText(status.is_connected ? "Connected"
						    : "Disconnected");
	connectedLabel->setStyleSheet(status.is_connected
					      ? "color: green;"
					      : "color: red;");

	bitrateLabel->setText(QString("%1 kbps").arg(status.bitrate / 1000));

	/* Format bytes */
	double mb = static_cast<double>(status.bytes_received) /
		    (1024.0 * 1024.0);
	bytesLabel->setText(QString("%1 MB").arg(mb, 0, 'f', 2));

	restreamer_api_free_playout_status(&status);

	statusLabel->setText(
		"<span style='color: green;'>✓ Status updated</span>");
}

void RestreamerPlayoutDialog::onRefreshClicked()
{
	loadPlayoutStatus();
}

void RestreamerPlayoutDialog::onSwitchInputClicked()
{
	if (!api || !processId || !inputId) {
		return;
	}

	bool ok;
	QString newUrl = QInputDialog::getText(
		this, "Switch Input",
		"Enter new input stream URL:\n(e.g., rtmp://..., http://..., file://...)",
		QLineEdit::Normal, "", &ok);

	if (!ok || newUrl.trimmed().isEmpty()) {
		return;
	}

	statusLabel->setText("Switching input source...");

	if (restreamer_api_switch_input_stream(
		    api, processId, inputId, newUrl.toUtf8().constData())) {
		statusLabel->setText(
			"<span style='color: green;'>✓ Input switched successfully</span>");
		loadPlayoutStatus();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to switch input</span>");
	}
}

void RestreamerPlayoutDialog::onReopenClicked()
{
	if (!api || !processId || !inputId) {
		return;
	}

	QMessageBox::StandardButton reply = QMessageBox::question(
		this, "Reconnect Input",
		"This will close and reopen the input connection.\n\n"
		"The stream may briefly interrupt. Continue?",
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (reply != QMessageBox::Yes) {
		return;
	}

	statusLabel->setText("Reconnecting input...");

	if (restreamer_api_reopen_input(api, processId, inputId)) {
		statusLabel->setText(
			"<span style='color: green;'>✓ Input reopened successfully</span>");
		loadPlayoutStatus();
	} else {
		statusLabel->setText(
			"<span style='color: red;'>✗ Failed to reopen input</span>");
	}
}
