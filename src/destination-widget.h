/*
 * OBS Polyemesis Plugin - Destination Widget
 * Individual streaming destination display
 */

#pragma once

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "restreamer-output-profile.h"

/*
 * DestinationWidget - Displays a single streaming destination
 *
 * Features:
 * - Destination status indicator (active/starting/error/inactive)
 * - Service name, resolution, bitrate
 * - Live statistics (current bitrate, dropped frames, duration)
 * - Inline start/stop/settings actions (shown on hover)
 * - Right-click context menu
 * - Double-click for detailed stats
 */
class DestinationWidget : public QWidget {
  Q_OBJECT

public:
  explicit DestinationWidget(profile_destination_t *destination,
                             size_t destIndex, const char *profileId,
                             QWidget *parent = nullptr);
  ~DestinationWidget() override;

  /* Update widget from destination pointer */
  void updateFromDestination();

  /* Get destination index */
  size_t getDestinationIndex() const { return m_destinationIndex; }

signals:
  /* Emitted when user requests actions */
  void startRequested(size_t destIndex);
  void stopRequested(size_t destIndex);
  void restartRequested(size_t destIndex);
  void editRequested(size_t destIndex);
  void removeRequested(size_t destIndex);

  /* Emitted when user wants to view details */
  void viewStatsRequested(size_t destIndex);
  void viewLogsRequested(size_t destIndex);

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

  /* Destination data */
  char *m_profileId;
  size_t m_destinationIndex;
  profile_destination_t *m_destination; // Pointer to destination data

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
