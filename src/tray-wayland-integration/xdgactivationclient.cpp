// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xdgactivationclient.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QPointer>
#include <QWindow>
#include <QtWaylandClient/QWaylandClientExtension>

#include "qwayland-xdg-activation-v1.h"
#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandwindow_p.h>

Q_LOGGING_CATEGORY(trayXdgActivation, "dde.tray.xdgactivation")

namespace tray {

class XdgActivationTokenV1 : public QObject, public QtWayland::xdg_activation_token_v1
{
    Q_OBJECT
public:
    ~XdgActivationTokenV1() override { destroy(); }

Q_SIGNALS:
    void done(const QString &token);

protected:
    void xdg_activation_token_v1_done(const QString &token) override
    {
        Q_EMIT done(token);
    }
};

class XdgActivationV1 : public QWaylandClientExtensionTemplate<XdgActivationV1>,
                        public QtWayland::xdg_activation_v1
{
public:
    XdgActivationV1()
        : QWaylandClientExtensionTemplate<XdgActivationV1>(1)
    {
        initialize();
    }

    ~XdgActivationV1() override
    {
        if (isInitialized())
            destroy();
    }

    XdgActivationTokenV1 *createTokenProvider(QWindow *window, const QString &appId)
    {
        auto *provider = new XdgActivationTokenV1;
        provider->init(get_activation_token());

        if (window) {
            if (auto *waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow *>(window->handle())) {
                if (auto *surface = waylandWindow->wlSurface())
                    provider->set_surface(surface);
                if (auto *inputDevice = waylandWindow->display()->lastInputDevice())
                    provider->set_serial(inputDevice->serial(), inputDevice->wl_seat());
            }
        }

        if (!appId.isEmpty())
            provider->set_app_id(appId);

        provider->commit();
        return provider;
    }
};

XdgActivationClient *XdgActivationClient::instance()
{
    static XdgActivationClient *s_instance = nullptr;
    if (!s_instance) {
        s_instance = new XdgActivationClient(qApp);
    }
    return s_instance;
}

XdgActivationClient::XdgActivationClient(QObject *parent)
    : QObject(parent)
    , m_activation(new XdgActivationV1)
{
    m_activation->setParent(this);
}

XdgActivationClient::~XdgActivationClient() = default;

bool XdgActivationClient::isActive() const
{
    return m_activation && m_activation->isActive();
}

void XdgActivationClient::requestToken(QWindow *window, const QString &appId, TokenCallback callback)
{
    if (m_pendingProvider) {
        qCWarning(trayXdgActivation) << "Token request already in progress";
        if (callback) callback(QString());
        return;
    }

    if (!isActive()) {
        qCDebug(trayXdgActivation) << "xdg_activation_v1 not active";
        if (callback) callback(QString());
        return;
    }

    auto effectiveWindow = window ? window : QGuiApplication::focusWindow();
    if (!effectiveWindow) {
        qCWarning(trayXdgActivation) << "No target window for activation token";
        if (callback) callback(QString());
        return;
    }

    auto *provider = m_activation->createTokenProvider(effectiveWindow, appId);
    m_pendingProvider = provider;

    connect(provider, &XdgActivationTokenV1::done, this, [this, provider, callback](const QString &token) {
        m_pendingProvider = nullptr;

        if (token.isEmpty())
            qCWarning(trayXdgActivation) << "Empty activation token received";
        else
            qCDebug(trayXdgActivation) << "Activation token received";

        if (callback) callback(token);
        provider->deleteLater();
    }, Qt::SingleShotConnection);
}

} // namespace tray

#include "xdgactivationclient.moc"
