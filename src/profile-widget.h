/*
 * OBS Polyemesis Plugin - Profile Widget
 * Individual profile display with expandable destinations
 */

#pragma once

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "restreamer-output-profile.h"

/* Forward declarations */
class DestinationWidget;

/*
 * ProfileWidget - Displays a single streaming profile with destinations
 *
 * Features:
 * - Profile header with status indicator
 * - Aggregate status (all active, some active, errors)
 * - Expandable to show destination list
 * - Start/stop/edit actions
 * - Right-click context menu
 * - Hover actions
 */
class ProfileWidget : public QWidget {
  Q_OBJECT

public:
  explicit ProfileWidget(output_profile_t *profile, QWidget *parent = nullptr);
  ~ProfileWidget() override;

  /* Get/set expanded state */
  bool isExpanded() const { return m_expanded; }
  void setExpanded(bool expanded);

  /* Update widget from profile data */
  void updateFromProfile();

  /* Get profile ID */
  const char *getProfileId() const;

signals:
  /* Emitted when user requests actions */
  void startRequested(const char *profileId);
  void stopRequested(const char *profileId);
  void editRequested(const char *profileId);
  void deleteRequested(const char *profileId);
  void duplicateRequested(const char *profileId);

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

  /* Destination widget signals */
  void onDestinationStartRequested(size_t destIndex);
  void onDestinationStopRequested(size_t destIndex);
  void onDestinationEditRequested(size_t destIndex);

private:
  void setupUI();
  void updateHeader();
  void updateDestinations();
  void showContextMenu(const QPoint &pos);

  /* Helper functions */
  QString getAggregateStatus() const;
  QString getSummaryText() const;
  QColor getStatusColor() const;
  QString getStatusIcon() const;

  /* Profile data */
  output_profile_t *m_profile;

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

  /* Content (destinations) */
  QWidget *m_contentWidget;
  QVBoxLayout *m_contentLayout;
  QList<DestinationWidget *> m_destinationWidgets;

  /* State */
  bool m_expanded;
  bool m_hovered;
};
