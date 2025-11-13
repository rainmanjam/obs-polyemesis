/*
 * OBS Polyemesis Plugin - Theme Utilities
 * Provides semantic color helpers that adapt to OBS's active theme
 */

#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get semantic colors that adapt to the active OBS theme
 * These colors work across all 6 OBS themes: Yami, Grey, Acri, Dark, Rachni, Light
 */

/* Success indicator color (typically green) */
QColor obs_theme_get_success_color();

/* Error indicator color (typically red) */
QColor obs_theme_get_error_color();

/* Warning indicator color (typically orange/yellow) */
QColor obs_theme_get_warning_color();

/* Info indicator color (typically blue) */
QColor obs_theme_get_info_color();

/* Muted/disabled text color from palette */
QColor obs_theme_get_muted_color();

/* Get the current OBS theme name (Yami, Grey, Acri, Dark, Rachni, Light) */
QString obs_theme_get_name();

/* Check if current theme is dark (vs light) */
bool obs_theme_is_dark();

/*
 * Initialize theme utilities
 * Should be called once during plugin initialization
 */
void obs_theme_utils_init();

/*
 * Cleanup theme utilities
 * Should be called during plugin shutdown
 */
void obs_theme_utils_cleanup();

#ifdef __cplusplus
}
#endif
