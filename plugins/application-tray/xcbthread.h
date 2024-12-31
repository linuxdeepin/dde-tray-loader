// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QThread>
#include <xcb/xcb.h>

namespace tray {
class XcbThread : public QThread {
    Q_OBJECT
public:
    XcbThread(xcb_connection_t *connection, QObject *parent = nullptr);
    ~XcbThread();

    void run() override;

private:
    xcb_connection_t *m_connection;
};
}
