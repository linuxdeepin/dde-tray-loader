// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbuspower.h"

DBusPower::DBusPower(QObject *parent)
    : DDBusExtendedAbstractInterface("org.deepin.dde.Power1", "/org/deepin/dde/Power1", staticInterfaceName(), QDBusConnection::sessionBus(), parent)
    , m_refreshTimer(this)
{
    qRegisterMetaType<BatteryStateMap>("BatteryStateMap");
    qDBusRegisterMetaType<BatteryStateMap>();
    qRegisterMetaType<BatteryPercentageMap>("BatteryPercentageMap");
    qDBusRegisterMetaType<BatteryPercentageMap>();

    // DTK handles live notifications only. Process GetAll replies separately
    // so a delayed snapshot cannot overwrite properties updated in flight.
    connect(this, &DBusPower::propertyChanged, this, [this](const QString &name, const QVariant &value) {
        if (m_pendingSnapshot)
            m_liveProperties.insert(name);
        updateProperties({{name, value}});
    });
    connect(this, &DBusPower::propertyInvalidated, this, [this](const QString &name) {
        if (m_pendingSnapshot)
            m_liveProperties.insert(name);
        getAllProperties();
    });

    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &DBusPower::getAllProperties);
    auto *ownerWatcher = new QDBusServiceWatcher(service(), connection(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(ownerWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &newOwner) {
        m_refreshTimer.stop();
        if (m_pendingSnapshot) {
            disconnect(m_pendingSnapshot, nullptr, this, nullptr);
            m_pendingSnapshot->deleteLater();
            m_pendingSnapshot = nullptr;
        }
        m_refreshPending = false;
        m_liveProperties.clear();
        // Let QDBusAbstractInterface update its owner before fetching again.
        if (!newOwner.isEmpty())
            m_refreshTimer.start(0);
    });

    setSync(false);
    getAllProperties();
}

void DBusPower::getAllProperties()
{
    if (m_pendingSnapshot) {
        m_refreshPending = true;
        return;
    }
    if (!isValid())
        return;

    m_refreshTimer.stop();
    m_liveProperties.clear();
    auto request = QDBusMessage::createMethodCall(service(), path(),
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));
    request << interface();
    m_pendingSnapshot = new QDBusPendingCallWatcher(connection().asyncCall(request), this);
    connect(m_pendingSnapshot, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        watcher->deleteLater();
        m_pendingSnapshot = nullptr;
        const bool refresh = m_refreshPending;
        m_refreshPending = false;
        if (reply.isError()) {
            if (isValid())
                m_refreshTimer.start(1000);
            return;
        }

        auto snapshot = reply.value();
        for (const auto &name : m_liveProperties)
            snapshot.remove(name);
        // An invalidation/request during GetAll needs a newer snapshot.
        if (refresh)
            m_refreshTimer.start(0);
        updateProperties(snapshot);
    });
}

void DBusPower::updateProperties(const QVariantMap &properties)
{
    const bool onBattery = properties.contains("OnBattery") ? properties.value("OnBattery").toBool() : m_onBattery;
    const auto percentage = properties.contains("BatteryPercentage")
        ? qdbus_cast<BatteryPercentageMap>(properties.value("BatteryPercentage")) : m_batteryPercentage;
    const auto state = properties.contains("BatteryState")
        ? qdbus_cast<BatteryStateMap>(properties.value("BatteryState")) : m_batteryState;
    const bool onBatteryChanged = m_onBattery != onBattery;
    const bool percentageChanged = m_batteryPercentage != percentage;
    const bool stateChanged = m_batteryState != state;
    // Publish the whole snapshot before notifying readers of any property.
    m_onBattery = onBattery;
    m_batteryPercentage = percentage;
    m_batteryState = state;
    if (onBatteryChanged)
        Q_EMIT OnBatteryChanged();
    if (percentageChanged)
        Q_EMIT BatteryPercentageChanged();
    if (stateChanged)
        Q_EMIT BatteryStateChanged();
}
