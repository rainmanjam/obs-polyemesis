#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>
#include <functional>
#include <obs.h>
#include <obs-module.h>
#include <unordered_map>

/**
 * @brief Helper class for dynamically generating Qt widgets from OBS properties
 *
 * This class introspects OBS encoder/service properties and creates appropriate
 * Qt widgets, handling all property types and dependencies automatically.
 */
class PropertyWidgetFactory {
public:
	PropertyWidgetFactory(QFormLayout *layout, obs_data_t *settings)
		: layout(layout), settings(settings)
	{
	}

	~PropertyWidgetFactory()
	{
		clearWidgets();
	}

	/**
	 * @brief Load encoder properties and generate UI widgets
	 * @param encoder_id OBS encoder ID (e.g., "obs_x264", "ffmpeg_nvenc")
	 */
	void loadEncoderProperties(const char *encoder_id);

	/**
	 * @brief Clear all generated widgets
	 */
	void clearWidgets();

	/**
	 * @brief Refresh widget visibility based on property dependencies
	 */
	void refreshProperties();

	/**
	 * @brief Get the settings data
	 */
	obs_data_t *getSettings() const { return settings; }

private:
	/**
	 * @brief Create a widget for a specific OBS property
	 * @param prop OBS property to create widget for
	 * @return Created Qt widget, or nullptr if property not visible/supported
	 */
	QWidget *createPropertyWidget(obs_property_t *prop);

	/**
	 * @brief Handle property value changes and trigger refresh if needed
	 */
	void onPropertyModified(obs_property_t *prop);

	/* Widget creators for each property type */
	QWidget *createBoolWidget(obs_property_t *prop);
	QWidget *createIntWidget(obs_property_t *prop);
	QWidget *createFloatWidget(obs_property_t *prop);
	QWidget *createTextWidget(obs_property_t *prop);
	QWidget *createPathWidget(obs_property_t *prop);
	QWidget *createListWidget(obs_property_t *prop);
	QWidget *createColorWidget(obs_property_t *prop);
	QWidget *createButtonWidget(obs_property_t *prop);
	QWidget *createFontWidget(obs_property_t *prop);

	/* Layout and data */
	QFormLayout *layout;
	obs_data_t *settings;
	obs_properties_t *props = nullptr;
	const char *currentEncoderId = nullptr;

	/* Widget tracking for refresh */
	struct WidgetInfo {
		QWidget *widget;
		QLabel *label;
		obs_property_t *property;
	};
	std::vector<WidgetInfo> widgetList;
};

#endif // PROPERTY_WIDGETS_HPP
