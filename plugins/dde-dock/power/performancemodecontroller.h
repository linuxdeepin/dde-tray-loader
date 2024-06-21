// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PERFORMANCEMODECONTROLLER_H
#define PERFORMANCEMODECONTROLLER_H

#include <QObject>
#include <QGSettings>
#include <QVariant>

#include <DSingleton>

#include <com_deepin_system_systempower.h>

#define PERFORMANCE "performance" // 高性能模式
#define BALANCE     "balance"     // 平衡模式
#define POWERSAVE   "powersave"   // 节能模式

using SysPowerInter = com::deepin::system::Power;

class PerformanceModeController : public QObject, public Dtk::Core::DSingleton<PerformanceModeController>
{
    friend Dtk::Core::DSingleton<PerformanceModeController>;

    Q_OBJECT
public:
    bool highPerformanceSupported() const { return m_highPerformanceSupported; }
    bool balanceSupported() const { return m_balanceSupported; }
    bool powerSaveSupported() const { return m_powerSaveSupported; }
    QList<QPair<QString, QString>> modeList() const { return m_modeList; }

    void setPowerMode(const QString &mode)
    {
        m_sysPowerInter->SetMode(mode);
    }
    QString getCurrentPowerMode() const { return m_currentMode; }

Q_SIGNALS:
    void powerModeChanged(const QString &mode);
    void highPerformanceSupportChanged(bool value);

private:
    PerformanceModeController()
        : QObject(nullptr)
        , m_highPerformanceSupported(false)
        , m_balanceSupported(false)
        , m_powerSaveSupported(false)
        , m_currentMode(QString())
        , m_sysPowerInter(new SysPowerInter("com.deepin.system.Power", "/com/deepin/system/Power", QDBusConnection::systemBus(), this))
    {
        m_sysPowerInter->setSync(false);

        QDBusInterface interface("com.deepin.system.Power",
                                 "/com/deepin/system/Power",
                                 "org.freedesktop.DBus.Properties",
                                 QDBusConnection::systemBus());
        QDBusMessage reply = interface.call("Get", "com.deepin.system.Power", "IsHighPerformanceSupported");
        QList<QVariant> outArgs = reply.arguments();
        if (outArgs.length() > 0) {
            m_highPerformanceSupported = outArgs.at(0).value<QDBusVariant>().variant().toBool();
            if (m_highPerformanceSupported) {
                m_modeList.append({PERFORMANCE, tr("High Performance")});
            }
        }

        reply = interface.call("Get", "com.deepin.system.Power", "IsBalanceSupported");
        outArgs = reply.arguments();
        if (outArgs.length() > 0) {
            m_balanceSupported = outArgs.at(0).value<QDBusVariant>().variant().toBool();
            if (m_balanceSupported) {
                m_modeList.append({BALANCE, tr("Balanced")});
            }
        }

        reply = interface.call("Get", "com.deepin.system.Power", "IsPowerSaveSupported");
        outArgs = reply.arguments();
        if (outArgs.length() > 0) {
            m_powerSaveSupported = outArgs.at(0).value<QDBusVariant>().variant().toBool();
            if (m_powerSaveSupported) {
                m_modeList.append({POWERSAVE, tr("Power Saver")});
            }
        }

        m_currentMode = m_sysPowerInter->mode();

        connect(m_sysPowerInter, &SysPowerInter::ModeChanged, this, [this](const QString &mode) {
            m_currentMode = mode;
            Q_EMIT powerModeChanged(m_currentMode);
        });
        connect(m_sysPowerInter, &SysPowerInter::IsHighPerformanceSupportedChanged, this, [this](bool value) {
            m_highPerformanceSupported = value;
            Q_EMIT highPerformanceSupportChanged(value);
        });
    }

    ~PerformanceModeController() {}

private:
    bool m_highPerformanceSupported;
    bool m_balanceSupported;
    bool m_powerSaveSupported;
    QList<QPair<QString, QString>> m_modeList;
    QString m_currentMode;

    SysPowerInter *m_sysPowerInter;
};

#endif
