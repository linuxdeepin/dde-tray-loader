// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "abstracttrayprotocol.h"
#include "traymanager1interface.h"
#include "xembedprotocolhandler.h"
#include "fdoselectionmanager.h"

#include "util.h"
#include "plugin.h"

#include <QBitmap>
#include <QWindow>
#include <QMouseEvent>
#include <QGuiApplication>
#include <DWindowManagerHelper>

#include <xcb/xcb.h>
#include <xcb/xtest.h>
#include <xcb/xcb_event.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

DGUI_USE_NAMESPACE
#define WM_HELPER DWindowManagerHelper::instance()

namespace tray {
extern uint16_t trayIconSize;
static QString xembedPfrefix = QStringLiteral("XEMBED:");

XembedProtocol::XembedProtocol(QObject *parent)
    : AbstractTrayProtocol(parent)
    , m_trayManager(new TrayManager("org.deepin.dde.TrayManager1", "/org/deepin/dde/TrayManager1", QDBusConnection::sessionBus(), this))
{
    qApp->installNativeEventFilter(this);

    connect(m_trayManager, &TrayManager::Added, this, &XembedProtocol::onTrayIconsChanged);
    connect(m_trayManager, &TrayManager::Removed, this, &XembedProtocol::onTrayIconsChanged);

    QMetaObject::invokeMethod(this, &XembedProtocol::onTrayIconsChanged, Qt::QueuedConnection);

    if ((qgetenv("XDG_SESSION_TYPE") == "wayland") && UTIL->isXAvaliable()) {
        qWarning() << "Not running on X11, registering FDO selection manager";
        m_selectionManager = new FdoSelectionManager(this);
    }

    QTimer::singleShot(0, this, [this](){
        m_trayManager->Manage();
    });
}

XembedProtocol::~XembedProtocol()
{
    m_registedItem.clear();
}

bool XembedProtocol::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)

    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    auto *ev = static_cast<xcb_generic_event_t *>(message);

    const auto responseType = XCB_EVENT_RESPONSE_TYPE(ev);
    if (responseType == XCB_LEAVE_NOTIFY) {
        xcb_leave_notify_event_t *lE = (xcb_leave_notify_event_t *)ev;
        UTIL->setX11WindowInputShape(lE->event, QSize(0, 0));
        return true;
    }

    return false;
}

void XembedProtocol::onTrayIconsChanged()
{
    QTimer::singleShot(200, this,[this](){
        auto currentRegistedItems = m_trayManager->trayIcons();

        if (currentRegistedItems == m_registedItem.keys()) {
            return;
        }

        for (auto currentRegistedItem : currentRegistedItems) {
            if (!m_registedItem.contains(currentRegistedItem)) {
                auto trayHandler = QSharedPointer<XembedProtocolHandler>(new XembedProtocolHandler(currentRegistedItem));
                connect(m_trayManager, &TrayManager::Changed, trayHandler.get(), &XembedProtocolHandler::xembedTrayIconChanged);
                m_registedItem.insert(currentRegistedItem, trayHandler);
                Q_EMIT AbstractTrayProtocol::trayCreated(trayHandler.get());
            }
        }

        for (auto alreadyRegistedItem : m_registedItem.keys()) {
            if (!currentRegistedItems.contains(alreadyRegistedItem)) {
                if (auto value = m_registedItem.value(alreadyRegistedItem, nullptr)) {
                    m_registedItem.remove(alreadyRegistedItem);
                }
            }
        }
    });

}

XembedProtocolHandler::XembedProtocolHandler(const uint32_t& id, QObject* parent)
    : AbstractTrayProtocolHandler(parent)
    , m_enabled(false)
    , m_windowId(id)
    , m_hoverTimer(new QTimer(this))
    , m_attentionTimer(new QTimer(this))
    , m_iconUpdateTimer(new QTimer(this))
{
    generateId();

    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(500);

    m_attentionTimer->setSingleShot(true);
    m_attentionTimer->setInterval(100);

    m_iconUpdateTimer->setSingleShot(true);
    m_iconUpdateTimer->setInterval(200);

    connect(m_hoverTimer, &QTimer::timeout, this, &XembedProtocolHandler::sendHover);
    connect(m_attentionTimer, &QTimer::timeout, this, [this](){
        m_attentionIcon = getPixmapFromWidnow();
        Q_EMIT attentionIconChanged();
        m_iconUpdateTimer->start();
    });

    connect(m_iconUpdateTimer, &QTimer::timeout, this, [this](){
        m_icon = getPixmapFromWidnow();
        Q_EMIT AbstractTrayProtocolHandler::iconChanged();
    });

    QMetaObject::invokeMethod(this, &XembedProtocolHandler::initX11resources, Qt::QueuedConnection);
}

XembedProtocolHandler::~XembedProtocolHandler()
{
    UTIL->removeUniqueId(m_id);
}

void XembedProtocolHandler::generateId()
{
    auto id = xembedPfrefix + UTIL->getProcExe(UTIL->getWindowPid(m_windowId));
    m_id = UTIL->generateUniqueId(id);
}

uint32_t XembedProtocolHandler::windowId() const
{
    return m_windowId;
}

QString XembedProtocolHandler::id() const
{
    return m_id;
}

QString XembedProtocolHandler::title() const
{
    return UTIL->getX11WindowName(m_windowId);
}

QString XembedProtocolHandler::status() const
{
    return "";
}

QString XembedProtocolHandler::category() const
{
    return "Application";
}

QIcon XembedProtocolHandler::overlayIcon() const
{
    return QIcon();
}

QIcon XembedProtocolHandler::attentionIcon() const
{
    return m_attentionIcon;
}

QIcon XembedProtocolHandler::icon() const
{
    return m_icon;
}

bool XembedProtocolHandler::enabled() const
{
    return m_enabled;
}

void XembedProtocolHandler::xembedTrayIconChanged(uint32_t windowId)
{
    if (windowId != m_windowId) return;
    m_attentionTimer->start();
}


bool XembedProtocolHandler::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == window()) {
        // 有透明通道时，可以做到container一直透明隐藏，就走Enter触发
        // 没有透明通道时，走旧dock的方式 QEvent::Move防止在 dock/container 之前一直切换
        if ((event->type() == QEvent::MouseMove)) {
            m_hoverTimer->start();
        } else if (event->type() == QEvent::Leave && m_hoverTimer->isActive()) {
            m_hoverTimer->stop();
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_hoverTimer->isActive()) {
                m_hoverTimer->stop();
            }

            auto p = UTIL->getMousePos();
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            sendClick(mouseEvent->button());
        }
    }
    return false;
}

void XembedProtocolHandler::initX11resources()
{
    auto c = Util::instance()->getX11Connection();
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
    m_containerWid = xcb_generate_id(c);
    uint32_t values[3];
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    values[0] = screen->black_pixel; // draw a solid background so the embedded icon doesn't get garbage in it
    values[1] = true; // bypass wM
    values[2] = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_ENTER_WINDOW;
    xcb_create_window(c,
                      XCB_COPY_FROM_PARENT,
                      m_containerWid,
                      screen->root,
                      0,
                      0,
                      1, 1,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      mask,
                      values);

    UTIL->setX11WindowInputShape(m_containerWid, QSize());
    UTIL->setX11WindowOpacity(m_containerWid, 0);

    xcb_map_window(c, m_containerWid);

    xcb_reparent_window(c, m_windowId, m_containerWid, 0, 0);
    xcb_composite_redirect_window(c, m_windowId, XCB_COMPOSITE_REDIRECT_MANUAL);
    xcb_change_save_set(c, XCB_SET_MODE_INSERT, m_windowId);
    UTIL->sendXembedMessage(m_windowId, 0, 0, m_containerWid, 0);

    xcb_map_window(c, m_windowId);
    xcb_clear_area(c, 0, m_windowId, 0, 0, 1, 1);
    xcb_flush(c);

    auto waCookie = xcb_get_window_attributes(c, m_windowId);
    QSharedPointer<xcb_get_window_attributes_reply_t> windowAttributes(xcb_get_window_attributes_reply(c, waCookie, nullptr));
    if (windowAttributes && !(windowAttributes->all_event_masks & XCB_EVENT_MASK_BUTTON_PRESS)) {
        m_injectMode = XTest;
    }

    QTimer::singleShot(100, this, [this](){
        m_icon = getPixmapFromWidnow();

        Q_EMIT AbstractTrayProtocolHandler::iconChanged();
        auto enable = !m_icon.isNull() || m_attentionTimer->isActive();
        if (m_enabled != enable) {
            m_enabled = enable;
            Q_EMIT AbstractTrayProtocolHandler::enabledChanged();
        }
    });
}

QPixmap XembedProtocolHandler::getPixmapFromWidnow()
{
    QPixmap res;
    QImage image = UTIL->getX11WindowImageNonComposite(m_windowId);
    if (image.isNull()) {
        return res;
    }

    int w = image.width();
    int h = image.height();

    res = QPixmap::fromImage(std::move(image));
    if (w > trayIconSize || h > trayIconSize) {
        res = res.scaled(trayIconSize, trayIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return res;
}

QPoint XembedProtocolHandler::updateEmbedWindowPosForGetInputEvent()
{
    // update pos
    if (qgetenv("XDG_SESSION_TYPE") == "wayland") {
        // Get `plugin_id` and `item_key` from EmbedPlugin.
        auto plugin = Plugin::EmbedPlugin::get(window()->windowHandle());
        // use move_xembed_window to move m_containerWid to plugin_id.
        // TODO: ...?
    } else {
        QPoint p = UTIL->getMousePos();
        UTIL->moveX11Window(m_containerWid, p.x(), p.y());
    }

    // make window normal and above for get input
    UTIL->setX11WindowInputShape(m_containerWid, QSize(1, 1));

    return UTIL->getX11WindowGeometry(m_containerWid).topLeft();
}

void XembedProtocolHandler::sendHover()
{
    QPoint p = updateEmbedWindowPosForGetInputEvent();

    Display *display = UTIL->getDisplay();

    if (m_injectMode == XTest) {
        // fake enter event
        XTestFakeMotionEvent(display, 0, p.x(), p.y(), CurrentTime);
        XFlush(display);
    } else {
        // 发送 montion notify event到client，实现hover事件
        xcb_motion_notify_event_t* event = new xcb_motion_notify_event_t;
        memset(event, 0x00, sizeof(xcb_motion_notify_event_t));
        event->response_type = XCB_MOTION_NOTIFY;
        event->event = m_windowId;
        event->same_screen = 1;
        event->root = UTIL->getRootWindow();
        event->time = 0;
        event->root_x = p.x();
        event->root_y = p.y();
        event->event_x = 0;
        event->event_y = 0;
        event->child = 0;
        event->state = 0;
        xcb_send_event(UTIL->getX11Connection(), false, m_windowId, XCB_EVENT_MASK_POINTER_MOTION, (char*)event);
        delete event;
        xcb_flush(UTIL->getX11Connection());
    }
}

void XembedProtocolHandler::sendClick(uint8_t qMouseButton)
{
    uint8_t mouseButton = XCB_BUTTON_INDEX_1;
    switch (qMouseButton) {
    case Qt::MiddleButton:
        mouseButton = XCB_BUTTON_INDEX_2;
        break;
    case Qt::RightButton:
        mouseButton = XCB_BUTTON_INDEX_3;
        break;
    }

    auto c = UTIL->getX11Connection();
    auto dis = UTIL->getDisplay();

    auto cookieSize = xcb_get_geometry(c, m_windowId);
    QSharedPointer<xcb_get_geometry_reply_t> clientGeom(xcb_get_geometry_reply(c, cookieSize, nullptr));

    if (!clientGeom) {
        return;
    }

    QPoint p = updateEmbedWindowPosForGetInputEvent();
    UTIL->setX11WindowInputShape(m_containerWid, QSize(1, 1));

    if (m_injectMode == Direct) {
        QSharedPointer<xcb_button_press_event_t> pressEvent =QSharedPointer<xcb_button_press_event_t>(new xcb_button_press_event_t);
        memset(pressEvent.get(), 0x00, sizeof(xcb_button_press_event_t));
        pressEvent->response_type = XCB_BUTTON_PRESS;
        pressEvent->event = m_windowId;
        pressEvent->time = XCB_CURRENT_TIME;
        pressEvent->same_screen = 1;
        pressEvent->root = Util::instance()->getRootWindow();
        pressEvent->root_x = p.x();
        pressEvent->root_y = p.y();
        pressEvent->event_x = static_cast<int16_t>(0);
        pressEvent->event_y = static_cast<int16_t>(0);
        pressEvent->child = 0;
        pressEvent->state = 0;
        pressEvent->detail = mouseButton;
        xcb_send_event(c, false, m_windowId, XCB_EVENT_MASK_BUTTON_PRESS, (char *)pressEvent.get());

        QSharedPointer<xcb_button_release_event_t> releaseEvent = QSharedPointer<xcb_button_release_event_t>(new xcb_button_release_event_t);
        memset(releaseEvent.get(), 0x00, sizeof(xcb_button_release_event_t));
        releaseEvent->response_type = XCB_BUTTON_RELEASE;
        releaseEvent->event = m_windowId;
        releaseEvent->time = XCB_CURRENT_TIME;
        releaseEvent->same_screen = 1;
        releaseEvent->root = Util::instance()->getRootWindow();
        releaseEvent->root_x = p.x();
        releaseEvent->root_y = p.y();
        releaseEvent->event_x = static_cast<int16_t>(0);
        releaseEvent->event_y = static_cast<int16_t>(0);
        releaseEvent->child = 0;
        releaseEvent->state = 0;
        releaseEvent->detail = mouseButton;
        xcb_send_event(c, false, m_windowId, XCB_EVENT_MASK_BUTTON_RELEASE, (char *)releaseEvent.get());
    } else {
        XTestFakeMotionEvent(dis, 0, p.x(), p.y(), CurrentTime);
        XFlush(dis);
        XTestFakeButtonEvent(dis, mouseButton, true, 0);
        XFlush(dis);
        XTestFakeButtonEvent(dis, mouseButton, false, 0);
        XFlush(dis);
    }

    xcb_flush(c);
    QTimer::singleShot(100,[this](){
        UTIL->setX11WindowInputShape(m_containerWid, QSize(0, 0));
    });
}
}
