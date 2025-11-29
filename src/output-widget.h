/*
 * OBS Polyemesis Plugin - Output Widget
 * Individual streaming output display
 */

#pragma once

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "restreamer-channel.h"

/*
 * OutputWidget - Displays a single streaming output
 *
 * Features:
 * - Output status indicator (active/starting/error/inactive)
 * - Service name, resolution, bitrate
 * - Live statistics (current bitrate, dropped frames, duration)
 * - Inline start/stop/settings actions (shown on hover)
 * - Right-click context menu
 * - Double-click for detailed stats
 */
class OutputWidget : public QWidget {
  Q_OBJECT

public:
  explicit OutputWidget(channel_output_t *output,
                             size_t outputIndex, const char *channelId,
                             QWidget *parent = nullptr);
  ~OutputWidget() override;

  /* Update widget from output pointer */
  void updateFromOutput();

  /* Get output index */
  size_t getOutputIndex() const { return m_outputIndex; }

signals:
  /* Emitted when user requests actions */
  void startRequested(size_t outputIndex);
  void stopRequested(size_t outputIndex);
  void restartRequested(size_t outputIndex);
  void editRequested(size_t outputIndex);
  void removeRequested(size_t outputIndex);

  /* Emitted when user wants to view details */
  void viewStatsRequested(size_t outputIndex);
  void viewLogsRequested(size_t outputIndex);

protected:
  /* Event handlers */
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;

private slots:
  void onStartStopClicked();
  void onSettingsClicked();
  void onDetailsToggled();

private:
  void setupUI();
  void updateStatus();
  void updateStats();
  void showContextMenu(const QPoint &pos);
  void toggleDetailsPanel();

  /* Helper functions */
  QColor getStatusColor() const;
  QString getStatusIcon() const;
  QString getStatusText() const;
  QString formatBitrate(int kbps) const;
  QString formatDuration(int seconds) const;

  /* Output data */
  char *m_channelId;
  size_t m_outputIndex;
  channel_output_t *m_output; // Pointer to output data

  /* UI components */
  QHBoxLayout *m_mainLayout;

  QLabel *m_statusIndicator;
  QWidget *m_infoWidget;
  QVBoxLayout *m_infoLayout;
  QLabel *m_serviceLabel;
  QLabel *m_detailsLabel;

  QWidget *m_statsWidget;
  QHBoxLayout *m_statsLayout;
  QLabel *m_bitrateLabel;
  QLabel *m_droppedLabel;
  QLabel *m_durationLabel;

  QWidget *m_actionsWidget;
  QHBoxLayout *m_actionsLayout;
  QPushButton *m_startStopButton;
  QPushButton *m_settingsButton;

  /* Expanded details panel */
  QWidget *m_detailsPanel;
  bool m_detailsExpanded;

  /* State */
  bool m_hovered;
};
