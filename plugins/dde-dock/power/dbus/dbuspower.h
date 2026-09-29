// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DBUSPOWER_H_1467941176
#define DBUSPOWER_H_1467941176

#include <DDBusExtendedAbstractInterface>

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

typedef QMap<QString, quint32> BatteryStateMap;
typedef QMap<QString, double> BatteryPercentageMap;
Q_DECLARE_METATYPE(BatteryPercentageMap)
Q_DECLARE_METATYPE(BatteryStateMap)

/*
 * Proxy class for interface org.deepin.dde.Power1
 */
class DBusPower: public Dtk::Core::DDBusExtendedAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "org.deepin.dde.Power1"; }

public:
    explicit DBusPower(QObject *parent = 0);

    ~DBusPower() override = default;

    // Keep snapshots separate from DTK's live propertyChanged notifications.
    void getAllProperties();

    // Icon refreshes must not wait for the session power service to reply.
    Q_PROPERTY(bool OnBattery READ onBattery NOTIFY OnBatteryChanged)
    inline bool onBattery() const
    { return m_onBattery; }

    Q_PROPERTY(BatteryPercentageMap BatteryPercentage READ batteryPercentage NOTIFY BatteryPercentageChanged)
    inline BatteryPercentageMap batteryPercentage() const
    { return m_batteryPercentage; }

    Q_PROPERTY(BatteryStateMap BatteryState READ batteryState NOTIFY BatteryStateChanged)
    inline BatteryStateMap batteryState() const
    { return m_batteryState; }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
// begin property changed signals
void OnBatteryChanged();
void BatteryPercentageChanged();
void BatteryStateChanged();

private:
    void updateProperties(const QVariantMap &properties);

    QTimer m_refreshTimer;
    QDBusPendingCallWatcher *m_pendingSnapshot = nullptr;
    QSet<QString> m_liveProperties;
    bool m_refreshPending = false;
    bool m_onBattery = false;
    BatteryPercentageMap m_batteryPercentage;
    BatteryStateMap m_batteryState;
};

namespace org {
  namespace deepin {
    namespace dde {
      typedef ::DBusPower Power;
    }
  }
}
#endif
