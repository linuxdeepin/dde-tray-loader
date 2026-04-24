// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plugin.h"
#include "pluginsurface_p.h"
#include "pluginmanagerintegration_p.h"

#include "qwayland-plugin-manager-v1.h"

#include <QTimer>
#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace Plugin {

class NullHandleGuard : public QObject {
public:
    static void install() {
        static bool installed = false;
        if (!installed && qApp) {
            qApp->installEventFilter(new NullHandleGuard(qApp));
            installed = true;
        }
    }

    NullHandleGuard(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::UpdateRequest) {
            QWindow *win = nullptr;
            QWidget *w = qobject_cast<QWidget *>(watched);
            QWindow *wnd = qobject_cast<QWindow *>(watched);
            
            if (w) {
                win = w->windowHandle();
                if (!win && w->window()) {
                    win = w->window()->windowHandle();
                }
            } else if (wnd) {
                win = wnd;
            }

            // Drop UpdateRequest to prevent QWaylandShmBackingStore from crashing
            // in beginPaint/decoration() when trying to paint a destroyed window.
            if (win) {
                // Case 1: The QWindow exists, but its underlying Wayland surface is destroyed.
                if (!win->handle()) {
                    qDebug() << "NullHandleGuard Dropped UpdateRequest for" << watched << "(null Wayland handle)";
                    return true;
                }
            } else if (w) {
                // Case 2: The QWidget has lost its QWindow entirely.
                // A widget without a QWindow cannot be painted to the screen.
                qDebug() << "NullHandleGuard Dropped UpdateRequest for" << watched << "(missing QWindow)";
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }
};
PluginSurface::PluginSurface(PluginManagerIntegration *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::plugin()
    , m_plugin(EmbedPlugin::get(window->window()))
    , m_window(window->window())
{
    init(manager->create_plugin(m_plugin->pluginId(), m_plugin->itemKey(), m_plugin->displayName(), m_plugin->pluginFlags(), m_plugin->pluginType(), m_plugin->pluginSizePolicy(), window->wlSurface()));
    dcc_icon(m_plugin->dccIcon());

    source_size(m_window->width(), m_window->height());
    connect(m_window, &QWindow::widthChanged, this, [this] (int width) {
        source_size(width, m_window->height());
    });

    connect(m_window, &QWindow::heightChanged, this, [this] (int height) {
        source_size(m_window->width(), height);
    });

    connect(manager, &PluginManagerIntegration::dockPositionChanged, m_plugin, &EmbedPlugin::dockPositionChanged);
    connect(manager, &PluginManagerIntegration::dockColorThemeChanged, m_plugin, &EmbedPlugin::dockColorThemeChanged);
    connect(manager, &PluginManagerIntegration::eventMessage, m_plugin, &EmbedPlugin::eventMessage);
    connect(manager, &PluginManagerIntegration::xembedWindowMoved, m_plugin, &EmbedPlugin::xembedWindowMoved);

    connect(m_plugin, &EmbedPlugin::requestMoveXembedWindow, this, [manager, this](uint32_t xembedWinId) {
        manager->moveXembedWindow(xembedWinId, m_plugin->pluginId(), m_plugin->itemKey());
    });

    connect(m_plugin, &EmbedPlugin::requestMessage, manager, [manager, this](const QString &msg) {
        manager->requestMessage(m_plugin->pluginId(), m_plugin->itemKey(), msg);
    });

    connect(m_plugin, &EmbedPlugin::pluginRecvMouseEvent, this, [this] (int type){
        mouse_event(type);
    });

    connect(m_plugin, &EmbedPlugin::pluginRequestShutdown, this, [this](const QString &type){
        request_shutdown(type);
    });

    connect(m_plugin, &EmbedPlugin::closeQuickPanel, this, [this] {
        qDebug() << "send close popup signal";
        close_quick_panel();
    });
}

PluginSurface::~PluginSurface()
{
    destroy();
}

void PluginSurface::plugin_close()
{
    // it would be delete this object directly.
    m_window->close();
}

void PluginSurface::plugin_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    QRect rect(x, y, width, height);
    if (width <= 0)
        rect.setWidth(m_window->width());
    if (height <= 0)
        rect.setHeight(m_window->height());

    m_plugin->setPluginPos(QPoint(x, y));
    Q_EMIT m_plugin->eventGeometry(rect);
}

void PluginSurface::plugin_margin(int32_t spacing)
{
    if (spacing > 0) {
        Q_EMIT m_plugin->contentMarginChanged(spacing);
    }
}

void PluginSurface::plugin_raw_global_pos(int32_t x, int32_t y)
{
    m_plugin->setRawGlobalPos(QPoint(x, y));
}

PluginPopupSurface::PluginPopupSurface(PluginManagerIntegration *manager, QtWaylandClient::QWaylandWindow *window)
    : QtWaylandClient::QWaylandShellSurface(window)
    , QtWayland::plugin_popup()
    , m_popup(PluginPopup::get(window->window()))
    , m_window(window->window())
    , m_dirtyTimer(new QTimer(this))
{
    init(manager->create_popup_at(m_popup->pluginId(), m_popup->itemKey(), m_popup->popupType(), m_popup->x(), m_popup->y(), window->wlSurface()));

    // merge multi signal of position changed to one.
    m_dirtyTimer->setInterval(0);
    m_dirtyTimer->setSingleShot(true);

    source_size(m_window->width(), m_window->height());
    connect(m_window, &QWindow::widthChanged, this, [this] (int width) {
        source_size(width, m_window->height());
    });

    connect(m_window, &QWindow::heightChanged, this, [this] (int height) {
        source_size(m_window->width(), height);
    });

    connect(m_popup, &PluginPopup::xChanged, this, &PluginPopupSurface::dirtyPosition);
    connect(m_popup, &PluginPopup::yChanged, this, &PluginPopupSurface::dirtyPosition);
    connect(m_dirtyTimer, &QTimer::timeout, this, [this]{
        set_position(m_popup->x(), m_popup->y());
    });

    connect(m_popup, &PluginPopup::requestSetCursor, this, [this](int cursorShape) {
        set_cursor(cursorShape);
    });
}

PluginPopupSurface::~PluginPopupSurface()
{
    destroy();
}

void PluginPopupSurface::plugin_popup_close()
{
    // Install the global safeguard just in case
    NullHandleGuard::install();

    // Use QPointer to ensure m_window is still valid when the queued lambda executes
    QPointer<QWindow> safeWindow(m_window);

    // DEFER the destruction!
    // Why: QWaylandShmBackingStore::beginPaint() can spin the Wayland event loop
    // (e.g., waiting for buffers) which synchronously dispatches this Wayland event.
    // If we destroy m_window here, beginPaint() resumes with a dangling pointer
    // and crashes immediately. Deferring to the next event loop iteration ensures
    // that the current paint frame completes before we destroy the Wayland surface.
    QMetaObject::invokeMethod(qApp, [safeWindow]() {
        if (!safeWindow) {
            return;
        }

        QWidget *popupWidget = nullptr;
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (w && w->windowHandle() == safeWindow.data()) {
                popupWidget = w;
                break;
            }
        }

        if (popupWidget) {
            popupWidget->hide();
        }

        // Safely close the QWindow. This destroys the Wayland surface.
        safeWindow->close();
    }, Qt::QueuedConnection);
}

void PluginPopupSurface::plugin_popup_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto plugin = PluginPopup::get(m_window);
    auto rect = QRect(x, y, width, height);
    if (rect.width() <= 0)
        rect.setWidth(m_window->width());

    if (rect.height() <= 0)
        rect.setHeight(m_window->height());

    m_popup->setPluginPos(QPoint(x, y));
    if (plugin) {
        Q_EMIT plugin->eventGeometry(rect);
    }
}

void PluginPopupSurface::dirtyPosition()
{
    m_dirtyTimer->start();
}

}
