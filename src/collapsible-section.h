/*
 * OBS Polyemesis Plugin - Collapsible Section Widget
 * A custom Qt widget for creating expandable/collapsible UI sections
 */

#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

/*
 * CollapsibleSection - A widget with a header and expandable content
 *
 * Features:
 * - Header with title label
 * - Expand/collapse chevron button
 * - Optional action buttons in header (right-aligned)
 * - Smooth animated expand/collapse (200ms)
 * - Content container that shows/hides
 * - Keyboard navigation support
 * - Adapts to OBS theme via QPalette
 */
class CollapsibleSection : public QWidget {
  Q_OBJECT

public:
  explicit CollapsibleSection(const QString &title, QWidget *parent = nullptr);
  ~CollapsibleSection() override;

  /* Set the content widget that will be shown/hidden */
  void setContent(QWidget *widget);

  /* Get/set expanded state */
  bool isExpanded() const { return m_expanded; }
  void setExpanded(bool expanded, bool animate = true);

  /* Toggle expanded state */
  void toggle();

  /* Add an action button to the header (right-aligned) */
  void addHeaderButton(QPushButton *button);

  /* Update section title (useful for adding status indicators) */
  void setTitle(const QString &title);

  /* Enable/disable state persistence (save/restore collapsed state) */
  void setStatePersistent(bool persistent, const QString &key = QString());

signals:
  /* Emitted when expanded state changes */
  void expandedChanged(bool expanded);

protected:
  /* Handle keyboard events */
  void keyPressEvent(QKeyEvent *event) override;
  void focusInEvent(QFocusEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private slots:
  void onChevronClicked();

private:
  void setupUI();
  void updateChevron();
  void saveState();
  void restoreState();

  /* UI components */
  QFrame *m_headerFrame;
  QLabel *m_titleLabel;
  QPushButton *m_chevronButton;
  QHBoxLayout *m_headerLayout;
  QHBoxLayout *m_headerButtonsLayout;
  QWidget *m_contentContainer;
  QVBoxLayout *m_contentLayout;
  QVBoxLayout *m_mainLayout;

  /* Animation */
  QPropertyAnimation *m_contentAnimation;
  QParallelAnimationGroup *m_animationGroup;

  /* State */
  bool m_expanded;
  int m_collapsedHeight;
  int m_expandedHeight;

  /* State persistence */
  bool m_statePersistent;
  QString m_stateKey;
};
