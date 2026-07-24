// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "singlequickpanel.h"

#include <QVBoxLayout>
#include <QMargins>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DWindowManagerHelper>
#include <DToolTip>

#define RADIUS 8

SignalQuickPanel::SignalQuickPanel(QWidget *parent)
    :QWidget(parent)
    , m_icon(new CommonIconButton(this))
    , m_description(new DLabel(this))
    , m_active(false)
    , m_iconEffect(new QGraphicsOpacityEffect(m_icon))
    , m_descEffect(new QGraphicsOpacityEffect(m_description))
{
    initUI();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SignalQuickPanel::refreshBg);
}

SignalQuickPanel::~SignalQuickPanel()
{

}

void SignalQuickPanel::initUI()
{
    m_icon->setFixedSize(QSize(24, 24));

    m_iconEffect->setOpacity(kNormalOpacity);
    m_icon->setGraphicsEffect(m_iconEffect);

    m_description->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_description, DToolTip::ShowWhenElided);
    DFontSizeManager::instance()->bind(m_description, DFontSizeManager::T10);

    m_descEffect->setOpacity(kNormalOpacity);
    m_description->setGraphicsEffect(m_descEffect);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_icon, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(m_description, 0, Qt::AlignCenter);
    layout->addStretch(1);

    setLayout(layout);
}

void SignalQuickPanel::setIcon(const QIcon &icon)
{
    m_icon->setIcon(icon, Qt::black, Qt::white);
}

void SignalQuickPanel::setDescription(const QString &description)
{
    m_description->setText(description);
}

void SignalQuickPanel::setWidgetState(WidgetState state)
{
    if (m_icon)
        m_icon->setActiveState(WS_ACTIVE == state);

    m_active = (WS_ACTIVE == state);

    refreshBg();
    updateOpacity(m_hovered);
}

void SignalQuickPanel::updateOpacity(bool hover)
{
    // icon: when active, always 1.0; otherwise follow hover
    m_iconEffect->setOpacity((m_active || hover) ? kHoverOpacity : kNormalOpacity);
    // text: always follow hover regardless of active state
    m_descEffect->setOpacity(hover ? kHoverOpacity : kNormalOpacity);
}

void SignalQuickPanel::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    updateOpacity(true);
    QWidget::enterEvent(event);
}

void SignalQuickPanel::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    updateOpacity(false);
}

void SignalQuickPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (underMouse()) {
        Q_EMIT clicked();
    }
    return QWidget::mouseReleaseEvent(event);
}

void SignalQuickPanel::refreshBg()
{
    m_description->setForegroundRole(m_icon->activeState() ? QPalette::Highlight : QPalette::NoRole);
    update();
}
