/*
    Registers as a embed container
    SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Konrad Materka <materka@gmail.com>
    SPDX-FileCopyrightText: 2025 Wang Zichong <wangzichong@deepin.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#include "fdoselectionmanager.h"

#include <QTimer>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <KSelectionOwner>

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>

#include "traymanager1.h"
#include "c_ptr.h"
#include "util.h"

using Util = tray::Util;

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

Q_LOGGING_CATEGORY(SELECTIONMGR, "org.deepin.dde.trayloader.selectionmgr")

FdoSelectionManager::FdoSelectionManager(QObject *parent)
    : QObject(parent)
    , m_selectionOwner(new KSelectionOwner(UTIL->getAtomFromDisplay("_NET_SYSTEM_TRAY"), UTIL->getX11Connection(), UTIL->getRootWindow(), this))
{
    qDebug(SELECTIONMGR) << "starting";

    // we may end up calling QCoreApplication::quit() in this method, at which point we need the event loop running
    QTimer::singleShot(0, this, &FdoSelectionManager::init);
}

FdoSelectionManager::~FdoSelectionManager()
{
    qCDebug(SELECTIONMGR) << "closing";
    m_selectionOwner->release();
}

void FdoSelectionManager::init()
{
    // load damage extension
    xcb_connection_t *c = Util::instance()->getX11Connection();
    xcb_prefetch_extension_data(c, &xcb_damage_id);
    const auto *reply = xcb_get_extension_data(c, &xcb_damage_id);
    if (reply && reply->present) {
        m_damageEventBase = reply->first_event;
        xcb_damage_query_version_unchecked(c, XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
    } else {
        // no XDamage means
        qCCritical(SELECTIONMGR) << "could not load damage extension. Quitting";
        qApp->exit(-1);
    }

    qApp->installNativeEventFilter(this);

    connect(m_selectionOwner, &KSelectionOwner::claimedOwnership, this, &FdoSelectionManager::onClaimedOwnership);
    connect(m_selectionOwner, &KSelectionOwner::failedToClaimOwnership, this, &FdoSelectionManager::onFailedToClaimOwnership);
    connect(m_selectionOwner, &KSelectionOwner::lostOwnership, this, &FdoSelectionManager::onLostOwnership);
    m_selectionOwner->claim(true);

    connect(m_trayManager, &TrayManager1::reclainRequested, this, [this](){
        m_selectionOwner->claim(true);
    });
}

bool FdoSelectionManager::addDamageWatch(xcb_window_t client)
{
    qCDebug(SELECTIONMGR) << "adding damage watch for " << client;

    xcb_connection_t *c = Util::instance()->getX11Connection();
    const auto attribsCookie = xcb_get_window_attributes_unchecked(c, client);

    const auto damageId = xcb_generate_id(c);
    m_damageWatches[client] = damageId;
    xcb_damage_create(c, damageId, client, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);

    xcb_generic_error_t *error = nullptr;
    UniqueCPointer<xcb_get_window_attributes_reply_t> attr(xcb_get_window_attributes_reply(c, attribsCookie, &error));
    UniqueCPointer<xcb_generic_error_t> getAttrError(error);
    uint32_t events = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    if (attr) {
        events = events | attr->your_event_mask;
    }
    // if window is already gone, there is no need to handle it.
    if (getAttrError && getAttrError->error_code == XCB_WINDOW) {
        return false;
    }
    // the event mask will not be removed again. We cannot track whether another component also needs STRUCTURE_NOTIFY (e.g. KWindowSystem).
    // if we would remove the event mask again, other areas will break.
    const auto changeAttrCookie = xcb_change_window_attributes_checked(c, client, XCB_CW_EVENT_MASK, &events);
    UniqueCPointer<xcb_generic_error_t> changeAttrError(xcb_request_check(c, changeAttrCookie));
    // if window is gone by this point, it will be caught by eventFilter, so no need to check later errors.
    if (changeAttrError && changeAttrError->error_code == XCB_WINDOW) {
        return false;
    }

    return true;
}

bool FdoSelectionManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)

    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    auto *ev = static_cast<xcb_generic_event_t *>(message);

    const auto responseType = XCB_EVENT_RESPONSE_TYPE(ev);
    if (responseType == XCB_CLIENT_MESSAGE) {
        const auto ce = reinterpret_cast<xcb_client_message_event_t *>(ev);
        if (ce->type == UTIL->getAtomByName("_NET_SYSTEM_TRAY_OPCODE")) {
            switch (ce->data.data32[1]) {
            case SYSTEM_TRAY_REQUEST_DOCK:
                dock(ce->data.data32[2]);
                return true;
            }
        }
    } else if (responseType == XCB_UNMAP_NOTIFY) {
        // const auto unmappedWId = reinterpret_cast<xcb_unmap_notify_event_t *>(ev)->window;
        // if (m_proxies.contains(unmappedWId)) {
        //     undock(unmappedWId);
        // }
    } else if (responseType == XCB_DESTROY_NOTIFY) {
        const auto destroyedWId = reinterpret_cast<xcb_destroy_notify_event_t *>(ev)->window;
        if (m_trayManager->haveIcon(destroyedWId)) {
            undock(destroyedWId);
        }
    } else if (responseType == m_damageEventBase + XCB_DAMAGE_NOTIFY) {
        const auto damagedWId = reinterpret_cast<xcb_damage_notify_event_t *>(ev)->drawable;
        m_trayManager->notifyIconChanged(damagedWId);
    } else if (responseType == XCB_CONFIGURE_REQUEST) {
        // const auto event = reinterpret_cast<xcb_configure_request_event_t *>(ev);
        // const auto tmProxy = m_proxies.value(event->window);
        // if (tmProxy) {
        //     // The embedded window tries to move or resize. Ignore move, handle resize only.
        //     if ((event->value_mask & XCB_CONFIG_WINDOW_WIDTH) || (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT)) {
        //         tmProxy->resizeWindow(event->width, event->height);
        //     }
        // }
    }

    return false;
}

void FdoSelectionManager::dock(xcb_window_t winId)
{
    Q_CHECK_PTR(m_trayManager);
    qCDebug(SELECTIONMGR) << "trying to dock window " << winId;

    if (m_trayManager->haveIcon(winId)) {
        return;
    }

    if (addDamageWatch(winId)) {
        // Register with TrayManager1 if available
        m_trayManager->registerIcon(winId);
    }
}

void FdoSelectionManager::undock(xcb_window_t winId)
{
    Q_CHECK_PTR(m_trayManager);
    qCDebug(SELECTIONMGR) << "trying to undock window " << winId;

    if (m_trayManager->haveIcon(winId)) {
        return;
    }
    
    // Unregister from TrayManager1 if available
    m_trayManager->unregisterIcon(winId);
    
    // m_proxies[winId]->deleteLater();
    // m_proxies.remove(winId);
}

void FdoSelectionManager::onClaimedOwnership()
{
    qCDebug(SELECTIONMGR) << "Manager selection claimed";

    initTrayManager();
    setSystemTrayVisual();
}

void FdoSelectionManager::onFailedToClaimOwnership()
{
    qCWarning(SELECTIONMGR) << "failed to claim ownership of Systray Manager";
}

void FdoSelectionManager::onLostOwnership()
{
    qCWarning(SELECTIONMGR) << "lost ownership of Systray Manager";
}

void FdoSelectionManager::setSystemTrayVisual()
{
    xcb_connection_t *c = Util::instance()->getX11Connection();
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
    auto trayVisual = screen->root_visual;
    xcb_depth_iterator_t depth_iterator = xcb_screen_allowed_depths_iterator(screen);
    xcb_depth_t *depth = nullptr;

    while (depth_iterator.rem) {
        if (depth_iterator.data->depth == 32) {
            depth = depth_iterator.data;
            break;
        }
        xcb_depth_next(&depth_iterator);
    }

    if (depth) {
        xcb_visualtype_iterator_t visualtype_iterator = xcb_depth_visuals_iterator(depth);
        while (visualtype_iterator.rem) {
            xcb_visualtype_t *visualtype = visualtype_iterator.data;
            if (visualtype->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
                trayVisual = visualtype->visual_id;
                break;
            }
            xcb_visualtype_next(&visualtype_iterator);
        }
    }

    xcb_change_property(c, XCB_PROP_MODE_REPLACE, m_selectionOwner->ownerWindow(), UTIL->getAtomByName("_NET_SYSTEM_TRAY_VISUAL"), XCB_ATOM_VISUALID, 32, 1, &trayVisual);
}

void FdoSelectionManager::initTrayManager()
{
    // Create and register the TrayManager1 DBus interface
    if (!m_trayManager) {
        m_trayManager = new TrayManager1(this);
        
        // Export the object on DBus
        QDBusConnection::sessionBus().registerObject(
            QStringLiteral("/org/deepin/dde/TrayManager1"),
            m_trayManager,
            QDBusConnection::ExportAdaptors
        );
        
        // Request the service name
        QDBusConnection::sessionBus().registerService(
            QStringLiteral("org.deepin.dde.TrayManager1")
        );
        
        qCDebug(SELECTIONMGR) << "TrayManager1 DBus interface registered";
    }
}

