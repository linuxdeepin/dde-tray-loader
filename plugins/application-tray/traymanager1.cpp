// Deepin DDE TrayManager1 implementation
//
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "traymanager1.h"
#include "traymanager1adaptor.h"

#include "util.h"

#include <KWindowInfo>
#include <QGuiApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(TRAYMGR, "org.deepin.dde.trayloader.traymgr")

TrayManager1::TrayManager1(QObject *parent)
    : QObject(parent)
    , m_adaptor(new TrayManager1Adaptor(this))
{
    qCDebug(TRAYMGR) << "TrayManager1 created";
}

TrayManager1::~TrayManager1()
{
    qCDebug(TRAYMGR) << "TrayManager1 destroyed";
}

void TrayManager1::registerIcon(xcb_window_t win)
{
    if (m_icons.contains(win)) {
        qCWarning(TRAYMGR) << "Icon already registered:" << win;
        return;
    }

    m_icons[win] = true;
    qCDebug(TRAYMGR) << "Icon registered:" << win ;//<< "name:" << proxy->name();

    Q_EMIT Added(static_cast<uint32_t>(win));
}

void TrayManager1::unregisterIcon(xcb_window_t win)
{
    if (!m_icons.contains(win)) {
        qCWarning(TRAYMGR) << "Icon not found for removal:" << win;
        return;
    }

    m_icons.remove(win);
    qCDebug(TRAYMGR) << "Icon unregistered:" << win;

    Q_EMIT Removed(static_cast<uint32_t>(win));
}

void TrayManager1::notifyIconChanged(xcb_window_t win)
{
    if (!m_icons.contains(win)) {
        return;
    }

    if (!m_icons[win]) {
        qCDebug(TRAYMGR) << "EnableNotification is false, not sending changed signal for:" << win;
        return;
    }

    qCDebug(TRAYMGR) << "Icon changed:" << win;
    Q_EMIT Changed(static_cast<uint32_t>(win));
}

TrayList TrayManager1::trayIcons() const
{
    qDebug() << "trayIcons:" << m_icons.keys();
    TrayList result;
    for (xcb_window_t win : m_icons.keys()) {
        result << static_cast<uint>(win);
    }
    return result;
}

bool TrayManager1::haveIcon(xcb_window_t win) const
{
    return m_icons.contains(win);
}

// DBus method implementations
bool TrayManager1::Manage()
{
    qCDebug(TRAYMGR) << "Manage() called via DBus";
    emit reclainRequested();
    return true;
}

QString TrayManager1::GetName(uint32_t win)
{
    using Util = tray::Util;
    return UTIL->getX11WindowName(win);
}

void TrayManager1::EnableNotification(uint32_t win, bool enabled)
{
    if (!m_icons.contains(win)) {
        return;
    }

    m_icons[win] = enabled;

    qCDebug(TRAYMGR) << "EnableNotification for" << win << "=" << enabled;
}
