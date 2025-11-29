/*
 * OBS Polyemesis Plugin - Channel Widget
 * Individual channel display with expandable outputs
 */

#pragma once

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "restreamer-channel.h"

/* Forward declarations */
class OutputWidget;

/*
 * ChannelWidget - Displays a single streaming channel with outputs
 *
 * Features:
 * - Channel header with status indicator
 * - Aggregate status (all active, some active, errors)
 * - Expandable to show output list
 * - Start/stop/edit actions
 * - Right-click context menu
 * - Hover actions
 */
class ChannelWidget : public QWidget {
  Q_OBJECT

public:
  explicit ChannelWidget(stream_channel_t *channel, QWidget *parent = nullptr);
  ~ChannelWidget() override;

  /* Get/set expanded state */
  bool isExpanded() const { return m_expanded; }
  void setExpanded(bool expanded);

  /* Update widget from channel data */
  void updateFromChannel();

  /* Get channel ID */
  const char *getChannelId() const;

signals:
  /* Emitted when user requests actions */
  void startRequested(const char *channelId);
  void stopRequested(const char *channelId);
  void editRequested(const char *channelId);
  void deleteRequested(const char *channelId);
  void duplicateRequested(const char *channelId);

  /* Emitted when output-specific actions are requested */
  void outputStartRequested(const char *channelId, size_t outputIndex);
  void outputStopRequested(const char *channelId, size_t outputIndex);
  void outputEditRequested(const char *channelId, size_t outputIndex);

  /* Emitted when expanded state changes */
  void expandedChanged(bool expanded);

protected:
  /* Event handlers */
  void contextMenuEvent(QContextMenuEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;

private slots:
  void onHeaderClicked();
  void onStartStopClicked();
  void onEditClicked();
  void onMenuClicked();

  /* Output widget signals */
  void onOutputStartRequested(size_t outputIndex);
  void onOutputStopRequested(size_t outputIndex);
  void onOutputEditRequested(size_t outputIndex);

private:
  void setupUI();
  void updateHeader();
  void updateOutputs();
  void showContextMenu(const QPoint &pos);

  /* Helper functions */
  QString getAggregateStatus() const;
  QString getSummaryText() const;
  QColor getStatusColor() const;
  QString getStatusIcon() const;

  /* Channel data */
  stream_channel_t *m_channel;

  /* UI components */
  QVBoxLayout *m_mainLayout;

  /* Header */
  QWidget *m_headerWidget;
  QHBoxLayout *m_headerLayout;
  QLabel *m_statusIndicator;
  QLabel *m_nameLabel;
  QLabel *m_summaryLabel;
  QPushButton *m_startStopButton;
  QPushButton *m_editButton;
  QPushButton *m_menuButton;

  /* Content (outputs) */
  QWidget *m_contentWidget;
  QVBoxLayout *m_contentLayout;
  QList<OutputWidget *> m_outputWidgets;

  /* State */
  bool m_expanded;
  bool m_hovered;
};
