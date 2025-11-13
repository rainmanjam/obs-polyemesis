/*
 * OBS Polyemesis Plugin - Theme Utilities Implementation
 */

#include "obs-theme-utils.h"

#include <QApplication>
#include <QWidget>
#include <obs-frontend-api.h>

/*
 * Get success color (green) that adapts to theme brightness
 * Uses green with appropriate lightness for the current theme
 */
QColor obs_theme_get_success_color() {
  QPalette palette = QApplication::palette();
  bool isDark = obs_theme_is_dark();

  /* Green hue (120 degrees), saturation 60-80%, lightness varies by theme */
  if (isDark) {
    /* Dark theme: Use bright, saturated green */
    return QColor::fromHsv(120, 180, 200); /* Bright green: #73d873 */
  } else {
    /* Light theme: Use darker, muted green */
    return QColor::fromHsv(120, 140, 120); /* Dark green: #3e783e */
  }
}

/*
 * Get error color (red) that adapts to theme brightness
 */
QColor obs_theme_get_error_color() {
  bool isDark = obs_theme_is_dark();

  /* Red hue (0 degrees), saturation 60-80%, lightness varies by theme */
  if (isDark) {
    /* Dark theme: Use bright, saturated red */
    return QColor::fromHsv(0, 180, 220); /* Bright red: #dc6464 */
  } else {
    /* Light theme: Use darker, muted red */
    return QColor::fromHsv(0, 160, 140); /* Dark red: #8c3838 */
  }
}

/*
 * Get warning color (orange/yellow) that adapts to theme brightness
 */
QColor obs_theme_get_warning_color() {
  bool isDark = obs_theme_is_dark();

  /* Orange hue (30 degrees), saturation 70-90%, lightness varies by theme */
  if (isDark) {
    /* Dark theme: Use bright, saturated orange */
    return QColor::fromHsv(30, 200, 220); /* Bright orange: #dcaa64 */
  } else {
    /* Light theme: Use darker, muted orange */
    return QColor::fromHsv(30, 180, 140); /* Dark orange: #8c6c38 */
  }
}

/*
 * Get info color (blue) that adapts to theme brightness
 */
QColor obs_theme_get_info_color() {
  bool isDark = obs_theme_is_dark();

  /* Blue hue (210 degrees), saturation 60-80%, lightness varies by theme */
  if (isDark) {
    /* Dark theme: Use bright, saturated blue */
    return QColor::fromHsv(210, 180, 220); /* Bright blue: #6496dc */
  } else {
    /* Light theme: Use darker, muted blue */
    return QColor::fromHsv(210, 160, 140); /* Dark blue: #38608c */
  }
}

/*
 * Get muted/disabled text color from the current palette
 */
QColor obs_theme_get_muted_color() {
  QPalette palette = QApplication::palette();
  return palette.color(QPalette::Disabled, QPalette::WindowText);
}

/*
 * Get the current OBS theme name
 * Returns "Dark" or "Light" based on palette detection
 * Note: OBS doesn't expose theme name via API, so we detect based on colors
 */
QString obs_theme_get_name() {
  /* Simplified implementation: detect dark vs light theme from palette */
  bool isDark = obs_theme_is_dark();
  QString themeName = isDark ? "Dark" : "Light";
  return themeName;
}

/*
 * Check if current theme is dark (vs light)
 * Uses QPalette window color lightness as the indicator
 */
bool obs_theme_is_dark() {
  QPalette palette = QApplication::palette();
  QColor windowColor = palette.color(QPalette::Window);

  /* Threshold: lightness < 128 means dark theme */
  return windowColor.lightness() < 128;
}

/*
 * Initialize theme utilities
 * Currently no initialization needed, but reserved for future use
 * (e.g., theme change listener)
 */
void obs_theme_utils_init() {
  /* Future: Register theme change callback with OBS */
  /* obs_frontend_add_event_callback(theme_change_callback, nullptr); */
}

/*
 * Cleanup theme utilities
 * Currently no cleanup needed, but reserved for future use
 */
void obs_theme_utils_cleanup() {
  /* Future: Unregister theme change callback */
  /* obs_frontend_remove_event_callback(theme_change_callback, nullptr); */
}
