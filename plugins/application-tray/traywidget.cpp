// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "traywidget.h"
#include "abstracttrayprotocol.h"

#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <dapplication.h>
#include <QTimer>

namespace tray {
uint16_t trayIconSize = 16;
TrayWidget::TrayWidget(QPointer<AbstractTrayProtocolHandler> handler)
    : QWidget()
    , m_handler(handler)
    , m_attentionTimer(new QTimer(this))
{
    m_attentionTimer->setSingleShot(true);

    // TODO: read from config get attention time
    m_attentionTimer->setInterval(3000);

    setWindowTitle(m_handler->id());
    setFixedSize(trayIconSize, trayIconSize);

    m_handler->setParent(this);
    installEventFilter(m_handler);
    setMouseTracking(true);

    connect(m_handler, &AbstractTrayProtocolHandler::iconChanged, this, [this](){update();});
    connect(m_handler, &AbstractTrayProtocolHandler::overlayIconChanged, this, [this](){update();});
    connect(m_handler, &AbstractTrayProtocolHandler::attentionIconChanged, this, [this](){
        m_attentionTimer->start();
        update();
    });
}

TrayWidget::~TrayWidget()
{
}

void TrayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    // TODO: support attentionIcon/overlayIcon
    QPainter painter(this);
    auto pixmap = m_attentionTimer->isActive() ? 
        m_handler->attentionIcon().pixmap(trayIconSize, trayIconSize) : 
        m_handler->icon().pixmap(trayIconSize, trayIconSize);
    painter.drawPixmap(0, 0, pixmap);
}
}