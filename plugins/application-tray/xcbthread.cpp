// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbthread.h"
#include "util.h"
#include <QDebug>

namespace tray {
XcbThread::XcbThread(xcb_connection_t *connection, QObject *parent)
    : QThread(parent), m_connection(connection)
{
}

XcbThread::~XcbThread()
{
}

void XcbThread::run()
{
    if (!m_connection) {
        return;
    }
    while (!isInterruptionRequested()) {
        xcb_generic_event_t *event = xcb_wait_for_event(m_connection);
        if (event) {
            uint8_t responseType = event->response_type & ~0x80;
            switch (responseType) {
            case XCB_LEAVE_NOTIFY: {
                xcb_leave_notify_event_t *lE = (xcb_leave_notify_event_t *)event;
                UTIL->setX11WindowInputShape(lE->event, QSize(0, 0));
                break;
            }
            }
        free(event);
        }
    }
}
}

