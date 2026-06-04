// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>

class QWindow;

namespace tray {

class XdgActivationTokenV1;

class XdgActivationClient : public QObject
{
    Q_OBJECT
public:
    static XdgActivationClient *instance();

    bool isActive() const;

    using TokenCallback = std::function<void(const QString &token)>;
    void requestToken(QWindow *window, const QString &appId, TokenCallback callback);

private:
    explicit XdgActivationClient(QObject *parent = nullptr);
    ~XdgActivationClient() override;

    class XdgActivationV1 *m_activation = nullptr;
    QPointer<XdgActivationTokenV1> m_pendingProvider;
};

} // namespace tray
