// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QImage>
#include <QSharedPointer>
#include <QSet>
#include <QObject>

#include <cstdint>
#include <sys/types.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_image.h>

struct _XDisplay;

namespace tray {
#define UTIL Util::instance()
class Util : public QObject
{

public:
    static Util* instance();

    bool isXAvaliable();
    xcb_connection_t* getX11Connection();
    xcb_window_t getRootWindow();
    _XDisplay* getDisplay();

    xcb_atom_t getAtomByName(const QString& name);
    QString getNameByAtom(const xcb_atom_t& atom);
    xcb_atom_t getAtomFromDisplay(const char * name);

    void moveX11Window(const xcb_window_t& window, const uint32_t& x, const uint32_t& y);
    void setX11WindowSize(const xcb_window_t& window, const QSize& size);
    [[nodiscard]] QRect getX11WindowGeometry(const xcb_window_t& window) const;
    QString getX11WindowName(const xcb_window_t& window);
    void setX11WindowInputShape(const xcb_window_t& widnow, const QSize& size);
    QImage getX11WindowImageNonComposite(const xcb_window_t& window);
    void setX11WindowOpacity(const xcb_window_t& window, const double& opacity);
    pid_t getWindowPid(const xcb_window_t& window);
    QString getProcExe(const pid_t& pid);

    void sendXembedMessage(const xcb_window_t& window, const long& message, const long& data1, const long& data2, const long& data3);

    QString generateUniqueId(const QString &id);
    void removeUniqueId(const QString &id);

    QPoint getMousePos() const;

private:
    Util();
    ~Util();
    Util(const Util&) = delete;
    Util& operator=(const Util&) = delete;

    enum class DispatchEventsMode {
        Poll,
        EventQueue
    };
    void dispatchEvents(DispatchEventsMode mode);

    bool isTransparentImage(const QImage &image);

    QImage convertFromNative(xcb_image_t* image);

private:
    xcb_ewmh_connection_t m_ewmh;
    QHash<QString, xcb_atom_t> m_atoms;

    xcb_connection_t* m_x11connection;
    xcb_window_t m_rootWindow;
    _XDisplay *m_display;

    QSet<QString> m_currentIds;
};

}
