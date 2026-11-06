// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util.h"

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QEventLoop>
#include <QProcess>
#include <QTimer>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

static void require(bool success, const char *message)
{
    if (!success)
        throw std::runtime_error(message);
}

static void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

class EventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *) override
    {
        const auto *event = static_cast<xcb_generic_event_t *>(message);
        if (type == "xcb_generic_event_t" && (event->response_type & 0x7f) == XCB_CLIENT_MESSAGE)
            ++messages;
        return false;
    }

    int messages = 0;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    try {
        // Own the server so disconnecting it cannot affect the user's desktop.
        QProcess server;
        server.start(app.arguments().value(1),
                     {"-displayfd", "1", "-screen", "0", "640x480x24", "-nolisten", "tcp"});
        require(server.waitForStarted(), "Could not start Xvfb");
        require(server.waitForReadyRead(5000), "Xvfb did not publish its display number");
        const QByteArray display = server.readLine().trimmed();
        bool validDisplay = false;
        display.toInt(&validDisplay);
        require(validDisplay, "Invalid Xvfb display number");
        qputenv("DISPLAY", ":" + display);
        qunsetenv("XAUTHORITY");

        auto *util = tray::Util::instance();
        require(util->isXAvaliable(), "X11 should be available before disconnect");
        auto *connection = util->getX11Connection();

        EventFilter filter;
        app.installNativeEventFilter(&filter);
        const xcb_window_t window = xcb_generate_id(connection);
        xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, util->getRootWindow(),
                          0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, 0, nullptr);
        xcb_client_message_event_t message{};
        message.response_type = XCB_CLIENT_MESSAGE;
        message.format = 32;
        message.window = window;
        xcb_send_event(connection, false, window, XCB_EVENT_MASK_NO_EVENT,
                       reinterpret_cast<const char *>(&message));
        xcb_flush(connection);
        processFor(100);
        require(filter.messages == 1, "Healthy X11 events must still be dispatched");
        app.removeNativeEventFilter(&filter);

        server.terminate();
        require(server.waitForFinished(5000), "Xvfb did not terminate");
        if (app.arguments().contains("--error-before-dispatch")) {
            // Also cover errors discovered by another XCB caller before the
            // socket notifier or aboutToBlock/awake callbacks run.
            free(xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), nullptr));
            require(xcb_connection_has_error(connection), "Expected an XCB connection error");
        }

        processFor(100);
        require(xcb_connection_has_error(connection), "The disconnected connection must report an error");
        const bool availableAfterDisconnect = util->isXAvaliable();

        int wakeups = 0;
        const auto wakeupConnection = QObject::connect(QAbstractEventDispatcher::instance(),
            &QAbstractEventDispatcher::awake, &app, [&wakeups]() { ++wakeups; });
        processFor(250);
        QObject::disconnect(wakeupConnection);
        std::printf("Event-loop wakeups after X11 disconnect: %d\n", wakeups);
        require(wakeups < 20, "The event loop is spinning on the disconnected X11 socket");
        require(!availableAfterDisconnect, "X11 availability must not remain cached after disconnect");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
