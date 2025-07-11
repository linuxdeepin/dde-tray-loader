// SPDX-FileCopyrightText: 2019 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "pagebutton.h"

#include <QMouseEvent>
#include <QPainter>
#include <QHBoxLayout>

#include <DPaletteHelper>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

PageButton::PageButton(QWidget *parent)
    : QFrame(parent)
    , m_hover(false)
    , m_mousePress(false)
    , m_iconButton(new CommonIconButton(this))
{
    initUI();
}

PageButton::~PageButton()
{
}

void PageButton::initUI()
{
    setFixedSize(30, 30);
    setForegroundRole(QPalette::BrightText);
    m_iconButton->setFixedSize(12, 12);
    m_iconButton->setForegroundRole(QPalette::BrightText);

    // 居中放置图标
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->addWidget(m_iconButton, 0, Qt::AlignCenter);
}

void PageButton::setIcon(const QIcon &icon)
{
    m_iconButton->setIcon(icon, Qt::black, Qt::white);
}

bool PageButton::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::Leave:
    case QEvent::Enter:
        m_hover = e->type() == QEvent::Enter;
        update();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void PageButton::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    QPainter p(this);
    QPalette palette = this->palette();
    QColor bgColor, textColor;
    if (m_mousePress) {
        textColor = palette.highlight().color();
        bgColor = palette.windowText().color();
        bgColor.setAlphaF(0.15);
    } else if(m_hover) {
        textColor = palette.windowText().color();
        bgColor = palette.windowText().color();
        bgColor.setAlphaF(0.1);
    } else {
        textColor = palette.windowText().color();
        bgColor = DPaletteHelper::instance()->palette(this).color(DPalette::ItemBackground);
        bgColor.setAlphaF(0);
    }
    palette.setBrush(QPalette::BrightText, textColor);
    m_iconButton->setPalette(palette);

    p.setBrush(bgColor);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 8, 8);
    return QFrame::paintEvent(e);
}

void PageButton::mousePressEvent(QMouseEvent *event)
{
    if(m_mousePress != true) {
        m_mousePress = true;
        update();
    }
    QWidget::mousePressEvent(event);
}

void PageButton::mouseReleaseEvent(QMouseEvent* event)
{
    if(m_mousePress == true) {
        m_mousePress = false;
        update();
    }

    if (underMouse() && this->rect().contains(event->pos())) {
        Q_EMIT clicked();
    }

    QWidget::mouseReleaseEvent(event);
} 