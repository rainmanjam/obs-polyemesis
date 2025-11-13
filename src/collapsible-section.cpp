/*
 * OBS Polyemesis Plugin - Collapsible Section Widget Implementation
 */

#include "collapsible-section.h"

#include <QKeyEvent>
#include <QSettings>
#include <QApplication>

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
  : QWidget(parent)
  , m_expanded(true)
  , m_collapsedHeight(0)
  , m_expandedHeight(0)
  , m_statePersistent(false)
{
  setupUI();
  m_titleLabel->setText(title);
  setFocusPolicy(Qt::StrongFocus);
}

CollapsibleSection::~CollapsibleSection()
{
  if (m_statePersistent) {
    saveState();
  }
}

void CollapsibleSection::setupUI()
{
  /* Create main layout */
  m_mainLayout = new QVBoxLayout(this);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_mainLayout->setSpacing(0);

  /* Create header frame */
  m_headerFrame = new QFrame(this);
  m_headerFrame->setFrameShape(QFrame::StyledPanel);
  m_headerFrame->setFrameShadow(QFrame::Raised);

  /* Create header layout */
  m_headerLayout = new QHBoxLayout(m_headerFrame);
  m_headerLayout->setContentsMargins(8, 6, 8, 6);
  m_headerLayout->setSpacing(6);

  /* Create chevron button */
  m_chevronButton = new QPushButton(m_headerFrame);
  m_chevronButton->setFlat(true);
  m_chevronButton->setFixedSize(16, 16);
  m_chevronButton->setFocusPolicy(Qt::NoFocus);
  connect(m_chevronButton, &QPushButton::clicked, this, &CollapsibleSection::onChevronClicked);

  /* Create title label */
  m_titleLabel = new QLabel(m_headerFrame);
  QFont titleFont = m_titleLabel->font();
  titleFont.setBold(true);
  m_titleLabel->setFont(titleFont);

  /* Create header buttons layout (right-aligned) */
  m_headerButtonsLayout = new QHBoxLayout();
  m_headerButtonsLayout->setSpacing(4);

  /* Add to header layout */
  m_headerLayout->addWidget(m_chevronButton);
  m_headerLayout->addWidget(m_titleLabel);
  m_headerLayout->addStretch();
  m_headerLayout->addLayout(m_headerButtonsLayout);

  /* Add header to main layout */
  m_mainLayout->addWidget(m_headerFrame);

  /* Create content container */
  m_contentContainer = new QWidget(this);
  m_contentLayout = new QVBoxLayout(m_contentContainer);
  m_contentLayout->setContentsMargins(8, 4, 8, 4);
  m_contentLayout->setSpacing(4);

  /* Add content to main layout */
  m_mainLayout->addWidget(m_contentContainer);

  /* Create animation */
  m_contentAnimation = new QPropertyAnimation(m_contentContainer, "maximumHeight", this);
  m_contentAnimation->setDuration(200);
  m_contentAnimation->setEasingCurve(QEasingCurve::InOutQuad);

  m_animationGroup = new QParallelAnimationGroup(this);
  m_animationGroup->addAnimation(m_contentAnimation);

  /* Initial state */
  updateChevron();
}

void CollapsibleSection::setContent(QWidget *widget)
{
  /* Clear existing content */
  while (m_contentLayout->count() > 0) {
    QLayoutItem *item = m_contentLayout->takeAt(0);
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  /* Add new content */
  if (widget) {
    m_contentLayout->addWidget(widget);
    widget->setVisible(m_expanded);

    /* Update expanded height */
    m_expandedHeight = m_headerFrame->sizeHint().height() + widget->sizeHint().height() + 8;
  }
}

void CollapsibleSection::setExpanded(bool expanded, bool animate)
{
  if (m_expanded == expanded) {
    return;
  }

  m_expanded = expanded;

  /* Get content widget */
  QWidget *contentWidget = nullptr;
  if (m_contentLayout->count() > 0) {
    contentWidget = m_contentLayout->itemAt(0)->widget();
  }

  if (!contentWidget) {
    return;
  }

  /* Calculate heights */
  m_collapsedHeight = 0;
  m_expandedHeight = contentWidget->sizeHint().height();

  if (animate) {
    /* Animate expansion/collapse */
    m_contentAnimation->setStartValue(m_expanded ? m_collapsedHeight : m_expandedHeight);
    m_contentAnimation->setEndValue(m_expanded ? m_expandedHeight : m_collapsedHeight);

    /* Show content before expanding animation */
    if (m_expanded) {
      m_contentContainer->setMaximumHeight(m_collapsedHeight);
      contentWidget->setVisible(true);
    }

    /* Start animation */
    m_animationGroup->start();

    /* Hide content after collapsing animation */
    if (!m_expanded) {
      connect(m_animationGroup, &QParallelAnimationGroup::finished, this, [this, contentWidget]() {
        contentWidget->setVisible(false);
        disconnect(m_animationGroup, &QParallelAnimationGroup::finished, this, nullptr);
      });
    }
  } else {
    /* Immediate change */
    contentWidget->setVisible(m_expanded);
    m_contentContainer->setMaximumHeight(m_expanded ? QWIDGETSIZE_MAX : 0);
  }

  updateChevron();
  emit expandedChanged(m_expanded);

  if (m_statePersistent) {
    saveState();
  }
}

void CollapsibleSection::toggle()
{
  setExpanded(!m_expanded, true);
}

void CollapsibleSection::addHeaderButton(QPushButton *button)
{
  if (button) {
    m_headerButtonsLayout->addWidget(button);
  }
}

void CollapsibleSection::setStatePersistent(bool persistent, const QString &key)
{
  m_statePersistent = persistent;
  m_stateKey = key;

  if (m_statePersistent && !m_stateKey.isEmpty()) {
    restoreState();
  }
}

void CollapsibleSection::onChevronClicked()
{
  toggle();
}

void CollapsibleSection::updateChevron()
{
  /* Use Unicode triangle characters for chevron */
  m_chevronButton->setText(m_expanded ? "▼" : "▶");
}

void CollapsibleSection::saveState()
{
  if (!m_stateKey.isEmpty()) {
    QSettings settings;
    settings.setValue("CollapsibleSection/" + m_stateKey + "/expanded", m_expanded);
  }
}

void CollapsibleSection::restoreState()
{
  if (!m_stateKey.isEmpty()) {
    QSettings settings;
    bool expanded = settings.value("CollapsibleSection/" + m_stateKey + "/expanded", true).toBool();
    setExpanded(expanded, false);
  }
}

void CollapsibleSection::keyPressEvent(QKeyEvent *event)
{
  switch (event->key()) {
  case Qt::Key_Space:
  case Qt::Key_Return:
  case Qt::Key_Enter:
    toggle();
    event->accept();
    return;

  case Qt::Key_Right:
    if (!m_expanded) {
      setExpanded(true, true);
      event->accept();
      return;
    }
    break;

  case Qt::Key_Left:
    if (m_expanded) {
      setExpanded(false, true);
      event->accept();
      return;
    }
    break;
  }

  QWidget::keyPressEvent(event);
}

void CollapsibleSection::focusInEvent(QFocusEvent *event)
{
  /* Highlight header when focused (use QPalette) */
  QPalette palette = m_headerFrame->palette();
  palette.setColor(QPalette::Window, palette.color(QPalette::Highlight).lighter(150));
  m_headerFrame->setAutoFillBackground(true);
  m_headerFrame->setPalette(palette);

  QWidget::focusInEvent(event);
}
