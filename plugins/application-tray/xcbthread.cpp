// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xcbthread.h"
#include "util.h"

namespace tray {
XcbThread::XcbThread(xcb_connection_t *connection, QObject *parent)
    : QThread(parent)
    , m_connection(connection)
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
    // The Xembed type tray needs to reset the xwindow state of the receiving event to the state of not receiving events after the mouse
    // leaves. This thread is used to receive the leave event and apply the operation.
    QScopedPointer<xcb_generic_event_t> event;
    while (!isInterruptionRequested()) {
        event.reset(xcb_wait_for_event(m_connection));
        if (event) {
            uint8_t responseType = event->response_type & ~0x80;
            switch (responseType) {
            case XCB_LEAVE_NOTIFY: {
                xcb_leave_notify_event_t *lE = (xcb_leave_notify_event_t *)event.data();
                UTIL->setX11WindowInputShape(lE->event, QSize(0, 0));
                break;
            }
            }
        }
    }
}
}

