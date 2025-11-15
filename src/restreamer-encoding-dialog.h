#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "restreamer-api.h"

/* Encoding Configuration Dialog
 * Provides user-friendly interface for configuring encoding settings
 */
class RestreamerEncodingDialog : public QDialog {
	Q_OBJECT

public:
	RestreamerEncodingDialog(QWidget *parent, restreamer_api_t *api,
				 const char *process_id, const char *output_id);
	~RestreamerEncodingDialog();

	/* Apply platform-specific preset */
	void applyPreset(const QString &platform);

private slots:
	void onVideoCodecChanged(int index);
	void onApplyClicked();
	void onProbeInputClicked();
	void onLoadFromProfileClicked();
	void onVideoBitrateSliderChanged(int value);
	void onAudioBitrateSliderChanged(int value);
	void onRefreshClicked();

private:
	void setupUI();
	void loadCurrentSettings();
	void createVideoTab();
	void createAudioTab();
	void createAdvancedTab();
	void createPresetsTab();
	bool validateAndApply();
	void showProbeResults(restreamer_probe_info_t *info);

	restreamer_api_t *api;
	char *processId;
	char *outputId;

	/* Video encoding widgets */
	QComboBox *videoCodecCombo;
	QComboBox *videoPresetCombo;
	QComboBox *videoProfileCombo;
	QComboBox *videoTuneCombo;
	QSpinBox *videoWidthSpin;
	QSpinBox *videoHeightSpin;
	QSlider *videoBitrateSlider;
	QLabel *videoBitrateLabel;
	QSpinBox *fpsNumeratorSpin;
	QSpinBox *fpsDenominatorSpin;
	QComboBox *pixelFormatCombo;
	QCheckBox *maintainAspectCheckbox;

	/* Audio encoding widgets */
	QComboBox *audioCodecCombo;
	QComboBox *audioPresetCombo;
	QSlider *audioBitrateSlider;
	QLabel *audioBitrateLabel;
	QComboBox *audioChannelsCombo;
	QComboBox *audioSampleRateCombo;

	/* Advanced settings */
	QSpinBox *gopSizeSpin;
	QSpinBox *bFramesSpin;
	QSpinBox *refFramesSpin;
	QComboBox *rcModeCombo; // Rate control: CBR, VBR, CRF
	QSpinBox *cqpSpin;	// Constant Quantizer Parameter
	QSpinBox *maxBitrateSpin;
	QSpinBox *bufferSizeSpin;

	/* Info labels */
	QLabel *currentSettingsLabel;
	QLabel *estimatedBitrateLabel;
	QLabel *validationLabel;

	/* Tabs */
	QTabWidget *tabWidget;

	/* Buttons */
	QDialogButtonBox *buttonBox;
	QPushButton *probeButton;
	QPushButton *refreshButton;
	QPushButton *loadFromProfileButton;
};
