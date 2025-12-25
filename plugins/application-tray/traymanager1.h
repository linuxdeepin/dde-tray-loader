// Deepin DDE TrayManager1 implementation
//
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QDBusContext>

#include <xcb/xcb.h>

class TrayManagerProxy;

typedef QList<uint> TrayList;

class TrayManager1Adaptor;
/**
 * @brief TrayManager1 implements the org.deepin.dde.TrayManager1 DBus interface
 *
 * This class manages all embedded X11 tray icons and exposes them via DBus.
 * It maintains a list of TrayManagerProxy objects and emits signals when
 * icons are added, removed, or changed.
 */
class TrayManager1 : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.TrayManager1")
    Q_PROPERTY(TrayList TrayIcons READ trayIcons)

public:
    explicit TrayManager1(QObject *parent = nullptr);
    ~TrayManager1() override;

    /**
     * @brief Register a new tray icon with the manager
     * @param win Window ID of the embedded tray icon
     * @param proxy Pointer to the TrayManagerProxy managing this icon
     */
    void registerIcon(xcb_window_t win);

    /**
     * @brief Unregister a tray icon
     * @param win Window ID of the icon
     */
    void unregisterIcon(xcb_window_t win);

    /**
     * @brief Notify that an icon has changed
     * @param win Window ID of the icon
     */
    void notifyIconChanged(xcb_window_t win);

    /**
     * @return List of all registered tray icon window IDs
     */
    TrayList trayIcons() const;

    bool haveIcon(xcb_window_t win) const;

public Q_SLOTS:
    // DBus methods
    bool Manage();
    QString GetName(uint32_t win);
    void EnableNotification(uint32_t win, bool enabled);

Q_SIGNALS:
    // DBus signals
    void Added(uint32_t id);
    void Removed(uint32_t id);
    void Changed(uint32_t id);
    void Inited();

    void reclainRequested();

private:
    TrayManager1Adaptor * m_adaptor;
    QHash<xcb_window_t, bool> m_icons; // <winid, enableNotify>
};
