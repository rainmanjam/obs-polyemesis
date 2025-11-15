#include "restreamer-encoding-dialog.h"
#include <QMessageBox>
#include <QScrollArea>
#include <obs-module.h>
#include <plugin-support.h>
#include <util/bmem.h>

/* Platform Encoding Presets */
struct EncodingPreset {
	QString name;
	QString videoCodec;
	QString audioCodec;
	int videoWidth;
	int videoHeight;
	int videoBitrate; // kbps
	int audioBitrate; // kbps
	int fpsNum;
	int fpsDen;
	QString preset;
	QString profile;
	QString tune;
	QString description;
};

static const EncodingPreset PRESETS[] = {
	// YouTube Presets
	{"YouTube HD 720p", "libx264", "aac", 1280, 720, 5000, 128, 30, 1,
	 "veryfast", "high", "zerolatency",
	 "Standard HD streaming for YouTube"},
	{"YouTube Full HD 1080p", "libx264", "aac", 1920, 1080, 8000, 128, 30,
	 1, "veryfast", "high", "zerolatency",
	 "Full HD streaming for YouTube"},
	{"YouTube 4K 2160p", "libx264", "aac", 3840, 2160, 35000, 192, 30, 1,
	 "fast", "high", "zerolatency", "4K Ultra HD for YouTube"},
	{"YouTube 60fps FHD", "libx264", "aac", 1920, 1080, 12000, 128, 60, 1,
	 "veryfast", "high", "zerolatency", "Full HD 60fps for YouTube"},

	// Facebook Presets
	{"Facebook Live HD", "libx264", "aac", 1280, 720, 4000, 128, 30, 1,
	 "veryfast", "main", "zerolatency",
	 "Optimized for Facebook Live HD"},
	{"Facebook Live FHD", "libx264", "aac", 1920, 1080, 6000, 128, 30, 1,
	 "veryfast", "main", "zerolatency",
	 "Full HD for Facebook Live"},

	// Twitch Presets
	{"Twitch HD 720p", "libx264", "aac", 1280, 720, 4500, 160, 30, 1,
	 "veryfast", "main", "zerolatency",
	 "Recommended for Twitch Partners"},
	{"Twitch Full HD 1080p", "libx264", "aac", 1920, 1080, 6000, 160, 60,
	 1, "veryfast", "main", "zerolatency",
	 "1080p 60fps for Twitch (requires Partner)"},
	{"Twitch Standard 720p30", "libx264", "aac", 1280, 720, 3000, 128, 30,
	 1, "veryfast", "main", "zerolatency",
	 "Safe bitrate for non-partners"},

	// TikTok / Instagram (Vertical)
	{"TikTok / Instagram Vertical", "libx264", "aac", 1080, 1920, 4000,
	 128, 30, 1, "veryfast", "main", "zerolatency",
	 "Vertical 9:16 format for TikTok/IG"},
	{"Instagram Reels", "libx264", "aac", 1080, 1920, 3500, 128, 30, 1,
	 "veryfast", "main", "zerolatency",
	 "Optimized for Instagram Reels"},

	// Kick
	{"Kick HD", "libx264", "aac", 1280, 720, 5000, 160, 60, 1, "veryfast",
	 "main", "zerolatency", "HD streaming for Kick.com"},
	{"Kick FHD", "libx264", "aac", 1920, 1080, 8000, 160, 60, 1,
	 "veryfast", "main", "zerolatency", "Full HD for Kick.com"},

	// Low Bandwidth
	{"Low Bandwidth SD", "libx264", "aac", 854, 480, 1500, 96, 30, 1,
	 "veryfast", "baseline", "zerolatency",
	 "Low bandwidth option for slow connections"},
};

static const int PRESET_COUNT = sizeof(PRESETS) / sizeof(EncodingPreset);

RestreamerEncodingDialog::RestreamerEncodingDialog(QWidget *parent,
						   restreamer_api_t *api,
						   const char *process_id,
						   const char *output_id)
	: QDialog(parent), api(api)
{
	processId = process_id ? bstrdup(process_id) : nullptr;
	outputId = output_id ? bstrdup(output_id) : nullptr;

	setWindowTitle("Encoding Configuration");
	setMinimumSize(700, 600);

	setupUI();
	loadCurrentSettings();
}

RestreamerEncodingDialog::~RestreamerEncodingDialog()
{
	bfree(processId);
	bfree(outputId);
}

void RestreamerEncodingDialog::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	/* Header with info */
	QLabel *headerLabel = new QLabel(
		QString("Configure encoding settings for process: %1")
			.arg(processId ? processId : "Unknown"));
	headerLabel->setStyleSheet("font-weight: bold; font-size: 12pt;");
	mainLayout->addWidget(headerLabel);

	/* Tabs for different settings categories */
	tabWidget = new QTabWidget();
	createVideoTab();
	createAudioTab();
	createAdvancedTab();
	createPresetsTab();
	mainLayout->addWidget(tabWidget);

	/* Validation label */
	validationLabel = new QLabel("");
	validationLabel->setWordWrap(true);
	mainLayout->addWidget(validationLabel);

	/* Buttons */
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply |
					 QDialogButtonBox::Close);

	probeButton = new QPushButton("Probe Input");
	buttonBox->addButton(probeButton, QDialogButtonBox::ActionRole);

	refreshButton = new QPushButton("Refresh");
	buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);

	loadFromProfileButton = new QPushButton("Load from Profile");
	buttonBox->addButton(loadFromProfileButton,
			     QDialogButtonBox::ActionRole);

	connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
		this, &RestreamerEncodingDialog::onApplyClicked);
	connect(buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked,
		this, &QDialog::reject);
	connect(probeButton, &QPushButton::clicked, this,
		&RestreamerEncodingDialog::onProbeInputClicked);
	connect(refreshButton, &QPushButton::clicked, this,
		&RestreamerEncodingDialog::onRefreshClicked);
	connect(loadFromProfileButton, &QPushButton::clicked, this,
		&RestreamerEncodingDialog::onLoadFromProfileClicked);

	mainLayout->addWidget(buttonBox);
}

void RestreamerEncodingDialog::createVideoTab()
{
	QWidget *videoTab = new QWidget();
	QFormLayout *form = new QFormLayout(videoTab);

	/* Video Codec */
	videoCodecCombo = new QComboBox();
	videoCodecCombo->addItems({"copy", "libx264", "libx265", "h264_nvenc",
				   "h264_qsv", "h264_videotoolbox"});
	connect(videoCodecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &RestreamerEncodingDialog::onVideoCodecChanged);
	form->addRow("Video Codec:", videoCodecCombo);

	/* Resolution */
	QHBoxLayout *resLayout = new QHBoxLayout();
	videoWidthSpin = new QSpinBox();
	videoWidthSpin->setRange(128, 7680);
	videoWidthSpin->setValue(1920);
	videoWidthSpin->setSingleStep(2);
	videoHeightSpin = new QSpinBox();
	videoHeightSpin->setRange(128, 4320);
	videoHeightSpin->setValue(1080);
	videoHeightSpin->setSingleStep(2);
	resLayout->addWidget(videoWidthSpin);
	resLayout->addWidget(new QLabel("×"));
	resLayout->addWidget(videoHeightSpin);

	maintainAspectCheckbox = new QCheckBox("Maintain Aspect Ratio");
	maintainAspectCheckbox->setChecked(true);
	resLayout->addWidget(maintainAspectCheckbox);
	resLayout->addStretch();
	form->addRow("Resolution:", resLayout);

	/* FPS */
	QHBoxLayout *fpsLayout = new QHBoxLayout();
	fpsNumeratorSpin = new QSpinBox();
	fpsNumeratorSpin->setRange(1, 120);
	fpsNumeratorSpin->setValue(30);
	fpsDenominatorSpin = new QSpinBox();
	fpsDenominatorSpin->setRange(1, 10);
	fpsDenominatorSpin->setValue(1);
	fpsLayout->addWidget(fpsNumeratorSpin);
	fpsLayout->addWidget(new QLabel("/"));
	fpsLayout->addWidget(fpsDenominatorSpin);
	fpsLayout->addStretch();
	form->addRow("Frame Rate:", fpsLayout);

	/* Video Bitrate */
	QVBoxLayout *vbLayout = new QVBoxLayout();
	videoBitrateSlider = new QSlider(Qt::Horizontal);
	videoBitrateSlider->setRange(500, 50000);
	videoBitrateSlider->setValue(5000);
	videoBitrateLabel = new QLabel("5000 kbps");
	connect(videoBitrateSlider, &QSlider::valueChanged, this,
		&RestreamerEncodingDialog::onVideoBitrateSliderChanged);
	vbLayout->addWidget(videoBitrateSlider);
	vbLayout->addWidget(videoBitrateLabel);
	form->addRow("Video Bitrate:", vbLayout);

	/* Preset */
	videoPresetCombo = new QComboBox();
	videoPresetCombo->addItems({"ultrafast", "superfast", "veryfast",
				    "faster", "fast", "medium", "slow",
				    "slower", "veryslow", "placebo"});
	videoPresetCombo->setCurrentText("veryfast");
	form->addRow("Encoder Preset:", videoPresetCombo);

	/* Profile */
	videoProfileCombo = new QComboBox();
	videoProfileCombo->addItems({"auto", "baseline", "main", "high"});
	videoProfileCombo->setCurrentText("high");
	form->addRow("Profile:", videoProfileCombo);

	/* Tune */
	videoTuneCombo = new QComboBox();
	videoTuneCombo->addItems({"none", "film", "animation", "grain",
				  "stillimage", "fastdecode", "zerolatency"});
	videoTuneCombo->setCurrentText("zerolatency");
	form->addRow("Tune:", videoTuneCombo);

	/* Pixel Format */
	pixelFormatCombo = new QComboBox();
	pixelFormatCombo->addItems(
		{"yuv420p", "yuv422p", "yuv444p", "nv12", "nv21"});
	pixelFormatCombo->setCurrentText("yuv420p");
	form->addRow("Pixel Format:", pixelFormatCombo);

	tabWidget->addTab(videoTab, "Video");
}

void RestreamerEncodingDialog::createAudioTab()
{
	QWidget *audioTab = new QWidget();
	QFormLayout *form = new QFormLayout(audioTab);

	/* Audio Codec */
	audioCodecCombo = new QComboBox();
	audioCodecCombo->addItems({"copy", "aac", "mp3", "opus", "none"});
	audioCodecCombo->setCurrentText("aac");
	form->addRow("Audio Codec:", audioCodecCombo);

	/* Audio Bitrate */
	QVBoxLayout *abLayout = new QVBoxLayout();
	audioBitrateSlider = new QSlider(Qt::Horizontal);
	audioBitrateSlider->setRange(64, 320);
	audioBitrateSlider->setValue(128);
	audioBitrateLabel = new QLabel("128 kbps");
	connect(audioBitrateSlider, &QSlider::valueChanged, this,
		&RestreamerEncodingDialog::onAudioBitrateSliderChanged);
	abLayout->addWidget(audioBitrateSlider);
	abLayout->addWidget(audioBitrateLabel);
	form->addRow("Audio Bitrate:", abLayout);

	/* Channels */
	audioChannelsCombo = new QComboBox();
	audioChannelsCombo->addItems({"mono", "stereo", "inherit"});
	audioChannelsCombo->setCurrentText("stereo");
	form->addRow("Channels:", audioChannelsCombo);

	/* Sample Rate */
	audioSampleRateCombo = new QComboBox();
	audioSampleRateCombo->addItems(
		{"inherit", "22050", "44100", "48000", "96000"});
	audioSampleRateCombo->setCurrentText("44100");
	form->addRow("Sample Rate:", audioSampleRateCombo);

	tabWidget->addTab(audioTab, "Audio");
}

void RestreamerEncodingDialog::createAdvancedTab()
{
	QWidget *advTab = new QWidget();
	QFormLayout *form = new QFormLayout(advTab);

	/* GOP Size */
	gopSizeSpin = new QSpinBox();
	gopSizeSpin->setRange(1, 600);
	gopSizeSpin->setValue(60);
	gopSizeSpin->setToolTip(
		"Group of Pictures size (keyframe interval). 2x FPS recommended.");
	form->addRow("GOP Size:", gopSizeSpin);

	/* B-Frames */
	bFramesSpin = new QSpinBox();
	bFramesSpin->setRange(0, 16);
	bFramesSpin->setValue(0);
	bFramesSpin->setToolTip("Number of B-frames (0 for low latency)");
	form->addRow("B-Frames:", bFramesSpin);

	/* Reference Frames */
	refFramesSpin = new QSpinBox();
	refFramesSpin->setRange(1, 16);
	refFramesSpin->setValue(3);
	refFramesSpin->setToolTip("Number of reference frames");
	form->addRow("Reference Frames:", refFramesSpin);

	/* Rate Control Mode */
	rcModeCombo = new QComboBox();
	rcModeCombo->addItems({"CBR", "VBR", "CRF"});
	rcModeCombo->setCurrentText("CBR");
	rcModeCombo->setToolTip(
		"CBR: Constant bitrate, VBR: Variable bitrate, CRF: Constant rate factor");
	form->addRow("Rate Control:", rcModeCombo);

	/* Max Bitrate */
	maxBitrateSpin = new QSpinBox();
	maxBitrateSpin->setRange(0, 100000);
	maxBitrateSpin->setValue(0);
	maxBitrateSpin->setSuffix(" kbps");
	maxBitrateSpin->setToolTip("Maximum bitrate (0 = use target bitrate)");
	form->addRow("Max Bitrate:", maxBitrateSpin);

	/* Buffer Size */
	bufferSizeSpin = new QSpinBox();
	bufferSizeSpin->setRange(0, 200000);
	bufferSizeSpin->setValue(0);
	bufferSizeSpin->setSuffix(" kbits");
	bufferSizeSpin->setToolTip(
		"VBV buffer size (0 = 2x target bitrate)");
	form->addRow("Buffer Size:", bufferSizeSpin);

	/* Info section */
	form->addRow(new QLabel(""));
	QLabel *infoLabel = new QLabel(
		"<b>Advanced settings:</b><br>"
		"These settings provide fine control over encoding quality and performance.<br>"
		"<b>GOP Size:</b> Recommended 2× frame rate for good seek performance.<br>"
		"<b>B-Frames:</b> Set to 0 for ultra-low latency streaming.<br>"
		"<b>CBR:</b> Best for streaming (constant network usage).<br>"
		"<b>VBR:</b> Better quality, variable network usage.<br>"
		"<b>CRF:</b> Best quality, not recommended for live streaming.");
	infoLabel->setWordWrap(true);
	infoLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
	form->addRow(infoLabel);

	tabWidget->addTab(advTab, "Advanced");
}

void RestreamerEncodingDialog::createPresetsTab()
{
	QWidget *presetsTab = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(presetsTab);

	QLabel *headerLabel = new QLabel(
		"<b>Platform Presets</b><br>"
		"Click a button to apply recommended encoding settings for each platform:");
	headerLabel->setWordWrap(true);
	layout->addWidget(headerLabel);

	QScrollArea *scrollArea = new QScrollArea();
	scrollArea->setWidgetResizable(true);
	QWidget *scrollWidget = new QWidget();
	QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);

	/* Create buttons for each preset */
	for (int i = 0; i < PRESET_COUNT; i++) {
		const EncodingPreset &preset = PRESETS[i];

		QGroupBox *presetBox = new QGroupBox(preset.name);
		QVBoxLayout *presetLayout = new QVBoxLayout();

		QString details = QString(
			"<b>Resolution:</b> %1x%2 @ %3fps<br>"
			"<b>Video:</b> %4 kbps (%5, %6 profile, %7 tune)<br>"
			"<b>Audio:</b> %8 kbps (%9)<br>"
			"<br>%10")
					  .arg(preset.videoWidth)
					  .arg(preset.videoHeight)
					  .arg(preset.fpsNum)
					  .arg(preset.videoBitrate)
					  .arg(preset.preset)
					  .arg(preset.profile)
					  .arg(preset.tune)
					  .arg(preset.audioBitrate)
					  .arg(preset.audioCodec)
					  .arg(preset.description);

		QLabel *detailsLabel = new QLabel(details);
		detailsLabel->setWordWrap(true);
		detailsLabel->setStyleSheet("font-size: 9pt;");
		presetLayout->addWidget(detailsLabel);

		QPushButton *applyButton = new QPushButton("Apply This Preset");
		connect(applyButton, &QPushButton::clicked,
			[this, i]() { applyPreset(PRESETS[i].name); });
		presetLayout->addWidget(applyButton);

		presetBox->setLayout(presetLayout);
		scrollLayout->addWidget(presetBox);
	}

	scrollLayout->addStretch();
	scrollWidget->setLayout(scrollLayout);
	scrollArea->setWidget(scrollWidget);
	layout->addWidget(scrollArea);

	tabWidget->addTab(presetsTab, "Presets");
}

void RestreamerEncodingDialog::loadCurrentSettings()
{
	if (!api || !processId || !outputId) {
		validationLabel->setText(
			"<span style='color: orange;'>⚠ No process/output selected. Using default values.</span>");
		return;
	}

	/* Get current encoding settings from API */
	encoding_params_t params;
	memset(&params, 0, sizeof(params));

	if (restreamer_api_get_output_encoding(api, processId, outputId,
					       &params)) {
		/* Update UI with current values */
		if (params.video_bitrate_kbps > 0) {
			videoBitrateSlider->setValue(params.video_bitrate_kbps);
		}
		if (params.audio_bitrate_kbps > 0) {
			audioBitrateSlider->setValue(params.audio_bitrate_kbps);
		}
		if (params.width > 0 && params.height > 0) {
			videoWidthSpin->setValue(params.width);
			videoHeightSpin->setValue(params.height);
		}
		if (params.fps_num > 0) {
			fpsNumeratorSpin->setValue(params.fps_num);
			fpsDenominatorSpin->setValue(
				params.fps_den > 0 ? params.fps_den : 1);
		}
		if (params.preset) {
			videoPresetCombo->setCurrentText(params.preset);
		}
		if (params.profile) {
			videoProfileCombo->setCurrentText(params.profile);
		}

		restreamer_api_free_encoding_params(&params);
		validationLabel->setText(
			"<span style='color: green;'>✓ Loaded current settings from Restreamer</span>");
	} else {
		validationLabel->setText(
			QString(
				"<span style='color: red;'>✗ Failed to load settings: %1</span>")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerEncodingDialog::onVideoCodecChanged(int index)
{
	(void)index; /* Unused - we get the value from the combo box */
	QString codec = videoCodecCombo->currentText();

	/* Disable encoding options if using 'copy' */
	bool enableOptions = (codec != "copy");
	videoPresetCombo->setEnabled(enableOptions);
	videoProfileCombo->setEnabled(enableOptions);
	videoTuneCombo->setEnabled(enableOptions);
	videoBitrateSlider->setEnabled(enableOptions);
	videoWidthSpin->setEnabled(enableOptions);
	videoHeightSpin->setEnabled(enableOptions);
	pixelFormatCombo->setEnabled(enableOptions);
}

void RestreamerEncodingDialog::onVideoBitrateSliderChanged(int value)
{
	videoBitrateLabel->setText(QString("%1 kbps").arg(value));
}

void RestreamerEncodingDialog::onAudioBitrateSliderChanged(int value)
{
	audioBitrateLabel->setText(QString("%1 kbps").arg(value));
}

bool RestreamerEncodingDialog::validateAndApply()
{
	if (!api || !processId || !outputId) {
		QMessageBox::critical(
			this, "Invalid State",
			"Cannot apply settings: no process or output selected.");
		return false;
	}

	/* Build encoding params structure */
	encoding_params_t params;
	memset(&params, 0, sizeof(params));

	params.video_bitrate_kbps = videoBitrateSlider->value();
	params.audio_bitrate_kbps = audioBitrateSlider->value();
	params.width = videoWidthSpin->value();
	params.height = videoHeightSpin->value();
	params.fps_num = fpsNumeratorSpin->value();
	params.fps_den = fpsDenominatorSpin->value();

	QString preset = videoPresetCombo->currentText();
	QString profile = videoProfileCombo->currentText();

	params.preset = bstrdup(preset.toUtf8().constData());
	params.profile = bstrdup(profile.toUtf8().constData());

	/* Apply via API */
	bool success = restreamer_api_update_output_encoding(
		api, processId, outputId, &params);

	restreamer_api_free_encoding_params(&params);

	if (success) {
		validationLabel->setText(
			"<span style='color: green;'>✓ Encoding settings applied successfully!</span>");
		QMessageBox::information(
			this, "Success",
			"Encoding settings have been applied. Changes will take effect on the next stream start.");
		return true;
	} else {
		validationLabel->setText(
			QString(
				"<span style='color: red;'>✗ Failed to apply settings: %1</span>")
				.arg(restreamer_api_get_error(api)));
		QMessageBox::critical(
			this, "Apply Failed",
			QString("Failed to update encoding settings:\n%1")
				.arg(restreamer_api_get_error(api)));
		return false;
	}
}

void RestreamerEncodingDialog::onApplyClicked()
{
	validateAndApply();
}

void RestreamerEncodingDialog::onRefreshClicked()
{
	loadCurrentSettings();
}

void RestreamerEncodingDialog::onProbeInputClicked()
{
	if (!api || !processId) {
		QMessageBox::warning(this, "No Process",
				     "No process selected for probing.");
		return;
	}

	restreamer_probe_info_t info;
	memset(&info, 0, sizeof(info));

	if (restreamer_api_probe_input(api, processId, &info)) {
		showProbeResults(&info);
		restreamer_api_free_probe_info(&info);
	} else {
		QMessageBox::critical(
			this, "Probe Failed",
			QString("Failed to probe input: %1")
				.arg(restreamer_api_get_error(api)));
	}
}

void RestreamerEncodingDialog::showProbeResults(restreamer_probe_info_t *info)
{
	QString result = QString("<b>Input Format:</b> %1<br>")
				 .arg(info->format_long_name
					      ? info->format_long_name
					      : "Unknown");

	if (info->duration > 0) {
		result += QString("<b>Duration:</b> %1 seconds<br>")
				  .arg(info->duration / 1000000);
	}
	if (info->bitrate > 0) {
		result += QString("<b>Bitrate:</b> %1 kbps<br>")
				  .arg(info->bitrate / 1000);
	}

	result += "<br><b>Streams:</b><br>";

	for (size_t i = 0; i < info->stream_count; i++) {
		const restreamer_stream_info_t &stream = info->streams[i];
		result += QString("<br><b>Stream %1 (%2):</b><br>")
				  .arg(i)
				  .arg(stream.codec_type ? stream.codec_type
							 : "unknown");

		if (stream.codec_name) {
			result += QString("Codec: %1<br>").arg(
				stream.codec_name);
		}
		if (strcmp(stream.codec_type, "video") == 0) {
			result += QString("Resolution: %1x%2<br>")
					  .arg(stream.width)
					  .arg(stream.height);
			if (stream.fps_num > 0) {
				result += QString("FPS: %1/%2<br>")
						  .arg(stream.fps_num)
						  .arg(stream.fps_den);
			}
		} else if (strcmp(stream.codec_type, "audio") == 0) {
			result += QString("Sample Rate: %1 Hz<br>")
					  .arg(stream.sample_rate);
			result += QString("Channels: %1<br>")
					  .arg(stream.channels);
		}
	}

	QMessageBox msgBox(this);
	msgBox.setWindowTitle("Input Probe Results");
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText(result);
	msgBox.exec();
}

void RestreamerEncodingDialog::onLoadFromProfileClicked()
{
	QMessageBox::information(
		this, "Not Implemented",
		"Profile loading will be implemented in the profile integration phase.");
}

void RestreamerEncodingDialog::applyPreset(const QString &presetName)
{
	const EncodingPreset *preset = nullptr;
	for (int i = 0; i < PRESET_COUNT; i++) {
		if (PRESETS[i].name == presetName) {
			preset = &PRESETS[i];
			break;
		}
	}

	if (!preset) {
		return;
	}

	/* Apply preset values to UI */
	videoCodecCombo->setCurrentText(preset->videoCodec);
	audioCodecCombo->setCurrentText(preset->audioCodec);
	videoWidthSpin->setValue(preset->videoWidth);
	videoHeightSpin->setValue(preset->videoHeight);
	videoBitrateSlider->setValue(preset->videoBitrate);
	audioBitrateSlider->setValue(preset->audioBitrate);
	fpsNumeratorSpin->setValue(preset->fpsNum);
	fpsDenominatorSpin->setValue(preset->fpsDen);
	videoPresetCombo->setCurrentText(preset->preset);
	videoProfileCombo->setCurrentText(preset->profile);
	videoTuneCombo->setCurrentText(preset->tune);

	/* Set GOP size to 2x FPS */
	gopSizeSpin->setValue(preset->fpsNum * 2);

	/* Zero B-frames for low latency */
	bFramesSpin->setValue(0);

	validationLabel->setText(
		QString(
			"<span style='color: blue;'>ℹ Preset '%1' loaded. Click Apply to save.</span>")
			.arg(presetName));

	/* Switch to Video tab to show applied settings */
	tabWidget->setCurrentIndex(0);

	QMessageBox::information(
		this, "Preset Applied",
		QString("Preset '%1' has been loaded.\n\n%2\n\nClick 'Apply' to save these settings.")
			.arg(presetName)
			.arg(preset->description));
}
